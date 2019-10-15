#pragma once

#include "common/analysis_utility.hpp"
#include "dispatch/eglimports.hpp"

// This file contains functions that call underscored GL functions, and
// useful GL memory structure definitions.

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

struct __attribute__((packed)) IndirectCompute
{
    GLuint x, y,z;
};

struct __attribute__((packed)) IndirectDrawArrays
{
    GLuint count;
    GLuint primCount;
    GLuint first;
    GLuint reservedMustBeZero; // is baseInstance in desktop GL
};

struct __attribute__((packed)) IndirectDrawElements
{
    GLuint count;
    GLuint instanceCount;
    GLuint firstIndex;
    GLint baseVertex;
    GLuint reservedMustBeZero;
};

static inline bool isCompressedFormat(GLenum format)
{
    switch (format)
    {
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
    case GL_COMPRESSED_RGBA_ASTC_4x4:
    case GL_COMPRESSED_RGBA_ASTC_5x4:
    case GL_COMPRESSED_RGBA_ASTC_5x5:
    case GL_COMPRESSED_RGBA_ASTC_6x5:
    case GL_COMPRESSED_RGBA_ASTC_6x6:
    case GL_COMPRESSED_RGBA_ASTC_8x5:
    case GL_COMPRESSED_RGBA_ASTC_8x6:
    case GL_COMPRESSED_RGBA_ASTC_8x8:
    case GL_COMPRESSED_RGBA_ASTC_10x5:
    case GL_COMPRESSED_RGBA_ASTC_10x6:
    case GL_COMPRESSED_RGBA_ASTC_10x8:
    case GL_COMPRESSED_RGBA_ASTC_10x10:
    case GL_COMPRESSED_RGBA_ASTC_12x10:
    case GL_COMPRESSED_RGBA_ASTC_12x12:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12:
    case GL_COMPRESSED_RGBA_ASTC_3x3x3_OES:
    case GL_COMPRESSED_RGBA_ASTC_4x3x3_OES:
    case GL_COMPRESSED_RGBA_ASTC_4x4x3_OES:
    case GL_COMPRESSED_RGBA_ASTC_4x4x4_OES:
    case GL_COMPRESSED_RGBA_ASTC_5x4x4_OES:
    case GL_COMPRESSED_RGBA_ASTC_5x5x4_OES:
    case GL_COMPRESSED_RGBA_ASTC_5x5x5_OES:
    case GL_COMPRESSED_RGBA_ASTC_6x5x5_OES:
    case GL_COMPRESSED_RGBA_ASTC_6x6x5_OES:
    case GL_COMPRESSED_RGBA_ASTC_6x6x6_OES:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x3x3_OES:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x3_OES:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x4_OES:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4x4_OES:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x4_OES:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x5_OES:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5x5_OES:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x5_OES:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x6_OES:
    case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        return true;
    default:
        break;
    }
    return false;
}

static inline GLenum samplerTypeToBindingType(GLenum samplerType)
{
    switch (samplerType)
    {
    case GL_SAMPLER_2D:
    case GL_UNSIGNED_INT_SAMPLER_2D:
    case GL_INT_SAMPLER_2D:
    case GL_SAMPLER_2D_SHADOW:
        return GL_TEXTURE_2D;
    case GL_SAMPLER_3D:
    case GL_INT_SAMPLER_3D:
    case GL_UNSIGNED_INT_SAMPLER_3D:
        return GL_TEXTURE_3D;
    case GL_SAMPLER_CUBE:
    case GL_SAMPLER_CUBE_SHADOW:
    case GL_INT_SAMPLER_CUBE:
    case GL_UNSIGNED_INT_SAMPLER_CUBE:
        return GL_TEXTURE_CUBE_MAP;
    case GL_SAMPLER_2D_ARRAY:
    case GL_INT_SAMPLER_2D_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
    case GL_SAMPLER_2D_ARRAY_SHADOW:
        return GL_TEXTURE_2D_ARRAY;
    }
    return GL_NONE;
}

/// Get the contents of an indirect buffer. Modifies the GL_DRAW_INDIRECT_BUFFER binding point.
bool getIndirectBuffer(void *params, size_t size, const void *indirect);

/// Map a buffer and save its contents to a file. Modifies the GL_COPY_READ_BUFFER binding point.
const char *mapBuffer(const std::string& rootname, unsigned bufferOffset, unsigned bufferSize, GLuint bufferId, const std::string& marker);

/// Unmap a buffer mapped with mapBuffer() above. Uses the GL_COPY_READ_BUFFER binding point.
void unmapBuffer();
