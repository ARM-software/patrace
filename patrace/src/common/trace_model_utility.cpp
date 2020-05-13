#include "common/trace_model_utility.hpp"

#include "retracer/glws.hpp"
#include "retracer/retracer.hpp"
#include "retracer/retrace_api.hpp"

GLvoid* getBufferPointer(common::ValueTM *value)
{
    // This case cannot be handled within trace model, since it does not generate and track client side buffers.
    if (value->mType == common::Opaque_Type && value->mOpaqueType == common::ClientSideBufferObjectReferenceType)
    {
        return retracer::gRetracer.mCSBuffers.translate_address(retracer::gRetracer.getCurTid(), value->mOpaqueIns->mClientSideBufferName, (ptrdiff_t)value->mOpaqueIns->mClientSideBufferOffset);
    }
    return reinterpret_cast<GLvoid*>(value->GetAsUInt64());
}

GLvoid* drawCallIndexPtr(const common::CallTM *call)
{
    if (call->mCallName == "glDrawElementsIndirect")
    {
        return getBufferPointer(call->mArgs[2]);
    }
    else if (call->mCallName == "glDrawElementsInstanced")
    {
        return getBufferPointer(call->mArgs[3]);
    }
    else if (call->mCallName == "glDrawElementsInstancedBaseVertex")
    {
        return getBufferPointer(call->mArgs[3]);
    }
    else if (call->mCallName == "glDrawElements")
    {
        return getBufferPointer(call->mArgs[3]);
    }
    else if (call->mCallName == "glDrawElementsBaseVertex")
    {
        return getBufferPointer(call->mArgs[3]);
    }
    else if (call->mCallName == "glDrawRangeElements")
    {
        return getBufferPointer(call->mArgs[5]);
    }
    else if (call->mCallName == "glDrawRangeElementsBaseVertex")
    {
        return getBufferPointer(call->mArgs[5]);
    }
    return NULL;
}

GLenum drawCallIndexType(const common::CallTM *call)
{
    if (call->mCallName == "glDrawElementsIndirect")
    {
        return call->mArgs[1]->GetAsUInt();
    }
    else if (call->mCallName == "glDrawElementsInstanced")
    {
        return call->mArgs[2]->GetAsUInt();
    }
    else if (call->mCallName == "glDrawElementsInstancedBaseVertex")
    {
        return call->mArgs[2]->GetAsUInt();
    }
    else if (call->mCallName == "glDrawElements")
    {
        return call->mArgs[2]->GetAsUInt();
    }
    else if (call->mCallName == "glDrawElementsBaseVertex")
    {
        return call->mArgs[2]->GetAsUInt();
    }
    else if (call->mCallName == "glDrawRangeElements")
    {
        return call->mArgs[4]->GetAsUInt();
    }
    else if (call->mCallName == "glDrawRangeElementsBaseVertex")
    {
        return call->mArgs[4]->GetAsUInt();
    }
    return GL_NONE;
}

int drawCallCount(const common::CallTM *call)
{
    if (call->mCallName == "glDrawArraysIndirect")
    {
        return 0;
    }
    else if (call->mCallName == "glDrawElementsIndirect")
    {
        return 0;
    }
    else if (call->mCallName == "glDrawElementsInstanced")
    {
        return call->mArgs[1]->GetAsUInt();
    }
    else if (call->mCallName == "glDrawElementsInstancedBaseVertex")
    {
        return call->mArgs[1]->GetAsUInt();
    }
    else if (call->mCallName == "glDrawArraysInstanced")
    {
        return call->mArgs[2]->GetAsUInt();
    }
    else if (call->mCallName == "glDrawElements")
    {
        return call->mArgs[1]->GetAsUInt();
    }
    else if (call->mCallName == "glDrawElementsBaseVertex")
    {
        return call->mArgs[1]->GetAsUInt();
    }
    else if (call->mCallName == "glDrawArrays")
    {
        return call->mArgs[2]->GetAsUInt();
    }
    else if (call->mCallName == "glDrawRangeElements")
    {
        return call->mArgs[3]->GetAsUInt();
    }
    else if (call->mCallName == "glDrawRangeElementsBaseVertex")
    {
        return call->mArgs[3]->GetAsUInt();
    }
    else
    {
        std::cerr << "Unhandled draw call type: " << call->mCallName << std::endl;
        abort();
    }
    return 0;
}

static void read_floats(Json::Value &ret, GLuint id, GLint location, int size)
{
    std::vector<GLfloat> param(size, 0.0f);
    _glGetUniformfv(id, location, param.data());
    for (int i = 0; i < size; i++)
    {
        ret.append(param[i]);
    }
}

static void read_ints(Json::Value &ret, GLuint id, GLint location, int size)
{
    std::vector<GLint> param(size, 0);
    _glGetUniformiv(id, location, param.data());
    for (int i = 0; i < size; i++)
    {
        ret.append(param[i]);
    }
}

static void read_unsigneds(Json::Value &ret, GLuint id, GLint location, int size)
{
    std::vector<GLuint> param(size, 0);
    _glGetUniformuiv(id, location, param.data());
    for (int i = 0; i < size; i++)
    {
        ret.append(param[i]);
    }
}

static Json::Value uniform_value_to_json(GLuint id, GLint size, GLint location, GLenum type)
{
    Json::Value ret = Json::arrayValue;
    for (int j = 0; location >= 0 && j < size; ++j)
    {
        switch (type)
        {
        case GL_UNSIGNED_INT:
            read_unsigneds(ret, id, location + j, 1);
            break;
        case GL_UNSIGNED_INT_VEC2:
            read_unsigneds(ret, id, location + j, 2);
            break;
        case GL_UNSIGNED_INT_VEC3:
            read_unsigneds(ret, id, location + j, 3);
            break;
        case GL_UNSIGNED_INT_VEC4:
            read_unsigneds(ret, id, location + j, 4);
            break;
        case GL_SAMPLER_2D:
        case GL_SAMPLER_3D:
        case GL_SAMPLER_CUBE:
        case GL_SAMPLER_2D_SHADOW:
        case GL_SAMPLER_2D_ARRAY:
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
        case GL_INT:
        case GL_BOOL:
            read_ints(ret, id, location + j, 1);
            break;
        case GL_INT_VEC2:
        case GL_BOOL_VEC2:
            read_ints(ret, id, location + j, 2);
            break;
        case GL_INT_VEC3:
        case GL_BOOL_VEC3:
            read_ints(ret, id, location + j, 3);
            break;
        case GL_INT_VEC4:
        case GL_BOOL_VEC4:
            read_ints(ret, id, location + j, 4);
            break;
        case GL_FLOAT:
            read_floats(ret, id, location + j, 1);
            break;
        case GL_FLOAT_VEC2:
            read_floats(ret, id, location + j, 2);
            break;
        case GL_FLOAT_VEC3:
            read_floats(ret, id, location + j, 3);
            break;
        case GL_FLOAT_VEC4:
            read_floats(ret, id, location + j, 4);
            break;
        case GL_FLOAT_MAT4:
            read_floats(ret, id, location + j, 16);
            break;
        case GL_FLOAT_MAT3:
            read_floats(ret, id, location + j, 9);
            break;
        case GL_FLOAT_MAT2:
            read_floats(ret, id, location + j, 4);
            break;
        case GL_FLOAT_MAT2x3:
            read_floats(ret, id, location + j, 6);
            break;
        case GL_FLOAT_MAT2x4:
            read_floats(ret, id, location + j, 8);
            break;
        case GL_FLOAT_MAT3x2:
            read_floats(ret, id, location + j, 6);
            break;
        case GL_FLOAT_MAT3x4:
            read_floats(ret, id, location + j, 12);
            break;
        case GL_FLOAT_MAT4x2:
            read_floats(ret, id, location + j, 8);
            break;
        case GL_FLOAT_MAT4x3:
            read_floats(ret, id, location + j, 12);
            break;
        default:
            DBG_LOG("Unsupported uniform type: %04x\n", (unsigned)type);
            break;
        }
    }
    return ret;
}

Json::Value saveProgramUniforms(GLuint program, int count)
{
    Json::Value result = Json::arrayValue;
    for (int i = 0; i < count; ++i)
    {
        GLsizei length = 0;
        GLint size = 0;
        GLenum type = GL_NONE;
        Json::Value entry;
        GLchar tmp[128];
        memset(tmp, 0, sizeof(tmp));
        _glGetActiveUniform(program, i, sizeof(tmp) - 1, &length, &size, &type, tmp);
        GLint location = _glGetUniformLocation(program, tmp);
        entry["name"] = tmp;
        entry["location"] = location;
        entry["size"] = size;
        entry["type"] = std::string(SafeEnumString(type)).replace(0, 3, "");
        entry["value"] = uniform_value_to_json(program, size, location, type);
        result.append(entry);
    }
    return result;
}
