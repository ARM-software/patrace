#include "common/trace_model_utility.hpp"

#include "retracer/glws.hpp"
#include "retracer/retracer.hpp"
#include "retracer/retrace_api.hpp"

static GLvoid* getBufferPointer(common::ValueTM *value)
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
