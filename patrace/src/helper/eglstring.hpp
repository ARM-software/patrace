#ifndef _EGL_STRING_HPP_
#define _EGL_STRING_HPP_

#include "common/os.hpp"

#include <algorithm>
#include <cctype>
#include <string>

namespace
{

inline std::string glTypeToString(GLuint type)
{
    switch (type)
    {
    case GL_FLOAT:
        return "float";
    case GL_FLOAT_VEC2:
        return "vec2";
    case GL_FLOAT_VEC3:
        return "vec3";
    case GL_FLOAT_VEC4:
        return "vec4";
    default:
        DBG_LOG("Uknown type\n");
        return "";
    }
}

inline void replaceAll(std::string& str, const std::string& oldStr, const std::string& newStr)
{
    std::string retString;
    size_t pos = 0;

    while ((pos = str.find(oldStr, pos)) != std::string::npos)
    {
        str.replace(pos, oldStr.length(), newStr);
        pos += newStr.size();
    }
}

inline void removeWhiteSpace(std::string& str)
{
    str.erase(std::remove_if(str.begin(), str.end(), isspace), str.end());
}

inline const char *cbsource(GLenum source)
{
    switch (source)
    {
    case GL_DEBUG_SOURCE_API_KHR:
        return "API";
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM_KHR:
        return "WM";
    case GL_DEBUG_SOURCE_SHADER_COMPILER_KHR:
        return "COMPILER";
    case GL_DEBUG_SOURCE_THIRD_PARTY_KHR:
        return "THIRD_PARTY";
    case GL_DEBUG_SOURCE_APPLICATION_KHR:
        return "APP";
    case GL_DEBUG_SOURCE_OTHER_KHR:
    default:
        return "OTHER";
    }
}

inline const char *cbtype(GLenum type)
{
    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR_KHR:
        return "ERROR";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_KHR:
        return "DEPRECATED_BEHAVIOR";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_KHR:
        return "UNDEFINED_BEHAVIOR";
    case GL_DEBUG_TYPE_PORTABILITY_KHR:
        return "PORTABILITY";
    case GL_DEBUG_TYPE_PERFORMANCE_KHR:
        return "PERFORMANCE";
    case GL_DEBUG_TYPE_MARKER_KHR:
        return "MARKER";
    case GL_DEBUG_TYPE_PUSH_GROUP_KHR:
        return "PUSH_GROUP";
    case GL_DEBUG_TYPE_POP_GROUP_KHR:
        return "POP_GROUP";
    case GL_DEBUG_TYPE_OTHER_KHR:
    default:
        return "OTHER";
    }
}

inline const char *cbseverity(GLenum severity)
{
    switch (severity)
    {
    case GL_DEBUG_SEVERITY_NOTIFICATION_KHR:
        return "NOTIFICATION";
    case GL_DEBUG_SEVERITY_LOW_KHR:
        return "LOW";
    case GL_DEBUG_SEVERITY_MEDIUM_KHR:
        return "MEDIUM";
    case GL_DEBUG_SEVERITY_HIGH_KHR:
        return "HIGH";
    default:
        return "UNKNOWN";
    }
}

};

#endif
