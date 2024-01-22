#include <common/in_file_mt.hpp>
#include <common/memoryinfo.hpp>
#include <common/trace_limits.hpp>

#include "json/writer.h"
#include "json/reader.h"

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

namespace common {

void InFile::rollback()
{
    if (mCheckpointOffset == -1)
    {
        DBG_LOG("No checkpoint set - not able to rollback!\n");
        abort();
    }
    mPreloadedChunks.push_front(mCurrentChunk);
    while (mFreeChunks.size())
    {
        mCurrentChunk = mFreeChunks.back();
        mFreeChunks.pop_back();
        mPreloadedChunks.push_front(mCurrentChunk);
    }
    mCurrentChunk = mPreloadedChunks.front();
    mPreloadedChunks.pop_front();
    mPtr = mCurrentChunk->data() + mCheckpointOffset;
    mFrameNo = mBeginFrame;
    mChunkEnd = mCurrentChunk->data() + mCurrentChunk->size();
}

// Read another uncompressed memory chunk from the memory mapped file
bool InFile::readChunk(std::vector<char> *buf)
{
    if (mCompressedRemaining < 4) { return false; }
    size_t compressedLength = *(unsigned*)mCompressedSource;
    mCompressedRemaining -= 4;
    mCompressedSource += 4;
    if ((int64_t)compressedLength <= mCompressedRemaining)
    {
        size_t uncompressedLength = 0;
        if (!snappy::GetUncompressedLength(mCompressedSource, compressedLength, &uncompressedLength))
        {
            DBG_LOG("Failed to parse chunk of size %u - file is corrupt - aborting!\n", (unsigned)compressedLength);
            abort();
        }
        buf->resize(uncompressedLength);
        if (!snappy::RawUncompress(mCompressedSource, compressedLength, buf->data()))
        {
            DBG_LOG("Failed to decompress chunk of size %u - file is corrupt - aborting!\n", (unsigned)compressedLength);
            abort();
        }
        mCompressedSource += compressedLength;
        mCompressedRemaining -= compressedLength;
        return true;
    }
    return false;
}

bool InFile::Open(const char* name, bool readHeaderAndExit)
{
    mFileName = name;
    mIsOpen = true;

    // Memory map file
    mFd = open(mFileName.c_str(), O_RDONLY);
    if (mFd == -1)
    {
        DBG_LOG("Failed to open %s: %s\n", mFileName.c_str(), strerror(errno));
        return false;
    }
    struct stat64 sb;
    if (fstat64(mFd, &sb) == -1)
    {
        DBG_LOG("Failed to stat %s: %s\n", mFileName.c_str(), strerror(errno));
        close(mFd);
        return false;
    }
    mCompressedSize = mCompressedRemaining = sb.st_size;
    mCompressedBuffer = (char*)mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, mFd, 0);
    if (mCompressedBuffer == MAP_FAILED)
    {
        DBG_LOG("Failed to mmap %s: %s\n", mFileName.c_str(), strerror(errno));
        close(mFd);
        return false;
    }
    madvise(mCompressedBuffer, sb.st_size, MADV_SEQUENTIAL);

    // Read Base Header that is common for all header versions
    common::BHeader* header = (common::BHeader*)mCompressedBuffer;
    if (header->magicNo != 0x20122012)
    {
        DBG_LOG("Error: %s seems to be an invalid trace file!\n", mFileName.c_str());
        close(mFd);
        return false;
    }
    mHeaderVer = static_cast<HeaderVersion>(header->version);
    DBG_LOG("### .pat file format Version %d ###\n", header->version - HEADER_VERSION_1 + 1);

    if (header->version == HEADER_VERSION_1)
    {
        if (!parseHeader(*(BHeaderV1*)mCompressedBuffer, mJsonHeader)) return false;
        mCompressedSource = mCompressedBuffer + sizeof(BHeaderV1);
    }
    else if (header->version == HEADER_VERSION_2)
    {
        if (!parseHeader(*(BHeaderV2*)mCompressedBuffer, mJsonHeader)) return false;
        mCompressedSource = mCompressedBuffer + sizeof(BHeaderV2);
    }
    else if (header->version == HEADER_VERSION_3 || header->version == HEADER_VERSION_4)
    {
        BHeaderV3 *hdr = (BHeaderV3*)mCompressedBuffer;
        Json::Reader reader;
        if (!reader.parse(mCompressedBuffer + hdr->jsonFileBegin, mCompressedBuffer + hdr->jsonFileEnd, mJsonHeader)
            || !checkJsonMembers(mJsonHeader))
        {
            DBG_LOG("Error: %s seems to have an invalid JSON header!\n", mFileName.c_str());
            close(mFd);
            return false;
        }
        mCompressedSource = mCompressedBuffer + hdr->jsonFileEnd;
    }
    else
    {
        DBG_LOG("Unsupported file format version: %d\n", header->version - HEADER_VERSION_1 + 1);
        close(mFd);
        return false;
    }
    mCompressedRemaining -= mCompressedSource - mCompressedBuffer;

    // when we only wanted to use -info to see header contents, no playback
    if (readHeaderAndExit)
    {
        close(mFd);
        return true;
    }

    // Read first chunk
    mCurrentChunk = new std::vector<char>;
    mPrevChunk = new std::vector<char>;
    if (!readChunk(mCurrentChunk))
    {
        DBG_LOG("Failed to read first chunk!\n");
        return false;
    }
    mPtr = mCurrentChunk->data();
    mChunkEnd = mCurrentChunk->data() + mCurrentChunk->size();

    ReadSigBook();
    return true;
}

void InFile::PreloadFrames(int frames_to_read, int tid)
{
    int frames_read = 0;
    std::vector<char> *newchunk = new std::vector<char>;
    mCheckpointOffset = mPtr - mCurrentChunk->data();
    while (readChunk(newchunk) && frames_read < frames_to_read)
    {
        mPreloadedChunks.push_back(newchunk);

        // Count the number of frames in the chunk
        char *ptr = newchunk->data();
        while (ptr < newchunk->data() + newchunk->size())
        {
            const common::BCall& call = *(common::BCall*)ptr;
            if (call.funcId == eglCreatePbufferSurface_id)
            {
                mPbufferSurfaces.insert(getCreatePbufferSurfaceRet(ptr + sizeof(common::BCall_vlen)));
            }
            if ((call.tid == tid || tid == -1) && (call.funcId == eglSwapBuffers_id || call.funcId == eglSwapBuffersWithDamageKHR_id || call.funcId == eglSwapBuffersWithDamageEXT_id))
            {
                char *src;
                const unsigned callLen = mExIdToLen[call.funcId];
                if (callLen == 0)
                {
                    src = ptr + sizeof(common::BCall_vlen);
                }
                else
                {
                    src = ptr + sizeof(common::BCall);
                }
                if (mPbufferSurfaces.count(getDpySurface(src))==0)
                {
                    frames_read++;
                }
            }
            unsigned int callLen = mExIdToLen[call.funcId];
            if (callLen == 0)
            {
                ptr += reinterpret_cast<common::BCall_vlen*>(ptr)->toNext;
            } else {
                ptr += callLen;
            }
       }

       newchunk = new std::vector<char>;
   }
   delete newchunk; // we always make one in excess
   mPreload = false;
}

bool InFile::GetNextCall(void*& fptr, common::BCall_vlen& call, char*& src)
{
    if (mFrameNo >= mEndFrame) return false; // we're done!

    if (mPtr + sizeof(common::BCall) > mChunkEnd) // read more data?
    {
        if (mPreloadedChunks.size() > 0)
        {
            if (mKeepAll)
            {
                mFreeChunks.push_back(mCurrentChunk);
                mCurrentChunk = mPreloadedChunks.front();
            }
            else
            {
                delete mPrevChunk;
                std::swap(mPrevChunk, mCurrentChunk);
                mCurrentChunk = mPreloadedChunks.front();
            }
            mPreloadedChunks.pop_front();
        }
        else
        {
            if (!readChunk(mPrevChunk)) return false;
            std::swap(mPrevChunk, mCurrentChunk);
        }
        mPtr = mCurrentChunk->data();
        mChunkEnd = mCurrentChunk->data() + mCurrentChunk->size();
    }

    common::BCall tmp;
    tmp = *(common::BCall*)mPtr;
    if (unlikely(tmp.funcId > mMaxSigId || tmp.funcId == 0))
    {
        DBG_LOG("funcId %d is out of range (%d max)!\n", (int)tmp.funcId, mMaxSigId);
        ::abort();
    }

    const unsigned callLen = mExIdToLen[tmp.funcId];
    if (callLen == 0)
    {
        // Call is in BCall_vlen format -- read it directly
        call = *(common::BCall_vlen*)mPtr;
        mDataPtr = src = mPtr + sizeof(common::BCall_vlen);
        mPtr += call.toNext;
    }
    else
    {
        call = tmp;
        mDataPtr = src = mPtr + sizeof(common::BCall);
        mPtr += callLen;
    }

    fptr = mExIdToFunc[call.funcId];

    // Count frames and check if we are done or need to start preloading
    if ((tmp.tid == mTraceTid || mTraceTid == -1) && (tmp.funcId == eglSwapBuffers_id || call.funcId == eglSwapBuffersWithDamageKHR_id || call.funcId == eglSwapBuffersWithDamageEXT_id))
    {
        if (mPbufferSurfaces.count(getDpySurface(src))==0)
        {
            mFrameNo++;
            if (mFrameNo >= mBeginFrame && mPreload)
            {
                // The below count does not include frames still remaining to be read in the current chunk, so we might possibly
                // be reading more chunks than we need here. Still room to optimize more.
                PreloadFrames(mEndFrame - mBeginFrame, mTraceTid);
            }
        }
    }

    if (tmp.funcId == eglCreatePbufferSurface_id)
    {
        mPbufferSurfaces.insert(getCreatePbufferSurfaceRet(src));
    }

    if (tmp.funcId == eglDestroySurface_id)
    {
        mPbufferSurfaces.erase(getDpySurface(src));
    }

    curCallNo++;
    return true;
}

void InFile::Close()
{
    if (!mIsOpen) return;
    munmap(mCompressedBuffer, mCompressedSize);
    close(mFd); mFd = 0;
    mIsOpen = false;
    mPreload = false;
    for (auto* b : mPreloadedChunks) delete b;
    for (auto* b : mFreeChunks) delete b;
    mPreloadedChunks.clear();
    mFreeChunks.clear();
    delete mCurrentChunk; mCurrentChunk = nullptr;
    delete mPrevChunk; mPrevChunk = nullptr;
    mExIdToName.clear();
    delete [] mExIdToLen; mExIdToLen = nullptr;
    delete [] mExIdToFunc; mExIdToFunc = nullptr;
}

void InFile::ReadSigBook()
{
    unsigned int toNext;
    mPtr = ReadFixed(mPtr, toNext);
    mPtr = ReadFixed(mPtr, mMaxSigId);

    mExIdToName.resize(mMaxSigId + 1);
    mExIdToName[0] = "";
    for (int id = 1; id <= mMaxSigId; ++id)
    {
        unsigned int id_notused;
        char *mPtr_tmp = nullptr;
        mPtr_tmp = ReadFixed<unsigned int>(mPtr, id_notused);
        if(id == (int)id_notused)
        {
            mPtr = mPtr_tmp;
            char *str;
            mPtr = ReadString(mPtr, str);
            mExIdToName[id] = str ? str : "";
        }
        else
        {
            mExIdToName[id] = "";
            continue;
        }
    }

    mExIdToLen = new int[mMaxSigId + 1];
    mExIdToFunc = new void*[mMaxSigId + 1];

    mExIdToLen[0] = 0;
    mExIdToFunc[0] = 0;
    for (int id = 1; id <= mMaxSigId; ++id)
    {
        const char* name = mExIdToName.at(id).c_str();
        // glTexStorageAttribs*DARM was a short-lived experiment that nonetheless lives on in some sigbooks, just not in use anywhere.
        // GetObjectLabel was a function typo introduced in version r1p0, fixed in r2p0.
        if (id > 4 && gApiInfo.NameToId(name) == 0 && strcmp(name, "") != 0 && strcmp(name, "glTexStorageAttribs2DARM") != 0 && strcmp(name, "glTexStorageAttribs3DARM") != 0
            && strcmp(name, "GetObjectLabel") != 0)
        {
            DBG_LOG("We have no implementation of function %s (id %d) - replayer likely too old to play this trace file!\n", name, id);
#ifndef __APPLE__
            exit(EXIT_FAILURE); // do not crash, as this is going to happen, and is not an application bug
#else
            os::abort(); // we want an exception for MacOSX
#endif
        }
        mExIdToLen[id] = gApiInfo.NameToLen(name);
        mExIdToFunc[id] = gApiInfo.NameToFptr(name);
    }
}

} // namespace
