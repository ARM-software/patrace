#ifndef _TEXTURE_HPP
#define _TEXTURE_HPP

#include "dispatch/eglproc_auto.hpp"

#include <iostream>
#include <ostream>

struct Texture
{
    unsigned int handle;
    GLint width;
    GLint height;
    GLint depth;
    GLint rSize;
    GLint gSize;
    GLint bSize;
    GLint aSize;
    GLint dSize;
    GLint rType;
    GLint gType;
    GLint bType;
    GLint aType;
    GLint dType;
    GLint internalFormat;
    GLint valid;
    GLenum target;
    GLenum cubeMapFace;
    GLenum binding;

    static GLenum _targetToBinding(GLenum target)
    {
        switch (target)
        {
        case GL_TEXTURE_2D:
            return GL_TEXTURE_BINDING_2D;
        case GL_TEXTURE_2D_ARRAY:
            return GL_TEXTURE_BINDING_2D_ARRAY;
        case GL_TEXTURE_3D:
            return GL_TEXTURE_BINDING_3D;
        case GL_TEXTURE_CUBE_MAP:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            return GL_TEXTURE_BINDING_CUBE_MAP;
        case GL_TEXTURE_2D_MULTISAMPLE:
            return GL_TEXTURE_BINDING_2D_MULTISAMPLE;
        case 0x8C2A: // GL_TEXTURE_BUFFER:
            return 0x8C2C; // GL_TEXTURE_BINDING_BUFFER;
        case 0x9009: // GL_TEXTURE_CUBE_MAP_ARRAY
            return 0x900A; // GL_TEXTURE_BINDING_CUBE_MAP_ARRAY
        case 0x9102: // GL_TEXTURE_2D_MULTISAMPLE_ARRAY
            return 0x9105; // GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY
        default:
            DBG_LOG("WARNING: Unhandled texture target 0x%04x\n", target);
            return 0;
        }
    }

    bool isTarget(GLenum target)
    {
        GLint oldId = 0;
        _glGetIntegerv(_targetToBinding(target), &oldId);

        _glDisable(GL_DEBUG_OUTPUT_KHR);
        _glBindTexture(target, handle);
        bool retval = _glGetError() != GL_INVALID_OPERATION;
        _glEnable(GL_DEBUG_OUTPUT_KHR);

        _glBindTexture(target, oldId);

        return retval;
    }

    // Queries the texture handle for its parameters (Works in >= GLES3.1 only)
    void update(int level, int face)
    {
        valid = _glIsTexture(handle);

        if (isTarget(GL_TEXTURE_2D))
        {
            target = GL_TEXTURE_2D;
        }
        else if (isTarget(GL_TEXTURE_2D_ARRAY))
        {
            target = GL_TEXTURE_2D_ARRAY;
        }
        else if (isTarget(GL_TEXTURE_3D))
        {
            target = GL_TEXTURE_3D;
        }
        else if (isTarget(GL_TEXTURE_CUBE_MAP))
        {
            switch(face)
            {
                case 0: //up
                    cubeMapFace = GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
                    break;
                case 1: //down
                    cubeMapFace = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
                    break;
                case 2: //left
                    cubeMapFace = GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
                    break;
                case 3: //right
                    cubeMapFace = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
                    break;
                case 4: // forward
                    cubeMapFace = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
                    break;
                case 5: //back
                    cubeMapFace = GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
                    break;
                default:
                    DBG_LOG("Error: face %i is not a valid face number[0-5]", face);
                    return;
            }
            
            target = GL_TEXTURE_CUBE_MAP;
        }

        binding = _targetToBinding(target);

        GLint oldId = 0;
        _glGetIntegerv(binding, &oldId);
        _glBindTexture(target, handle);

        GLenum targetToQuery;
        if (target == GL_TEXTURE_CUBE_MAP)
        {
            targetToQuery = cubeMapFace;
        }
        else
        {
            targetToQuery = target;
        }
        _glGetTexLevelParameteriv(targetToQuery, level, GL_TEXTURE_WIDTH, &width);
        _glGetTexLevelParameteriv(targetToQuery, level, GL_TEXTURE_HEIGHT, &height);
        _glGetTexLevelParameteriv(targetToQuery, level, GL_TEXTURE_DEPTH, &depth);
        _glGetTexLevelParameteriv(targetToQuery, level, GL_TEXTURE_RED_TYPE, &rType);
        _glGetTexLevelParameteriv(targetToQuery, level, GL_TEXTURE_GREEN_TYPE, &gType);
        _glGetTexLevelParameteriv(targetToQuery, level, GL_TEXTURE_BLUE_TYPE, &bType);
        _glGetTexLevelParameteriv(targetToQuery, level, GL_TEXTURE_ALPHA_TYPE, &aType);
        _glGetTexLevelParameteriv(targetToQuery, level, GL_TEXTURE_DEPTH_TYPE, &dType);
        _glGetTexLevelParameteriv(targetToQuery, level, GL_TEXTURE_RED_SIZE, &rSize);
        _glGetTexLevelParameteriv(targetToQuery, level, GL_TEXTURE_GREEN_SIZE, &gSize);
        _glGetTexLevelParameteriv(targetToQuery, level, GL_TEXTURE_BLUE_SIZE, &bSize);
        _glGetTexLevelParameteriv(targetToQuery, level, GL_TEXTURE_ALPHA_SIZE, &aSize);
        _glGetTexLevelParameteriv(targetToQuery, level, GL_TEXTURE_DEPTH_SIZE, &dSize);
        _glGetTexLevelParameteriv(targetToQuery, level, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);

        _glBindTexture(target, oldId);
    }

    // Returns true if parameters match the texture size parameters
    bool hasSize(GLint red, GLint green, GLint blue, GLint alpha, GLint depth)
    {
        return
            rSize == red &&
            gSize == green &&
            bSize == blue &&
            aSize == alpha &&
            dSize == depth;
    }

    std::string bindingAsString() const
    {
        switch (binding)
        {
        case GL_TEXTURE_BINDING_2D:
            return "GL_TEXTURE_BINDING_2D";
        case GL_TEXTURE_BINDING_2D_ARRAY:
            return "GL_TEXTURE_BINDING_2D_ARRAY";
        case GL_TEXTURE_BINDING_3D:
            return "GL_TEXTURE_BINDING_3D";
        case 0x8514:
            return "GL_TEXTURE_BINDING_CUBE_MAP";
        case 0x9104:
            return "GL_TEXTURE_BINDING_2D_MULTISAMPLE";
        case 0x8C2C:
            return "GL_TEXTURE_BINDING_BUFFER";
        case 0x900A:
            return "GL_TEXTURE_BINDING_CUBE_MAP_ARRAY";
        case 0x9105:
            return "GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY";
        default:
            return "UNKNOWN BINDING";
        }
    }
};

inline std::ostream& operator << (std::ostream& o, const Texture& t)
{
    std::ios::fmtflags f = std::cout.flags();

    o << "id: " << t.handle << ", ";
    o << "binding: " << t.bindingAsString() << ", ";
    o << "dim: (" << t.width << ", " << t.height << ", " << t.depth << "), ";
    o << "rgbadSize (" << t.rSize << ", ";
    o << t.gSize << ", ";
    o << t.bSize << ", ";
    o << t.aSize << ", ";
    o << t.dSize << "), ";

    o << std::hex;
    o << "internalFormat: 0x" << t.internalFormat << ", ";
    o << "rgbadType (0x" << t.rType << ", ";
    o << "0x" << t.gType << ", ";
    o << "0x" << t.bType << ", ";
    o << "0x" << t.aType << ", ";
    o << "0x" << t.dType << "), ";

    std::cout.flags(f);
    return o;
}

#endif  /* _TEXTURE_HPP */
