#include "shaderutility.hpp"

void printShaderInfoLog(GLuint shader, const std::string shader_name)
{
	int infologLen = 0;
	int charsWritten = 0;
	_glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLen);
	if (infologLen > 0) {
		GLchar *infoLog = new GLchar[infologLen];
		if (infoLog == NULL)
		{
            DBG_LOG("ERROR: cannot create shader InfoLog cache\n");
            return;
		}
		_glGetShaderInfoLog(shader, infologLen, &charsWritten, infoLog);
        DBG_LOG("%s compilation information: %s\n", shader_name.c_str(), infoLog);
		delete []infoLog;
	}
}

void printProgramInfoLog(GLuint program, const std::string program_name)
{
	int infologLen = 0;
	int charsWritten = 0;
	_glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLen);
	if (infologLen > 0) {
		GLchar *infoLog = new GLchar[infologLen];
		if (infoLog == NULL)
		{
            DBG_LOG("ERROR: cannot create program InfoLog cache\n");
            return;
		}
		glGetProgramInfoLog(program, infologLen, &charsWritten, infoLog);
        DBG_LOG("shader program %s's linking information: %s\n", program_name.c_str(), infoLog);
		delete []infoLog;
	}
}

GLint getUniLoc(GLuint program, const GLchar *name)
{
    GLint loc;
    loc = _glGetUniformLocation(program, name);
    if (loc == -1)
        DBG_LOG("unifor parameter %s hasn't been defined.\n", name);

    return loc;
}
