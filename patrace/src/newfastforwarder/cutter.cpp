#include "common/out_file.hpp"
#include <newfastforwarder/parser.hpp>


namespace RetraceAndTrim
{
class ScratchBuffer
{
public:
    ScratchBuffer(size_t initialCapacity = 0)
        : mVector()
    {
        resizeToFit(initialCapacity);
    }
    // Only use the returned ptr after making sure the buffer is large enough
    char* bufferPtr()
    {
        return mVector.data();
    }
    // Pointers returned from bufferPtr() previously
    // are invalid after calling this!
    void resizeToFit(size_t len)
    {
        mVector.reserve(len);
    }
private:
    // Noncopyable
    ScratchBuffer(const ScratchBuffer&);
    ScratchBuffer& operator=(const ScratchBuffer&);
    std::vector<char> mVector;
};
}//RetraceAndTrim

bool retraceAndTrim(common::OutFile &out, const CmdOptions& mOptions)
{
    using namespace retracer;
    RetraceAndTrim::ScratchBuffer buffer;
    retracer::Retracer& retracer = gRetracer;
    //------------------------for debug
    //std::ifstream inFile;
    //std::ofstream outFile;
    //outFile.open("api.txt");
    //inFile.open("inputForDebug.txt.ff.zz");
    //std::string str;
    //std::istringstream fileLine;
    //getline(inFile, str, '\n');
    //fileLine.str(str);
    //---------------------------
    unsigned int judge;
    std::set<unsigned int>::iterator set_iter = retracer.callNoListOut.begin();
    if(*set_iter == 0)set_iter++;
    judge=*set_iter;

    for ( ;; retracer.IncCurCallId())
    {
        void *fptr = NULL;
        char *src = NULL;
        if (!retracer.mFile.GetNextCall(fptr, retracer.mCurCall, src))// /*|| retracer.mFinish*/
        {
            // No more calls.
            retracer.CloseTraceFile();
            return true;
        }
        const char *funcName = retracer.mFile.ExIdToName(retracer.mCurCall.funcId);
        if (retracer.getCurTid() != retracer.mOptions.mRetraceTid)
        {
            continue; // we're skipping calls from out file here, too; probably what user wants?
        }
        // Save calls.
        // Calling the function might modify what's pointed to by src (e.g. ReadStringArray does this),
        // so it's important that we copy the call before actually calling the function.
        if (true)
        {
            bool shouldSkip = false;//(strstr(funcName, "SwapBuffers"));
            bool targetFrameOrLater = false;//(retracer.GetCurFrameId() == mOptions.fastforwadFrame);
            //--------------- for debug
            //fileLine.str("");
            //fileLine.clear();
            if(retracer.mCurCallNo>=0)
            {
                if(retracer.mCurCallNo!=judge)shouldSkip=true;
                if(retracer.mCurCallNo == judge)
                {
                    shouldSkip=false;
                    set_iter++;
                    judge = *set_iter;

                    //for debug
                    //getline(inFile, str, '\n');
                    //outFile<<judge<<"    "<<retracer.mCurCallNo;
                    //outFile<<funcName<<std::endl;
                    //fileLine.str(str);
                }
            }//if

            //Note: callNo will_vary_depending_on_the_content. This clip of code is to go to the specific draw for debugging
            //if(retracer.mCurCallNo <= callNo)
            //{
            //      shouldSkip=false;
            //}
            //----------------------

            if ( (strstr(funcName, "SwapBuffers")||strstr(funcName, "eglSwapBuffersWithDamageKHR")) && (retracer.GetCurFrameId()+1 == mOptions.fastforwadFrame0))///*ffOptions.mTargetFrame*/
            {
                // We save the call before the call is executed, and GetCurFrameId() isn't
                // updated until the call (SwapBuffers) is made. This handles the case where this
                // is the last swap before the target frame, so that it isn't wrongly skipped.
                // (I.e., this swap marks the start of the target frame -- or equivalently, the end
                // of frame 0.)
                targetFrameOrLater = true;
            }
            if ( (strstr(funcName, "SwapBuffers")||strstr(funcName, "eglSwapBuffersWithDamageKHR")) && (retracer.GetCurFrameId() >= mOptions.fastforwadFrame0 && retracer.GetCurFrameId() <= mOptions.fastforwadFrame1))///*ffOptions.mTargetFrame*/
            {
                // We save the call before the call is executed, and GetCurFrameId() isn't
                // updated until the call (SwapBuffers) is made. This handles the case where this
                // is the last swap before the target frame, so that it isn't wrongly skipped.
                // (I.e., this swap marks the start of the target frame -- or equivalently, the end
                // of frame 0.)
                targetFrameOrLater = true;
            }
            // Until the target frame, output everything but skipped calls. After that, output everything.
            if (targetFrameOrLater || !shouldSkip)
            {
                // Translate funcId for call to id in current sigbook.
                unsigned short newId = common::gApiInfo.NameToId(funcName);
                common::BCall_vlen outBCall = retracer.mCurCall;
                outBCall.funcId = newId;
                if (outBCall.toNext == 0)
                {
                    // It's really a BCall-struct, so only copy the BCall part of it
                    buffer.resizeToFit(sizeof(common::BCall) + common::gApiInfo.IdToLenArr[newId]);
                    char* curScratch = buffer.bufferPtr();
                    memcpy(curScratch, &outBCall, sizeof(common::BCall));
                    curScratch += sizeof(common::BCall);
                    memcpy(curScratch, src, common::gApiInfo.IdToLenArr[newId] - sizeof(common::BCall));
                    curScratch += common::gApiInfo.IdToLenArr[newId] - sizeof(common::BCall);
                    out.Write(buffer.bufferPtr(), curScratch - buffer.bufferPtr());
                }
                else
                {
                    // It's a BCall_vlen
                    buffer.resizeToFit(sizeof(outBCall) + outBCall.toNext);
                    char* curScratch = buffer.bufferPtr();
                    memcpy(curScratch, &outBCall, sizeof(outBCall));
                    curScratch += sizeof(outBCall);
                    memcpy(curScratch, src, outBCall.toNext - sizeof(outBCall));
                    curScratch += outBCall.toNext - sizeof(outBCall);
                    out.Write(buffer.bufferPtr(), curScratch - buffer.bufferPtr());
                }
            }
        }
        // Call function
        if (fptr)
        {
            //(*(RetraceFunc)fptr)(src); // increments src to point to end of parameter block
            if(strstr(funcName, "SwapBuffers")||strstr(funcName, "eglSwapBuffersWithDamageKHR"))retracer.OnNewFrame();
        }
    }
    // Should never get here, as we return once there are no more calls.
    DBG_LOG("Got to the end of %s somehow. Please report this as a bug.\n", __func__);
    assert(0);
    return true;
}

