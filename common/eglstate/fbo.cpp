#include "fbo.hpp"
#include "render_target.hpp"

#include <cstdio>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

namespace pat
{

/////////////////////////////////////////////////////////////////
// class FramebufferObject
/////////////////////////////////////////////////////////////////

unsigned int FramebufferObject::NextID = 0;

void FramebufferObject::BindRenderTarget(unsigned int attachment, RenderTarget *target)
{
    if (!target) return;

    if (attachment == GL_COLOR_ATTACHMENT0)
    {
        if (_colorAttachment && _colorAttachment != target)
        {
            PAT_DEBUG_LOG("%s changes color attachment from %s to %s.\n", IDString().c_str(), _colorAttachment->IDString().c_str(), target->IDString().c_str());
        }
        _colorAttachment = target;
    }
    else if (attachment == GL_DEPTH_ATTACHMENT)
    {
        if (_depthAttachment && _depthAttachment == target)
        {
            PAT_DEBUG_LOG("%s changes depth attachment from %s to %s.\n", IDString().c_str(), _depthAttachment->IDString().c_str(), target->IDString().c_str());
        }
        _depthAttachment = target;
    }
    else if (attachment == GL_STENCIL_ATTACHMENT)
    {
        assert(!_stencilAttachment || _stencilAttachment == target);
        _stencilAttachment = target;
    }
    else
        PAT_DEBUG_LOG("Invalid attachment for framebuffer object : %d\n", attachment);
}

const RenderTarget *FramebufferObject::BoundRenderTarget(unsigned int attachment) const
{
    if (attachment == GL_COLOR_ATTACHMENT0)
        return _colorAttachment;
    else if (attachment == GL_DEPTH_ATTACHMENT)
        return _depthAttachment;
    else if (attachment == GL_STENCIL_ATTACHMENT)
        return _stencilAttachment;
    else
    {
        PAT_DEBUG_LOG("Invalid attachment for framebuffer object : %d\n", attachment);
        return NULL;
    }
}

const std::string FramebufferObject::IDString() const
{
    char buffer[32];
    sprintf(buffer, "FramebufferObject[%d]", _id);
    return buffer;
}

}

