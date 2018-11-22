#ifndef _STATE_FBO_HPP_
#define _STATE_FBO_HPP_

#include "common.hpp"

namespace pat
{

class RenderTarget;
class FramebufferObject
{
public:
    FramebufferObject()
    : _id(NextID++), _colorAttachment(NULL),
      _depthAttachment(NULL), _stencilAttachment(NULL)
    {
    }

    const std::string IDString() const;

    void BindRenderTarget(unsigned int attachment, RenderTarget *target);
    const RenderTarget *BoundRenderTarget(unsigned int attachment) const;

    void SetType(unsigned int) {}

private:
    static unsigned int NextID;

    unsigned int _id;
    RenderTarget * _colorAttachment;
    RenderTarget * _depthAttachment;
    RenderTarget * _stencilAttachment;
};

}

#endif // _STATE_FBO_HPP_
