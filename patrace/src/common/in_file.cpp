#include <common/in_file.hpp>

namespace common {

bool InFileBase::parseHeader(BHeaderV1 hdrV1, Json::Value &jsonRoot)
{
    jsonRoot["defaultTid"] = 0; // v1 does not store any default
    jsonRoot["glesVersion"] = hdrV1.api;
    jsonRoot["texCompress"] = hdrV1.texCompress;
    jsonRoot["callCnt"] = (Json::Value::UInt64)hdrV1.callCnt;
    jsonRoot["frameCnt"] = hdrV1.frameCnt;
    jsonRoot["threads"] = Json::Value( Json::arrayValue );

    Json::Value jsonThreadArray;
    for (int i=0; i<MAX_RETRACE_THREADS; i++) {
        Json::Value jsThread;
        jsThread["id"] = i;
        // hdrV1 does not record clientSideBufferSize
        //jsThread["clientSideBufferSize"] = 0;

        jsThread["winW"] = hdrV1.winW;
        jsThread["winH"] = hdrV1.winH;

        jsThread["EGLConfig"] = Json::Value( Json::objectValue );
        jsThread["EGLConfig"]["red"] = hdrV1.winRedSize;
        jsThread["EGLConfig"]["green"] = hdrV1.winGreenSize;
        jsThread["EGLConfig"]["blue"] = hdrV1.winBlueSize;
        jsThread["EGLConfig"]["alpha"] = hdrV1.winAlphaSize;
        jsThread["EGLConfig"]["depth"] = hdrV1.winDepthSize;
        jsThread["EGLConfig"]["stencil"] = hdrV1.winStencilSize;
        // hdr v1 does not record samples
        //jsThread["EGLConfig"]["msaaSamples"] = 0;

        jsonRoot["threads"].append( jsThread );
    }

    return true;
}

bool InFileBase::parseHeader(BHeaderV2 hdrV2, Json::Value &jsonRoot)
{
    jsonRoot["defaultTid"] = hdrV2.defaultThreadid;
    jsonRoot["glesVersion"] = hdrV2.api;
    jsonRoot["texCompress"] = hdrV2.texCompress;
    jsonRoot["callCnt"] = (Json::Value::UInt64)hdrV2.callCnt;
    jsonRoot["frameCnt"] = hdrV2.frameCnt;
    jsonRoot["threads"] = Json::Value( Json::arrayValue );

    for (int i=0; i<MAX_RETRACE_THREADS; i++) {
        Json::Value jsThread;
        jsThread["id"] = i;
        // hdrV1 does not record clientSideBufferSize
        jsThread["clientSideBufferSize"] = hdrV2.perThreadClientSideBufferSize[i];

        jsThread["winW"] = hdrV2.winW;
        jsThread["winH"] = hdrV2.winH;

        jsThread["EGLConfig"] = Json::Value( Json::objectValue );
        jsThread["EGLConfig"]["red"] = hdrV2.perThreadEGLConfigs[i].redSize;
        jsThread["EGLConfig"]["green"] = hdrV2.perThreadEGLConfigs[i].greenSize;
        jsThread["EGLConfig"]["blue"] = hdrV2.perThreadEGLConfigs[i].blueSize;
        jsThread["EGLConfig"]["alpha"] = hdrV2.perThreadEGLConfigs[i].alphaSize;
        jsThread["EGLConfig"]["depth"] = hdrV2.perThreadEGLConfigs[i].depthSize;
        jsThread["EGLConfig"]["stencil"] = hdrV2.perThreadEGLConfigs[i].stencilSize;
        jsThread["EGLConfig"]["msaaSamples"] = hdrV2.perThreadEGLConfigs[i].msaaSamples;

        jsonRoot["threads"].append( jsThread );
    }

    return true;
}

static bool checkMember(Json::Value &value, const char* memberToTest)
{
    bool isMember = value.isMember(memberToTest);
    if ( !isMember ) {
        DBG_LOG("Required json member threads[\"%s\"] missing\n", memberToTest);
    }
    return isMember;
}

bool InFileBase::checkJsonMembers(Json::Value &root)
{
    bool rootMembersOk = true;
    rootMembersOk |= checkMember(root, "defaultTid");
    rootMembersOk |= checkMember(root, "glesVersion");
    rootMembersOk |= checkMember(root, "callCnt");
    rootMembersOk |= checkMember(root, "frameCnt");
    rootMembersOk |= checkMember(root, "threads");
    if (!rootMembersOk) return false;

    Json::Value threadArray = root["threads"];
    for (Json::ArrayIndex i=0; i<threadArray.size(); i++)
    {
        bool threadMembersOk = true;
        Json::Value threadItem = threadArray[i];
        threadMembersOk |= checkMember(threadItem, "id");
        threadMembersOk |= checkMember(threadItem, "EGLConfig");
        threadMembersOk |= checkMember(threadItem, "winW");
        threadMembersOk |= checkMember(threadItem, "winH");
        if (!threadMembersOk) return false;
    }
    return true;
}

bool InFileBase::parseHeader(BHeaderV3 hdrV3, Json::Value &jsonRoot)
{
    bool parsingSuccessful = false;
    if ( hdrV3.jsonLength > 0 ) {
        std::ios::streampos fileG = mStream.tellg();
        mStream.seekg(hdrV3.jsonFileBegin, std::ios_base::beg);
        char* jsonBegStr = new char[hdrV3.jsonLength];
        mStream.read(jsonBegStr, hdrV3.jsonLength);
        const char* jsonEndStr= jsonBegStr+hdrV3.jsonLength;
        Json::Reader reader;
        parsingSuccessful = reader.parse(jsonBegStr, jsonEndStr, jsonRoot);
        delete[] jsonBegStr;
        mStream.seekg(fileG, std::ios_base::beg);
    } else {
        DBG_LOG("hdrV3.jsonLength <= 0 \n");
        return false;
    }

    if (!parsingSuccessful) {
        DBG_LOG("parse json failed\n");
        return false;
    } else {
        return checkJsonMembers(mJsonHeader);
    }

    return false; // unreachable
}

const std::string InFileBase::getJSONHeaderAsString(bool prettyPrint)
{
    if (prettyPrint) {
        Json::StyledWriter writer;
        return writer.write( mJsonHeader );
    } else {
        Json::FastWriter writer;
        return writer.write( mJsonHeader );
    }
    return ""; // unreachable
}

int InFileBase::getDefaultThreadID() const
{
    return mJsonHeader.get("defaultTid", 0).asInt();
}

const Json::Value InFileBase::getJSONThreadById(int id) const
{
    const Json::Value threadArray = mJsonHeader["threads"];
    for (const auto& i : threadArray) if (i["id"].asInt() == id) return i;
    DBG_LOG("Could not find thread id %d in json value\n", id);
    return Json::Value();
}

HeaderVersion InFileBase::getHeaderVersion() const
{
    return mHeaderVer;
}

void InFileBase::printHeaderInfo()
{
    const int tid = mJsonHeader.get("defaultTid", 0).asInt();
    Json::Value defaultThread = getJSONThreadById(tid);
    Json::Value defaultEGL = defaultThread["EGLConfig"];
    printf("default tid  %d\n", defaultThread["id"].asInt() );
    printf("red bits     %d\n", defaultEGL["red"].asInt() );
    printf("green bits   %d\n", defaultEGL["green"].asInt() );
    printf("blue bits    %d\n", defaultEGL["blue"].asInt() );
    printf("alpha bits   %d\n", defaultEGL["alpha"].asInt() );
    printf("depth bits   %d\n", defaultEGL["depth"].asInt() );
    printf("stencil bits %d\n", defaultEGL["stencil"].asInt() );
    printf("msaa         %d\n", defaultEGL["msaaSamples"].asInt() );
    printf("winWidth     %d\n", defaultThread["winW"].asInt() );
    printf("winHeight    %d\n", defaultThread["winH"].asInt() );
    printf("texCompress  %d\n", defaultThread["texCompress"].asInt() );
    printf("frame count  %d\n", mJsonHeader["frameCnt"].asInt() );
    printf("GLES version %d\n", mJsonHeader["glesVersion"].asInt() );
    printf("tracer       %s\n", mJsonHeader["tracer"].asString().c_str());
}

void InFileBase::setFrameRange(unsigned startFrame, unsigned endFrame, int tid, bool preload, bool keep_all)
{
    mKeepAll = keep_all;
    mPreload = preload;
    mBeginFrame = startFrame;
    mEndFrame = endFrame;
    mTraceTid = tid;
    eglSwapBuffers_id = NameToExId("eglSwapBuffers");
    eglSwapBuffersWithDamage_id = NameToExId("eglSwapBuffersWithDamageKHR");
}

} // namespace common
