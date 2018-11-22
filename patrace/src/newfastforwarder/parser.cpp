#define __STDC_FORMAT_MACROS

#include "newfastforwarder/parser.hpp"
#include "newfastforwarder/newfastforwad.hpp"
#include <string.h>

using namespace common;
using namespace std;

namespace retracer {

Retracer gRetracer;

const unsigned int MAX_API_NUMBER = 100000000;
const unsigned int LOG_STEP = 1000000;

Retracer::Retracer()
 : mFile()
 , mOptions()
 , mCurCall()
 , mCurCallNo(0)
 , mCurFrameNo(0)
 , mDispatchFrameNo(0)
 , curContextState(NULL)
 , defaultContextState(glClientSideBuffDataNoListForShare)
 , defaultContextJudge(0)
 , curFrameStoreState()
 , timeCount(0)
 , preExecute(0)
 , curPreExecuteState()
 , mExIdEglSwapBuffers(0)
 , mExIdEglSwapBuffersWithDamage(0)
 , mFileFormatVersion(INVALID_VERSION)
 , mCurDrawNo(0)
{
    curContextState = &defaultContextState;
    fill(callNoListJudge, callNoListJudge+MAX_API_NUMBER, 0);
}

Retracer::~Retracer()
{}

bool Retracer::OpenTraceFile(const char* filename)
{
    mCurCallNo = 0;
    mCurDrawNo = 0;
    mCurFrameNo = 0;
    mDispatchFrameNo = 0;
    mExIdEglSwapBuffers = 0;
    mExIdEglSwapBuffersWithDamage = 0;
    mFile.prepareChunks();
    if (!mFile.Open(filename))
    {
        DBG_LOG("Failed to open\n");
        return false;
    }

    mFileFormatVersion = mFile.getHeaderVersion();
    loadRetraceOptionsFromHeader();
    mExIdEglSwapBuffers = mFile.NameToExId("eglSwapBuffers");
    mExIdEglSwapBuffersWithDamage = mFile.NameToExId("eglSwapBuffersWithDamageKHR");

    return true;
}

void Retracer::CloseTraceFile()
{
    mFile.Close();
    mFileFormatVersion = INVALID_VERSION;
}

void Retracer::loadRetraceOptionsFromHeader()
{
    // Load values from headers first, then any valid commandline parameters override the header defaults.
    const Json::Value jsHeader = mFile.getJSONHeader();
    mOptions.mRetraceTid = jsHeader.get("defaultTid", -1).asInt();
}

bool Retracer::overrideWithCmdOptions( const CmdOptions &cmdOptions )
{
    //for fastforwad
    mOptions.fastforwadFrame0 = cmdOptions.fastforwadFrame0;
    mOptions.fastforwadFrame1 = cmdOptions.fastforwadFrame1;
    mOptions.allDraws = cmdOptions.allDraws;
    mOptions.clearMask = cmdOptions.clearMask;
    mOptions.mOutputFileName = cmdOptions.mOutputFileName;
    return true;
}

bool Retracer::RetraceUntilSwapBuffers()
{
    void* fptr;
    char* src;
    UnCompressedChunk *callChunk;

    for (;;mCurCallNo++)
    {
        if(mCurCallNo%LOG_STEP == 0)
        {
            printf("callNo %d\n",mCurCallNo);
        }
        //curContextState->curBufferState.debug(mCurCallNo);//forDebug
        if (!mFile.GetNextCall(fptr, mCurCall, src, callChunk))
        {
            CloseTraceFile();
            return false;
        }
        if ( mCurCall.tid != mOptions.mRetraceTid)
        {
            continue;
        }
        const bool isSwapBuffers = (mCurCall.funcId == mExIdEglSwapBuffers || mCurCall.funcId == mExIdEglSwapBuffersWithDamage);
        //const char *funcName = mFile.ExIdToName(mCurCall.funcId);
        //bool doSkip =false;// mOptions.mSkipCallSet && (mOptions.mSkipCallSet->contains(mCurCallNo, funcName));
        if (fptr)
        {
            (*(RetraceFunc)fptr)(src);
            //DispatchWork(mCurCall.tid, mDispatchFrameNo, mCurCallNo, fptr, src, funcName, callChunk);
            if (isSwapBuffers && mCurCall.tid == mOptions.mRetraceTid)
            {
                mDispatchFrameNo++;
            }
        }
        // End conditions
    }
}

void Retracer::OnNewFrame()
{
    if (getCurTid() == mOptions.mRetraceTid)
    {
        // swap (or flush for offscreen) called between OnFrameComplete() and OnNewFrame()
        mCurFrameNo++;
        // End conditions
    }
}

//void Retracer::DispatchWork(int tid, unsigned frameId, int callID, void* fptr, char* src, const char* name, common::UnCompressedChunk *chunk)
//{
//    (*(RetraceFunc)fptr)(src);
//}


void ignore(char* )
{
}

void unsupported(char* )
{
    DBG_LOG("Unsupported: \n");
}

} /* namespace retracer */
