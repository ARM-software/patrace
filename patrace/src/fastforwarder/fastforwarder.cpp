#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <unistd.h>
#include <ctime>

#include "common/out_file.hpp"
#include "common/image.hpp"
#include "common/trace_model.hpp"

#include "dispatch/eglproc_auto.hpp"
#include "helper/eglsize.hpp"

#include "jsoncpp/include/json/reader.h"
#include "jsoncpp/include/json/writer.h"

#include "tool/utils.hpp"

#include <retracer/cmd_options.hpp>
#include <retracer/config.hpp>
#include <retracer/glws.hpp>
#include <retracer/retrace_api.hpp>
#include <retracer/retracer.hpp>
#include <retracer/state.hpp>

#define FASTFORWARDER_VERSION_MAJOR 0
#define FASTFORWARDER_VERSION_MINOR 0
#define FASTFORWARDER_VERSION_COMMIT_HASH PATRACE_REVISION
#define FASTFORWARDER_VERSION_TYPE PATRACE_VERSION_TYPE

// Enable this to print enum strings in debug messages
#define FF_ENUMSTRING 0
#if FF_ENUMSTRING
    // Also need to add "common_eglstate" to target_link_libraries
    #include "eglstate/common.hpp"
#else
    #define EnumString(x) "<Missing EnumString>"
#endif

// Needed for glGetTexLevelParameteriv and related enums
#include "../../thirdparty/opengl-registry/api/GLES3/gl31.h"
#include "../../thirdparty/opengl-registry/api/GLES2/gl2ext.h"


// Need to fetch the function ourselves to check if it is available
typedef void (GLES_CALLCONVENTION * PFN_GLGETTEXLEVELPARAMETERIV)(GLenum target, GLint level, GLenum pname, GLint *params);
static PFN_GLGETTEXLEVELPARAMETERIV ff_glGetTexLevelParameteriv;

static std::string fastforwarderVersionString()
{
    std::stringstream ss;
    ss << "r" << FASTFORWARDER_VERSION_MAJOR << "p" << FASTFORWARDER_VERSION_MINOR  << " " << FASTFORWARDER_VERSION_COMMIT_HASH << " " << FASTFORWARDER_VERSION_TYPE;
    return ss.str();
}

enum FastForwardOptionFlags
{
    FASTFORWARD_RESTORE_TEXTURES  = 1 << 0,
};

struct FastForwardOptions
{
    std::string mOutputFileName;
    std::string mComment;
    unsigned int mTargetFrame;
    unsigned int mFlags;

    FastForwardOptions()
        : mOutputFileName("fastforward.pat")
        , mComment("")
        , mTargetFrame(0)
        , mFlags(0)
    {}
};

namespace RetraceAndTrim
{
class ScratchBuffer
{
public:
    ScratchBuffer(size_t initialCapacity = 0)
        : mVector()
    {
        resizeToFit(initialCapacity);
    }

    // Only use the returned ptr after making sure the buffer is large enough
    char* bufferPtr()
    {
        return mVector.data();
    }

    // Pointers returned from bufferPtr() previously
    // are invalid after calling this!
    void resizeToFit(size_t len)
    {
        mVector.reserve(len);
    }

private:
    // Noncopyable
    ScratchBuffer(const ScratchBuffer&);
    ScratchBuffer& operator=(const ScratchBuffer&);
    std::vector<char> mVector;
};

// NOTE: This emits commands in the _V4_ trace file format!
class TraceCommandEmitter
{
public:
    enum TexDimension{
        Tex2D,
        Tex3D
    };
    TraceCommandEmitter(common::OutFile& outFile, int threadId)
        : mScratchBuff(0)
        , mOutFile(outFile)
        , mThreadId(threadId)
        // NOTE: getId checks that the ids are valid, and aborts if not.
        , mGlBufferDataId(getId("glBufferData"))
        , mGlBindBufferId(getId("glBindBuffer"))
        , mGlBindFramebufferId(getId("glBindFramebuffer"))
        , mGlTexSubImage2DId(getId("glTexSubImage2D"))
        , mGlTexSubImage3DId(getId("glTexSubImage3D"))
        , mGlActiveTextureId(getId("glActiveTexture"))
        , mGlBindTextureId(getId("glBindTexture"))
        , mGlTexParameteriId(getId("glTexParameteri"))
        , mGlTexParameterfId(getId("glTexParameterf"))
        , mGlPixelStoreiId(getId("glPixelStorei"))
        , mGlEnable(getId("glEnable"))
        , mGlDisable(getId("glDisable"))
        , mGlClear(getId("glClear"))
        , mGlClearColor(getId("glClearColor"))
    {}

    void emitDisable(GLenum cap)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(int));

        char* const bufStart = mScratchBuff.bufferPtr();
        char* dest = bufStart;
        dest = writeBCall(dest, mGlDisable);
        dest = common::WriteFixed<int>(dest, cap); // enum

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitEnable(GLenum cap)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(int));

        char* const bufStart = mScratchBuff.bufferPtr();
        char* dest = bufStart;
        dest = writeBCall(dest, mGlEnable);
        dest = common::WriteFixed<int>(dest, cap); // enum

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitClear(GLbitfield mask)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(int));

        char* const bufStart = mScratchBuff.bufferPtr();
        char* dest = bufStart;
        dest = writeBCall(dest, mGlClear);
        dest = common::WriteFixed<int>(dest, mask);

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(GLfloat) * 4);

        char* const bufStart = mScratchBuff.bufferPtr();
        char* dest = bufStart;
        dest = writeBCall(dest, mGlClearColor);
        dest = common::WriteFixed<float>(dest, red);
        dest = common::WriteFixed<float>(dest, green);
        dest = common::WriteFixed<float>(dest, blue);
        dest = common::WriteFixed<float>(dest, alpha);

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall_vlen) + sizeof(int) * 3 + size + 32);

        char* const bufStart = mScratchBuff.bufferPtr();
        char* tmpBuf = bufStart;

        // Make room for BCall_vlen at start of buffer
        tmpBuf += sizeof(common::BCall_vlen);

        tmpBuf = common::WriteFixed<int>(tmpBuf, target); // enum
        tmpBuf = common::WriteFixed<int>(tmpBuf, size); // literal
        tmpBuf = common::Write1DArray<char>(tmpBuf, (unsigned int)size, (const char*)data); // blob
        tmpBuf = common::WriteFixed<int>(tmpBuf, usage); // enum

        // Write BCall_vlen to bufStart
        int toNext = tmpBuf - bufStart;
        writeBCall_vlen(bufStart, mGlBufferDataId, toNext);

        mOutFile.Write(bufStart, toNext);
    }

    void emitTexSubImage(TexDimension dimension, GLenum target, GLint level,
                         GLint xoffset, GLint yoffset, GLint zoffset,
                         GLsizei width, GLsizei height, GLsizei depth,
                         GLenum format, GLenum type, unsigned int textureSize, const char* data)
    {
        if (dimension == Tex2D)
            mScratchBuff.resizeToFit(sizeof(common::BCall_vlen) + sizeof(int) * 9 + textureSize + 32);
        else if (dimension == Tex3D)
            mScratchBuff.resizeToFit(sizeof(common::BCall_vlen) + sizeof(int) * 11 + textureSize + 32);

        char* const bufStart = mScratchBuff.bufferPtr();
        char* dest = bufStart;

        // Written last (need to know toNext)
        dest += sizeof(common::BCall_vlen);

        dest = common::WriteFixed<int>(dest, target); // enum target
        dest = common::WriteFixed<int>(dest, level); // literal level
        dest = common::WriteFixed<int>(dest, xoffset); // literal xoffset
        dest = common::WriteFixed<int>(dest, yoffset); // literal yoffset
        if (dimension == Tex3D)
            dest = common::WriteFixed<int>(dest, zoffset); // literal zoffset
        dest = common::WriteFixed<unsigned int>(dest, width); // literal width
        dest = common::WriteFixed<unsigned int>(dest, height); // literal height
        if (dimension == Tex3D)
            dest = common::WriteFixed<unsigned int>(dest, depth); // literal depth
        dest = common::WriteFixed<int>(dest, format); // enum format
        dest = common::WriteFixed<int>(dest, type); // enum type
        dest = common::WriteFixed<unsigned int>(dest, common::BlobType);
        dest = common::Write1DArray<char>(dest, textureSize, data);

        // NOTE: written to bufStart, not dest
        int toNext = dest - bufStart;
        if (dimension == Tex2D)
            writeBCall_vlen(bufStart, mGlTexSubImage2DId, toNext);
        else if (dimension == Tex3D)
            writeBCall_vlen(bufStart, mGlTexSubImage3DId, toNext);

        mOutFile.Write(bufStart, toNext);
    }

    void emitPixelStorei(GLenum pname, GLint param)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(int) * 2);

        char* const bufStart = mScratchBuff.bufferPtr();
        char* dest = bufStart;
        dest = writeBCall(dest, mGlPixelStoreiId);
        dest = common::WriteFixed<int>(dest, pname); // enum
        dest = common::WriteFixed<unsigned int>(dest, param); // literal

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitTexParameteri(unsigned int target, GLenum pname, unsigned int param)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(int) * 3 + 32);

        char* bufStart = mScratchBuff.bufferPtr();
        char* dest = bufStart;
        dest = writeBCall(dest, mGlTexParameteriId);
        dest = common::WriteFixed<int>(dest, target); // enum
        dest = common::WriteFixed<int>(dest, pname); // enum
        dest = common::WriteFixed<int>(dest, param); // literal

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitTexParameterf(unsigned int target, GLenum pname, float param)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(int) * 3);

        char* bufStart = mScratchBuff.bufferPtr();
        char* dest = bufStart;
        dest = writeBCall(dest, mGlTexParameterfId);
        dest = common::WriteFixed<int>(dest, target); // enum
        dest = common::WriteFixed<int>(dest, pname); // enum
        dest = common::WriteFixed<float>(dest, param); // literal

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitBindTexture(GLenum target, GLuint tex)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(int) * 2);

        char* const bufStart = mScratchBuff.bufferPtr();
        char* dest = bufStart;
        dest = writeBCall(dest, mGlBindTextureId);
        dest = common::WriteFixed<int>(dest, target); // enum
        dest = common::WriteFixed<unsigned int>(dest, tex); // literal

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitBindFramebuffer(GLenum target, GLint id)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(int) * 2 + 32);

        char* const bufStart = mScratchBuff.bufferPtr();
        char* dest = bufStart;
        dest = writeBCall(dest, mGlBindFramebufferId);
        dest = common::WriteFixed<int>(dest, (int) target); // enum
        dest = common::WriteFixed<unsigned int>(dest, id); // literal

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitBindBuffer(GLenum target, GLint id)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(int) * 2 + 32);

        char* const bufStart = mScratchBuff.bufferPtr();
        char* dest = bufStart;
        dest = writeBCall(dest, mGlBindBufferId);
        dest = common::WriteFixed<int>(dest, (int) target); // enum
        dest = common::WriteFixed<unsigned int>(dest, id); // literal

        mOutFile.Write(bufStart, dest - bufStart);
    }

private:
    ScratchBuffer mScratchBuff;
    common::OutFile& mOutFile;
    int mThreadId;
    int mGlBufferDataId;
    int mGlBindBufferId;
    int mGlBindFramebufferId;
    int mGlTexSubImage2DId;
    int mGlTexSubImage3DId;
    int mGlActiveTextureId;
    int mGlBindTextureId;
    int mGlTexParameteriId;
    int mGlTexParameterfId;
    int mGlPixelStoreiId;
    int mGlEnable;
    int mGlDisable;
    int mGlClear;
    int mGlClearColor;

    int getId(const char* name)
    {
        int id = common::gApiInfo.NameToId(name);

        if (id == 0)
        {
            DBG_LOG("Error: couldn't get sigbook-id for function %s", name);
            os::abort();
        }

        return id;
    }

    char* writeBCall(char* dest, int funcId)
    {
        common::BCall bcall;
        bcall.funcId = funcId;
        bcall.tid = mThreadId;
        bcall.reserved = 0;
        bcall.errNo = 0;

        unsigned int bcallSize = sizeof(bcall);
        memcpy(dest, &bcall, bcallSize);

        return dest + bcallSize;
    }

    char* writeBCall_vlen(char* dest, int funcId, int toNext)
    {
        common::BCall_vlen bcv;
        bcv.funcId = funcId;
        bcv.tid = mThreadId;
        bcv.reserved = 0;
        bcv.errNo = 0;
        bcv.toNext = toNext;

        unsigned int bcallSize = sizeof(bcv);
        memcpy(dest, &bcv, bcallSize);

        return dest + bcallSize;
    }
};

class BufferSaver
{
public:
    static void run(retracer::Context& retracerContext, common::InFile& inFile, common::OutFile& outFile, int threadId)
    {
        const auto buffers = retracerContext.getBufferMap().GetCopy();
        const auto revBuffers = retracerContext.getBufferRevMap().GetCopy();

        // Create helper which adds command to tracefile
        TraceCommandEmitter traceCommandEmitter(outFile, threadId);

        // Read buffer-id bound to GL_ARRAY_BUFFER locally (to restore when
        // done)
        GLint oldBoundBuffer = 0;
        _glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &oldBoundBuffer);

        // Find the corresponding trace-buffer-id as well (to rebind
        // when done in trace)
        GLint oldBoundBufferTrace = 0;
        auto search = revBuffers.find(oldBoundBuffer);
        if (search != revBuffers.end())
            oldBoundBufferTrace = search->second;

        for (const auto it : buffers)
        {
            const unsigned int traceBufferId = it.first;
            const unsigned int retraceBufferId = it.second;

            DBG_LOG("Saving buffer %d\n", traceBufferId);

            // Output glBindBuffer(GL_ARRAY_BUFFER, traceBufferId) to new
            // tracefile.
            traceCommandEmitter.emitBindBuffer(GL_ARRAY_BUFFER, traceBufferId);

            // Output glBufferData to new tracefile
            {
                // Bind buffer
                _glBindBuffer(GL_ARRAY_BUFFER, retraceBufferId);

                // Get buffer info
                GLint64 buffLength = 0;
                _glGetBufferParameteri64v(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &buffLength);
                GLint buffUsage = 0;
                _glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_USAGE, &buffUsage);

                // TODO: Skipping these buffers for now, but the right thing might actually be to output a glBufferData with size 0.
                if (buffLength == 0)
                {
                    DBG_LOG("Got 0 as buffLength. Bug or empty buffer?\n");
                    continue;
                }

                // Map and output to trace
                void* data = _glMapBufferRange(GL_ARRAY_BUFFER, 0, buffLength, GL_MAP_READ_BIT);
                {
                    if (!data)
                    {
                        DBG_LOG("glMapBufferRange returned NULL\n");
                        os::abort();
                    }

                    // Emit glBufferData(GL_ARRAY_BUFFER, len, data, usage);
                    traceCommandEmitter.emitBufferData(GL_ARRAY_BUFFER, buffLength, data, buffUsage);
                }
                _glUnmapBuffer(GL_ARRAY_BUFFER);
            }
        }

        // Rebind previously bound buffer in trace:
        // output glBindBuffer(GL_ARRAY_BUFFER, oldBoundBufferTrace)
        traceCommandEmitter.emitBindBuffer(GL_ARRAY_BUFFER, oldBoundBufferTrace);

        // Bind previously bound buffer locally
        _glBindBuffer(GL_ARRAY_BUFFER, oldBoundBuffer);
    }
};

bool checkError(const std::string &msg)
{
    GLenum error = glGetError();

    if (error == GL_NO_ERROR)
    {
        return false;
    }

    DBG_LOG("glGetError: 0x%x (context/msg: %s)\n", error, msg.c_str());

    return true;
}

class TextureSaver
{
public:
    TextureSaver(retracer::Context& retracerContext, common::InFile& inFile, common::OutFile& outFile, int threadId)
        : mRetracerContext(retracerContext), mOutFile(outFile), mThreadId(threadId), mScratchBuff()
    {
        TexTypeInfo info2d("2D", GL_TEXTURE_2D, GL_TEXTURE_BINDING_2D, TraceCommandEmitter::Tex2D);
        TexTypeInfo info2dArray("2D_Array", GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BINDING_2D_ARRAY, TraceCommandEmitter::Tex3D);
        TexTypeInfo infoCubemap("Cubemap", GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BINDING_CUBE_MAP, TraceCommandEmitter::Tex2D);
        TexTypeInfo infoCubemapArray("Cubemap_Array", GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_BINDING_CUBE_MAP_ARRAY, TraceCommandEmitter::Tex3D);
        texTypeToInfo[Tex2D] = info2d;
        texTypeToInfo[Tex2DArray] = info2dArray;
        texTypeToInfo[TexCubemap] = infoCubemap;
        texTypeToInfo[TexCubemapArray] = infoCubemapArray;
    }

    ~TextureSaver()
    {

    }

    void run()
    {
        checkError("TextureSaver::run begin");

        TraceCommandEmitter traceCommandEmitter(mOutFile, mThreadId);

        const auto textures = mRetracerContext.getTextureMap().GetCopy();
        const auto revTextures = mRetracerContext.getTextureRevMap().GetCopy();
        const auto revBuffers = mRetracerContext.getBufferRevMap().GetCopy();

        // To download textures, we need GL_PIXEL_PACK_BUFFER to be unbound.
        // To upload textures in the trace, we need GL_PIXEL_UNPACK_BUFFER to be unbound.

        // Find currently bound GL_PIXEL_UNPACK_BUFFER in the tracefile
        GLint oldUnpackBuffer = 0;
        _glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &oldUnpackBuffer);

        // Find corresponding buffer-id in the trace-file
        GLint oldUnpackBufferTrace = 0;
        auto search = revBuffers.find(oldUnpackBuffer);
        if (search != revBuffers.end())
            oldUnpackBufferTrace = search->second;
        // Emit command to clear the binding. We restore it when done below.
        traceCommandEmitter.emitBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        // Read buffer-id bound to GL_PIXEL_PACK_BUFFER_BINDING locally (to restore when done)
        GLint oldPackBuffer;
        _glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &oldPackBuffer);

        // Clear binding locally (so we can download texture data)
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

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

        // Save glPixelStorei-params
        for (unsigned int i = 0; i < sizeof(storeParams)/sizeof(storeParams[0]); i++)
        {
            _glGetIntegerv(storeParams[i].e, &storeParams[i].value);
        }

        // Set and emit sane values
        for (unsigned int i = 0; i < sizeof(storeParams)/sizeof(storeParams[0]); i++)
        {
            // Set all to 0...
            GLint value = 0;

            // ... except alignment
            if (storeParams[i].e == GL_UNPACK_ALIGNMENT || storeParams[i].e == GL_PACK_ALIGNMENT)
                value = 1;

            // Set locally
            _glPixelStorei(storeParams[i].e, value);

            // Emit command to trace
            traceCommandEmitter.emitPixelStorei(storeParams[i].e, value);
        }

        // Save textures
        for (const auto it : textures)
        {
            const unsigned int traceTextureId = it.first;
            const unsigned int retraceTextureId = it.second;

            //assert(revTextures[retraceTextureId] == traceTextureId);
            if (revTextures.at(retraceTextureId) != traceTextureId)
            {
                DBG_LOG("WARNING: Reverse texture lookup failed: retrace-ID %d's rev. was %d, but should be %d.\n", retraceTextureId, revTextures.at(retraceTextureId), traceTextureId);
            }

            if (retraceTextureId == 0)
            {
                continue;
            }

            bool ok = false;

            // We don't know the appropriate target for the texture, so we try different kinds
            // until we find a match.
            // These methods will return false if it's not a texture of this type.

            // supports GL_TEXTURE_2D, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_ARRAY textures for now
            for (unsigned int i = Tex2D; i < TexEnd; ++i)
            {
                ok = saveTexture(static_cast<TexType>(i), traceCommandEmitter, traceTextureId, retraceTextureId);
                if (ok)
                    break;
            }

            if (!ok)
            {
                DBG_LOG("DEBUG: Texture %d (retrace-id %d) couldn't be saved.\n", traceTextureId, retraceTextureId);
            }
        }

        // Restore values
        for (unsigned int i = 0; i < sizeof(storeParams)/sizeof(storeParams[0]); i++)
        {
            // Set
            _glPixelStorei(storeParams[i].e, storeParams[i].value);

            // Emit
            traceCommandEmitter.emitPixelStorei(storeParams[i].e, storeParams[i].value);
        }

        // Rebind previously bound UNPACK-buffer in trace
        traceCommandEmitter.emitBindBuffer(GL_PIXEL_UNPACK_BUFFER, oldUnpackBufferTrace);

        // Bind previously bound PACK-buffer locally
        glBindBuffer(GL_PIXEL_PACK_BUFFER, oldPackBuffer);

        checkError("TextureSaver::run end");
    }

private:
    enum TexType
    {
        Tex2D = 0,
        Tex2DArray,
        TexCubemap,
        TexCubemapArray,
        TexEnd
    };

    struct TexTypeInfo
    {
        std::string name;
        GLenum target;
        GLenum bindTarget;
        TraceCommandEmitter::TexDimension texDimension;
        TexTypeInfo()
        {}
        TexTypeInfo(std::string _name, GLenum _target, GLenum _bindTarget, TraceCommandEmitter::TexDimension _texDimension):
            name(_name),
            target(_target),
            bindTarget(_bindTarget),
            texDimension(_texDimension)
        {}
    };

    std::map<TexType, TexTypeInfo> texTypeToInfo;

    bool isArrayTex(TexType type)
    {
        if (type == Tex2D || type == TexCubemap)
            return false;
        return true;
    }

    bool isCubemapTex(TexType type)
    {
        if (type == TexCubemap)
            return true;
        return false;
    }

    struct TextureInfo
    {
        struct MipmapSize
        {
            GLint width;
            GLint height;
            GLint depth;
        };
        std::vector<MipmapSize> mMipmapSizes;

        GLint mInternalFormat;
        bool mIsCompressed;

        // Only set if not compressed!
        // Can be GL_NONE, GL_SIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_FLOAT, GL_INT, and GL_UNSIGNED_INT
        GLint mRedType;
        GLint mGreenType;
        GLint mBlueType;
        GLint mAlphaType;
        GLint mDepthType;

        GLint mRedSize;
        GLint mGreenSize;
        GLint mBlueSize;
        GLint mAlphaSize;
        GLint mDepthSize;
    };

    // Gets info for texture bound to target (e.g. GL_TEXTURE_2D)
    static TextureInfo getTextureInfo(GLuint target)
    {
        if (target == GL_TEXTURE_CUBE_MAP)
            target = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
        checkError("getTextureInfo begin");

        TextureInfo info;
        ff_glGetTexLevelParameteriv(target, 0, GL_TEXTURE_INTERNAL_FORMAT, &info.mInternalFormat);

        GLint isCompressed;
        ff_glGetTexLevelParameteriv(target, 0, GL_TEXTURE_COMPRESSED, &isCompressed);
        if (isCompressed == GL_TRUE)
        {
            info.mIsCompressed = true;
        }
        else
        {
            // _glGetTexLevelParameteriv with GL_TEXTURE_COMPRESSED can return GL_FALSE even if the
            // texture is compressed (e.g. for GL_ETC1_RGB8_OES). So we have to do some additional checks.
            switch (info.mInternalFormat)
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


        // Midgard DDK (4aa1dd85062b) asserts on texture 7 of Egypt HD 25fps (ETC1) when asking for GL_TEXTURE_RED_TYPE.
        // ==>[ASSERT] (eglretrace) : In file: midg/src/helpers/src/mali_midg_pixel_format.c   line: 734 midg_pixel_format_get_info
        // !midg_pixel_format_is_compressed( pfs )
        // .. even though the texture is correctly reported to be compressed when querying GL_TEXTURE_COMPRESSED.
        if (!info.mIsCompressed)
        {
            ff_glGetTexLevelParameteriv(target, 0, GL_TEXTURE_RED_TYPE, &info.mRedType);
            ff_glGetTexLevelParameteriv(target, 0, GL_TEXTURE_GREEN_TYPE, &info.mGreenType);
            ff_glGetTexLevelParameteriv(target, 0, GL_TEXTURE_BLUE_TYPE, &info.mBlueType);
            ff_glGetTexLevelParameteriv(target, 0, GL_TEXTURE_ALPHA_TYPE, &info.mAlphaType);
            ff_glGetTexLevelParameteriv(target, 0, GL_TEXTURE_DEPTH_TYPE, &info.mDepthType);

            ff_glGetTexLevelParameteriv(target, 0, GL_TEXTURE_RED_SIZE, &info.mRedSize);
            ff_glGetTexLevelParameteriv(target, 0, GL_TEXTURE_GREEN_SIZE, &info.mGreenSize);
            ff_glGetTexLevelParameteriv(target, 0, GL_TEXTURE_BLUE_SIZE, &info.mBlueSize);
            ff_glGetTexLevelParameteriv(target, 0, GL_TEXTURE_ALPHA_SIZE, &info.mAlphaSize);
            ff_glGetTexLevelParameteriv(target, 0, GL_TEXTURE_DEPTH_SIZE, &info.mDepthSize);
        }

        // Find number of mipmap levels
        GLint maxLevel;
        GLint baseLevel;
        if (target == GL_TEXTURE_CUBE_MAP_POSITIVE_X) {
            _glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, &maxLevel);
            _glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, &baseLevel);
        }
        else {
            _glGetTexParameteriv(target, GL_TEXTURE_MAX_LEVEL, &maxLevel);
            _glGetTexParameteriv(target, GL_TEXTURE_BASE_LEVEL, &baseLevel);
        }
        DBG_LOG("maxLevel by _glGetTexParameteriv = %d\n", maxLevel);
        DBG_LOG("baseLevel by _glGetTexParameteriv = %d\n", baseLevel);

        // Read size of each mipmap
        for (int i = 0; i <= maxLevel; i++)
        {
            TextureInfo::MipmapSize s;
            ff_glGetTexLevelParameteriv(target, i, GL_TEXTURE_WIDTH,  &s.width);
            ff_glGetTexLevelParameteriv(target, i, GL_TEXTURE_HEIGHT, &s.height);
            ff_glGetTexLevelParameteriv(target, i, GL_TEXTURE_DEPTH, &s.depth);

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

    // This tries to bind texture 'id' to 'target'; return true on success
    // false on failure. Does NOT restore the old binding if it fails!
    bool tryBindTexture(GLenum target, GLuint id)
    {
        // Clear errors
        checkError("tryBindTexture begin");

        // Try to bind texture
        _glBindTexture(target, id);

        GLenum error = glGetError();
        if (error == GL_NO_ERROR)
        {
            // Bingo!
            return true;
        }

        // Must not have been a texture appropriate for this target
        return false;
    }

    // Returns TRUE if the texture is a texture of the type we want (even if the function failed to save it), and FALSE if it isn't
    bool saveTexture(TexType texType, TraceCommandEmitter& traceCommandEmitter, unsigned int traceTextureId, unsigned int retraceTextureId)
    {
        auto typeInfoIter = texTypeToInfo.find(texType);
        if (typeInfoIter == texTypeToInfo.end())
        {
            DBG_LOG("Cannot find texture type %d\n", texType);
            return false;
        }
        const TexTypeInfo &typeInfo = typeInfoIter->second;
        bool isArray = isArrayTex(texType);
        bool isCubemap = isCubemapTex(texType);
        checkError("saveTexture" + typeInfo.name + " begin");

        // Remember currently bound texture
        GLint retraceRestoreTexure;
        _glGetIntegerv(typeInfo.bindTarget, &retraceRestoreTexure);

        // Try to bind the to-be-saved texture to the appropriate target
        if (!tryBindTexture(typeInfo.target, retraceTextureId))
        {
            // It's not this kind of texture; restore binding and return false.
            _glBindTexture(typeInfo.target, retraceRestoreTexure);
            return false;
        }

        // It's a texture of the type we want, so we return true (we attempted to save it)
        bool retval = true;

        // Find corresponding currently-bound texture in trace
        GLint traceRestoreTexture = mRetracerContext.getTextureRevMap().RValue(retraceRestoreTexure);

        DBG_LOG("Looking up %s texture %d (retrace-id %d).\n", typeInfo.name.c_str(), traceTextureId, retraceTextureId);
        DBG_LOG("Texture %d was bound in retracer. This corresponds to %d in trace. This binding will be saved and restored.\n", retraceRestoreTexure, traceRestoreTexture);

        TextureInfo texInfo = getTextureInfo(typeInfo.target);

        if (texInfo.mIsCompressed)
        {
            DBG_LOG("NOTE: Skipped saving texture %d (retrace-id %d) because it is compressed (internalformat is %s (%d)). (Assumption: compressed textures can't be changed after upload.)\n", traceTextureId, retraceTextureId, EnumString(texInfo.mInternalFormat), texInfo.mInternalFormat);
            _glBindTexture(typeInfo.target, retraceRestoreTexure);
            return retval;
        }

        DBG_LOG("Attempting to save %s texture %d (retrace-id %d).\n", typeInfo.name.c_str(), traceTextureId, retraceTextureId);
        DBG_LOG("Texture has red-type:   %s (0x%x)\n", EnumString(texInfo.mRedType), texInfo.mRedType);
        DBG_LOG("Texture has green-type: %s (0x%x)\n", EnumString(texInfo.mGreenType), texInfo.mGreenType);
        DBG_LOG("Texture has blue-type:  %s (0x%x)\n", EnumString(texInfo.mBlueType), texInfo.mBlueType);
        DBG_LOG("Texture has alpha-type: %s (0x%x)\n", EnumString(texInfo.mAlphaType), texInfo.mAlphaType);
        DBG_LOG("Texture has depth-type: %s (0x%x)\n", EnumString(texInfo.mDepthType), texInfo.mDepthType);
        DBG_LOG("Texture has red-size:   (%d)\n", texInfo.mRedSize);
        DBG_LOG("Texture has green-size: (%d)\n", texInfo.mGreenSize);
        DBG_LOG("Texture has blue-size:  (%d)\n", texInfo.mBlueSize);
        DBG_LOG("Texture has alpha-size: (%d)\n", texInfo.mAlphaSize);
        DBG_LOG("Texture has depth-size: (%d)\n", texInfo.mDepthSize);
        DBG_LOG("Texture has internalformat: %s (0x%x)\n", EnumString(texInfo.mInternalFormat), texInfo.mInternalFormat);

        // Write BindTexture
        traceCommandEmitter.emitBindTexture(typeInfo.target, traceTextureId);

        // Save each mipmap level
        const int numMipmapLevels = texInfo.mMipmapSizes.size();
        for (int curMipmapLevel = 0; curMipmapLevel < numMipmapLevels; curMipmapLevel++)
        {
            TextureInfo::MipmapSize mipmapSize = texInfo.mMipmapSizes.at(curMipmapLevel);

            GLint readTexFormat = texInfo.mInternalFormat;
            GLint readTexType = GL_UNSIGNED_BYTE;

            if (texInfo.mInternalFormat == GL_LUMINANCE ||
                texInfo.mInternalFormat == GL_LUMINANCE_ALPHA ||
                texInfo.mInternalFormat == GL_LUMINANCE8_OES ||
                texInfo.mInternalFormat == GL_LUMINANCE4_ALPHA4_OES ||
                texInfo.mInternalFormat == GL_LUMINANCE8_ALPHA8_OES ||
                texInfo.mInternalFormat == GL_LUMINANCE32F_EXT ||
                texInfo.mInternalFormat == GL_LUMINANCE_ALPHA32F_EXT ||
                texInfo.mInternalFormat == GL_LUMINANCE16F_EXT ||
                texInfo.mInternalFormat == GL_LUMINANCE_ALPHA16F_EXT)
            {
                DBG_LOG("NOTE: Skipped saving texture %d (retrace-id %d) because "
                        "its internalformat is %s. It cannot be bound to a FBO. "
                        "(Assumption: it can't be changed after upload.)\n",
                        traceTextureId, retraceTextureId, EnumString(texInfo.mInternalFormat));
                traceCommandEmitter.emitBindTexture(typeInfo.target, traceRestoreTexture);
                _glBindTexture(typeInfo.target, retraceRestoreTexure);
                return retval;
            }
            if (texInfo.mRedType && texInfo.mGreenType && texInfo.mBlueType)
            {
                if (texInfo.mAlphaType)
                    if (texInfo.mInternalFormat == GL_RGBA16UI)
                        readTexFormat = GL_RGBA_INTEGER;
                    else
                        readTexFormat = GL_RGBA;
                else
                    if (texInfo.mInternalFormat == GL_RGB16UI)
                        readTexFormat = GL_RGB_INTEGER;
                    else
                        readTexFormat = GL_RGB;
                if (texInfo.mRedSize == 32 && texInfo.mGreenSize == 32 && texInfo.mBlueSize == 32 && texInfo.mAlphaSize == 32) // GL_RGBAF16
                {
                    readTexType = GL_FLOAT;
                }
                else if (texInfo.mRedSize == 16 && texInfo.mGreenSize == 16 && texInfo.mBlueSize == 16 && texInfo.mAlphaSize == 16) // GL_RGBAF16
                {
                    if (texInfo.mInternalFormat == GL_RGBA16UI)
                        readTexType = GL_UNSIGNED_INT;
                    else
                        readTexType = GL_HALF_FLOAT;
                }
                else if (texInfo.mRedSize == 11 && texInfo.mGreenSize == 11 && texInfo.mBlueSize == 10) // GL_R11G11B10
                {
                    readTexType = GL_UNSIGNED_INT_10F_11F_11F_REV;
                }
                else if (texInfo.mRedSize == 10 && texInfo.mGreenSize == 10 && texInfo.mBlueSize == 10 && texInfo.mAlphaSize == 2) // GL_RGB10_A2
                {
                    readTexType = GL_UNSIGNED_INT_2_10_10_10_REV;
                }
                else if (texInfo.mRedSize == 4 && texInfo.mGreenSize == 4 && texInfo.mBlueSize == 4 && texInfo.mAlphaSize == 4) // GL_RGBA4
                {
                    readTexType = GL_UNSIGNED_SHORT_4_4_4_4;
                }
                else if (texInfo.mRedSize == 5 && texInfo.mGreenSize == 5 && texInfo.mBlueSize == 5 && texInfo.mAlphaSize == 1) // GL_RGB5A_1
                {
                    readTexType = GL_UNSIGNED_SHORT_5_5_5_1;
                }
                else if (texInfo.mRedSize == 5 && texInfo.mGreenSize == 6 && texInfo.mBlueSize == 5) // GL_RGB565
                {
                    readTexType = GL_UNSIGNED_SHORT_5_6_5;
                }
            }
            else
            {
                if (texInfo.mInternalFormat == GL_R16F)
                {
                    readTexFormat = GL_RED_EXT;
                    readTexType = GL_HALF_FLOAT;
                }
                else if (texInfo.mInternalFormat == GL_R32F)
                {
                    readTexFormat = GL_RED;
                    readTexType = GL_FLOAT;
                }
                else if (texInfo.mInternalFormat == GL_RG16F)
                {
                    readTexFormat = GL_RG_EXT;
                    readTexType = GL_HALF_FLOAT;
                }
                else if (texInfo.mInternalFormat == GL_DEPTH_COMPONENT24)
                {
                    readTexFormat = GL_DEPTH_COMPONENT;
                    readTexType = GL_UNSIGNED_INT_24_8;
                }
                else if (texInfo.mInternalFormat == GL_DEPTH_COMPONENT32F)
                {
                    readTexFormat = GL_DEPTH_COMPONENT;
                    readTexType = GL_FLOAT;
                }
                else if (texInfo.mInternalFormat == GL_DEPTH24_STENCIL8)
                {
                    readTexFormat = GL_DEPTH_COMPONENT;
                    readTexType = GL_UNSIGNED_INT_24_8;
                }
                else
                {
                    DBG_LOG("Couldn't figure out format to use for internalformat 0x%x\n", texInfo.mInternalFormat);
                    DBG_LOG("Querying the texture determined it wasn't RGB(A) (using glGetTexLevelParameteriv with GL_TEXTURE_X_TYPE), and those cases aren't handled yet.\n");
                    traceCommandEmitter.emitBindTexture(GL_TEXTURE_2D, traceRestoreTexture);
                    _glBindTexture(GL_TEXTURE_2D, retraceRestoreTexure);
                    return retval;
                }
            }

            size_t textureSize = _gl_image_size(readTexFormat, readTexType, mipmapSize.width, mipmapSize.height, 1, true);
            assert(textureSize > 0);


// TODO: Investigate what's needed to get the data for as many types of textures as possible
#if 0
            if (readTexFormat == GL_ALPHA)
                readTexFormat = GL_RED;

            GLint implReadFormat, implReadType;
            _glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &implReadFormat);
            _glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE,   &implReadType);

            switch(texInfo.mInternalFormat)
            {
            case GL_RGBA:
                readTexFormat = GL_RGBA;
                break;
            case GL_RGB:
                readTexFormat = GL_RGB;
                break;
            default:
                DBG_LOG("Not sure which type to use for glReadPixels for internalformat %x. Trying %x.\n", texInfo.mInternalFormat, readTexFormat);
            }
#endif

            ScratchBuffer texData(textureSize);
            bool readError = false;

            // Read texture data
            checkError("Read texture data begin");

            GLuint fbo = 0;
            GLint prev_fbo = 0;
            GLint prev_readBuff = 0;
            GLint prev_pixelPackBuffer = 0;
            GLenum status = 0;

            // Remember old bindings
            _glGetIntegerv(GL_READ_BUFFER, &prev_readBuff);
            _glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &prev_pixelPackBuffer);
            _glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prev_fbo);

            _glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

            int textureNum = isCubemap ? 6 : mipmapSize.depth;
            for (int i = 0; i < textureNum; ++i)          // all layers of array texture
            {
                _glGenFramebuffers(1, &fbo);
                _glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);

                if (readTexFormat == GL_DEPTH_COMPONENT) {
                    if (isArray) {
                        _glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, retraceTextureId, curMipmapLevel, i);
                    }
                    else {
                        if (texType == TexCubemap) {
                            _glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, retraceTextureId, curMipmapLevel);
                        }
                        else {
                            _glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, retraceTextureId, curMipmapLevel);
                        }
                    }
                }
                else {
                    if (isArray) {
                        _glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, retraceTextureId, curMipmapLevel, i);
                    }
                    else {
                        if (texType == TexCubemap) {
                            _glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, retraceTextureId, curMipmapLevel);
                        }
                        else {
                            _glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, retraceTextureId, curMipmapLevel);
                        }
                    }
                }
                status = _glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
                if (status != GL_FRAMEBUFFER_COMPLETE) {
                    DBG_LOG("glCheckFramebufferStatus error: 0x%x\n", status);
                    readError |= true;
                }

                _glReadBuffer(GL_COLOR_ATTACHMENT0);

                checkError("_glReadPixels begin");
                {
                    DBG_LOG("ReadPixels: w=%d, h=%d, format=0x%x=%s, type=0x%x=%s, data=%p\n", mipmapSize.width, mipmapSize.height, readTexFormat, EnumString(readTexFormat), readTexType, EnumString(readTexType), texData.bufferPtr());
                    _glReadPixels(0, 0, mipmapSize.width, mipmapSize.height, readTexFormat, readTexType, texData.bufferPtr());
                }
                readError |= checkError("_glReadPixels end");

                _glDeleteFramebuffers(1, &fbo);

                // Restore old state
                _glBindBuffer(GL_PIXEL_PACK_BUFFER, prev_pixelPackBuffer);
                _glReadBuffer(prev_readBuff);
                _glBindFramebuffer(GL_READ_FRAMEBUFFER, prev_fbo);

                checkError("Read texture data end");

                // Write glTexSubImage2D for this level
                if(!readError)
                {
                    GLenum target = 0;
                    int zoffset = 0, depth = 0;
                    switch (texType) {
                    case Tex2D:
                        target = GL_TEXTURE_2D;
                        zoffset = 0;
                        depth = 0;
                        break;
                    case Tex2DArray:
                        target = GL_TEXTURE_2D_ARRAY;
                        zoffset = i;
                        depth = 1;
                        break;
                    case TexCubemap:
                        target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
                        zoffset = 0;
                        depth = 0;
                        break;
                    case TexCubemapArray:
                        target = GL_TEXTURE_CUBE_MAP_ARRAY;
                        zoffset = i;
                        depth = 1;
                        break;
                    default:
                        break;
                    }
                    traceCommandEmitter.emitTexSubImage(typeInfo.texDimension, // dimension
                        target,             // target
                        curMipmapLevel,     // level
                        0,                  // xoffset
                        0,                  // yoffset
                        zoffset,            // zoffset
                        mipmapSize.width,   // width
                        mipmapSize.height,  // height
                        depth,              // depth
                        readTexFormat,      // format
                        readTexType,        // type
                        textureSize,
                        (const char*) texData.bufferPtr());
                }
                else
                {
                    DBG_LOG("---->> Failed to save the texture because glReadPixels failed. <<-----\n");
                }
            }
        }

        // Restore old tex in trace
        traceCommandEmitter.emitBindTexture(typeInfo.target, traceRestoreTexture);

        // Restore previously bound texture
        _glBindTexture(typeInfo.target, retraceRestoreTexure);

        checkError("saveTexture" + typeInfo.name + " end");
        return retval;
    }
private:
    retracer::Context& mRetracerContext;
    common::OutFile& mOutFile;
    int mThreadId;
    ScratchBuffer mScratchBuff;
};

} // End RetraceAndTrim namespace

static void injectClear(retracer::Context& retracerContext, common::OutFile& out, retracer::Retracer& retracer)
{
    // Save state
    GLint oldBoundFramebuffer = 0;
    _glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldBoundFramebuffer);
    const auto revFramebuffers = retracerContext.getFramebufferRevMap().GetCopy();
    GLint oldBoundFramebufferTrace = 0;
    auto search = revFramebuffers.find(oldBoundFramebufferTrace);
    if (search != revFramebuffers.end())
        oldBoundFramebufferTrace = search->second;
    GLint oldColors[4];
    GLboolean oldColorMask[4];
    GLboolean stencil;
    GLboolean scissor;
    _glGetBooleanv(GL_STENCIL_TEST, &stencil);
    _glGetBooleanv(GL_SCISSOR_TEST, &scissor);
    _glGetBooleanv(GL_COLOR_WRITEMASK, oldColorMask);
    _glGetIntegerv(GL_COLOR_CLEAR_VALUE, oldColors);

    if (!oldColorMask[0] || !oldColorMask[1] || !oldColorMask[2] || !oldColorMask[3])
    {
        DBG_LOG("ERROR: Color mask will prevent initial glClear\n");
    }

    // Inject glClear
    RetraceAndTrim::TraceCommandEmitter traceCommandEmitter(out, retracer.getCurTid());
    traceCommandEmitter.emitBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    traceCommandEmitter.emitClearColor(1.0, 0.0, 0.0, 1.0); // use ugly red color to highlight errors
    traceCommandEmitter.emitClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    // Restore states
    traceCommandEmitter.emitClearColor(oldColors[0], oldColors[1], oldColors[2], oldColors[3]);
    traceCommandEmitter.emitBindFramebuffer(GL_DRAW_FRAMEBUFFER, oldBoundFramebufferTrace);
    if (stencil)
    {
        traceCommandEmitter.emitEnable(GL_STENCIL_TEST);
    }
    else
    {
        traceCommandEmitter.emitDisable(GL_STENCIL_TEST);
    }
    if (scissor)
    {
        traceCommandEmitter.emitEnable(GL_SCISSOR_TEST);
    }
    else
    {
        traceCommandEmitter.emitDisable(GL_SCISSOR_TEST);
    }
}


static bool retraceAndTrim(common::OutFile &out, const FastForwardOptions& ffOptions, Json::Value& ffJson)
{
    using namespace retracer;

    RetraceAndTrim::ScratchBuffer buffer;
    retracer::Retracer& retracer = gRetracer;

    if (retracer.getFileFormatVersion() <= common::HEADER_VERSION_3)
    {
        // Because they have different binary format for e.g. the pixel-array portion
        // of glTexSubImage2D, and we don't want to write emitter-functions for both the old
        // and new version.
        DBG_LOG("ERROR: Can't retrace traces with header-version <= HEADER_VERSION_3\n");
        os::abort();
    }

    ff_glGetTexLevelParameteriv = (PFN_GLGETTEXLEVELPARAMETERIV) eglGetProcAddress("glGetTexLevelParameteriv");

    if ((ffOptions.mFlags & FASTFORWARD_RESTORE_TEXTURES) && (ff_glGetTexLevelParameteriv == NULL))
    {
        DBG_LOG("ERROR: Couldn't fetch pointer to GLES31 function glGetTexLevelParameteriv, which is needed when creating a fastforward-trace doing texture-restoration.\n");
        os::abort();
    }


    for ( ;; retracer.IncCurCallId())
    {
        void *fptr = NULL;
        char *src = NULL;
        if (!retracer.mFile.GetNextCall(fptr, retracer.mCurCall, src) || retracer.mFinish)
        {
            // No more calls.
            GLWS::instance().Cleanup();
            retracer.CloseTraceFile();
            return true;
        }

        const char *funcName = retracer.mFile.ExIdToName(retracer.mCurCall.funcId);

        if (retracer.GetCurFrameId() == ffOptions.mTargetFrame-1 && strcmp(funcName, "eglSwapBuffers") == 0)
        {
            DBG_LOG("Started saving GL state\n");
            RetraceAndTrim::checkError("RetraceAndTrim state-saving begin");

            // Should have a valid GL state at this point
            char* version = (char*) _glGetString(GL_VERSION);
            ffJson["GL_VERSION"]  = version;
            ffJson["GL_VENDOR"]   = (char*) _glGetString(GL_VENDOR);
            ffJson["GL_RENDERER"] = (char*) _glGetString(GL_RENDERER);

            // Sync
            _glMemoryBarrier(GL_ALL_BARRIER_BITS);
            _glFlush();
            _glFinish();

            // Save buffers
            {
                RetraceAndTrim::BufferSaver::run(retracer.getCurrentContext(), retracer.mFile, out, retracer.getCurTid());
            }

            // Save texture
            if (ffOptions.mFlags & FASTFORWARD_RESTORE_TEXTURES)
            {
                RetraceAndTrim::TextureSaver ts(retracer.getCurrentContext(), retracer.mFile, out, retracer.getCurTid());
                ts.run();
            }

            injectClear(retracer.getCurrentContext(), out, retracer);

            RetraceAndTrim::checkError("RetraceAndTrim state-saving end");
            DBG_LOG("Done saving GL state\n");
        }

        if (retracer.getCurTid() != retracer.mOptions.mRetraceTid)
        {
            continue; // we're skipping calls from out file here, too; probably what user wants?
        }

        // Save calls.
        // Calling the function might modify what's pointed to by src (e.g. ReadStringArray does this),
        // so it's important that we copy the call before actually calling the function.
        if (true)
        {
            bool shouldSkip = (strstr(funcName, "SwapBuffers"))
                || (strstr(funcName, "glDraw") && strcmp(funcName, "glDrawBuffers") != 0) // By excluding glDrawBuffers, glDraw* matches all drawing funcs.
                || (strstr(funcName, "glDispatchCompute")) // Matches glDispatchCompute*
                || (strstr(funcName, "glClearBuffer")) // Matches glClearBuffer*
                || (strcmp(funcName, "glBlitFramebuffer") == 0) // NOTE: strCMP == 0
                || (strcmp(funcName, "glClear") == 0); // NOTE: strCMP == 0
            bool targetFrameOrLater = (retracer.GetCurFrameId() >= ffOptions.mTargetFrame);
            if (strstr(funcName, "SwapBuffers") && (retracer.GetCurFrameId()+1 == ffOptions.mTargetFrame))
            {
                // We save the call before the call is executed, and GetCurFrameId() isn't
                // updated until the call (SwapBuffers) is made. This handles the case where this
                // is the last swap before the target frame, so that it isn't wrongly skipped.
                // (I.e., this swap marks the start of the target frame -- or equivalently, the end
                // of frame 0.)
                targetFrameOrLater = true;
            }


            // Until the target frame, output everything but skipped calls. After that, output everything.
            if (targetFrameOrLater || !shouldSkip)
            {
                // Translate funcId for call to id in current sigbook.
                unsigned short newId = common::gApiInfo.NameToId(funcName);

                common::BCall_vlen outBCall = retracer.mCurCall;
                outBCall.funcId = newId;

                if (outBCall.toNext == 0)
                {
                    // It's really a BCall-struct, so only copy the BCall part of it
                    buffer.resizeToFit(sizeof(common::BCall) + common::gApiInfo.IdToLenArr[newId]);

                    char* curScratch = buffer.bufferPtr();

                    memcpy(curScratch, &outBCall, sizeof(common::BCall));
                    curScratch += sizeof(common::BCall);

                    memcpy(curScratch, src, common::gApiInfo.IdToLenArr[newId] - sizeof(common::BCall));
                    curScratch += common::gApiInfo.IdToLenArr[newId] - sizeof(common::BCall);

                    out.Write(buffer.bufferPtr(), curScratch - buffer.bufferPtr());
                }
                else
                {
                    // It's a BCall_vlen
                    buffer.resizeToFit(sizeof(outBCall) + outBCall.toNext);

                    char* curScratch = buffer.bufferPtr();
                    memcpy(curScratch, &outBCall, sizeof(outBCall));
                    curScratch += sizeof(outBCall);

                    memcpy(curScratch, src, outBCall.toNext - sizeof(outBCall));
                    curScratch += outBCall.toNext - sizeof(outBCall);

                    out.Write(buffer.bufferPtr(), curScratch - buffer.bufferPtr());
                }
            }
        }

        // Call function
        if (fptr)
        {
            (*(RetraceFunc)fptr)(src); // increments src to point to end of parameter block
        }

        // In the retracer, this handles returning at the appropriate time in
        // RetraceUntilSwapBuffers, but we don't care about that here.
        if (retracer.mDoPresentFramebuffer)
        {
            retracer.mDoPresentFramebuffer = false;
        }
    }

    // Should never get here, as we return once there are no more calls.
    DBG_LOG("Got to the end of %s somehow. Please report this as a bug.\n", __func__);
    assert(0);

    return true;
}

static void
usage(const char *argv0)
{
    fprintf(stderr,
        "Usage: %s [OPTIONS] \n"
        "Fastforward tracefile.\n"
        "\n"
        "  --input <input_trace> Target frame to fastforward [REQUIRED]\n"
        "  --output <output_file_name> Where to write fastforwarded trace file [REQUIRED]\n"
        "  --targetFrame <target> The frame number that should be fastforwarded to [REQUIRED]\n"
        "  --offscreen Run in offscreen mode\n"
        "  --noscreen Run in pbuffer output mode\n"
        "  --restoretex When generating a fastforward trace, inject commands to restore the contents of textures to what the would've been when retracing the original. (NOTE: EXPERIMENTAL FEATURE)\n"
        "  --version Output the version of this program\n"
        "\n"
        , argv0);
}

static int readValidValue(const char* v)
{
    char* endptr;
    errno = 0;

    int val = strtol(v, &endptr, 10);

    if(errno)
    {
        perror("strtol");
        exit(1);
    }

    if(endptr == v || *endptr != '\0')
    {
        fprintf(stderr, "Invalid parameter value: %s\n", v);
        exit(1);
    }

    return val;
}

static bool ParseCommandLine(int argc, char** argv, FastForwardOptions& ffOptions, CmdOptions& cmdOpts )
{
    bool gotOutput = false;
    bool gotInput = false;
    bool gotTargetFrame = false;

    // Parse all except first (executable name)
    for (int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];

        if (!strcmp(arg, "--offscreen"))
        {
            cmdOpts.forceOffscreen = true;
        }
        else if (!strcmp(arg, "--input"))
        {
            cmdOpts.fileName = argv[++i];
            gotInput = true;
        }
        else if (!strcmp(arg, "--noscreen"))
        {
            cmdOpts.pbufferRendering = true;
            gotInput = true;
        }
        else if (!strcmp(arg, "--output"))
        {
            ffOptions.mOutputFileName = argv[++i];
            gotOutput = true;
        }
        else if (!strcmp(arg, "--comment"))
        {
            ffOptions.mComment = argv[++i];
        }
        else if (!strcmp(arg, "--targetFrame"))
        {
            ffOptions.mTargetFrame = readValidValue(argv[++i]);
            gotTargetFrame = true;
        }
        else if (!strcmp(arg, "--restoretex"))
        {
            ffOptions.mFlags |= FASTFORWARD_RESTORE_TEXTURES;
        }
        else if (!strcmp(arg, "--version"))
        {
            std::cout << "Version:" << std::endl;
            std::cout << "- Retracer: " PATRACE_VERSION << std::endl;
            std::cout << "- Fastforwarder: " << fastforwarderVersionString() << std::endl;
            exit(0);
        }
        else
        {
            DBG_LOG("error: unknown option %s\n", arg);
            usage(argv[0]);
            return false;
        }
    }

    if (!gotOutput)
    {
        DBG_LOG("error: missing output file name\n");
    }

    if (!gotInput)
    {
        DBG_LOG("error: missing input file name\n");
    }

    if (!gotTargetFrame)
    {
        DBG_LOG("error: missing target frame number\n");
    }

    bool success = gotOutput && gotInput && gotTargetFrame;
    if (!success)
    {
        usage(argv[0]);
    }
    return success;
}

extern "C"
int main(int argc, char** argv)
{
    using namespace retracer;

    CmdOptions cmdOptions;
    FastForwardOptions ffOptions;
    if (!ParseCommandLine(argc, argv, /*out*/ ffOptions, /*out*/ cmdOptions))
    {
        return 1;
    }

    if (cmdOptions.fileName.empty())
    {
        std::cerr << "No trace file name specified.\n";
        usage(argv[0]);
        return 1;
    }

    // Register Entries before opening tracefile as sigbook is read there
    common::gApiInfo.RegisterEntries(gles_callbacks);
    common::gApiInfo.RegisterEntries(egl_callbacks);

    // 1. Load defaults from file
    if (!gRetracer.OpenTraceFile(cmdOptions.fileName.c_str()))
    {
        DBG_LOG("Failed to open %s\n", cmdOptions.fileName.c_str());
        return 1;
    }

    // 2. Now that tracefile is opened and defaults loaded, override
    if (!gRetracer.overrideWithCmdOptions(cmdOptions))
    {
        DBG_LOG("Failed to override Cmd Options\n");
        return 1;
    }

    // 3. init egl and gles, using final combination of settings (header + override)
    GLWS::instance().Init(gRetracer.mOptions.mApiVersion);

    // Create fastforward-part of trace-file JSON header
    Json::Value ffJson(Json::objectValue);

    // Prepare ffJson
    {
        // Target frame
        ffJson["originalFrame"] = ffOptions.mTargetFrame;

        // Misc.
        std::stringstream cmdlineSS;
        cmdlineSS << argv[0];
        for (int i = 1; i < argc; i++)
        {
            cmdlineSS << " " << argv[i];
        }
        ffJson["generationCommand"] = cmdlineSS.str();
        ffJson["comment"]           = ffOptions.mComment;

        // Restoration info
        Json::Value ffRestoreInfoJson(Json::objectValue);
        ffRestoreInfoJson["textures"] = (ffOptions.mFlags & FASTFORWARD_RESTORE_TEXTURES) ? true : false;
        ffRestoreInfoJson["buffers"]  = true;
        ffJson["restoreOptions"] = ffRestoreInfoJson;

        // Version of fastforwarder and retracer
        Json::Value ffVersions(Json::objectValue);
        ffVersions["retracer"]      = PATRACE_VERSION;
        ffVersions["fastforwarder"] = fastforwarderVersionString();
        ffJson["versions"] = ffVersions;
    }

    // Open output file
    common::OutFile out(ffOptions.mOutputFileName.c_str());



    // Do fastforwarding: pass ffJson in case we want to add anything
    retraceAndTrim(out, ffOptions, ffJson);

    // Get existing header
    Json::Value jsonRoot = gRetracer.mFile.getJSONHeader();

    // Add our conversion to the list
    addConversionEntry(jsonRoot, "fastforward", cmdOptions.fileName, ffJson);

    // Serialize header
    Json::FastWriter writer;
    std::string jsonData = writer.write(jsonRoot);

    // Write header to file
    out.WriteHeader(jsonData.c_str(), jsonData.length());

    // Close and cleanup
    out.Close();
    GLWS::instance().Cleanup();

    return 0;
}
