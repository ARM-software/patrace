#include <common/in_file_mt.hpp>
#include <common/memoryinfo.hpp>
#include <common/trace_limits.hpp>

#include "jsoncpp/include/json/writer.h"
#include "jsoncpp/include/json/reader.h"
#include <unistd.h>

using namespace os;
using namespace std;

namespace common {

unsigned int    gCompBufLen = 0;
char*           gCompBuf = NULL;

InFile::InFile():
    InFileBase(),
    Thread(),
    mCurChunk(NULL),
    mReadP(NULL),
    mCallNo(),
    mFrameNo(0),
    mMaxSigId(),
    mIsOpen(false),
    mStatus(UNKOWN),
    callChunkQueue(MAX_CHUNK_QUEUE_SIZE),
    pendChunkQueue(MAX_PRELOAD_QUEUE_SIZE),
    freeChunkQueue(MAX_CHUNK_QUEUE_SIZE + 1),
    mIsPreloadMode(false),
    mBeginPreload(false),
    mBeginPreloadFrame(0),
    mTotalPreloadFrames(0),
    mTraceTid(0),
    eglSwapBuffers_id(0),
    eglSwapBuffersWithDamage_id(0),
    mPreloadedChunks(MAX_PRELOAD_QUEUE_SIZE)
{
}

InFile::~InFile()
{
    if(mIsOpen)
        Close();
    releaseChunkQueues();
}

void InFile::releaseChunkQueues()
{
    UnCompressedChunk* chunk;

    do
    {
        chunk = callChunkQueue.trypop();
        releaseChunk(chunk);
    }while(chunk);

    do
    {
        chunk = pendChunkQueue.trypop();
        if (chunk)
            chunk->release();
    }while(chunk);

    do
    {
        chunk = freeChunkQueue.trypop();
        if (chunk)
            chunk->release();
    }while(chunk);
}

void InFile::prepareChunks()
{
    for (int i = 0; i <= MAX_CHUNK_QUEUE_SIZE; i++)
    {
        UnCompressedChunk* chunk = new UnCompressedChunk();
        chunk->retain();
        if(!freeChunkQueue.trypush(chunk))
            chunk->release();
    }
}

bool InFile::Open(const char* name, bool readHeaderAndExit)
{
    mFileName = name;

    if (!InFileBase::Open())
    {
        return false;
    }

    // when we only wanted to use -info to see header contents, no playback
    if (readHeaderAndExit)
    {
        return true;
    }

    if (!BeginBackendRead())
    {
        return false;
    }

    // Get the first chunk so that we can read the sig book.
    if (!MoveToNextChunk())
    {
        DBG_LOG("fail to move to next chunk, chunk has zero length?\n");
    }

    ReadSigBook();
    mIsOpen = true;
    mBeginPreload = false;
    mCallNo = 0;
    mFrameNo  = 0;
    return true;
}

void InFile::Close()
{
    StopBackendRead();

    mStream.close();
    mIsOpen = false;

    releaseChunk(mCurChunk);
    releaseChunkQueues();

    mCurChunk = NULL;
    mReadP = NULL;
    mBeginPreload = false;

    delete [] mExIdToName;
    mExIdToName = NULL;
    delete [] mExIdToLen;
    mExIdToLen = NULL;
    delete [] mExIdToFunc;
    mExIdToFunc = NULL;

    InFileBase::Close();
}

void InFile::ReadSigBook()
{
    char *beg = NULL, *end = NULL;
    GetNextBlock(beg, end);

    char *src = beg;
    unsigned int toNext;
    src = ReadFixed(src, toNext);
    src = ReadFixed(src, mMaxSigId);

    if (mMaxSigId > ApiInfo::MaxSigId)
    {
        DBG_LOG("Reading the trace file failed: The reader seems to be too old, since it supports fewer functions than the trace file.\n");
#ifndef __APPLE__
        exit(EXIT_FAILURE); // do not crash, as this is going to happen, and is not an application bug
#else
        os::abort(); // we want an exception for MacOSX
#endif
    }

    mExIdToName = new std::string[mMaxSigId + 1];
    mExIdToName[0] = "";
    for (unsigned short id = 1; id <= mMaxSigId; ++id)
    {
        unsigned int id_notused;
        src = ReadFixed<unsigned int>(src, id_notused);
        char *str;
        src = ReadString(src, str);
        mExIdToName[id] = str ? str : "";
    }

    mExIdToLen = new int[mMaxSigId + 1];
    mExIdToFunc = new void*[mMaxSigId + 1];

    mExIdToLen[0] = 0;
    mExIdToFunc[0] = 0;
    for (unsigned short id = 1; id <= mMaxSigId; ++id)
    {
        const char* name = mExIdToName[id].c_str();
        mExIdToLen[id] = gApiInfo.NameToLen(name);
        mExIdToFunc[id] = gApiInfo.NameToFptr(name);
    }
}

void InFile::SetPreloadRange(unsigned startFrame, unsigned endFrame, int tid)
{
    mIsPreloadMode      = true;
    mBeginPreloadFrame  = startFrame;
    mTotalPreloadFrames = endFrame - startFrame;
    mTraceTid           = tid;
    eglSwapBuffers_id   = NameToExId("eglSwapBuffers");
    eglSwapBuffersWithDamage_id = NameToExId("eglSwapBuffersWithDamageKHR");
}

int InFile::PreloadFrames(unsigned int frameCnt, int tid)
{
    bool reachEnd = false;
    StopBackendRead();

    unsigned long memBefore = MemoryInfo::getFreeMemoryRaw();
    // pre-load data
    int preloadedFrameCnt = 0;
    char *readP = mReadP;
    UnCompressedChunk *curChunk = mCurChunk;
    DBG_LOG("Started preloading content\n");
    while (static_cast<unsigned int>(preloadedFrameCnt) < frameCnt)
    {

        if (readP >= curChunk->mData+curChunk->mLen)
        {
            // Check available memory before loading another chunk
            // Checking for zero might seem stupid, but keep in mind that there is a margin in MemoryInfo for some devices.
            unsigned long long freeMemory = MemoryInfo::getFreeMemory();
            if (freeMemory == 0)
            {
                DBG_LOG("Out of memory in preload, aborting! Frame %d, available mem: %lld\n", preloadedFrameCnt, freeMemory);
                return -1;
            }

            // Either move to the next chunk that is already loaded by the backend thread
            curChunk = callChunkQueue.trypop();
            if (curChunk == NULL)
            // Or pre-load another chunk from the file stream
            {
                curChunk = freeChunkQueue.trypop();
                if (curChunk == NULL)
                {
                    curChunk = new UnCompressedChunk();
                    curChunk->retain();
                }
                curChunk->setStatus(UnCompressedChunk::READING);
                curChunk->LoadFromFileStream(mStream);
            }
            else
            {
                curChunk->release();
            }
            readP = curChunk->mData;
            curChunk->retain();
            curChunk->setStatus(UnCompressedChunk::PRECALLING);
            if (curChunk->mLen == 0)
            {
                reachEnd = true;
            }
            mPreloadedChunks.push(curChunk);

            if (reachEnd)
                break;
        }

        common::BCall& call = *(common::BCall*)readP;
        if (call.tid == tid && (call.funcId == eglSwapBuffers_id || call.funcId == eglSwapBuffersWithDamage_id))
        {
            preloadedFrameCnt++;
        }

        unsigned int callLen = mExIdToLen[call.funcId];
        if (callLen == 0) {
            readP += reinterpret_cast<common::BCall_vlen*>(readP)->toNext;
        } else {
            readP += callLen;
        }
    }

    DBG_LOG("Preloading finished, loaded %d frames, consumed %ld MiB\n", preloadedFrameCnt, (memBefore - MemoryInfo::getFreeMemoryRaw())/(1024*1024));
    return preloadedFrameCnt;
}

bool InFile::BeginBackendRead()
{
    if (!start()) {
        DBG_LOG("Unable to create the thread for reading trace file.\n");
        return false;
    }
    return true;
}

void InFile::StopBackendRead()
{
    // request the backend reading thread to stop now
    setStatus(TERMINATE);

    freeChunkQueue.terminate();

    freeChunkQueue.wakeup();
    callChunkQueue.wakeup();

    // wait until the backend reading thread stopped
    waitUntilExit();
}

void InFile::run()
{
    UnCompressedChunk* newChunk = NULL;
    setStatus(IDLE);

    while (getStatus() != TERMINATE)
    {
        setStatus(IDLE);
        callChunkQueue.waitEmptyPos();
        while (!callChunkQueue.isFull() && getStatus() != TERMINATE)
        {
            setStatus(READING);
            newChunk = freeChunkQueue.reserve_pop();
            if (newChunk == NULL) break;
            newChunk->setStatus(UnCompressedChunk::READING);
            newChunk->LoadFromFileStream(mStream);
            newChunk->retain();
            newChunk->setStatus(UnCompressedChunk::PRECALLING);

            if (newChunk->mLen == 0)
            {
                DBG_LOG("Reach the end of the trace file, finishing this back thread!\n");
                setStatus(TERMINATE);
            }

            callChunkQueue.push(newChunk);
        }
    }

    setStatus(END);
}

bool InFile::GetNextCall(void*& fptr, common::BCall_vlen& call, char*& src)
{
    if (unlikely(!mReadP || !mCurChunk))
        return false; // this should never happen

    if (mReadP >= mCurChunk->mData + mCurChunk->mLen)
        if (!MoveToNextChunk())
            return false;

    common::BCall tmpCall;
    tmpCall = *(common::BCall*)mReadP;

    if (unlikely(tmpCall.funcId > mMaxSigId))
    {
        DBG_LOG("funcId %d is out of range (%d max)!\n", (int)tmpCall.funcId, (int)mMaxSigId);
        return false;
    }

    unsigned int callLen = mExIdToLen[tmpCall.funcId];

    if (callLen == 0)
    {
        // Call is in BCall_vlen format -- read it directly
        call = *(common::BCall_vlen*) mReadP;
        mDataPtr = src = mReadP + sizeof(common::BCall_vlen);
        mReadP += call.toNext;
    }
    else
    {
        call = tmpCall;
        mDataPtr = src = mReadP + sizeof(common::BCall);
        mReadP += callLen;
    }

    mFuncPtr = fptr = mExIdToFunc[call.funcId];

    if (mIsPreloadMode)
    {
        //DBG_LOG("tmpCall.tid(%d), tmpCall.funcId(%d)\n", (int)tmpCall.tid, (int)tmpCall.funcId);
        if (tmpCall.tid == mTraceTid && (tmpCall.funcId == eglSwapBuffers_id || tmpCall.funcId == eglSwapBuffersWithDamage_id))
        {
            mFrameNo++;
        }
        //DBG_LOG("mFrameNo(%d)\n", (int)mFrameNo);
        if (mFrameNo >= mBeginPreloadFrame && !mBeginPreload)
        {
            //DBG_LOG("mBeginPreloadFrame(%d), mTotalPreloadFrames(%d), mTraceTid(%d)\n", (int)mBeginPreloadFrame, (int)mTotalPreloadFrames, mTraceTid);
            mBeginPreload = true;
            PreloadFrames(mTotalPreloadFrames, mTraceTid);
        }
    }
    return true;
}

bool InFile::GetNextCall(void*& fptr, common::BCall_vlen& call, char*& src, UnCompressedChunk*& curChunk)
{
    bool ret = GetNextCall(fptr, call, src);
    curChunk = mCurChunk;
    return ret;
}

bool InFile::MoveToNextChunk()
{
    if (mCurChunk)
    {
        mCurChunk->setStatus(UnCompressedChunk::SWITCHING);
        releaseChunk(mCurChunk);
    }
    mCurChunk = NULL;

    if (mIsPreloadMode && mBeginPreload)
    {
        mCurChunk = mPreloadedChunks.trypop();
        if (mCurChunk == NULL)
        {
            return false;
        }
    }
    else
    {
        mCurChunk = callChunkQueue.pop();
    }

    if (mCurChunk->mLen == 0)
    {
        mCurChunk->setStatus(UnCompressedChunk::SWITCHING);
        releaseChunk(mCurChunk);
        mCurChunk = NULL;
        return false;
    }

    mCurChunk->setStatus(UnCompressedChunk::CALLING);
    mReadP = mCurChunk->mData;
    return true;
}

void InFile::releaseChunk(UnCompressedChunk* chunk)
{
    if (!chunk) return;
    if (chunk->getRefCount() == 1)
    {
        chunk->release();
        return;
    }

    chunk->release();

    _access();
    if (chunk->getRefCount() > 1)
    {
        if (chunk->getStatus() == UnCompressedChunk::SWITCHING)
        {
            chunk->setStatus(UnCompressedChunk::PENDING);
            pendChunkQueue.push(chunk);
        }
    }
    else if (chunk->getRefCount() == 1)
    {
        if (chunk->getStatus() == UnCompressedChunk::PENDING)
        {
            pendChunkQueue.erase(chunk);
        }
        chunk->mLen = 0;
        chunk->setStatus(UnCompressedChunk::FREE);
        if(!freeChunkQueue.trypush(chunk))
        {
            chunk->release();
        }
    }

    _exit();
}

} // namespace
