#ifndef _COMMON_IN_FILE_MT_HPP_
#define _COMMON_IN_FILE_MT_HPP_

#include "common/pa_exception.h"
#include <common/api_info.hpp>
#include <common/os_time.hpp>
#include <common/in_file.hpp>

#include <snappy.h>
#include <deque>
#include <set>

namespace common {

class InFile : public InFileBase
{
public:
    InFile() { Close(); }
    ~InFile() {}

    bool Open(const char *name, bool readHeaderAndExit = false);
    void Close();
    bool GetNextCall(void*& fptr, common::BCall_vlen& call, char*& src);

    void rollback();

    long memoryUsed()
    {
        long s = 0;
        if (mCurrentChunk) s += mCurrentChunk->size();
        if (mPrevChunk) s += mPrevChunk->size();
        for (const auto* c : mPreloadedChunks) s += c->size();
        for (const auto* c : mFreeChunks) s += c->size();
        return s;
    }

    int curCallNo = -1;

private:
    void ReadSigBook();
    void PreloadFrames(int frames_to_read, int tid);
    bool readChunk(std::vector<char> *buf);

    std::deque<std::vector<char>*> mPreloadedChunks;
    /// The free list is used for loop tracing.
    std::deque<std::vector<char>*> mFreeChunks;
    std::vector<char> *mCurrentChunk = nullptr;
    /// We cannot immediately free the previous chunk since pointers may still be pointing
    /// into its memory area which are consumed by calls in the next.
    std::vector<char> *mPrevChunk = nullptr;
    // record created pbuffer surfaces
    std::set<int> mPbufferSurfaces;

    /// Offset into first packet that we should start a rollback at
    intptr_t mCheckpointOffset = -1;

    char *mPtr = nullptr;
    void *mChunkEnd = nullptr;
    int64_t mCompressedRemaining = 0;
    int64_t mCompressedSize = 0;
    char *mCompressedBuffer = nullptr;
    char *mCompressedSource = nullptr;
    int mFrameNo = 0;
    int mFd = 0;
};

}

#endif
