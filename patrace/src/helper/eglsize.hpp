#ifndef _EGL_SIZE_HPP_
#define _EGL_SIZE_HPP_

#include <common/os.hpp>
#include <common/gl_extension_supported.hpp>

#include <dispatch/eglimports.hpp>
#include <dispatch/eglproc_auto.hpp>
#include <specs/khronos_enums.hpp>

#ifndef GL_HALF_FLOAT
#define GL_HALF_FLOAT 0x140B
#endif
#ifndef GL_BGR
#define GL_BGR        0x80E0
#endif

#ifndef UINT8_MAX
# define UINT8_MAX              (255)
#endif
#ifndef UINT16_MAX
# define UINT16_MAX             (65535)
#endif
#ifndef UINT32_MAX
# define UINT32_MAX             (4294967295U)
#endif

static inline size_t
_glClearBuffer_size(GLenum buffer)
{
    switch (buffer) {
    case PA_GL_COLOR:
    case PA_GL_FRONT:
    case PA_GL_BACK:
    case PA_GL_LEFT:
    case PA_GL_RIGHT:
    case PA_GL_FRONT_AND_BACK:
        return 4;
    case PA_GL_DEPTH:
    case PA_GL_STENCIL:
        return 1;
    default:
        DBG_LOG("apitrace: warning: %s: unexpected buffer GLenum 0x%04X\n", __FUNCTION__, buffer);
        return 0;
    }
}

static inline size_t
_gl_type_size(GLenum type, GLint size = 1)
{
    switch (type) {
    case GL_BOOL:
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
        return size * 1;
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
    case GL_HALF_FLOAT:
        return size * 2;
    case GL_INT:
    case GL_UNSIGNED_INT:
    case GL_FLOAT:
    case GL_FIXED:
        return size * 4;
    case GL_INT_2_10_10_10_REV:
    case GL_INT_10_10_10_2_OES:
    case GL_UNSIGNED_INT_2_10_10_10_REV:
    case GL_UNSIGNED_INT_10_10_10_2_OES:
    case GL_UNSIGNED_INT_10F_11F_11F_REV:
        // packed
        return 4;
    default:
        DBG_LOG("apitrace: warning: %s: unknown GLenum 0x%04X\n", __FUNCTION__, type);
        return 0;
    }
}

static inline void
_gl_uniform_size(GLenum type, GLenum &elemType, GLint &numElems) {
    switch (type) {
    case GL_FLOAT:
        elemType = GL_FLOAT;
        numElems = 1;
        break;
    case GL_FLOAT_VEC2:
        elemType = GL_FLOAT;
        numElems = 2;
        break;
    case GL_FLOAT_VEC3:
        elemType = GL_FLOAT;
        numElems = 3;
        break;
    case GL_FLOAT_VEC4:
        elemType = GL_FLOAT;
        numElems = 4;
        break;
    case GL_INT:
        elemType = GL_INT;
        numElems = 1;
        break;
    case GL_INT_VEC2:
        elemType = GL_INT;
        numElems = 2;
        break;
    case GL_INT_VEC3:
        elemType = GL_INT;
        numElems = 3;
        break;
    case GL_INT_VEC4:
        elemType = GL_INT;
        numElems = 4;
        break;
    case GL_UNSIGNED_INT:
        elemType = GL_UNSIGNED_INT;
        numElems = 1;
        break;
    case GL_UNSIGNED_INT_VEC2:
        elemType = GL_UNSIGNED_INT;
        numElems = 2;
        break;
    case GL_UNSIGNED_INT_VEC3:
        elemType = GL_UNSIGNED_INT;
        numElems = 3;
        break;
    case GL_UNSIGNED_INT_VEC4:
        elemType = GL_UNSIGNED_INT;
        numElems = 4;
        break;
    case GL_BOOL:
        elemType = GL_BOOL;
        numElems = 1;
        break;
    case GL_BOOL_VEC2:
        elemType = GL_BOOL;
        numElems = 2;
        break;
    case GL_BOOL_VEC3:
        elemType = GL_BOOL;
        numElems = 3;
        break;
    case GL_BOOL_VEC4:
        elemType = GL_BOOL;
        numElems = 4;
        break;
    case GL_FLOAT_MAT2:
        elemType = GL_FLOAT;
        numElems = 2*2;
        break;
    case GL_FLOAT_MAT3:
        elemType = GL_FLOAT;
        numElems = 3*3;
        break;
    case GL_FLOAT_MAT4:
        elemType = GL_FLOAT;
        numElems = 4*4;
        break;
    case GL_FLOAT_MAT2x3:
        elemType = GL_FLOAT;
        numElems = 2*3;
        break;
    case GL_FLOAT_MAT2x4:
        elemType = GL_FLOAT;
        numElems = 2*4;
        break;
    case GL_FLOAT_MAT3x2:
        elemType = GL_FLOAT;
        numElems = 3*2;
        break;
    case GL_FLOAT_MAT3x4:
        elemType = GL_FLOAT;
        numElems = 3*4;
        break;
    case GL_FLOAT_MAT4x2:
        elemType = GL_FLOAT;
        numElems = 4*2;
        break;
    case GL_FLOAT_MAT4x3:
        elemType = GL_FLOAT;
        numElems = 4*3;
        break;
    case GL_SAMPLER_2D:
    case GL_SAMPLER_3D:
    case GL_SAMPLER_CUBE:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_2D_ARRAY:
    case GL_SAMPLER_CUBE_MAP_ARRAY_EXT:
    case GL_SAMPLER_2D_ARRAY_SHADOW:
    case GL_SAMPLER_CUBE_SHADOW:
    case GL_INT_SAMPLER_2D:
    case GL_INT_SAMPLER_3D:
    case GL_INT_SAMPLER_CUBE:
    case GL_INT_SAMPLER_2D_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_2D:
    case GL_UNSIGNED_INT_SAMPLER_3D:
    case GL_UNSIGNED_INT_SAMPLER_CUBE:
    case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
    case GL_UNSIGNED_INT_ATOMIC_COUNTER:
    case GL_IMAGE_2D:
    case GL_IMAGE_3D:
        elemType = GL_INT;
        numElems = 1;
        break;
    default:
        DBG_LOG("warning: unknown GLenum 0x%04X\n", type);
        elemType = GL_NONE;
        numElems = 0;
        return;
    }
}

size_t _gl_param_size(GLenum pname);
size_t paramSizeGlGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname);

static inline unsigned
_gl_format_channels(GLenum format) {
    switch (format) {
    case GL_ALPHA:
    case GL_LUMINANCE:
    case GL_DEPTH_COMPONENT:
    case GL_RED_EXT:    // GL_EXT_texture_rg
    case GL_RED_INTEGER:
    case GL_STENCIL_INDEX:
        return 1;
    case GL_LUMINANCE_ALPHA:
    case GL_DEPTH_STENCIL_OES:
    case GL_RG_EXT:     // GL_EXT_texture_rg
    case GL_RG_INTEGER:
        return 2;
    case GL_RGB:
    case GL_RGB_INTEGER:
    case GL_BGR:
    case GL_SRGB8:
        return 3;
    case GL_RGBA:
    case GL_RGBA_INTEGER:
    case GL_BGRA_EXT:
    case GL_SRGB8_ALPHA8_EXT:
        return 4;
    // invalid formats
    case GL_NONE:
        return 0;
    default:
        DBG_LOG("warning: unexpected format GLenum 0x%04X\n", format);
        return 0;
    }
}

template<class X>
static inline bool
_is_pot(X x) {
    return (x & (x - 1)) == 0;
}

template<class X, class Y>
static inline X
_align(X x, Y y) {
    return (x + (y - 1)) & ~(y - 1);
}

static inline void
_gl_format_size(GLenum format, GLenum type,
                unsigned & bits_per_element, unsigned & bits_per_pixel)
{
    unsigned num_channels = _gl_format_channels(format);

    switch (type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
    case 0x8035: // GL_UNSIGNED_INT_8_8_8_8 - a GL-only format used in Yebis for some reason
        bits_per_element = 8;
        bits_per_pixel = bits_per_element * num_channels;
        break;
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
    case GL_HALF_FLOAT:
    case GL_HALF_FLOAT_OES:
        bits_per_element = 16;
        bits_per_pixel = bits_per_element * num_channels;
        break;
    case GL_INT:
    case GL_UNSIGNED_INT:
    case GL_FLOAT:
        bits_per_element = 32;
        bits_per_pixel = bits_per_element * num_channels;
        break;
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT:
    case GL_UNSIGNED_SHORT_5_5_5_1:
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT:
        bits_per_pixel = bits_per_element = 16;
        break;
    case GL_UNSIGNED_INT_2_10_10_10_REV_EXT:
    case GL_UNSIGNED_INT_24_8_OES:
    //case GL_UNSIGNED_INT_2_10_10_10_REV:
    //case GL_UNSIGNED_INT_24_8:
    case GL_UNSIGNED_INT_10F_11F_11F_REV:
    case GL_UNSIGNED_INT_5_9_9_9_REV:
        bits_per_pixel = bits_per_element = 32;
        break;
    case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
        bits_per_pixel = bits_per_element = 64;
        break;
    // invalid formats
    case GL_NONE:
        bits_per_pixel = bits_per_element = 0;
    default:
        DBG_LOG("warning: unexpected type GLenum 0x%04X\n", type);
        bits_per_pixel = bits_per_element = 0;
        break;
    }
}

static inline size_t
_glClearBufferData_size(GLenum format, GLenum type)
{
    unsigned bits_per_pixel = 0;
    unsigned bits_per_element = 0;
    _gl_format_size(format, type, bits_per_element, bits_per_pixel);
    return (bits_per_pixel + 7)/8;
}

/**
 * "packing" should be true when trying to calculate the size of the buffer needed for e.g. glReadPixels,
 * but false when looking for the number of bytes read by GL from the buffer passed to e.g. glTexImage2D.
 * This influences which of GL_PACK_ALIGNMENT or GL_UNPACK_ALIGNMENT is taken into account.
 */
static inline size_t
_gl_image_size(GLenum format, GLenum type, GLsizei width, GLsizei height, GLsizei depth, bool packing = false) {

    unsigned bits_per_element;
    unsigned bits_per_pixel;
    _gl_format_size(format, type, bits_per_element, bits_per_pixel);

    GLint alignment = 4;
    GLint row_length = 0;
    GLint image_height = 0;
    GLint skip_rows = 0;
    GLint skip_pixels = 0;
    GLint skip_images = 0;

    if (packing)
    {
        _glGetIntegerv(GL_PACK_ALIGNMENT, &alignment);
    }
    else
    {
        _glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
    }

    int gles_version_major = gGlesFeatures.glesVersion() / 100;
    // These are available from GLES3 onwards
    if (gles_version_major >= 3) {
        _glGetIntegerv(GL_UNPACK_ROW_LENGTH,   &row_length);
        _glGetIntegerv(GL_UNPACK_IMAGE_HEIGHT, &image_height);
        _glGetIntegerv(GL_UNPACK_SKIP_ROWS,    &skip_rows);
        _glGetIntegerv(GL_UNPACK_SKIP_PIXELS,  &skip_pixels);
        _glGetIntegerv(GL_UNPACK_SKIP_IMAGES,  &skip_images);
    }

    if (row_length <= 0) {
        row_length = width;
    }

    size_t row_stride = (row_length*bits_per_pixel + 7)/8;

    if ((bits_per_element == 1*8 ||
         bits_per_element == 2*8 ||
         bits_per_element == 4*8 ||
         bits_per_element == 8*8) &&
        (GLint)bits_per_element < alignment*8) {
        row_stride = _align(row_stride, alignment);
    }

    if (image_height <= 0) {
        image_height = height;
    }

    size_t image_stride = image_height*row_stride;

    /*
     * We can't just do
     *
     *   size = depth*image_stride
     *
     * here as that could result in reading beyond the end of the buffer when
     * selecting sub-rectangles via GL_UNPACK_SKIP_*.
     */
    size_t size = (width*bits_per_pixel + 7)/8;
    if (height > 1) {
        size += (height - 1)*row_stride;
    }
    if (depth > 1) {
        size += (depth - 1)*image_stride;
    }

    /* XXX: GL_UNPACK_IMAGE_HEIGHT and GL_UNPACK_SKIP_IMAGES should probably
     * not be considered for pixel rectangles. */

    size += (skip_pixels*bits_per_pixel + 7)/8;
    size += skip_rows*row_stride;
    size += skip_images*image_stride;

    return size;
}

#define _glTexImage1D_size(format, type, width)                _gl_image_size(format, type, width, 1, 1)
#define _glTexImage2D_size(format, type, width, height)        _gl_image_size(format, type, width, height, 1)
#define _glTexImage3D_size(format, type, width, height, depth) _gl_image_size(format, type, width, height, depth)

#define _glTexSubImage1D_size(format, type, width)                _glTexImage1D_size(format, type, width)
#define _glTexSubImage2D_size(format, type, width, height)        _glTexImage2D_size(format, type, width, height)
#define _glTexSubImage3D_size(format, type, width, height, depth) _glTexImage3D_size(format, type, width, height, depth)

template<class T>
static inline size_t
_AttribPairList_size(const T *pAttribList, const T terminator = static_cast<T>(0))
{
    size_t size = 0;

    if (pAttribList) {
        while (pAttribList[size] != terminator) {
            size += 2;
        }
        // terminator also counts
        ++size;
    }

    return size;
}

static inline GLuint
_glDrawArrays_count(GLint first, GLsizei count)
{
    if (!count) {
        return 0;
    }
    return first + count;
}

static inline size_t
_glArrayPointer_size(GLint size, GLenum type, GLsizei stride, GLsizei count)
{
    if (!count)
    {
        return 0;
    }

    size_t elementSize = _gl_type_size(type, size);
    if (!stride) {
        stride = (GLsizei)elementSize;
    }
    return stride*(count - 1) + elementSize;
}

#define _glVertexPointer_size(size, type, stride, count) _glArrayPointer_size(size, type, stride, count)
#define _glNormalPointer_size(type, stride, count) _glArrayPointer_size(3, type, stride, count)
#define _glColorPointer_size(size, type, stride, count) _glArrayPointer_size(size, type, stride, count)
#define _glTexCoordPointer_size(size, type, stride, count) _glArrayPointer_size(size, type, stride, count)
#define _glVertexAttribPointer_size(size, type, normalized, stride, count) _glArrayPointer_size(size, type, stride, count)

static inline size_t _instanced_index_array_size(GLint size, GLenum type, GLsizei stride, GLsizei instancecount, GLsizei divisor)
{
    // The number of bytes a stride contains
    GLsizei realStride = stride;

    // Size an element occupies in bytes
    size_t elementSize = size * _gl_type_size(type);

    if (!stride)
    {
        // When incomming stride is 0, we use the elementSize as the stride.
        realStride = static_cast<GLsizei>(elementSize);
    }

    // The array size is ceil(instancecount / divisor) * stride
    GLsizei numElementsInArray = ((instancecount - 1) / divisor) + 1;

    if (numElementsInArray)
    {
        return (numElementsInArray - 1) * realStride + elementSize;
    }
    else
    {
        return 0;
    }
}

static inline GLuint _glDrawElementsBaseVertex_count(GLsizei count, GLenum type, const GLvoid *indices, GLint basevertex)
{
    GLint element_array_buffer = 0;
    GLint size = 0;

    if (!count)
    {
        return 0;
    }

    _glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &element_array_buffer);
    if (element_array_buffer) // Read indices from index buffer object
    {
        _glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
        GLintptr offset = (GLintptr)indices;
        indices = _glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, offset, size - offset, GL_MAP_READ_BIT);
        if (!indices)
        {
            DBG_LOG("ERROR: Unable to read indices in buffer: %d. Retracing this trace will result in using undefined data!\n", element_array_buffer);
            return 0;
        }
    }
    else if (!indices)
    {
        DBG_LOG("ERROR: No index buffer bound, and no index pointer set for draw call!\n");
        return 0;
    }

    GLboolean restart_enabled = _glIsEnabled(GL_PRIMITIVE_RESTART_FIXED_INDEX);
    while ((_glGetError() == GL_INVALID_ENUM)) ;

    GLuint maxindex = 0;
    GLsizei i;
    if (type == GL_UNSIGNED_BYTE) {
        const GLubyte *p = (const GLubyte *)indices;
        for (i = 0; i < count; ++i) {
            if (p[i] > maxindex && !(restart_enabled && p[i] == UINT8_MAX)) {
                maxindex = p[i];
            }
        }
    } else if (type == GL_UNSIGNED_SHORT) {
        const unsigned short *p = (const unsigned short *)indices;
        for (i = 0; i < count; ++i) {
            if (p[i] > maxindex && !(restart_enabled && p[i] == UINT16_MAX)) {
                maxindex = p[i];
            }
        }
    } else if (type == GL_UNSIGNED_INT) {
        const GLuint *p = (const GLuint *)indices;
        for (i = 0; i < count; ++i) {
            if (p[i] > maxindex && !(restart_enabled && p[i] == UINT32_MAX)) {
                maxindex = p[i];
            }
        }
    } else {
        DBG_LOG("ERROR: Unhandled GLenum 0x%04x\n", type);
    }

    if (element_array_buffer) {
        _glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
    }

    maxindex += basevertex;

    return maxindex + 1;
}

static inline GLuint _glDrawRangeElementsBaseVertex_count(GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex)
{
    if (end < start || count < 0)
    {
        return 0;
    }
    return end + basevertex + 1;
}

#define _glDrawElementsBaseVertexOES_count(mode, count, type, indices, basevertex) _glDrawElementsBaseVertex_count(count, type, indices, basevertex)
#define _glDrawElementsBaseVertexEXT_count(mode, count, type, indices, basevertex) _glDrawElementsBaseVertex_count(count, type, indices, basevertex)
#define _glDrawRangeElementsBaseVertex_count(start, end, count, type, indices, basevertex) _glDrawRangeElements_count(start, end, count, type, indices)
#define _glDrawRangeElementsBaseVertexEXT_count(start, end, count, type, indices, basevertex) _glDrawRangeElements_count(start, end, count, type, indices)
#define _glDrawRangeElementsBaseVertexOES_count(start, end, count, type, indices, basevertex) _glDrawRangeElements_count(start, end, count, type, indices)
#define _glDrawElementsInstancedBaseVertex_count(count, type, indices, primcount, basevertex) _glDrawElementsBaseVertex_count(count, type, indices, basevertex)
#define _glDrawElementsInstancedBaseVertexOES_count(count, type, indices, primcount, basevertex) _glDrawElementsBaseVertex_count(count, type, indices, basevertex)
#define _glDrawElementsInstancedBaseVertexEXT_count(count, type, indices, primcount, basevertex) _glDrawElementsBaseVertex_count(count, type, indices, basevertex)
#define _glDrawArraysInstanced_count(first, count, primcount) _glDrawArrays_count(first, count)
#define _glDrawElements_count(count, type, indices) _glDrawElementsBaseVertex_count(count, type, indices, 0);
#define _glDrawRangeElements_count(start, end, count, type, indices) _glDrawElements_count(count, type, indices)
#define _glDrawElementsInstanced_count(count, type, indices, primcount) _glDrawElements_count(count, type, indices)
#define _glDrawArraysInstancedBaseInstanceEXT(mode, first, count, instancecount, baseinstance) _glDrawArrays_count(first, count)
#define _glDrawElementsInstancedBaseInstanceEXT(mode, count, type, indices, instancecount, baseinstance) _glDrawElementsBaseVertex_count(count, type, indices, 0)
#define _glDrawElementsInstancedBaseVertexBaseInstanceEXT(mode, count, type, indices, instancecount, basevertex, baseinstance) _glDrawElementsBaseVertex_count(count, type, indices, basevertex)

struct image_info
{
    GLint internalformat;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    GLsizei size;
    GLvoid* pixels;

    image_info()
    :internalformat(0),
     width(0),
     height(0),
     format(0),
     type(0),
     size(0),
     pixels(0)
    {}
};

struct image_info* _EGLImageKHR_get_image_info(EGLImageKHR image, GLenum target, int width, int height);

void _EGLImageKHR_free_image_info(struct image_info *info);

#endif
