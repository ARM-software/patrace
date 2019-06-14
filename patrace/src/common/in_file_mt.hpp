#ifndef _COMMON_IN_FILE_MT_HPP_
#define _COMMON_IN_FILE_MT_HPP_

#include "common/pa_exception.h"
#include "common/os_thread.hpp"
#include <common/api_info.hpp>
#include <common/os_time.hpp>
#include <common/in_file.hpp>

#include <atomic>
#include <snappy.h>
#include <queue>
#include <sstream>

namespace common {

extern unsigned int gCompBufLen;
extern char*        gCompBuf;

template<typename T>
class ReserveQueue : public os::MTQueue <T>
{
public:
    enum
    {
        RESERVED_QUEUE_SIZE = 2,
    };
    ReserveQueue() :
        _terminate(false)
    {}

    ReserveQueue(int size) :
       os::MTQueue<T>::MTQueue(size),
        _terminate(false)
    {}

    T reserve_pop()
    {
        while (os::MTQueue<T>::size() < RESERVED_QUEUE_SIZE)
        {
            os::MTQueue<T>::setUsedCondWait();
            if(_terminate == true)break;
        }
        return os::MTQueue<T>::trypop();
    }

    void terminate()
    {
        _terminate = true;
    }
private:
    bool _terminate;
};

class UnCompressedChunk {
public:
    enum ChunkStatus
    {
        FREE,
        READING,
        PRECALLING,
        CALLING,
        SWITCHING,
        PENDING,
        UNKNOW,
    };

    UnCompressedChunk():
        mData(NULL),
        mLen(0),
        mCapacity(0)
    {
        setStatus(FREE);
        mRef = 0;
    }

    ~UnCompressedChunk() {
        mLen = 0;
        mCapacity = 0;
        delete [] mData;
    }

    void LoadFromFileStream(std::fstream& stream) {
        const unsigned int compressedLength = ReadCompressedLength(stream);
        if (compressedLength)
        {
            if (gCompBufLen < compressedLength) {
                delete [] gCompBuf;
                gCompBufLen = compressedLength;
                gCompBuf = new char [compressedLength];
            }

            stream.read(gCompBuf, compressedLength);

            mLen = 0;
            ::snappy::GetUncompressedLength(gCompBuf, (size_t)compressedLength,
                (size_t*)&mLen);
            if (mCapacity < mLen) {
                delete [] mData;
                mCapacity = mLen;
                mData = new char [mLen];
            }
            ::snappy::RawUncompress(gCompBuf, compressedLength,
                mData);
        }
    }

    inline void retain()
    {
        mRef.fetch_add(1, std::memory_order_acquire);
    }

    inline void release()
    {
        int left = mRef.fetch_sub(1, std::memory_order_release);
        if (left <= 0)
            delete this;
    }

    int  getRefCount()
    {
        return mRef;
    }

    ChunkStatus getStatus()
    {
        return status;
    }

    void setStatus(ChunkStatus s)
    {
        status = s;
    }

    char*                   mData;
    size_t                  mLen;
    size_t                  mCapacity;

private:
    void SetCapacity(unsigned int cap) {
        if (cap > mCapacity) {
            mCapacity = cap;
            delete [] mData;
            mData = new char[mCapacity];
        }
    }
    unsigned int ReadCompressedLength(std::fstream& stream) {
        unsigned char buf[4];
        unsigned int length;
        stream.read((char *)buf, sizeof(buf));
        if (stream.fail()) {
            length = 0;
        } else {
            length  =  (size_t)buf[0];
            length |= ((size_t)buf[1] <<  8);
            length |= ((size_t)buf[2] << 16);
            length |= ((size_t)buf[3] << 24);
        }
        return length;
    }

    std::atomic_int mRef;
    std::atomic<ChunkStatus> status;
};


class InFile : public InFileBase, os::Thread
{
public:
    enum
    {
        MAX_CHUNK_QUEUE_SIZE = 5,
        MAX_PRELOAD_QUEUE_SIZE = 1000000,
    };
    enum ReadingThreadStatus
    {
        IDLE,
        READING,
        TERMINATE,
        END,
        UNKOWN,
    };

    InFile();
    virtual ~InFile();
    void prepareChunks();
    bool Open(const char* name, bool readHeaderAndExit=false);
    virtual void Close();
    void releaseChunk(UnCompressedChunk* chunk);
    void SetPreloadRange(unsigned startFrame, unsigned endFrame, int tid);
    int  PreloadFrames(unsigned int frameCnt, int tid);
    bool BeginBackendRead();
    void StopBackendRead();
    bool GetNextCall(void*& fptr, common::BCall_vlen& call, char*& src);
    bool GetNextCall(void*& fptr, common::BCall_vlen& call, char*& src, UnCompressedChunk*& chunk);

    inline bool EndOfData() const
    {
        return mStream.eof();
    }

    inline unsigned short NameToExId(const char* str) const
    {
        for (unsigned short id = 1; id <= mMaxSigId; ++id) {
            if (strcmp(ExIdToName(id), str) == 0)
                return id;
        }
        return 0;
    }

    inline void setStatus(ReadingThreadStatus s)
    {
        mStatus = s;
    }

    inline ReadingThreadStatus getStatus()
    {
        return mStatus;
    }

    void releaseChunkQueues();

private:
    void ReadSigBook();
    virtual void run();
    bool MoveToNextChunk();

    inline int GetNextBlock(char*& beg, char*& end) {
        if (mReadP >= mCurChunk->mData+mCurChunk->mLen)
            MoveToNextChunk();
        common::Block* pB = (common::Block*)mReadP;
        beg = mReadP;
        mReadP += pB->toNext;
        end = mReadP;
        return pB->toNext;
    }

    UnCompressedChunk*  mCurChunk;
    char*               mReadP;

    unsigned int        mCallNo;
    unsigned int        mFrameNo;
    unsigned int        mMaxSigId;

    bool                mIsOpen;
    std::atomic<ReadingThreadStatus> mStatus;

    // double buffer, for backend thread loading
    os::MTQueue<UnCompressedChunk*> callChunkQueue;
    os::MTQueue<UnCompressedChunk*> pendChunkQueue;
    ReserveQueue<UnCompressedChunk*> freeChunkQueue;

    // for pre-load feature
    bool                mIsPreloadMode;
    bool                mBeginPreload;
    unsigned            mBeginPreloadFrame;
    unsigned            mTotalPreloadFrames;
    int                 mTraceTid;
    unsigned short      eglSwapBuffers_id;
    unsigned short      eglSwapBuffersWithDamage_id;
    os::MTQueue<UnCompressedChunk*> mPreloadedChunks;
};

}

#endif
