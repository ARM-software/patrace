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

// Return _a_ valid type for a given internalformat. There may be others that are also valid.
static inline GLenum sized_to_unsized_type(GLenum format)
{
    switch (format)
    {
    case GL_R8: return GL_UNSIGNED_BYTE;
    case GL_R8_SNORM: return GL_BYTE;
    case GL_R16F: return GL_FLOAT; // or GL_HALF_FLOAT
    case GL_R32F: return GL_FLOAT;
    case GL_R8UI: return GL_UNSIGNED_BYTE;
    case GL_R8I: return GL_BYTE;
    case GL_R16UI: return GL_UNSIGNED_SHORT;
    case GL_R16I: return GL_SHORT;
    case GL_R32UI: return GL_UNSIGNED_INT;
    case GL_R32I: return GL_INT;
    case GL_RG8: return GL_UNSIGNED_BYTE;
    case GL_RG8_SNORM: return GL_BYTE;
    case GL_RG16F: return GL_FLOAT; // or GL_HALF_FLOAT
    case GL_RG32F: return GL_FLOAT;
    case GL_RG8UI: return GL_UNSIGNED_BYTE;
    case GL_RG8I: return GL_BYTE;
    case GL_RG16UI: return GL_UNSIGNED_SHORT;
    case GL_RG16I: return GL_SHORT;
    case GL_RG32UI: return GL_UNSIGNED_INT;
    case GL_RG32I: return GL_INT;
    case GL_RGB8: return GL_UNSIGNED_BYTE;
    case GL_SRGB8: return GL_UNSIGNED_BYTE;
    case GL_RGB565: return GL_UNSIGNED_SHORT_5_6_5; // or GL_UNSIGNED_BYTE
    case GL_RGB8_SNORM: return GL_BYTE;
    case GL_R11F_G11F_B10F: return GL_UNSIGNED_INT_10F_11F_11F_REV; // or GL_HALF_FLOAT, GL_FLOAT
    case GL_RGB9_E5: return GL_UNSIGNED_INT_5_9_9_9_REV; // or GL_HALF_FLOAT, GL_FLOAT
    case GL_RGB16F: return GL_FLOAT; // or GL_HALF_FLOAT
    case GL_RGB32F: return GL_FLOAT;
    case GL_RGB8UI: return GL_UNSIGNED_BYTE;
    case GL_RGB8I: return GL_BYTE;
    case GL_RGB16UI: return GL_UNSIGNED_SHORT;
    case GL_RGB16I: return GL_SHORT;
    case GL_RGB32UI: return GL_UNSIGNED_INT;
    case GL_RGB32I: return GL_INT;
    case GL_RGBA8: return GL_UNSIGNED_BYTE;
    case GL_SRGB8_ALPHA8: return GL_UNSIGNED_BYTE;
    case GL_RGBA8_SNORM: return GL_BYTE;
    case GL_RGB5_A1: return GL_UNSIGNED_SHORT_5_5_5_1; // or GL_UNSIGNED_INT_2_10_10_10_REV, GL_UNSIGNED_BYTE
    case GL_RGBA4: return GL_UNSIGNED_SHORT_4_4_4_4; // or GL_UNSIGNED_BYTE
    case GL_RGB10_A2: return GL_UNSIGNED_INT_2_10_10_10_REV;
    case GL_RGBA16F: return GL_FLOAT; // orGL_HALF_FLOAT
    case GL_RGBA32F: return GL_FLOAT;
    case GL_RGBA8UI: return GL_UNSIGNED_BYTE;
    case GL_RGBA8I: return GL_BYTE;
    case GL_RGB10_A2UI: return GL_UNSIGNED_INT_2_10_10_10_REV;
    case GL_RGBA16UI: return GL_UNSIGNED_SHORT;
    case GL_RGBA16I: return GL_SHORT;
    case GL_RGBA32I: return GL_INT;
    case GL_RGBA32UI: return GL_UNSIGNED_INT;
    case GL_DEPTH_COMPONENT16: return GL_UNSIGNED_INT; // or GL_UNSIGNED_SHORT
    case GL_DEPTH_COMPONENT24: return GL_UNSIGNED_INT;
    case GL_DEPTH_COMPONENT32F: return GL_FLOAT;
    case GL_DEPTH24_STENCIL8: return GL_UNSIGNED_INT_24_8;
    case GL_DEPTH32F_STENCIL8: return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
    default: abort(); break;
    }
}

static inline GLenum sized_to_unsized_format(GLenum format)
{
    switch (format)
    {
    case GL_R8: return GL_RED;
    case GL_R8_SNORM: return GL_RED;
    case GL_R16F: return GL_RED;
    case GL_R32F: return GL_RED;
    case GL_R8UI: return GL_RED_INTEGER;
    case GL_R8I: return GL_RED_INTEGER;
    case GL_R16UI: return GL_RED_INTEGER;
    case GL_R16I: return GL_RED_INTEGER;
    case GL_R32UI: return GL_RED;
    case GL_R32I: return GL_RED_INTEGER;
    case GL_RG8: return GL_RG;
    case GL_RG8_SNORM: return GL_RG;
    case GL_RG16F: return GL_RG;
    case GL_RG32F: return GL_RG;
    case GL_RG8UI: return GL_RG_INTEGER;
    case GL_RG8I: return GL_RG_INTEGER;
    case GL_RG16UI: return GL_RG_INTEGER;
    case GL_RG16I: return GL_RG_INTEGER;
    case GL_RG32UI: return GL_RG_INTEGER;
    case GL_RG32I: return GL_RG_INTEGER;
    case GL_RGB8: return GL_RGB;
    case GL_SRGB8: return GL_RGB;
    case GL_RGB565: return GL_RGB;
    case GL_RGB8_SNORM: return GL_RGB;
    case GL_R11F_G11F_B10F: return GL_RGB;
    case GL_RGB9_E5: return GL_RGB;
    case GL_RGB16F: return GL_RGB;
    case GL_RGB32F: return GL_RGB;
    case GL_RGB8UI: return GL_RGB_INTEGER;
    case GL_RGB8I: return GL_RGB_INTEGER;
    case GL_RGB16UI: return GL_RGB_INTEGER;
    case GL_RGB16I: return GL_RGB_INTEGER;
    case GL_RGB32UI: return GL_RGB_INTEGER;
    case GL_RGB32I: return GL_RGB_INTEGER;
    case GL_RGBA8: return GL_RGBA;
    case GL_SRGB8_ALPHA8: return GL_RGBA;
    case GL_RGBA8_SNORM: return GL_RGBA;
    case GL_RGB5_A1: return GL_RGBA;
    case GL_RGBA4: return GL_RGBA;
    case GL_RGB10_A2: return GL_RGBA;
    case GL_RGBA16F: return GL_RGBA;
    case GL_RGBA32F: return GL_RGBA;
    case GL_RGBA8UI: return GL_RGBA_INTEGER;
    case GL_RGBA8I: return GL_RGBA_INTEGER;
    case GL_RGB10_A2UI: return GL_RGBA_INTEGER;
    case GL_RGBA16UI: return GL_RGBA_INTEGER;
    case GL_RGBA16I: return GL_RGBA_INTEGER;
    case GL_RGBA32I: return GL_RGBA_INTEGER;
    case GL_RGBA32UI: return GL_RGBA_INTEGER;
    case GL_DEPTH_COMPONENT16: return GL_DEPTH_COMPONENT;
    case GL_DEPTH_COMPONENT24: return GL_DEPTH_COMPONENT;
    case GL_DEPTH_COMPONENT32F: return GL_DEPTH_COMPONENT;
    case GL_DEPTH24_STENCIL8: return GL_DEPTH_STENCIL;
    case GL_DEPTH32F_STENCIL8: return GL_DEPTH_STENCIL;
    default: abort(); break;
    }
}

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
