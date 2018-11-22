#ifndef _TRACER_OUT_FILE_HPP_
#define _TRACER_OUT_FILE_HPP_

#include <fstream>
#include <string>

#include <common/file_format.hpp>
#include <common/os_string.hpp>

namespace common {

#define SNAPPY_CHUNK_SIZE (1*1024*1024)

class OutFile {
public:
    OutFile();
    OutFile(const char *name);
    ~OutFile();

    bool Open(const char* name = NULL, bool writeSigBook = true);
    void Close();
    void Flush();
    void WriteHeader(const char* buf, unsigned int len, bool verbose = true);

    inline void Write(const void* buf, unsigned int len) {
        if (len == 0 || !mIsOpen)
            return;

        if (FreeSize() > len) {
            memcpy(mCacheP, buf, len);
            mCacheP += len;
        } else if (FreeSize() == len) {
            memcpy(mCacheP, buf, len);
            mCacheP += len;
            Flush();
        } else {
            Flush();
            if (mCacheLen < int(len))
                CreateCache(len);
            memcpy(mCacheP, buf, len);
            mCacheP += len;
        }
    }

    std::string getFileName() const;

    common::BHeaderV3   mHeader;

private:
    void CreateCache(int len);

    inline unsigned int UsedSize() const {
        return mCacheP - mCache;
    }

    inline unsigned int FreeSize() const {
        return mCacheLen - UsedSize();
    }

    void WriteCompressedLength(unsigned int len) {
        unsigned char buf[4];
        buf[0] = len & 0xff; len >>= 8;
        buf[1] = len & 0xff; len >>= 8;
        buf[2] = len & 0xff; len >>= 8;
        buf[3] = len & 0xff; len >>= 8;
        mStream.write((const char *)buf, sizeof(buf));
    }

    void FlushHeader();

    void WriteSigBook();

    os::String AutogenTraceFileName();

    bool                mIsOpen;
    std::fstream        mStream;

    char*               mCache;
    int                 mCacheLen;
    char*               mCacheP;
    char*               mCompressedCache;
    int                 mCompressedCacheLen;


    std::string         mFileName;
};

}

#endif
