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
     ,mMaxSigId()
    {
    }

    virtual ~InFileRA()
    {
        delete [] mExIdToFunc;
        delete [] mExIdToLen;
        delete [] mExIdToName;
        delete [] mCache;
    }

    bool Open(const char* name, bool readHeaderAndExit=false, const std::string& target = std::string());
    virtual void Close();

    void SetReadPos(std::streamoff pos) {
        mStream.seekg(pos, std::ios_base::beg);
    }

    inline bool GetNextCall(void*& fptr, common::BCall& call, char*& src) {
        mStream.read(mCache, sizeof(common::BCall));
        if (mStream.fail()) {
            mStream.clear();
            return false;
        }

        call = *(common::BCall*)mCache;
        unsigned int callLen = mExIdToLen[call.funcId];
        unsigned int contentLen = callLen - sizeof(common::BCall);
        if (callLen == 0) {
            mStream.read((char*)&callLen, sizeof(unsigned int));
            contentLen = callLen - sizeof(common::BCall_vlen);
        }

        if (!this->ReadChunk(contentLen))
        {
            mStream.clear();
            return false;
        }

        mDataPtr = src = mCache;
        mFuncPtr = fptr = mExIdToFunc[call.funcId];

        return true;
    }

    unsigned short NameToExId(const char* str) const {
        for (unsigned short id = 1; id <= mMaxSigId; ++id) {
        if (strcmp(ExIdToName(id), str) == 0)
            return id;
        }
        return 0;
    }
    void copySigBook(std::vector<std::string> &sigbook);

private:
    bool ReadChunk(unsigned int len) {
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

    unsigned int        mCacheLen;
    char*               mCache;

    unsigned int        mMaxSigId;
};

}

#endif
