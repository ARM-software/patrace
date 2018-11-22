#ifndef _STATE_FRAME_HPP_
#define _STATE_FRAME_HPP_

#include <vector>
#include <string>
#include "common.hpp"
#include "fbo.hpp"

namespace record
{

// Record all draw calls in a FBO job
class FBOJob
{
public:
    FBOJob(const state::FramebufferObject *boundFBO);
    
    const state::FramebufferObject *BoundFramebufferObject() const;

    const Cube2D GetViewport() const;

private:
    const state::FramebufferObject *_boundFBO;
    Cube2D _viewport;
};

// Record all FBO jobs in a frame
class Frame
{
public:
    Frame(unsigned int index);
    ~Frame();

    const std::string IDString() const;

    unsigned int FBOJobCount() const;
    FBOJob *CurrentFBOJob();
    const FBOJob *GetFBOJob(unsigned int index) const;

    void DrawArrays(unsigned int mode, unsigned int first, unsigned int count);
    void DrawElements(unsigned int mode, unsigned int count, unsigned int type, const void *indices);

private:
    unsigned int _index;

    std::vector<FBOJob *> _fboJobs;
};

} // namespace state

#endif // _STATE_FRAME_HPP_
