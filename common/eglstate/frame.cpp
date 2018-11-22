#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "frame.hpp"
#include "common.hpp"
#include "fbo.hpp"
#include "state_manager.hpp"

namespace record
{

/////////////////////////////////////////////////////////////////
// FBOJob
/////////////////////////////////////////////////////////////////

FBOJob::FBOJob(const state::FramebufferObject *boundFBO)
: _boundFBO(boundFBO)
{
    _viewport = StateManagerInstance->GetViewport();
}

const state::FramebufferObject *FBOJob::BoundFramebufferObject() const
{
    return _boundFBO;
}

const Cube2D FBOJob::GetViewport() const
{
    return _viewport;
}

/////////////////////////////////////////////////////////////////
// Frame
/////////////////////////////////////////////////////////////////

Frame::Frame(unsigned int index)
: _index(index)
{
}

Frame::~Frame()
{
    CLEAR_POINTER_ARRAY(_fboJobs);
}

const std::string Frame::IDString() const
{
    char buffer[128];
    sprintf(buffer, "Frame_%d", _index);
    return buffer;
}

unsigned int Frame::FBOJobCount() const
{
    return _fboJobs.size();
}

FBOJob * Frame::CurrentFBOJob()
{
    return _fboJobs.back();
}

const FBOJob *Frame::GetFBOJob(unsigned int index) const
{
    return _fboJobs[index];
}

void Frame::DrawArrays(unsigned int mode, unsigned int first, unsigned int count)
{
    const state::FramebufferObject *boundFBO = StateManagerInstance->BoundFramebufferObject(GL_FRAMEBUFFER);
    if (_fboJobs.size() == 0 || boundFBO != _fboJobs.back()->BoundFramebufferObject())
    {
        _fboJobs.push_back(new FBOJob(boundFBO));
    }
}

void Frame::DrawElements(unsigned int mode, unsigned int count, unsigned int type, const void *indices)
{
    const state::FramebufferObject *boundFBO = StateManagerInstance->BoundFramebufferObject(GL_FRAMEBUFFER);
    if (_fboJobs.size() == 0 || boundFBO != _fboJobs.back()->BoundFramebufferObject())
    {
        _fboJobs.push_back(new FBOJob(boundFBO));
    }
}

} // namespace state
