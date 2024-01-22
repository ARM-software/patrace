#include <fcntl.h>
#include <fstream>
#include <unistd.h>
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>

#include "common/gl_utility.hpp"
#include "common/os.hpp"
#include "dispatch/eglproc_auto.hpp"

#include <stdint.h>
#include <string.h>

GLenum interpret_texture_target(GLenum target) // https://www.opengl.org/wiki/Cubemap_Texture
{
    switch (target)
    {
    case  GL_TEXTURE_CUBE_MAP_POSITIVE_X: return GL_TEXTURE_CUBE_MAP;
    case  GL_TEXTURE_CUBE_MAP_NEGATIVE_X: return GL_TEXTURE_CUBE_MAP;
    case  GL_TEXTURE_CUBE_MAP_POSITIVE_Y: return GL_TEXTURE_CUBE_MAP;
    case  GL_TEXTURE_CUBE_MAP_NEGATIVE_Y: return GL_TEXTURE_CUBE_MAP;
    case  GL_TEXTURE_CUBE_MAP_POSITIVE_Z: return GL_TEXTURE_CUBE_MAP;
    case  GL_TEXTURE_CUBE_MAP_NEGATIVE_Z: return GL_TEXTURE_CUBE_MAP;
    default: return target;
    }
}

// This function does not and cannot account for use of primitive restart.
// TBD support adjacency modes
long calculate_primitives(GLenum mode, long vertices, GLuint patchSize)
{
    switch (mode)
    {
    case GL_POINTS:
    case GL_LINE_LOOP:
        return vertices;
    case GL_LINE_STRIP:
        return vertices - 1;
    case GL_TRIANGLE_FAN:
    case GL_TRIANGLE_STRIP:
        return vertices - 2;
    case GL_LINES:
        return vertices / 2;
    case GL_TRIANGLES:
        return vertices / 3;
    case GL_PATCHES:
        return vertices / std::max<GLuint>(1, patchSize);
    default:
        DBG_LOG("Error: Unrecognized draw method\n");
        break;
    }
    return 0;
}

long get_num_output_vertices(GLenum mode, long vertices)
{
    switch (mode)
    {
    case GL_POINTS:
    case GL_LINE_LOOP:
        return vertices;
    case GL_LINE_STRIP:
        return vertices - 1;
    case GL_TRIANGLE_FAN:
    case GL_TRIANGLE_STRIP:
        return (vertices - 2) * 3;
    case GL_LINES:
        return vertices;
    case GL_TRIANGLES:
        return vertices;
    case GL_PATCHES:
        return vertices;
    default:
        DBG_LOG("Error: Unrecognized draw method\n");
        break;
    }
    return 0;
}

bool isImageSamplerType(GLenum type)
{
    return (type == GL_IMAGE_2D || type == GL_IMAGE_3D || type == GL_IMAGE_CUBE || type == GL_IMAGE_2D_ARRAY
            || type == GL_INT_IMAGE_2D || type == GL_INT_IMAGE_3D || type == GL_INT_IMAGE_CUBE || type == GL_INT_IMAGE_2D_ARRAY);
}

bool isUniformSamplerType(GLenum type)
{
    return (type == GL_SAMPLER_2D || type == GL_SAMPLER_3D || type == GL_SAMPLER_CUBE || type == GL_SAMPLER_2D_SHADOW
            || type == GL_SAMPLER_2D_ARRAY || type == GL_SAMPLER_2D_ARRAY_SHADOW || type == GL_SAMPLER_CUBE_SHADOW
            || type == GL_INT_SAMPLER_2D || type == GL_INT_SAMPLER_3D || type == GL_INT_SAMPLER_2D_ARRAY
            || type == GL_UNSIGNED_INT_SAMPLER_2D || type == GL_UNSIGNED_INT_SAMPLER_3D || type == GL_UNSIGNED_INT_SAMPLER_CUBE
            || type == GL_UNSIGNED_INT_SAMPLER_2D_ARRAY);
}

const std::string shader_extension(GLenum type)
{
    switch (type)
    {
    case GL_FRAGMENT_SHADER:
        return ".frag";
    case GL_VERTEX_SHADER:
        return ".vert";
    case GL_GEOMETRY_SHADER:
        return ".geom";
    case GL_TESS_EVALUATION_SHADER:
        return ".tess";
    case GL_TESS_CONTROL_SHADER:
        return ".tscc";
    case GL_COMPUTE_SHADER:
        return ".comp";
    default:
        return ".unknown";
    }
}
