#ifndef _IN_FILE_HPP_
#define _IN_FILE_HPP_

#include <fstream>
#include <string>
#include <vector>

#include "json/writer.h"
#include "json/reader.h"
#include "common/file_format.hpp"

namespace common {

class InFileBase
{
public:
    void printHeaderInfo();
    const std::string getJSONHeaderAsString(bool prettyPrint=false);
    const Json::Value getJSONHeader() const { return mJsonHeader; }
    const Json::Value getJSONThreadById(int id) const;
    HeaderVersion getHeaderVersion() const;

    const bool isFFTrace() const;

    const char* ExIdToName(unsigned short id) const { return mExIdToName.at(id).c_str(); }
    int getDefaultThreadID() const;
    inline bool getMultithread() const { return mMultithread; }
    char* dataPointer() const { return mDataPtr; }

    inline unsigned short NameToExId(const char* str) const
    {
        for (int id = 1; id <= mMaxSigId; ++id)
        {
            if (strcmp(ExIdToName(id), str) == 0) return id;
        }
        return 0;
    }

    inline int getCreatePbufferSurfaceRet(char *src)
    {
        int dpy;
        int config;
        Array<unsigned int> attrib_list;
        int ret;
        src = ReadFixed(src, dpy);
        src = ReadFixed(src, config);
        src = Read1DArray(src, attrib_list);
        src = ReadFixed(src, ret);
        return ret;
    }

    inline int getDpySurface(char *src)
    {
        int dpy;
        int surface;
        src = ReadFixed(src, dpy);
        src = ReadFixed(src, surface);
        return surface;
    }

    void setFrameRange(unsigned startFrame, unsigned endFrame, int tid, bool preload, bool keep_all = false);

    inline int getMaxSigId() const { return mMaxSigId; }
    inline const std::vector<std::string>& getFuncNames() const { return mExIdToName; }

protected:
    bool parseHeader(BHeaderV1 hdrV1, Json::Value &value);
    bool parseHeader(BHeaderV2 hdrV2, Json::Value &value);
    bool parseHeader(BHeaderV3 hdrV3, Json::Value &value);
    bool checkJsonMembers(Json::Value &root);

    bool                mIsOpen = false;
    bool                mMultithread = false;
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
    int eglSwapBuffersWithDamageKHR_id = -1;
    int eglSwapBuffersWithDamageEXT_id = -1;
    int eglCreatePbufferSurface_id = -1;
    int eglDestroySurface_id = -1;
    bool mPreload = false;

    HeaderVersion mHeaderVer = HEADER_VERSION_1;
};

} // namespace common

#endif // _IN_FILE_HPP_
