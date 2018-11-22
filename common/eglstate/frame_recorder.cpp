#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "frame_recorder.hpp"
#include "state_manager.hpp"
#include "frame.hpp"

#include "dump.hpp"

namespace record
{

/////////////////////////////////////////////////////////////////
// Initialization & Uninitialization
/////////////////////////////////////////////////////////////////

FrameRecorder::FrameRecorder()
: _drawCallIndex(0)
{
}

FrameRecorder::~FrameRecorder()
{
    dump::DumpFrames("frames.json");

    // In some cases, eglTerminate will not be called
    Reset();
}

void FrameRecorder::Initialize()
{
    Reset();

    // Prepare for the first frame
    _frames.push_back(new Frame(0));
}

void FrameRecorder::Uninitialize()
{
    Reset();
}

void FrameRecorder::Reset()
{
    CLEAR_POINTER_ARRAY(_frames);
}

void FrameRecorder::SwapBuffers()
{
    _frames.push_back(new Frame(_frames.size()));
    _drawCallIndex = 0;
}

/////////////////////////////////////////////////////////////////
// Frame
/////////////////////////////////////////////////////////////////

unsigned int FrameRecorder::FrameCount() const
{
    return _frames.size();
}

Frame * FrameRecorder::GetFrame(unsigned int index)
{
    return _frames[index];
}

const Frame * FrameRecorder::GetFrame(unsigned int index) const
{
    return _frames[index];
}

FBOJob * FrameRecorder::CurrentFBOJob()
{
    return _frames.back()->CurrentFBOJob();
}

/////////////////////////////////////////////////////////////////
// Operations on current FBO job
/////////////////////////////////////////////////////////////////

void FrameRecorder::DrawArrays(unsigned int mode, unsigned int first, unsigned int count)
{
    PreDrawCall();
    _frames.back()->DrawArrays(mode, first, count);
    PostDrawCall();
}

void FrameRecorder::DrawElements(unsigned int mode, unsigned int count, unsigned int type, const void *indices)
{
    PreDrawCall();
    _frames.back()->DrawElements(mode, count, type, indices);
    PostDrawCall();
}

void FrameRecorder::Clear()
{
    PreDrawCall();
    PostDrawCall();
}

void FrameRecorder::PreDrawCall()
{
    const unsigned int frameIndex = _frames.size();
    const unsigned int drawcallIndex = _drawCallIndex;
    if (frameIndex == 521 && drawcallIndex == 25)
    {
        char buffer[256];
        sprintf(buffer, "frame%d_drawcall%d.json", frameIndex, drawcallIndex);

        dump::DumpCurrentState(buffer);
    }
}

void FrameRecorder::PostDrawCall()
{
    _drawCallIndex++;
}

};
