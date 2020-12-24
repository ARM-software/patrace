#ifndef _IN_FILE_HPP_
#define _IN_FILE_HPP_

#include <fstream>
#include <string>
#include <vector>
#include <jsoncpp/include/json/writer.h>
#include <jsoncpp/include/json/reader.h>

#include <common/file_format.hpp>

namespace common {

class InFileBase
{
public:
    void printHeaderInfo();
    const std::string getJSONHeaderAsString(bool prettyPrint=false);
    const Json::Value getJSONHeader() const { return mJsonHeader; }
    const Json::Value getJSONThreadById(int id) const;
    HeaderVersion getHeaderVersion() const;

    const char* ExIdToName(unsigned short id) const { return mExIdToName.at(id).c_str(); }
    int getDefaultThreadID() const;
    char* dataPointer() const { return mDataPtr; }

    inline unsigned short NameToExId(const char* str) const
    {
        for (int id = 1; id <= mMaxSigId; ++id)
        {
            if (strcmp(ExIdToName(id), str) == 0) return id;
        }
        return 0;
    }

    void setFrameRange(unsigned startFrame, unsigned endFrame, int tid, bool preload, bool keep_all = false);

protected:
    bool parseHeader(BHeaderV1 hdrV1, Json::Value &value);
    bool parseHeader(BHeaderV2 hdrV2, Json::Value &value);
    bool parseHeader(BHeaderV3 hdrV3, Json::Value &value);
    bool checkJsonMembers(Json::Value &root);

    bool                mIsOpen = false;
    std::fstream        mStream;
    std::string         mFileName;
    bool mKeepAll = false;

    Json::Value mJsonHeader;
    bool mHeaderParseComplete = false;
    char *mDataPtr = nullptr;
    std::vector<std::string> mExIdToName;
    int *mExIdToLen = nullptr;
    void **mExIdToFunc = nullptr;

    int mMaxSigId = -1;
    int mBeginFrame = -1;
    int mEndFrame = 2147483647; // or INT32_MAX (which does not work on Android)
    int mTraceTid = -1;
    int eglSwapBuffers_id = -1;
    int eglSwapBuffersWithDamage_id = -1;
    bool mPreload = false;

    HeaderVersion mHeaderVer = HEADER_VERSION_1;
};

} // namespace common

#endif // _IN_FILE_HPP_
