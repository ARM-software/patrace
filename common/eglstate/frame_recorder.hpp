#ifndef _FRAME_RECORDER_HPP_
#define _FRAME_RECORDER_HPP_

#include <vector>

namespace record
{

class Frame;
class FBOJob;

class FrameRecorder
{
public:
    static FrameRecorder * Instance()
    {
        static FrameRecorder instance;
        return &instance;
    }
    FrameRecorder();
    ~FrameRecorder();

    void Initialize();
    void Uninitialize();

    // Frame records
    void SwapBuffers();
    unsigned int FrameCount() const;
    const Frame * GetFrame(unsigned int index) const;
    Frame * GetFrame(unsigned int index);
    FBOJob * CurrentFBOJob();

    // Operations on the last FBOJob
    void Clear();
    void DrawArrays(unsigned int mode, unsigned int first, unsigned int count);
    void DrawElements(unsigned int mode, unsigned int count, unsigned int type, const void *indices);

private:
    void PreDrawCall();
    void PostDrawCall();

    void Reset();

    unsigned int _drawCallIndex; // draw call index in a frame
    std::vector<Frame *> _frames;
};

} // namespace record

#endif // _FRAME_RECORDER_HPP_
