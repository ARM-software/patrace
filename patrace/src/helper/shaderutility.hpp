#ifndef _SHADER_UTILITY_HPP_
#define _SHADER_UTILITY_HPP_

#include <common/os.hpp>
#include <common/gl_extension_supported.hpp>

#include <specs/khronos_enums.hpp>
#include <string>

void printShaderInfoLog(GLuint shader, const std::string shader_name);
void printProgramInfoLog(GLuint program, const std::string program_name);
GLint getUniLoc(GLuint program, const GLchar *name);

#endif
