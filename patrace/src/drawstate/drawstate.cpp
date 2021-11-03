#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <unistd.h>
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>
#include <set>

#include "common/image.hpp"
#include "common/trace_model.hpp"
#include "eglstate/common.hpp"
#include "base/base.hpp"

#include "dispatch/eglproc_auto.hpp"
#include "helper/eglsize.hpp"
#include "common/gl_utility.hpp"
#include "common/trace_model_utility.hpp"

#include "json/writer.h"

#include "tool/utils.hpp"

#include <retracer/glstate.hpp>
#include <retracer/config.hpp>
#include <retracer/glws.hpp>
#include <retracer/retrace_api.hpp>
#include <retracer/retracer.hpp>
#include <retracer/state.hpp>

#define DS_VERSION_MAJOR 0
#define DS_VERSION_MINOR 1
#define DS_VERSION_COMMIT_HASH PATRACE_REVISION
#define DS_VERSION_TYPE PATRACE_VERSION_TYPE

#include "../../thirdparty/opengl-registry/api/GLES3/gl31.h"

static unsigned int startCall = 0; //Used if --range is used
static unsigned int endCall = 0; //Used if --range is used
static unsigned drawCall; //Used for a single drawcall
static Json::Value result;
static std::string outputdir;
static std::vector<GLuint> colorAttachments;
static std::set<GLuint> textures2d; // texture units (as retraced) that are actually in use
static std::set<GLuint> textures_cubemap;
// TBD separate sets for other texture types

struct ProgramState
{
    GLint linkStatus;
    GLint activeFBO;
    GLint maxColorAttachments;
    GLint activeAttributes;
    GLint activeUniforms;
    GLint activeUniformBlocks;
    GLint activeAtomicCounterBuffers;
    GLint activeInputs;
    GLint activeOutputs;
    GLint activeTransformFeedbackVaryings;
    GLint activeBufferVariables;
    GLint activeSSBOs;
    GLint activeTransformFeedbackBuffers;
};

struct TextureInfo
{
    struct MipmapSize
    {
        GLint width;
        GLint height;
    };
    std::vector<MipmapSize> mMipmapSizes;

    GLint mInternalFormat;
    bool mIsCompressed;

    // Can be GL_NONE, GL_SIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_FLOAT, GL_INT, and GL_UNSIGNED_INT
    GLint mRedType;
    GLint mGreenType;
    GLint mBlueType;
    GLint mAlphaType;
    GLint mDepthType;

    GLint mSamples;
    GLint mFixedSampleLocations;
    GLint mRedSize;
    GLint mGreenSize;
    GLint mBlueSize;
    GLint mAlphaSize;
    GLint mDepthSize;

    GLint mBaseLevel;
    GLint mCompareFunc;
    GLint mCompareMode;
    GLint mImmutableFormat;
    GLint mImmutableLevels;
    GLint mMagFilter;
    GLint mMinFilter;
    GLint mMaxLevel;
    GLint mMaxLOD;
    GLint mMinLOD;
    GLint mSwizzleR;
    GLint mSwizzleG;
    GLint mSwizzleB;
    GLint mSwizzleA;
    GLint mWrapS;
    GLint mWrapT;
    GLint mWrapR;
};

static inline const std::string TexEnumString(unsigned int enumToFind)
{
    return SafeEnumString(enumToFind, "glTexParameter");
}

static std::string shader_filename(int id, GLenum type)
{
    return outputdir + "/shader_" + std::to_string(id) + shader_extension(type);
}

bool checkError(const char* msg)
{
    GLenum error = glGetError();

    if (error == GL_NO_ERROR)
    {
        return false;
    }

    DBG_LOG("glGetError: 0x%04x (%s) (context/msg: %s)\n", error, SafeEnumString(error).c_str(), msg);

    return true;
}

template<class T>
static Json::Value dumpBuffer(const char *ptr, unsigned size, unsigned stride, uintptr_t offset, GLuint elemCount)
{
    Json::Value ret = Json::arrayValue;

    // size:        9 * sizeof(float)
    //         [ A1 A2 A3 A4 | B1 B2 B3 | C1 C2 ] = float[9] = 1 block
    // stride: |<------------------------------>| 36 bytes
    // ofs:     ^0           ^16        ^28
    // elemCnt |<-----4----->|<----3--->|<--2-->|
    if (!stride)
    {
        stride = (unsigned)sizeof(T); // for uniform elements
    }
    size_t num_blocks = size/stride;
    for (size_t block_idx = 0; block_idx < num_blocks; block_idx++)
    {
        ptrdiff_t pBlock = (ptrdiff_t)ptr + block_idx*stride + offset;
        for (size_t elem_idx = 0; elem_idx < elemCount; elem_idx++)
        {
            const T *pElement = reinterpret_cast<const T*>(pBlock + elem_idx*sizeof(T));
            ret.append(*pElement);
        }
    }
    return ret;
}

static Json::Value dumpValues(GLenum type, const char *ptr, unsigned bufferSize, unsigned stride, GLvoid *bufOffsetPtr,
    GLuint elemCount)
{
    switch (type)
    {
    case GL_FLOAT:
        return dumpBuffer<GLfloat>(ptr, bufferSize, stride, reinterpret_cast<intptr_t>(bufOffsetPtr), elemCount);
    case GL_UNSIGNED_INT:
        return dumpBuffer<GLuint>(ptr, bufferSize, stride, reinterpret_cast<intptr_t>(bufOffsetPtr), elemCount);
    case GL_UNSIGNED_SHORT:
        return dumpBuffer<GLushort>(ptr, bufferSize, stride, reinterpret_cast<intptr_t>(bufOffsetPtr), elemCount);
    case GL_UNSIGNED_BYTE:
        return dumpBuffer<GLubyte>(ptr, bufferSize, stride, reinterpret_cast<intptr_t>(bufOffsetPtr), elemCount);
    case GL_BYTE:
        return dumpBuffer<GLbyte>(ptr, bufferSize, stride, reinterpret_cast<intptr_t>(bufOffsetPtr), elemCount);
    case GL_INT:
        return dumpBuffer<GLint>(ptr, bufferSize, stride, reinterpret_cast<intptr_t>(bufOffsetPtr), elemCount);
    case GL_SHORT:
        return dumpBuffer<GLshort>(ptr, bufferSize, stride, reinterpret_cast<intptr_t>(bufOffsetPtr), elemCount);
    default:
        DBG_LOG("Unknown attribute type: %s\n", TexEnumString(type).c_str());
        break;
    }
    return Json::Value();
}

// Gets info for texture bound to target (e.g. GL_TEXTURE_2D)
static TextureInfo getTextureInfo(GLenum target)
{
    checkError("getTextureInfo begin");

    TextureInfo info;
    GLenum original_target = target;

    if (target == GL_TEXTURE_CUBE_MAP)
    {
        target = GL_TEXTURE_CUBE_MAP_POSITIVE_X; // assuming all sides are similar
    }

    GLint isCompressed;
    _glGetTexLevelParameteriv(target, 0, GL_TEXTURE_COMPRESSED, &isCompressed);
    if (isCompressed == GL_TRUE)
    {
        info.mIsCompressed = true;
    }
    else
    {
        // _glGetTexLevelParameteriv with GL_TEXTURE_COMPRESSED can return GL_FALSE even if the
        // texture is compressed (e.g. for GL_ETC1_RGB8_OES). So we have to do some additional checks.
        switch(info.mInternalFormat)
        {
        case GL_ETC1_RGB8_OES:
        case GL_COMPRESSED_R11_EAC:
        case GL_COMPRESSED_SIGNED_R11_EAC:
        case GL_COMPRESSED_RG11_EAC:
        case GL_COMPRESSED_SIGNED_RG11_EAC:
        case GL_COMPRESSED_RGB8_ETC2:
        case GL_COMPRESSED_SRGB8_ETC2:
        case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
        case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
        case GL_COMPRESSED_RGBA8_ETC2_EAC:
        case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
        case GL_COMPRESSED_RGBA_ASTC_4x4_KHR :
        case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:
        case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:
        case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:
        case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:
        case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:
        case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:
        case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:
        case GL_COMPRESSED_RGBA_ASTC_10x5_KHR :
        case GL_COMPRESSED_RGBA_ASTC_10x6_KHR :
        case GL_COMPRESSED_RGBA_ASTC_10x8_KHR :
        case GL_COMPRESSED_RGBA_ASTC_10x10_KHR:
        case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:
        case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:
        case GL_PALETTE4_RGB8_OES:
        case GL_PALETTE4_RGBA8_OES:
        case GL_PALETTE4_R5_G6_B5_OES :
        case GL_PALETTE4_RGBA4_OES:
        case GL_PALETTE4_RGB5_A1_OES:
        case GL_PALETTE8_RGB8_OES:
        case GL_PALETTE8_RGBA8_OES:
        case GL_PALETTE8_R5_G6_B5_OES :
        case GL_PALETTE8_RGBA4_OES:
        case GL_PALETTE8_RGB5_A1_OES:
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG:
        case GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG:
        case GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG:
        case GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG:
            info.mIsCompressed = true;
            break;
        default:
            info.mIsCompressed = false;
        }
    }

    _glGetTexLevelParameteriv(target, 0, GL_TEXTURE_INTERNAL_FORMAT, &info.mInternalFormat);

    // Midgard DDK (4aa1dd85062b) asserts on texture 7 of Egypt HD 25fps (ETC1) when asking for GL_TEXTURE_RED_TYPE.
    // ==>[ASSERT] (eglretrace) : In file: midg/src/helpers/src/mali_midg_pixel_format.c   line: 734 midg_pixel_format_get_info
    // !midg_pixel_format_is_compressed( pfs )
    // .. even though the texture is correctly reported to be compressed when querying GL_TEXTURE_COMPRESSED.
    _glGetTexLevelParameteriv(target, 0, GL_TEXTURE_RED_TYPE, &info.mRedType);
    _glGetTexLevelParameteriv(target, 0, GL_TEXTURE_GREEN_TYPE, &info.mGreenType);
    _glGetTexLevelParameteriv(target, 0, GL_TEXTURE_BLUE_TYPE, &info.mBlueType);
    _glGetTexLevelParameteriv(target, 0, GL_TEXTURE_ALPHA_TYPE, &info.mAlphaType);
    _glGetTexLevelParameteriv(target, 0, GL_TEXTURE_DEPTH_TYPE, &info.mDepthType);
    _glGetTexLevelParameteriv(target, 0, GL_TEXTURE_SAMPLES, &info.mSamples);
    _glGetTexLevelParameteriv(target, 0, GL_TEXTURE_FIXED_SAMPLE_LOCATIONS, &info.mFixedSampleLocations);
    _glGetTexLevelParameteriv(target, 0, GL_TEXTURE_RED_SIZE, &info.mRedSize);
    _glGetTexLevelParameteriv(target, 0, GL_TEXTURE_GREEN_SIZE, &info.mGreenSize);
    _glGetTexLevelParameteriv(target, 0, GL_TEXTURE_BLUE_SIZE, &info.mBlueSize);
    _glGetTexLevelParameteriv(target, 0, GL_TEXTURE_ALPHA_SIZE, &info.mAlphaSize);
    _glGetTexLevelParameteriv(target, 0, GL_TEXTURE_DEPTH_SIZE, &info.mDepthSize);

    _glGetTexParameteriv(original_target, GL_TEXTURE_BASE_LEVEL, &info.mBaseLevel);
    _glGetTexParameteriv(original_target, GL_TEXTURE_COMPARE_FUNC, &info.mCompareFunc);
    _glGetTexParameteriv(original_target, GL_TEXTURE_COMPARE_MODE, &info.mCompareMode);
    _glGetTexParameteriv(original_target, GL_TEXTURE_IMMUTABLE_FORMAT, &info.mImmutableFormat);
    _glGetTexParameteriv(original_target, GL_TEXTURE_IMMUTABLE_LEVELS, &info.mImmutableLevels);
    _glGetTexParameteriv(original_target, GL_TEXTURE_MAG_FILTER, &info.mMagFilter);
    _glGetTexParameteriv(original_target, GL_TEXTURE_MAX_LEVEL, &info.mMaxLevel);
    _glGetTexParameteriv(original_target, GL_TEXTURE_MAX_LOD, &info.mMaxLOD);
    _glGetTexParameteriv(original_target, GL_TEXTURE_MIN_FILTER, &info.mMinFilter);
    _glGetTexParameteriv(original_target, GL_TEXTURE_MIN_LOD, &info.mMinLOD);
    _glGetTexParameteriv(original_target, GL_TEXTURE_SWIZZLE_R, &info.mSwizzleR);
    _glGetTexParameteriv(original_target, GL_TEXTURE_SWIZZLE_G, &info.mSwizzleG);
    _glGetTexParameteriv(original_target, GL_TEXTURE_SWIZZLE_B, &info.mSwizzleB);
    _glGetTexParameteriv(original_target, GL_TEXTURE_SWIZZLE_A, &info.mSwizzleA);
    _glGetTexParameteriv(original_target, GL_TEXTURE_WRAP_S, &info.mWrapS);
    _glGetTexParameteriv(original_target, GL_TEXTURE_WRAP_T, &info.mWrapT);
    _glGetTexParameteriv(original_target, GL_TEXTURE_WRAP_R, &info.mWrapR);

    // Find number of mipmap levels
    GLint maxLevel;
    _glGetTexParameteriv(original_target, GL_TEXTURE_MAX_LEVEL, &maxLevel);

    // Read size of each mipmap
    for (int i = 0; i <= maxLevel; i++)
    {
        TextureInfo::MipmapSize s;
        _glGetTexLevelParameteriv(target, i, GL_TEXTURE_WIDTH,  &s.width);
        _glGetTexLevelParameteriv(target, i, GL_TEXTURE_HEIGHT, &s.height);

        if (s.width == 0 || s.height == 0)
        {
            // This level is not valid. Stop querying.
            break;
        }
        else
        {
            info.mMipmapSizes.push_back(s);
        }
    }

    checkError("getTextureInfo end");
    return info;
}

static Json::Value saveTexture(retracer::Context& mRetracerContext, int mThreadId, GLuint textureUnit, GLenum bindingType, GLenum target)
{
    checkError("saveTexture begin");
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    GLint tmp;
    glGetIntegerv(bindingType, &tmp);
    GLuint retraceTextureId = tmp;
    GLuint traceTextureId = mRetracerContext.getTextureRevMap().RValue(retraceTextureId);

    struct StoreParams
    {
        GLenum e;
        GLint value;
    } storeParams[] = {
        {GL_PACK_ROW_LENGTH, 0},
        {GL_UNPACK_IMAGE_HEIGHT, 0},
        {GL_PACK_SKIP_ROWS, 0},
        {GL_PACK_SKIP_PIXELS, 0},
        {GL_UNPACK_SKIP_IMAGES, 0},
        {GL_PACK_ALIGNMENT, 4},
        {GL_UNPACK_ROW_LENGTH, 0},
        {GL_UNPACK_IMAGE_HEIGHT, 0},
        {GL_UNPACK_SKIP_ROWS, 0},
        {GL_UNPACK_SKIP_PIXELS, 0},
        {GL_UNPACK_SKIP_ROWS, 0},
        {GL_UNPACK_ALIGNMENT, 4},
    };

    // Set and emit sane values
    for (unsigned int i = 0; i < sizeof(storeParams)/sizeof(storeParams[0]); i++)
    {
        // Set all to 0...
        GLint value = 0;

        // ... except alignment
        if (storeParams[i].e == GL_UNPACK_ALIGNMENT || storeParams[i].e == GL_PACK_ALIGNMENT)
        {
            value = 1;
        }

        // Set locally
        _glPixelStorei(storeParams[i].e, value);
    }

    Json::Value result;
    TextureInfo texInfo = getTextureInfo(target);

    result["id"] = traceTextureId;
    result["unit"] = textureUnit;
    result["target"] = (int)target;
    result["target_name"] = TexEnumString(target);
    result["binding"] = (int)bindingType;
    result["binding_name"] = TexEnumString(bindingType);
    result["mipmap_levels"] = (int)texInfo.mMipmapSizes.size();
    result["red_type"] = texInfo.mRedType;
    result["red_type_name"] = TexEnumString(texInfo.mRedType);
    result["green_type"] = texInfo.mGreenType;
    result["green_type_name"] = TexEnumString(texInfo.mGreenType);
    result["red_type"] = texInfo.mRedType;
    result["red_type_name"] = TexEnumString(texInfo.mRedType);
    result["alpha_type"] = texInfo.mAlphaType;
    result["alpha_type_name"] = TexEnumString(texInfo.mAlphaType);
    result["depth_type"] = texInfo.mDepthType;
    result["depth_type_name"] = TexEnumString(texInfo.mDepthType);
    result["internal_format"] = texInfo.mInternalFormat;
    result["internal_format_name"] = TexEnumString(texInfo.mInternalFormat);

    result["samples"] = texInfo.mSamples;
    result["fixed_sample_locations"] = (bool)texInfo.mFixedSampleLocations;
    result["red_size"] = texInfo.mRedSize;
    result["green_size"] = texInfo.mGreenSize;
    result["blue_size"] = texInfo.mBlueSize;
    result["alpha_size"] = texInfo.mAlphaSize;
    result["depth_size"] = texInfo.mDepthSize;
    result["base_level"] = texInfo.mBaseLevel;
    result["compare_func"] = texInfo.mCompareFunc;
    result["compare_mode"] = texInfo.mCompareMode;
    result["compare_func_name"] = TexEnumString(texInfo.mCompareFunc);
    result["compare_mode_name"] = TexEnumString(texInfo.mCompareMode);
    result["immutable_format"] = texInfo.mImmutableFormat;
    result["immutable_levels"] = texInfo.mImmutableLevels;
    result["mag_filter"] = texInfo.mMagFilter;
    result["min_filter"] = texInfo.mMinFilter;
    result["mag_filter_name"] = TexEnumString(texInfo.mMagFilter);
    result["min_filter_name"] = TexEnumString(texInfo.mMinFilter);
    result["max_level"] = texInfo.mMaxLevel;
    result["max_lod"] = texInfo.mMaxLOD;
    result["min_lod"] = texInfo.mMinLOD;
    result["swizzle_r"] = texInfo.mSwizzleR;
    result["swizzle_g"] = texInfo.mSwizzleG;
    result["swizzle_b"] = texInfo.mSwizzleB;
    result["swizzle_a"] = texInfo.mSwizzleA;
    result["swizzle_r_name"] = TexEnumString(texInfo.mSwizzleR);
    result["swizzle_g_name"] = TexEnumString(texInfo.mSwizzleG);
    result["swizzle_b_name"] = TexEnumString(texInfo.mSwizzleB);
    result["swizzle_a_name"] = TexEnumString(texInfo.mSwizzleA);
    result["wrap_s"] = texInfo.mWrapS;
    result["wrap_t"] = texInfo.mWrapT;
    result["wrap_r"] = texInfo.mWrapR;
    result["wrap_s_name"] = TexEnumString(texInfo.mWrapS);
    result["wrap_t_name"] = TexEnumString(texInfo.mWrapT);
    result["wrap_r_name"] = TexEnumString(texInfo.mWrapR);

    retracer::gRetracer.mOptions.mSnapshotPrefix = outputdir + "/";
    if (target == GL_TEXTURE_CUBE_MAP)
    {
        /* If the current texture is a cube map, we create six textures, each representing one of the
         * cube map's faces. Our position in space is essentially (0.0, 0.0, 0.0), while the cube map surrounds us. 
         * We therefore specify vectors in 3D space in order to sample one face at a time.
        */
        GLfloat up_face_vertices[20] = {
            -1.0f, +1.0f,    -1.0f, +1.0f, -1.0f,
            -1.0f, -1.0f,    -1.0f, +1.0f, +1.0f,
            +1.0f, -1.0f,    +1.0f, +1.0f, +1.0f,
            +1.0f, +1.0f,    +1.0f, +1.0f, -1.0f
        };
        GLfloat down_face_vertices[20] = {
            -1.0f, +1.0f,    -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,    -1.0f, -1.0f, +1.0f,
            +1.0f, -1.0f,    +1.0f, -1.0f, +1.0f,
            +1.0f, +1.0f,    +1.0f, -1.0f, -1.0f
        };
        GLfloat right_face_vertices[20] = {
            -1.0f, +1.0f,    +1.0f, +1.0f, +1.0f,
            -1.0f, -1.0f,    +1.0f, -1.0f, +1.0f,
            +1.0f, -1.0f,    +1.0f, -1.0f, -1.0f,
            +1.0f, +1.0f,    +1.0f, +1.0f, -1.0f
        };
        GLfloat left_face_vertices[20] = {
            -1.0f, +1.0f,    -1.0f, +1.0f, +1.0f,
            -1.0f, -1.0f,    -1.0f, -1.0f, +1.0f,
            +1.0f, -1.0f,    -1.0f, -1.0f, -1.0f,
            +1.0f, +1.0f,    -1.0f, +1.0f, -1.0f
        };
        GLfloat forward_face_vertices[20] = {
            -1.0f, +1.0f,    -1.0f, +1.0f, +1.0f,
            -1.0f, -1.0f,    -1.0f, -1.0f, +1.0f,
            +1.0f, -1.0f,    +1.0f, -1.0f, +1.0f,
            +1.0f, +1.0f,    +1.0f, +1.0f, +1.0f
        };
        GLfloat back_face_vertices[20] = {
            -1.0f, +1.0f,    -1.0f, +1.0f, -1.0f,
            -1.0f, -1.0f,    -1.0f, -1.0f, -1.0f,
            +1.0f, -1.0f,    +1.0f, -1.0f, -1.0f,
            +1.0f, +1.0f,    +1.0f, +1.0f, -1.0f
        };
        GLfloat* faces[6] = { &up_face_vertices[0], &down_face_vertices[0], &right_face_vertices[0], &left_face_vertices[0], &forward_face_vertices[0], &back_face_vertices[0] };
        GLuint indices[6] = { 0, 1, 2, 2, 3, 0 };

        Texture cubemap;
        cubemap.handle = traceTextureId;
        result["filenames"] = Json::arrayValue;
        for (int i = 0; i < 6; i++) //For each face
        {
            std::vector<std::string> filenames = glstate::dumpTexture(cubemap, drawCall, faces[i], i, &indices[0]);
            for (const std::string& f : filenames)
            {
                result["filenames"].append(f);
            }
        }
    }
    else
    {
        GLfloat vertices[8] = { 0.0f, 1.0f,
                                0.0f, 0.0f,
                                1.0f, 1.0f,
                                1.0f, 0.0f };
        Texture texture;
        texture.handle = traceTextureId;
        result["filenames"] = Json::arrayValue;
        std::vector<std::string> filenames = glstate::dumpTexture(texture, drawCall, &vertices[0]);
        for (const std::string& f : filenames)
        {
            result["filenames"].append(f);
        }
    }

    // Save each mipmap level
    result["mipmaps"] = Json::arrayValue;
    for (unsigned curMipmapLevel = 0; curMipmapLevel < texInfo.mMipmapSizes.size(); curMipmapLevel++)
    {
        const TextureInfo::MipmapSize mipmapSize = texInfo.mMipmapSizes.at(curMipmapLevel);
        Json::Value entry;

        entry["level"] = curMipmapLevel;
        entry["width"] = mipmapSize.width;
        entry["height"] = mipmapSize.height;
        result["mipmaps"].append(entry);
    }
    return result;
}

static void read_buffer_values(Json::Value& membervalue, const char *ptr, GLenum type, GLuint position)
{
    switch (type)
    {
    case GL_FLOAT:
        membervalue["value"].append(*((GLfloat*)(ptr + position)));
        break;
    case GL_FLOAT_VEC2:
        membervalue["value"].append(*((GLfloat*)(ptr + position)));
        membervalue["value"].append(*((GLfloat*)(ptr + 4 + position)));
        break;
    case GL_FLOAT_VEC3:
        membervalue["value"].append(*((GLfloat*)(ptr + position)));
        membervalue["value"].append(*((GLfloat*)(ptr + 4 + position)));
        membervalue["value"].append(*((GLfloat*)(ptr + 8 + position)));
        break;
    case GL_FLOAT_VEC4:
        membervalue["value"].append(*((GLfloat*)(ptr + position)));
        membervalue["value"].append(*((GLfloat*)(ptr + 4 + position)));
        membervalue["value"].append(*((GLfloat*)(ptr + 8 + position)));
        membervalue["value"].append(*((GLfloat*)(ptr + 12 + position)));
        break;
    default:
        DBG_LOG("Unsupported buffer type for reading out value pos=%u type=0x%04x\n", position, type);
        break;
    }
}

static void saveBlock(GLuint programid, GLenum bufferType, GLenum bufferSubType, GLenum bufferBindingType, GLenum bufferSizeType, GLenum bufferOffsetType, int count, const std::string& name)
{
    if (count == 0)
    {
        return;
    }
    result[name + "s"] = Json::arrayValue; // pluralize name
    for (int i = 0; i < count; i++) // iterate over each block
    {
        DBG_LOG("Saving block %d / %d of type %04x\n", i, count - 1, bufferType);
        Json::Value jsonvalue;
        std::vector<GLenum> properties = { GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE,
                                           GL_REFERENCED_BY_VERTEX_SHADER, GL_REFERENCED_BY_FRAGMENT_SHADER,
                                           GL_REFERENCED_BY_COMPUTE_SHADER, GL_REFERENCED_BY_GEOMETRY_SHADER,
                                           GL_REFERENCED_BY_TESS_CONTROL_SHADER, GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
                                           GL_NUM_ACTIVE_VARIABLES, GL_NAME_LENGTH
                                         };
        std::vector<GLint> values(properties.size(), 0);
        _glGetProgramResourceiv(programid, bufferType, i, properties.size(), properties.data(), values.size(), NULL, values.data());
        jsonvalue["binding"] = values[0];
        jsonvalue["dataSize"] = values[1];
        jsonvalue["refByVS"] = (bool)values[2];
        jsonvalue["refByFS"] = (bool)values[3];
        jsonvalue["refByCS"] = (bool)values[4];
        jsonvalue["refByGS"] = (bool)values[5];
        jsonvalue["refByTCS"] = (bool)values[6];
        jsonvalue["refByTES"] = (bool)values[7];
        jsonvalue["activeVars"] = values[8];
        std::vector<char> cname(values[9] + 1, 0);
        _glGetProgramResourceName(programid, bufferType, i, cname.size(), NULL, cname.data());
        jsonvalue["name"] = std::string(cname.data(), cname.size() - 1);
        jsonvalue["values"] = Json::arrayValue;

        GLint bufferId;
        GLint bufferSize;
        GLint bufferOffset;
        glGetIntegeri_v(bufferBindingType, values[0], &bufferId);
        glGetIntegeri_v(bufferSizeType, values[0], &bufferSize);
        glGetIntegeri_v(bufferOffsetType, values[0], &bufferOffset);
        DBG_LOG("bufferId=%d bufferSize=%d bufferOffset=%d from buffer bound at index %d\n", bufferId, bufferSize, bufferOffset, values[0]);
        if(bufferSize <= 0) //When using glBindBufferBase, GL_BUFFER_SIZE returns 0 -> need to check further
        {
            //Use GL_UNIFORM_BUFFER as binding point (remember state to set it back)
            GLint oldBufferId;
            glGetIntegerv(GL_UNIFORM_BUFFER_BINDING, &oldBufferId);
            glBindBuffer(GL_UNIFORM_BUFFER, bufferId);
            glGetBufferParameteriv(GL_UNIFORM_BUFFER, GL_BUFFER_SIZE, &bufferSize);
            glBindBuffer(GL_UNIFORM_BUFFER, oldBufferId);
            if (bufferSize <= 0) //The size of the current uniform buffer is actually 0
            {
                DBG_LOG("Empty buffer, not saving"); //Should in reality never be reached
                continue;
            }
        }
        const char *ptr = mapBuffer(outputdir + "/" + name, bufferOffset, bufferSize, bufferId, std::to_string(values[0]));

        std::vector<GLint> refs(values[8], 0);
        const GLenum refprop = GL_ACTIVE_VARIABLES;
        _glGetProgramResourceiv(programid, bufferType, i, 1, &refprop, values[8], NULL, refs.data());
        // Finally, for each active variable in the block, get its type, index and value
        for (unsigned j = 0; j < refs.size(); j++)
        {
            Json::Value membervalue;
            std::vector<GLenum> valueproperties = { GL_TYPE, GL_OFFSET, GL_BLOCK_INDEX, GL_NAME_LENGTH };
            std::vector<GLint> varValues(valueproperties.size(), 0);
            _glGetProgramResourceiv(programid, bufferSubType, refs[j], valueproperties.size(), valueproperties.data(), varValues.size(), NULL, varValues.data());
            std::vector<char> varName(varValues[3] + 1, 0);
            glGetProgramResourceName(programid, bufferSubType, refs[j], varName.size(), NULL, varName.data());
            membervalue["name"] = std::string(varName.data(), varName.size() - 1);
            membervalue["type"] = varValues[0];
            membervalue["type_name"] = TexEnumString(varValues[0]);
            membervalue["offset"] = varValues[1];
            membervalue["block_index"] = varValues[2];
            read_buffer_values(membervalue, ptr, varValues[0], varValues[1]);
            jsonvalue["values"].append(membervalue);
        }
        unmapBuffer();

        result[name + "s"].append(jsonvalue);
    }
}

static Json::Value saveProgramIO(GLuint program, GLenum type, int count)
{
    Json::Value ret = Json::arrayValue;
    for (int i = 0; i < count; i++)
    {
        Json::Value v;
        std::vector<GLenum> properties = { GL_LOCATION, GL_TYPE, GL_ARRAY_SIZE, GL_NAME_LENGTH, GL_REFERENCED_BY_VERTEX_SHADER, GL_REFERENCED_BY_FRAGMENT_SHADER };
        std::vector<GLint> values(properties.size(), 0);
        _glGetProgramResourceiv(program, type, i, properties.size(), properties.data(), values.size(), NULL, values.data());
        std::vector<char> name(values[3] + 1, 0);
        glGetProgramResourceName(program, type, i, name.size(), NULL, name.data());
        v["name"] = std::string(name.data(), name.size() - 1);
        v["location"] = values[0];
        v["type"] = values[1];
        v["type_name"] = SafeEnumString(values[1]);
        v["size"] = values[2];
        v["refByVS"] = (bool)values[4];
        v["refByFS"] = (bool)values[5];
        ret.append(v);
    }
    return ret;
}

static Json::Value saveColorAttachments(GLint activeFBO)
{
    Json::Value result = Json::arrayValue;
    for (size_t i = 0; i < colorAttachments.size(); i++)
    {
        Json::Value entry;
        entry["fbo"] = activeFBO;
        entry["attachment"] = "COLOR_ATTACHMENT" + std::to_string(i);
        entry["texture"] = colorAttachments[i];
        result.append(entry);
    }
    return result;
}

static Json::Value saveProgramAttributes(GLuint program, int count)
{
    Json::Value result = Json::arrayValue;
    // TBD - handle vertex buffers properly
    // when using vertex buffers, the offset is reported wrongly (on nvidia) for non-vb queries
    // but not sure when to know when this is the case
    GLint maxVertexBuffers = 0;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &maxVertexBuffers);
    for (int i = 0; i < maxVertexBuffers; i++)
    {
        Json::Value entry;
        GLint vb_binding = 0;
        GLint vb_stride = 0;
        GLint vb_offset = 0;
        GLint vb_divisor = 0;
        glGetIntegeri_v(GL_VERTEX_BINDING_OFFSET, i, &vb_offset);
        glGetIntegeri_v(GL_VERTEX_BINDING_STRIDE, i, &vb_stride);
        glGetIntegeri_v(GL_VERTEX_BINDING_BUFFER, i, &vb_binding);
        glGetIntegeri_v(GL_VERTEX_BINDING_DIVISOR, i, &vb_divisor);
        if (vb_binding != 0)
        {
            DBG_LOG("VB%d binding=%d stride=%d offset=%d divisor=%d\n", i, vb_binding, vb_stride, vb_offset, vb_divisor);
        }
    }
    // Q: iterate up to max value, to get built-ins as well?
    for (int i = 0; i < count; ++i)
    {
        GLsizei length = 0;
        GLint size = 0;
        GLenum type = GL_NONE;
        Json::Value entry;
        GLchar tmp[128];
        memset(tmp, 0, sizeof(tmp));
        _glGetActiveAttrib(program, i, 128, &length, &size, &type, tmp);
        GLint location = _glGetAttribLocation(program, tmp);
        const int idx = location; // not sure why GLES spec says 'index' when it apparently means 'location'
        GLint enabled = GL_FALSE;
        GLint binding = -1;
        GLint elemCount;
        GLint stride;
        GLint typeV;
        GLint normalized;
        GLint integer;
        GLint divisor;
        GLvoid* bufOffsetPtr;
        _glGetVertexAttribiv(idx, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
        _glGetVertexAttribiv(idx, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &binding);
        _glGetVertexAttribiv(idx, GL_VERTEX_ATTRIB_ARRAY_SIZE, &elemCount);
        _glGetVertexAttribiv(idx, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &stride);
        _glGetVertexAttribiv(idx, GL_VERTEX_ATTRIB_ARRAY_TYPE, &typeV);
        _glGetVertexAttribiv(idx, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &normalized);
        _glGetVertexAttribiv(idx, GL_VERTEX_ATTRIB_ARRAY_INTEGER, &integer);
        _glGetVertexAttribiv(idx, GL_VERTEX_ATTRIB_ARRAY_DIVISOR, &divisor);
        _glGetVertexAttribPointerv(idx, GL_VERTEX_ATTRIB_ARRAY_POINTER, &bufOffsetPtr);
        if (binding && _glIsBuffer(binding))
        {
            GLint accessFlags = 0;
            GLint isMapped = 0;
            GLint bufferSize = 0;
            GLint64 mapLength = 0;
            GLint64 mapOffset = 0;
            _glBindBuffer(GL_ARRAY_BUFFER, binding);
            _glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_ACCESS_FLAGS, &accessFlags);
            _glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_MAPPED, &isMapped);
            _glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
            _glGetBufferParameteri64v(GL_ARRAY_BUFFER, GL_BUFFER_MAP_LENGTH, &mapLength);
            _glGetBufferParameteri64v(GL_ARRAY_BUFFER, GL_BUFFER_MAP_OFFSET, &mapOffset);
            entry["buffer_access"] = accessFlags;
            entry["buffer_mapped"] = isMapped;
            entry["buffer_size"] = bufferSize;
            entry["buffer_length"] = (Json::Int64)mapLength;
            entry["buffer_offset"] = (Json::Int64)mapOffset;
            const char *ptr = mapBuffer(outputdir + "/attribs_i" + std::to_string(i), mapOffset, bufferSize, binding, "l" + std::to_string(location) + "_b" + std::to_string(binding));
            entry["values"] = dumpValues(typeV, ptr, bufferSize, stride, bufOffsetPtr, elemCount);
            unmapBuffer();
        }
        entry["offset"] = reinterpret_cast<Json::Int64>(bufOffsetPtr);
        entry["name"] = tmp;
        entry["size"] = size;
        entry["type"] = type;
        entry["location"] = location;
        entry["binding"] = binding;
        entry["enabled"] = enabled;
        entry["vector_size"] = elemCount;
        entry["stride"] = stride;
        entry["vector_type"] = typeV; // type inside array
        entry["vector_type_name"] = SafeEnumString(typeV);
        entry["normalized"] = normalized;
        entry["integer"] = integer;
        entry["divisor"] = divisor;
        entry["type_name"] = SafeEnumString(type);
        result.append(entry);
    }
    return result;
}

// Save to file all resources related to the currently bound program
static void saveProgramInfo(GLuint id)
{
    ProgramState p = { 0 };

    _glGetProgramiv(id, GL_LINK_STATUS, &p.linkStatus);
    _glGetProgramiv(id, GL_ACTIVE_ATTRIBUTES, &p.activeAttributes);
    _glGetProgramInterfaceiv(id, GL_UNIFORM, GL_ACTIVE_RESOURCES, &p.activeUniforms);
    _glGetProgramInterfaceiv(id, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &p.activeUniformBlocks);
    _glGetProgramInterfaceiv(id, GL_ATOMIC_COUNTER_BUFFER, GL_ACTIVE_RESOURCES, &p.activeAtomicCounterBuffers);
    _glGetProgramInterfaceiv(id, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &p.activeInputs);
    _glGetProgramInterfaceiv(id, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, &p.activeOutputs);
    _glGetProgramInterfaceiv(id, GL_TRANSFORM_FEEDBACK_BUFFER, GL_ACTIVE_RESOURCES, &p.activeTransformFeedbackBuffers);
    _glGetProgramInterfaceiv(id, GL_TRANSFORM_FEEDBACK_VARYING, GL_ACTIVE_RESOURCES, &p.activeTransformFeedbackVaryings);
    _glGetProgramInterfaceiv(id, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &p.activeSSBOs);
    _glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &p.activeFBO);
    _glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &p.maxColorAttachments);
    GLint tmpColorAttachment;
    for (int i = 0; i < p.maxColorAttachments; i++)
    {
        _glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &tmpColorAttachment);
        if (tmpColorAttachment == 0)
        {
            //Reach end of color attachments for current fbo
            break;
        }
        colorAttachments.emplace_back(static_cast<GLuint>(tmpColorAttachment));
    }

    if (p.activeTransformFeedbackBuffers) // TBD. for now, these are just here to warn about transform feedback being used
    {
        result["transform_feedback_buffers"] = p.activeTransformFeedbackBuffers;
    }

    if (p.activeTransformFeedbackVaryings)
    {
        result["transform_feedback_varyings"] = p.activeTransformFeedbackVaryings;
    }

    if (p.activeAttributes)
    {
        result["attributes"] = saveProgramAttributes(id, p.activeAttributes);
    }

    if (p.activeUniforms > 0)
    {
        result["uniforms"] = saveProgramUniforms(id, p.activeUniforms);
    }

    if (colorAttachments.size() > 0)
    {
        result["fbo color attachments"] = saveColorAttachments(p.activeFBO);
    }

    if (p.activeInputs)
    {
        result["programInputs"] = saveProgramIO(id, GL_PROGRAM_INPUT, p.activeInputs);
    }

    if (p.activeOutputs)
    {
        result["programOutputs"] = saveProgramIO(id, GL_PROGRAM_OUTPUT, p.activeOutputs);
    }

    GLint vao_binding;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao_binding);
    result["vao_binding"] = vao_binding;
    GLint SSBO_alignment = 0;
    _glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &SSBO_alignment);
    result["SSBO_buffer_offset_alignment"] = SSBO_alignment;
    GLint UBO_alignment = 0;
    _glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &UBO_alignment);
    result["UBO_buffer_offset_alignment"] = UBO_alignment;
    saveBlock(id, GL_SHADER_STORAGE_BLOCK, GL_BUFFER_VARIABLE, GL_SHADER_STORAGE_BUFFER_BINDING, GL_SHADER_STORAGE_BUFFER_SIZE, GL_SHADER_STORAGE_BUFFER_START, p.activeSSBOs, "SSBO");
    saveBlock(id, GL_ATOMIC_COUNTER_BUFFER, GL_UNIFORM, GL_UNIFORM_BUFFER_BINDING, GL_UNIFORM_BUFFER_SIZE, GL_UNIFORM_BUFFER_START, p.activeAtomicCounterBuffers, "atomicCounterBuffer");
    saveBlock(id, GL_UNIFORM_BLOCK, GL_UNIFORM, GL_UNIFORM_BUFFER_BINDING, GL_UNIFORM_BUFFER_SIZE, GL_UNIFORM_BUFFER_START, p.activeUniformBlocks, "UBO");

    // -- SHADERS --
    GLint attachedShaders;
    _glGetProgramiv(id, GL_ATTACHED_SHADERS, &attachedShaders);
    assert(attachedShaders > 0);
    std::vector<GLuint> shaders(attachedShaders);
    _glGetAttachedShaders(id, attachedShaders, 0, shaders.data());
    result["shaders"] = Json::arrayValue;
    for (GLuint shaderid : shaders)
    {
        GLint type = 0;
        GLint sourceLength = 0;
        GLint writtenLength = 0;
        GLint infoLogLength = 0;
        GLint compileStatus = 0;
        _glGetShaderiv(shaderid, GL_SHADER_TYPE, &type);
        _glGetShaderiv(shaderid, GL_SHADER_SOURCE_LENGTH, &sourceLength);
        _glGetShaderiv(shaderid, GL_INFO_LOG_LENGTH, &infoLogLength);
        _glGetShaderiv(shaderid, GL_COMPILE_STATUS, &compileStatus);
        std::vector<char> source(sourceLength + 1);
        _glGetShaderSource(shaderid, sourceLength, &writtenLength, source.data());
        std::string filename = shader_filename(shaderid, type);
        FILE *fp = fopen(filename.c_str(), "w");
        if (fp)
        {
            fwrite(source.data(), source.size(), 1, fp);
            fclose(fp);
        }
        else
        {
            DBG_LOG("Failed to write %s: %s\n", filename.c_str(), strerror(errno));
        }
        Json::Value value;
        value["type"] = type;
        value["type_name"] = SafeEnumString(type);
        value["file"] = filename;
        // TBD write out log message, if any
        result["shaders"].append(value);
    }
}

static void drawCallExtraInfo(const common::CallTM *call, Json::Value &stmt)
{
    if (call->mCallName == "glDrawElementsInstanced")
    {
        stmt["prim_count"] = call->mArgs[4]->GetAsUInt();
    }
    else if (call->mCallName == "glDrawElementsInstancedBaseVertex")
    {
        stmt["prim_count"] = call->mArgs[4]->GetAsUInt();
        stmt["base_vertex"] = call->mArgs[5]->GetAsUInt();
    }
    else if (call->mCallName == "glDrawArraysInstanced")
    {
        stmt["first_index"] = call->mArgs[1]->GetAsUInt();
        stmt["prim_count"] = call->mArgs[3]->GetAsUInt();;
    }
    else if (call->mCallName == "glDrawElementsBaseVertex")
    {
        stmt["base_vertex"] = call->mArgs[4]->GetAsUInt();
    }
    else if (call->mCallName == "glDrawArrays")
    {
        stmt["first_index"] = call->mArgs[1]->GetAsUInt();
    }
    else if (call->mCallName == "glDrawRangeElements")
    {
        stmt["min_index"] = call->mArgs[1]->GetAsUInt();
        stmt["max_index"] = call->mArgs[2]->GetAsUInt();
    }
    else if (call->mCallName == "glDrawRangeElementsBaseVertex")
    {
        stmt["min_index"] = call->mArgs[1]->GetAsUInt();
        stmt["max_index"] = call->mArgs[2]->GetAsUInt();
        stmt["baseVertex"] = call->mArgs[6]->GetAsUInt();
    }
}

static bool retrace()
{
    using namespace retracer;

    retracer::Retracer& retracer = gRetracer;

    for ( ;; retracer.curCallNo++)
    {
        void *fptr = NULL;
        char *src = NULL;
        if (!retracer.mFile.GetNextCall(fptr, retracer.mCurCall, src) || retracer.mFinish)
        {
            // No more calls.
            DBG_LOG("Draw call not found!\n");
            GLWS::instance().Cleanup();
            retracer.CloseTraceFile();
            return true;
        }

        if (retracer.GetCurCallId() == drawCall)
        {
            common::CallTM mCall(retracer.mFile, retracer.GetCurCallId(), retracer.mCurCall);
            common::CallTM *call = &mCall; // analyze tool style

            DBG_LOG("Reached target draw call: %s\n", call->mCallName.c_str());
            checkError("begin");

            if (retracer.getCurTid() != retracer.mOptions.mRetraceTid)
            {
                DBG_LOG("Specified draw call is not on a retraced thread!\n");
                exit(1);
            }

            if (call->mCallName.find("glDraw") == std::string::npos) // quick sanity check
            {
                DBG_LOG("Specified call is not a draw call!\n");
                exit(1);
            }

            // Sync
            _glMemoryBarrier(GL_ALL_BARRIER_BITS);
            _glFlush();
            _glFinish();

            // Generate a unique short-name for output directory
            std::string shortname = result["trace_file"].asString();
            const size_t last_slash_idx = shortname.find_last_of("\\/");
            if (std::string::npos != last_slash_idx)
            {
                shortname.erase(0, last_slash_idx + 1);
            }
            const size_t period_idx = shortname.rfind('.');
            if (std::string::npos != period_idx)
            {
                shortname.erase(period_idx);
            }
            outputdir = shortname + "_f" + std::to_string(retracer.GetCurFrameId()) + "_c" + std::to_string(drawCall);
            mkdir(outputdir.c_str(), 0775);
            DBG_LOG("Making output directory %s\n", outputdir.c_str());

            // Find bound shader
            GLint program = 0;
            _glGetIntegerv(GL_CURRENT_PROGRAM, &program);
            result["frame"] = retracer.GetCurFrameId();
            result["program_id"] = program;

            // Save program resources
            saveProgramInfo(program);

            // Save framebuffer color attachments from the beginning of the renderpass
            GLint oldBind;
            _glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldBind);
            for (GLuint unit : colorAttachments)
            {
                _glBindTexture(GL_TEXTURE_2D, unit);
                result["textures"].append(saveTexture(retracer.getCurrentContext(), retracer.getCurTid(), unit, GL_TEXTURE_BINDING_2D, GL_TEXTURE_2D));
            }
            _glBindTexture(GL_TEXTURE_2D, oldBind);

            // Save used textures
            result["textures"] = Json::arrayValue;
            for (GLuint unit : textures2d)
            {
                result["textures"].append(saveTexture(retracer.getCurrentContext(), retracer.getCurTid(), unit, GL_TEXTURE_BINDING_2D, GL_TEXTURE_2D));
            }
            for (GLuint unit : textures_cubemap)
            {
                result["textures"].append(saveTexture(retracer.getCurrentContext(), retracer.getCurTid(), unit,  GL_TEXTURE_BINDING_CUBE_MAP, GL_TEXTURE_CUBE_MAP));
            }

            int count = drawCallCount(&mCall);
            GLenum valueType = drawCallIndexType(&mCall);
            GLvoid *indices = drawCallIndexPtr(&mCall);
            result["draw"] = Json::Value();
            if (call->mCallName == "glDrawElementsIndirect")
            {
                IndirectDrawElements params;
                GLvoid *bufptr = reinterpret_cast<GLvoid*>(call->mArgs[2]->GetAsUInt64());
                if (getIndirectBuffer(&params, sizeof(params), bufptr))
                {
                    count = params.count;
                    result["draw"]["instance_count"] = params.instanceCount;
                    result["draw"]["first_index"] = params.firstIndex;
                    result["draw"]["base_vertex"] = params.baseVertex;
                }
            }
            else if (call->mCallName == "glDrawArraysIndirect")
            {
                IndirectDrawArrays params;
                GLvoid *bufptr = reinterpret_cast<GLvoid*>(call->mArgs[1]->GetAsUInt64());
                if (getIndirectBuffer(&params, sizeof(params), bufptr))
                {
                    count = params.count;
                    result["draw"]["prim_count"] = params.primCount;
                    result["draw"]["first"] = params.first;
                    // last parameter is always empty for GLES, not baseInstance
                    //result["draw"]["base_instance"] = params.baseInstance;
                }
            }
            result["draw"]["count"] = count;

            if (call->mCallName.find("Elements") != std::string::npos)
            {
                const unsigned bufferSize = _gl_type_size(valueType, count);
                GLint bufferId = 0;
                _glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &bufferId);
                const char *ptr;
                if (bufferId == 0) // buffer is in client memory
                {
                    ptr = reinterpret_cast<const char *>(indices);
                    indices = NULL;
                }
                else
                {
                    GLint bufSize = 0;
                    _glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufSize);
                    assert((unsigned)bufSize >= bufferSize);
                    ptr = mapBuffer(outputdir + "/index", 0, bufSize, bufferId, "buffer");
                }
                result["draw"]["value_type"] = valueType;
                result["draw"]["value_type_name"] = TexEnumString(valueType);
                result["index_buffer_size"] = bufferSize;
                result["index_buffer_offset"] = indices;
                result["index_buffer"] = dumpValues(valueType, ptr, bufferSize,
                                                    0 /* compute stride automatically */, indices,
                                                    1 /* element count is always 1 for IBO */);
                if (bufferId != 0)
                {
                    unmapBuffer();
                }
            }
            drawCallExtraInfo(&mCall, result["draw"]); // add any as-of-yet unhandled parameters to the json

            std::string output_data = result.toStyledString();
            std::string jsonfilename = outputdir + "/result.json";
            FILE *fp = fopen(jsonfilename.c_str(), "w");
            if (!fp)
            {
                DBG_LOG("Failed to open %s: %s\n", jsonfilename.c_str(), strerror(errno));
                abort();
            }
            fwrite(output_data.data(), output_data.size(), 1, fp);
            fclose(fp);

            DBG_LOG("Done saving GL state\n");
            // No more calls.
            GLWS::instance().Cleanup();
            retracer.CloseTraceFile();
            return true;
        }

        if (retracer.getCurTid() != retracer.mOptions.mRetraceTid)
        {
            continue; // we're skipping calls from out file here, too; probably what user wants?
        }

        // Call function
        const char *funcName = retracer.mFile.ExIdToName(retracer.mCurCall.funcId);
        if (fptr && strcmp(funcName, "glDetachShader") != 0)
        {
            (*(RetraceFunc)fptr)(src); // increments src to point to end of parameter block
        }
    }

    // Should never get here, as we return once there are no more calls.
    DBG_LOG("Got to the end of %s somehow. Please report this as a bug.\n", __func__);
    assert(0);

    return true;
}

static bool retraceRange()
{
    using namespace retracer;

    retracer::Retracer& retracer = gRetracer;

    for ( ;; retracer.curCallNo++)
    {
        void *fptr = NULL;
        char *src = NULL;
        if (!retracer.mFile.GetNextCall(fptr, retracer.mCurCall, src) || retracer.mFinish)
        {
            // No more calls.
            DBG_LOG("Draw call not found!\n");
            GLWS::instance().Cleanup();
            retracer.CloseTraceFile();
            return true;
        }

        unsigned int callId = retracer.GetCurCallId();
        common::CallTM mCall(retracer.mFile, callId, retracer.mCurCall);
        common::CallTM *call = &mCall; // analyze tool style
        if (callId >= startCall && callId <= endCall && call->mCallName.find("glDraw") != std::string::npos)
        {
            checkError("begin");

            // Sync
            _glMemoryBarrier(GL_ALL_BARRIER_BITS);
            _glFlush();
            _glFinish();

            // Generate a unique short-name for output directory
            std::string shortname = result["trace_file"].asString();

            const size_t last_slash_idx = shortname.find_last_of("\\/");
            if (std::string::npos != last_slash_idx)
            {
                shortname.erase(0, last_slash_idx + 1);
            }

            const size_t period_idx = shortname.rfind('.');
            if (std::string::npos != period_idx)
            {
                shortname.erase(period_idx);
            }
            outputdir = shortname + "_f" + std::to_string(retracer.GetCurFrameId()) + "_c" + std::to_string(callId);
            mkdir(outputdir.c_str(), 0775);

            // Find bound shader
            GLint program = 0;
            _glGetIntegerv(GL_CURRENT_PROGRAM, &program);
            result["frame"] = retracer.GetCurFrameId();
            result["program_id"] = program;

            // Save program resources
            saveProgramInfo(program);

            // Save framebuffer color attachments
            for (GLuint unit : colorAttachments)
            {
                _glBindTexture(GL_TEXTURE_2D, unit);
                result["textures"].append(saveTexture(retracer.getCurrentContext(), retracer.getCurTid(), unit, GL_TEXTURE_BINDING_2D, GL_TEXTURE_2D));
            }

            // Save used textures
            result["textures"] = Json::arrayValue;
            for (GLuint unit : textures2d)
            {
                result["textures"].append(saveTexture(retracer.getCurrentContext(), retracer.getCurTid(), unit, GL_TEXTURE_BINDING_2D, GL_TEXTURE_2D));
            }
            for (GLuint unit : textures_cubemap)
            {
                result["textures"].append(saveTexture(retracer.getCurrentContext(), retracer.getCurTid(), unit,  GL_TEXTURE_BINDING_CUBE_MAP, GL_TEXTURE_CUBE_MAP));
            }

            int count = drawCallCount(&mCall);
            GLenum valueType = drawCallIndexType(&mCall);
            GLvoid *indices = drawCallIndexPtr(&mCall);
            result["draw"] = Json::Value();
            if (call->mCallName == "glDrawElementsIndirect")
            {
                IndirectDrawElements params;
                GLvoid *bufptr = reinterpret_cast<GLvoid*>(call->mArgs[2]->GetAsUInt64());
                if (getIndirectBuffer(&params, sizeof(params), bufptr))
                {
                    count = params.count;
                    result["draw"]["instance_count"] = params.instanceCount;
                    result["draw"]["first_index"] = params.firstIndex;
                    result["draw"]["base_vertex"] = params.baseVertex;
                }
            }
            else if (call->mCallName == "glDrawArraysIndirect")
            {
                IndirectDrawArrays params;
                GLvoid *bufptr = reinterpret_cast<GLvoid*>(call->mArgs[1]->GetAsUInt64());
                if (getIndirectBuffer(&params, sizeof(params), bufptr))
                {
                    count = params.count;
                    result["draw"]["prim_count"] = params.primCount;
                    result["draw"]["first"] = params.first;
                    // last parameter is always empty for GLES, not baseInstance
                    //result["draw"]["base_instance"] = params.baseInstance;
                }
            }
            result["draw"]["count"] = count;

            if (call->mCallName.find("Elements") != std::string::npos)
            {
                const unsigned bufferSize = _gl_type_size(valueType, count);
                GLint bufferId = 0;
                _glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &bufferId);
                const char *ptr;
                if (bufferId == 0) // buffer is in client memory
                {
                    ptr = reinterpret_cast<const char *>(indices);
                    indices = NULL;
                }
                else
                {
                    GLint bufSize = 0;
                    _glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufSize);
                    assert((unsigned)bufSize >= bufferSize);
                    ptr = mapBuffer(outputdir + "/index", 0, bufSize, bufferId, "buffer");
                }
                result["draw"]["value_type"] = valueType;
                result["draw"]["value_type_name"] = TexEnumString(valueType);
                result["index_buffer_size"] = bufferSize;
                result["index_buffer_offset"] = indices;
                result["index_buffer"] = dumpValues(valueType, ptr, bufferSize,
                                                    0 /* compute stride automatically */, indices,
                                                    1 /* element count is always 1 for IBO */);
                if (bufferId != 0)
                {
                    unmapBuffer();
                }
            }
            drawCallExtraInfo(&mCall, result["draw"]); // add any as-of-yet unhandled parameters to the json

            std::string output_data = result.toStyledString();
            std::string jsonfilename = outputdir + "/result.json";
            FILE *fp = fopen(jsonfilename.c_str(), "w");
            if (!fp)
            {
                DBG_LOG("Failed to open %s: %s\n", jsonfilename.c_str(), strerror(errno));
                abort();
            }
            fwrite(output_data.data(), output_data.size(), 1, fp);
            fclose(fp);

            if (callId == endCall)
            {
                // No more calls.
                DBG_LOG("Done saving GL state\n");
                GLWS::instance().Cleanup();
                retracer.CloseTraceFile();
                return true;
            }
            continue;
        }

        if (retracer.getCurTid() != retracer.mOptions.mRetraceTid)
        {
            continue; // we're skipping calls from out file here, too; probably what user wants?
        }

        // Call function
        const char *funcName = retracer.mFile.ExIdToName(retracer.mCurCall.funcId);
        if (fptr && strcmp(funcName, "glDetachShader") != 0)
        {
            (*(RetraceFunc)fptr)(src); // increments src to point to end of parameter block
        }
    }

    // Should never get here, as we return once there are no more calls.
    DBG_LOG("Got to the end of %s somehow. Please report this as a bug.\n", __func__);
    assert(0);

    return true;
}

static void usage(const char *argv0)
{
    fprintf(stderr,
            "Usage: %s [OPTIONS] <trace file> <call number of draw call>\n"
            "Dump draw state information into directory f<call> in current working directory. Options:\n"
            "\n"
            "  --version Output the version of this program\n"
            "  --offscreen Run offscreen\n"
            "  --noscreen Write to pbuffers\n"
            "  --range START STOP Run specified frame range\n"
            "  --overrideEGL RED GREEN BLUE ALPHA DEPTH STENCIL Set speficified EGL configuration\n"
            "\n"
            , argv0);
}

static bool ParseCommandLine(int argc, char** argv, retracer::RetraceOptions& mOptions)
{
    // Parse all except first (executable name)
    int argIndex = 1;
    for (; argIndex < argc; ++argIndex)
    {
        std::string arg = argv[argIndex];

        if (arg[0] != '-')
        {
            break;
        }
        else if (arg == "--offscreen")
        {
            mOptions.mForceOffscreen = true;
        }
        else if (arg == "--overrideEGL")
        {
            mOptions.mOverrideConfig.red = atoi(argv[++argIndex]);
            mOptions.mOverrideConfig.green = atoi(argv[++argIndex]);
            mOptions.mOverrideConfig.blue = atoi(argv[++argIndex]);
            mOptions.mOverrideConfig.alpha = atoi(argv[++argIndex]);
            mOptions.mOverrideConfig.depth = atoi(argv[++argIndex]);
            mOptions.mOverrideConfig.stencil = atoi(argv[++argIndex]);
        }
        else if (arg == "--noscreen")
        {
            mOptions.mPbufferRendering = true;
        }
        else if (arg == "--version")
        {
            std::cout << "Version:" << PATRACE_VERSION << std::endl;
            exit(0);
        }
        else if (arg == "--range")
        {
            mOptions.mBeginMeasureFrame = atoi(argv[++argIndex]);
            mOptions.mEndMeasureFrame = atoi(argv[++argIndex]);
        }
        else
        {
            DBG_LOG("Error: Unknown option %s\n", arg.c_str());
            usage(argv[0]);
            return false;
        }
    }

    if (argIndex + 2 > argc)
    {
        usage(argv[0]);
        return false;
    }

    if (startCall != 0 && endCall != 0)
    {
        argIndex += 2;
        mOptions.mFileName = argv[argIndex++];
    }
    else
    {
        mOptions.mFileName = argv[argIndex++];
        drawCall = atoi(argv[argIndex++]);
    }
    result["trace_file"] = mOptions.mFileName;
    result["call_number"] = drawCall;
    result["retracer_version"] = PATRACE_VERSION;

    return true;
}

extern "C"
int main(int argc, char** argv)
{
    using namespace retracer;

    if (!ParseCommandLine(argc, argv, gRetracer.mOptions))
    {
        return 1;
    }

    // Register Entries before opening tracefile as sigbook is read there
    common::gApiInfo.RegisterEntries(gles_callbacks);
    common::gApiInfo.RegisterEntries(egl_callbacks);

    // 1. Load defaults from file
    if (!gRetracer.OpenTraceFile(gRetracer.mOptions.mFileName.c_str()))
    {
        DBG_LOG("Failed to open %s\n", gRetracer.mOptions.mFileName.c_str());
        return 1;
    }

    gRetracer.mOptions.mDebug = 1;

    // 3. init egl and gles, using final combination of settings (header + override)
    GLWS::instance().Init(gRetracer.mOptions.mApiVersion);

    if (startCall != 0 && endCall != 0)
    {
        retraceRange();
    }
    else
    {
        retrace();
    }

    // Close and cleanup
    GLWS::instance().Cleanup();

    return 0;
}
