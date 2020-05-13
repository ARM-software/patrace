// Despite its name, this file is _only_ for use cases where you mix retracer and trace model.

#pragma once

#include "eglstate/common.hpp"

#include "common/trace_model.hpp"

#define GL_GLEXT_PROTOTYPES
#include <GLES3/gl32.h>

GLvoid* getBufferPointer(common::ValueTM *value);
GLvoid* drawCallIndexPtr(const common::CallTM *call);
GLenum drawCallIndexType(const common::CallTM *call);
int drawCallCount(const common::CallTM *call);

static inline const std::string SafeEnumString(unsigned int enumToFind, const std::string &funName = std::string())
{
    const char *retval = EnumString(enumToFind, funName);
    return retval ? retval : "Unknown";
}

Json::Value saveProgramUniforms(GLuint program, int count);
