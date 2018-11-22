#ifndef _RETRACER_HPP_
#define _RETRACER_HPP_

#include "newfastforwarder/framestore.hpp"
#include "newfastforwarder/newfastforwad.hpp"
#include "common/in_file.hpp" //save
#include "common/memory.hpp" //save
#include <string>
#include <unordered_set>

#include <common/api_info.hpp>
#include <string.h>

struct CmdOptions
{
    CmdOptions()
      : fileName()
    {}

    std::string fileName;
    //for fastforwad
    unsigned int fastforwadFrame0 = 0;
    unsigned int fastforwadFrame1 = 0;
    bool allDraws = 0;
    unsigned int clearMask = 17664;//clear GL_COLOR_BUFFER_BIT = 16384; GL_DEPTH_BUFFER_BIT = 256; GL_STENCIL_BUFFER_BIT = 1024
    std::string mOutputFileName = "fastforwardFile.pat";
};

namespace retracer {

struct RetraceOptions
{
    RetraceOptions()
    {}

    ~RetraceOptions()
    {}

    std::string         mFileName;
    int                 mRetraceTid = -1;
    //for fastforwad
    unsigned int fastforwadFrame0 = 0;
    unsigned int fastforwadFrame1 = 0;
    bool allDraws = 0;
    unsigned int clearMask = 17664;//GL_COLOR_BUFFER_BIT = 16384; GL_DEPTH_BUFFER_BIT = 256; GL_STENCIL_BUFFER_BIT = 1024
    std::string mOutputFileName = "fastforwardFile.pat";
private:
};

//class Retracer;

class Retracer
{
public:
    Retracer();
    virtual ~Retracer();
    bool OpenTraceFile(const char* filename);
    bool overrideWithCmdOptions( const CmdOptions &cmdOptions );
    void CloseTraceFile();
    bool RetraceUntilSwapBuffers();
    void OnFrameComplete();
    void OnNewFrame();
    common::HeaderVersion getFileFormatVersion() const { return mFileFormatVersion; }

    int getCurTid();
    unsigned GetCurCallId();
    void IncCurCallId();
    void ResetCurCallId();
    const char* GetCurCallName();
    unsigned GetCurDrawId();
    void IncCurDrawId();
    void ResetCurDrawId();
    unsigned GetCurFrameId();
    void IncCurFrameId();
    void ResetCurFrameId();
    void DispatchWork(int tid, unsigned frameId, int callID, void* fptr, char* src, const char* name, common::UnCompressedChunk *chunk = NULL);

    common::InFile      mFile;
    RetraceOptions      mOptions;
    common::BCall_vlen  mCurCall;
    common::ClientSideBufferObjectSet mCSBuffers;
    unsigned int        mCurCallNo;
    unsigned int        mCurFrameNo;
    unsigned int        mDispatchFrameNo;

    //for fastforward

    newfastforwad::contextState *curContextState;
    newfastforwad::contextState defaultContextState;
    bool defaultContextJudge;
    std::map<int, newfastforwad::contextState> multiThreadContextState;
    std::unordered_set<unsigned int> callNoList;
    bool * callNoListJudge = new bool[100000000];//max API number
    std::set<unsigned int> callNoListOut;
    frameStoreState curFrameStoreState;
    float               timeCount;
    bool                preExecute;
    preExecuteState     curPreExecuteState;
    std::map<unsigned int, unsigned long long> glClientSideBuffDataNoListForShare;
    std::map<int, int> share_context_map;

private:
    bool loadRetraceOptionsByThreadId(int tid);
    void loadRetraceOptionsFromHeader();
    unsigned short mExIdEglSwapBuffers;
    unsigned short mExIdEglSwapBuffersWithDamage;
    common::HeaderVersion mFileFormatVersion;
    unsigned int        mCurDrawNo;
};

inline int Retracer::getCurTid()
{
    int map_tid = 0;
    map_tid = mCurCall.tid;
    return map_tid;
}

inline unsigned Retracer::GetCurCallId()
{
    unsigned id = 0;
    id = mCurCallNo;
    return id;
}

inline const char* Retracer::GetCurCallName()
{
    return mFile.ExIdToName(mCurCall.funcId);
}

inline void Retracer::IncCurCallId()
{
    mCurCallNo++;
}

inline void Retracer::ResetCurCallId()
{
    mCurCallNo = 0;
}

inline unsigned Retracer::GetCurDrawId()
{
    return mCurDrawNo;
}

inline void Retracer::IncCurDrawId()
{
    mCurDrawNo++;
}

inline void Retracer::ResetCurDrawId()
{
    mCurDrawNo = 0;
}

inline unsigned Retracer::GetCurFrameId()
{
    unsigned id = 0;
    id = mCurFrameNo;
    return id;
}

inline void Retracer::IncCurFrameId()
{
    mCurFrameNo++;
}

inline void Retracer::ResetCurFrameId()
{
    mCurFrameNo = 0;
}

extern Retracer gRetracer;


typedef void (*RetraceFunc)(char*);

void ignore(char*);
void unsupported(char*);

extern const common::EntryMap gles_callbacks;
extern const common::EntryMap egl_callbacks;

} /* namespace retracer */

#endif /* _RETRACE_HPP_ */
