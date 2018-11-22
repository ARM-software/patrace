#ifndef _IN_FILE_HPP_
#define _IN_FILE_HPP_

#include <fstream>
#include <string>
#include <jsoncpp/include/json/writer.h>
#include <jsoncpp/include/json/reader.h>

#include <common/file_format.hpp>

namespace common {

class InFileBase
{
public:
    InFileBase()
     :mIsOpen(false)
     ,mStream()
     ,mFileName()
     ,mJsonHeader()
     ,mHeaderParseComplete(false)
     ,mFuncPtr(NULL)
     ,mDataPtr(NULL)
     ,mExIdToName(NULL)
     ,mExIdToLen(NULL)
     ,mExIdToFunc(NULL)
     ,mHeaderVer(HEADER_VERSION_1)
    {
    }

    virtual ~InFileBase()
    {
        Close();
    }

    bool Open();
    virtual void Close();

    void printHeaderInfo();
    const std::string getJSONHeaderAsString(bool prettyPrint=false);
    const Json::Value getJSONHeader() { return mJsonHeader; }
    const Json::Value getJSONThreadDefault();
    const Json::Value getJSONThreadById(int id);
    HeaderVersion getHeaderVersion() const;

    const char* ExIdToName(unsigned short id) const
    {
        return mExIdToName[id].c_str();
    }

    int getDefaultThreadID() const;

    std::streamoff GetReadPos()
    {
        return mStream.tellg();
    }

    void* functionPointer()
    {
        return mFuncPtr;
    }

    char* dataPointer()
    {
        return mDataPtr;
    }

protected:
    bool parseHeader(BHeaderV1 hdrV1, Json::Value &value);
    bool parseHeader(BHeaderV2 hdrV2, Json::Value &value);
    bool parseHeader(BHeaderV3 hdrV3, Json::Value &value);

    bool                mIsOpen;
    std::fstream        mStream;
    std::string         mFileName;

    Json::Value         mJsonHeader;
    bool                mHeaderParseComplete;
    void*               mFuncPtr;
    char*               mDataPtr;
    std::string*        mExIdToName;
    int*                mExIdToLen;
    void**              mExIdToFunc;

private:
    HeaderVersion        mHeaderVer;
};

} // namespace common

#include <common/in_file_mt.hpp>

#endif // _IN_FILE_HPP_
