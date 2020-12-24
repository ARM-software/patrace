#ifndef _COMMON_IN_FILE_RA_HPP_
#define _COMMON_IN_FILE_RA_HPP_

#include <common/api_info.hpp>
#include <common/in_file.hpp>

#include <snappy.h>

namespace common {

class InFileRA : public InFileBase {
public:
    InFileRA()
     :InFileBase()
     ,mCacheLen(1024)
     ,mCache(new char[mCacheLen])
    {
    }

    ~InFileRA()
    {
        delete [] mExIdToFunc;
        delete [] mExIdToLen;
        mExIdToName.clear();
        delete [] mCache;
    }

    void setTarget(const std::string& target) { mTarget = target; }
    bool Open(const char *name, bool readHeaderAndExit = false);
    void Close() { mStream.close(); }

    std::streamoff GetReadPos()
    {
        return mStream.tellg();
    }

    void SetReadPos(std::streamoff pos)
    {
        mStream.seekg(pos, std::ios_base::beg);
    }

    bool GetNextCall(void*& fptr, common::BCall_vlen& call, char*& src)
    {
        mStream.read(mCache, sizeof(common::BCall));
        if (mStream.fail())
        {
            mStream.clear();
            return false;
        }

        common::BCall tmpCall;
        tmpCall = *(common::BCall*)mCache;
        unsigned int callLen = mExIdToLen[tmpCall.funcId];
        unsigned int contentLen;
        if (callLen == 0)
        {
            mStream.read((char*)&callLen, sizeof(unsigned int));
            contentLen = callLen - sizeof(common::BCall_vlen);
            call = *(common::BCall_vlen*)mCache;
        }
        else
        {
            contentLen = callLen - sizeof(common::BCall);
            call = tmpCall;
        }

        if (!this->ReadChunk(contentLen))
        {
            mStream.clear();
            return false;
        }

        mDataPtr = src = mCache;
        fptr = mExIdToFunc[call.funcId];

        return true;
    }

    void copySigBook(std::vector<std::string> &sigbook);

private:
    inline bool ReadChunk(unsigned int len)
    {
        if (mCacheLen < len) {
            mCacheLen = len * 2;
            delete [] mCache;
            mCache = new char[mCacheLen];
        }
        mStream.read(mCache, len);
        if (mStream.fail())
            return false;
        return true;
    }

    unsigned int ReadCompressedLength(std::fstream& inStream);
    bool CreateRAFile(const char* name, const std::string& target);
    void ReadSigBook();

    unsigned int mCacheLen;
    char *mCache;
    std::string mTarget;
};

}

#endif
