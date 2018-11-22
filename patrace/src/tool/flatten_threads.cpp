#include <vector>
#include <list>
#include <map>
#include <unordered_set>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "common/in_file.hpp"
#include "common/file_format.hpp"
#include "common/out_file.hpp"
#include "common/api_info.hpp"
#include "common/parse_api.hpp"
#include "common/trace_model.hpp"
#include "common/os.hpp"
#include "eglstate/context.hpp"
#include "tool/config.hpp"
#include "base/base.hpp"
#include "tool/utils.hpp"
#include <common/trace_model.hpp>

const unsigned int CSB_THREAD_REMAP_BASE = 10000000;
const unsigned int MAX_THREAD_IDX = 428;

static void printHelp()
{
    std::cout <<
        "Usage : flatten_threads [OPTIONS] source_trace target_trace\n"
        "Options:\n"
        "  -h            print help\n"
        "  -v            print version\n"
        "  -d            dump lots of debug information\n"
        "  -t THREADS    flatten only given comma separated list of threads (by default flatten all threads)\n"
        "  -w            add forceSingleWindow to trace header\n"
        "  -f FIRST LAST your existing frame range, returns -1 if synchronization has to be added inside it\n"
        ;
}

static void printVersion()
{
    std::cout << PATRACE_VERSION << std::endl;
}

void clientSideBufferRemap(unsigned int idx, common::CallTM * call);
unsigned int remap_csb_handle(unsigned int idx, unsigned int name);

struct thread_state
{
    int display;
    int draw;
    int read;
    int context;

    thread_state() : display(-1), draw(-1), read(-1), context(-1) {}
};

typedef std::list<common::CallTM *> call_list;

static common::FrameTM* _curFrame = NULL;
static unsigned _curFrameIndex = 0;
static unsigned _curCallIndexInFrame = 0;
static bool debug = false;

static void writeout(common::OutFile &outputFile, common::CallTM *call)
{
    const unsigned int WRITE_BUF_LEN = 150*1024*1024;
    static char buffer[WRITE_BUF_LEN];
    char *dest = buffer;
    dest = call->Serialize(dest);
    outputFile.Write(buffer, dest-buffer);
}

static common::CallTM* next_call(common::TraceFileTM &_fileTM)
{
    if (_curCallIndexInFrame >= _curFrame->GetLoadedCallCount())
    {
        _curFrameIndex++;
        if (_curFrameIndex >= _fileTM.mFrames.size())
            return NULL;
        if (_curFrame) _curFrame->UnloadCalls();
        _curFrame = _fileTM.mFrames[_curFrameIndex];
        _curFrame->LoadCalls(_fileTM.mpInFileRA);
        _curCallIndexInFrame = 0;
    }
    common::CallTM *call = _curFrame->mCalls[_curCallIndexInFrame];
    _curCallIndexInFrame++;
    return call;
}

std::vector<int> extract_ints(std::string const& input_str)
{
    std::vector<int> ints;
    std::istringstream input(input_str);
    std::string number;
    while (std::getline(input, number, ','))
    {
        std::istringstream iss(number);
        int i;
        iss >> i;
        ints.push_back(i);
    }
    return ints;
}

class NeedsContext
{
    static std::unordered_set<std::string> eglCallsNeedContext;
public:
    static void init()      // insert all the EGL calls need context
    {
        NeedsContext::eglCallsNeedContext.insert("eglCreateImageKHR");
        NeedsContext::eglCallsNeedContext.insert("eglSwapBuffers");
        NeedsContext::eglCallsNeedContext.insert("eglSwapBuffersWithDamageKHR");
        NeedsContext::eglCallsNeedContext.insert("eglCreateSyncKHR");
        NeedsContext::eglCallsNeedContext.insert("eglClientWaitSyncKHR");
        NeedsContext::eglCallsNeedContext.insert("eglDestroySyncKHR");
        NeedsContext::eglCallsNeedContext.insert("eglSignalSyncKHR");
    }
    static bool judge(const std::string &callName)  // judge whether a call needs context or not
    {
        if (callName[0] != 'e') // OpenGL ES call, needs context
            return true;
        auto iter = NeedsContext::eglCallsNeedContext.find(callName);
        if (iter != NeedsContext::eglCallsNeedContext.end())  // This is an EGL call who needs context
            return true;
        else                    // This is an EGL call who doesn't need context
            return false;
    }
};

std::unordered_set<std::string> NeedsContext::eglCallsNeedContext;

int main(int argc, char **argv)
{
    Json::Value info;
    int argIndex = 1;
    std::map<int, bool> flatten_only;
    bool insequence = false;
    bool addforcesinglewindow = false;
    int countInjected = 0;
    int count_inside = 0; // injected inside frame range
    int first_frame = 0;
    int last_frame = -1;
    for (; argIndex < argc; ++argIndex)
    {
        const char *arg = argv[argIndex];

        if (arg[0] != '-')
            break;

        if (!strcmp(arg, "-h"))
        {
            printHelp();
            return 1;
        }
        else if (!strcmp(arg, "-v"))
        {
            printVersion();
            return 1;
        }
        else if (!strcmp(arg, "-w"))
        {
            addforcesinglewindow = true;
            info["addForceSingleWindow"] = true;
        }
        else if (!strcmp(arg, "-d"))
        {
            debug = true;
        }
        else if (!strcmp(arg, "-t"))
        {
            argIndex++;
            std::vector<int> l = extract_ints(argv[argIndex]);
            for (int i : l) flatten_only[i] = true;
        }
        else if (!strcmp(arg, "-f"))
        {
            argIndex++;
            first_frame = atoi(argv[argIndex]);
            argIndex++;
            last_frame = atoi(argv[argIndex]);
            info["framerange"] = std::to_string(first_frame) + "-" + std::to_string(last_frame);
        }
        else if (!strcmp(arg, "-insequence"))
        {
            // -insequence means keeping the order of the calls in the original pat file.
            // But the retracing efficiency of the result is very low.
            // Only for verification. Default off.
            insequence = true;
            info["insequence"] = true;
        }
        else
        {
            printf("Error: Unknow option %s\n", arg);
            printHelp();
            return 1;
        }
    }

    if (argIndex + 2 > argc)
    {
        printHelp();
        return 1;
    }

    const char* source_trace_filename = argv[argIndex++];
    const char* target_trace_filename = argv[argIndex++];

    common::TraceFileTM inputFile;
    common::gApiInfo.RegisterEntries(common::parse_callbacks);
    if (!inputFile.Open(source_trace_filename))
    {
        PAT_DEBUG_LOG("Failed to open for reading: %s\n", source_trace_filename);
        return 1;
    }
    _curFrame = inputFile.mFrames[0];
    _curFrame->LoadCalls(inputFile.mpInFileRA);

    common::OutFile outputFile;
    if (!outputFile.Open(target_trace_filename))
    {
        PAT_DEBUG_LOG("Failed to open for writing: %s\n", target_trace_filename);
        return 1;
    }

    Json::Value header = inputFile.mpInFileRA->getJSONHeader();
    Json::Value threadArray = header["threads"];

    unsigned numThreads = threadArray.size();
    if (numThreads == 0)
    {
        DBG_LOG("Bad number of threads: %d\n", numThreads);
        return 1;
    }
    std::vector<call_list> calls(numThreads);
    std::map<unsigned, thread_state> contexts;
    std::map<unsigned, unsigned> tid_remapping; // because thread IDs may be discontinuous
    int display = -1;
    NeedsContext::init();
    for (unsigned i = 0; i < numThreads; i++)
    {
        unsigned tid = threadArray[i]["id"].asUInt();
        if (debug) DBG_LOG("remapping tid %u => idx %u\n", tid, i);
        tid_remapping[tid] = i;
        contexts[tid] = thread_state();
    }
    Json::Value newThreadArray = Json::Value(Json::arrayValue); // make empty array
    for (auto &a : threadArray) // only use defaultTid's thread info
    {
        if (a["id"] == header["defaultTid"])
        {
            if (debug) DBG_LOG("  found thread %d - appending\n", a["id"].asInt());
            a["id"] = 0;
            newThreadArray.append(a);
        }
    }
    header["defaultTid"] = 0;
    if (addforcesinglewindow)
    {
        header["forceSingleWindow"] = true;
    }
    header["threads"] = newThreadArray;
    common::CallTM *call = NULL;
    std::map<unsigned, thread_state> initial_contexts = contexts;
    unsigned int previous_tid = 9999;
    bool injected = false;
    for (int callNo = 0 ;(call = next_call(inputFile)); ++callNo)
    {
        const unsigned tid = call->mTid;

        if (callNo == 0)
        {
            previous_tid = tid;
        }

        if (tid != previous_tid)
        {
            injected = false;
            previous_tid = tid;
        }

        if (tid_remapping.count(tid) == 0)
        {
            DBG_LOG("WARNING: Header JSON did not include all threads! Missed tid %u!\n", tid);
            tid_remapping[tid] = numThreads;
            initial_contexts[tid] = contexts[tid] = thread_state();
            numThreads++;
            calls.resize(numThreads);
        }

        const unsigned idx = tid_remapping.at(tid);

        assert(idx < numThreads);
        if (flatten_only.size() > 0 && flatten_only.count(tid) == 0 && call->mCallName != "eglInitialize")
        {
            continue;
        }

        clientSideBufferRemap(idx, call);

        if (call->mCallName == "eglMakeCurrent")
        {
            auto &context = contexts[call->mTid];
            context.display = call->mArgs[0]->GetAsInt();
            context.draw = call->mArgs[1]->GetAsInt();
            context.read = call->mArgs[2]->GetAsInt();
            context.context = call->mArgs[3]->GetAsInt();
            if (context.display == -1)
            {
                DBG_LOG("ERROR: Bad display stored!\n");
                exit(1);
            }
            if (context.display != display && display != -1)
            {
                DBG_LOG("ERROR: More than one display not supported yet!\n");
                exit(1);
            }
            display = context.display;
        }
        else if (call->mCallName[0] != 'e') // sanity check
        {
            const auto &context = contexts.at(call->mTid);
            if (context.display == -1)
            {
                DBG_LOG("ERROR: Bad display -- eglMakeCurrent() not called before GL calls!\n");
                exit(1);
            }
        }

        // Add some extra logging
        if (call->mCallName == "eglCreateWindowSurface" || call->mCallName == "eglCreateWindowSurface2"
                 || call->mCallName == "eglCreatePbufferSurface" || call->mCallName == "eglCreatePixmapSurface")
        {
            if (debug) DBG_LOG("[%u] Creating surface %d at %u\n", tid, call->mRet.GetAsInt(), call->mCallNo);
        }
        else if (call->mCallName == "eglCreateContext")
        {
            if (debug) DBG_LOG("[%u] Creating context %d at %u\n", tid, call->mRet.GetAsInt(), call->mCallNo);
        }
        else if (call->mCallName == "eglDestroyContext")
        {
            if (debug) DBG_LOG("[%u] Destroying context %d at %u\n", tid, call->mArgs[1]->GetAsInt(), call->mCallNo);
        }
        else if (!insequence && call->mCallName == "eglDestroySurface")
        {
            if (debug) DBG_LOG("[%u] Destroying surface %d at %u\n", tid, call->mArgs[1]->GetAsInt(), call->mCallNo);
        }

        if (call->mCallName != "eglTerminate") // make sure eglTerminate gets called last
        {
            assert(calls.at(idx).size() == 0 || calls.at(idx).front()->mTid == call->mTid);
            calls[idx].push_back(call);
        }

        if (insequence)
        {
            if (!injected && call->mCallName == "eglMakeCurrent")
            {
                // Special case. The app supplied eglMakeCurrent takes priority.
                injected = true;
            }
            else if (NeedsContext::judge(call->mCallName) && !injected) {
                const auto &context = contexts.at(call->mTid);
                // Inject an eglMakeCurrent to set context and display for each
                // flattened thread section, to reproduce original behaviour.
                // However, do not put it in front of EGL calls, since they do
                // not need it (and may break, eg eglInitialize).
                if (context.display != -1) {
                    common::CallTM makeCurrent("eglMakeCurrent");
                    makeCurrent.mArgs.push_back(new common::ValueTM(context.display));
                    makeCurrent.mArgs.push_back(new common::ValueTM(context.draw));
                    makeCurrent.mArgs.push_back(new common::ValueTM(context.read));
                    makeCurrent.mArgs.push_back(new common::ValueTM(context.context));
                    makeCurrent.mRet = common::ValueTM((int)EGL_TRUE);
                    makeCurrent.mTid = 0;
                    injected = true;
                    countInjected++;
                    if (last_frame != -1 && (int)_curFrameIndex >= first_frame && (int)_curFrameIndex <= last_frame)
                    {
                        count_inside++;
                    }
                    writeout(outputFile, &makeCurrent);
                }
            }
            call->mTid = 0;
            writeout(outputFile, call);
        }
        else {      // !insequence
            if (_curCallIndexInFrame >= _curFrame->GetLoadedCallCount()) // flush calls
            {
                if (debug) DBG_LOG("-- writeout frame %u! %u threads --\n", _curFrameIndex, (unsigned)calls.size());
                for (auto &thread : calls)
                {
                    if (thread.size() == 0)
                    {
                        continue;
                    }
                    const unsigned curTid = thread.front()->mTid;
                    const auto &context = initial_contexts.at(curTid);
                    // Inject an eglMakeCurrent to set context and display for each
                    // flattened thread section, to reproduce original behaviour.
                    // However, do not put it in front of EGL calls, since they do
                    // not need it (and may break, eg eglInitialize).
                    common::CallTM makeCurrent("eglMakeCurrent");
                    makeCurrent.mArgs.push_back(new common::ValueTM(context.display));
                    makeCurrent.mArgs.push_back(new common::ValueTM(context.draw));
                    makeCurrent.mArgs.push_back(new common::ValueTM(context.read));
                    makeCurrent.mArgs.push_back(new common::ValueTM(context.context));
                    makeCurrent.mRet = common::ValueTM((int)EGL_TRUE);
                    if (debug) DBG_LOG("  writing (%u calls)[s=%d,c=%d] tid=%u\n", (unsigned)thread.size(), context.read, context.context, curTid);
                    bool injected = false;
                    for (auto &out : thread)
                    {
                        assert(curTid == out->mTid);
                        if (!injected && out->mCallName == "eglMakeCurrent")
                        {
                            // Special case. The app supplied eglMakeCurrent takes priority.
                            injected = true;
                        }
                        else if (NeedsContext::judge(out->mCallName) && !injected)
                        {
                            if (context.display == -1)
                            {
                                DBG_LOG("  Warning: eglMakeCurrent(%d,%d,%d,%d) at %u(%u) - invalid display\n",
                                        context.display, context.draw, context.read, context.context, call->mCallNo, callNo);
                            }

                            writeout(outputFile, &makeCurrent);
                            injected = true;
                            countInjected++;
                        }
                        out->mTid = 0;
                        writeout(outputFile, out);
                    }
                    thread.clear();
                }
                // We need to remember the context of the latest eglMakeCurrent not in the current frame.
                // This is because we need to know the 'default' context state to set at the start of each
                // frame+thread, valid until we hit another, app-supplied eglMakeCurrent.
                initial_contexts = contexts;
            }
        }
    }

    common::CallTM eglTerminate("eglTerminate");
    eglTerminate.mArgs.push_back(new common::ValueTM(display));
    eglTerminate.mRet = common::ValueTM((int)EGL_TRUE);
    writeout(outputFile, &eglTerminate);

    DBG_LOG("Injected %d extra eglMakeCurrent() calls\n", countInjected);

    info["injected_total"] = countInjected;
    if (last_frame != -1)
    {
        info["injected_eglMakeCurrent_in_framerange"] = count_inside;
    }
    addConversionEntry(header, "flatten_threads", source_trace_filename, info);
    Json::FastWriter writer;
    const std::string json_header = writer.write(header);
    outputFile.mHeader.jsonLength = json_header.size();
    outputFile.WriteHeader(json_header.c_str(), json_header.size());

    inputFile.Close();
    outputFile.Close();

    DBG_LOG("Injected %d extra eglMakeCurrent() calls\n", countInjected);
    if (last_frame != -1)
    {
        DBG_LOG("Injected %d extra eglMakeCurrent() calls inside the defined frame range\n", count_inside);
        return -1;
    }

    return 0;
}

void clientSideBufferRemap(unsigned int idx, common::CallTM * call)
{
    if(idx > MAX_THREAD_IDX)
    {
        DBG_LOG("ERROR: The thread index is bigger than max number!\n");
        exit(1);
    }
    if (call->mCallName == "glCreateClientSideBuffer")
    {
        unsigned int ret = call->mRet.GetAsUInt();
        unsigned int newIdx = remap_csb_handle(idx, ret);
        call->mRet.SetAsUInt(newIdx);
    }
    if (call->mCallName == "glDeleteClientSideBuffer" || call->mCallName == "glClientSideBufferData" || call->mCallName == "glClientSideBufferSubData")
    {
        unsigned int name = call->mArgs[0]->GetAsUInt();
        unsigned int newIdx = remap_csb_handle(idx, name);
        call->mArgs[0]->SetAsUInt(newIdx);
    }

    if (call->mCallName == "glCopyClientSideBuffer")
    {
        unsigned int name = call->mArgs[1]->GetAsUInt();
        unsigned int newIdx = remap_csb_handle(idx, name);
        call->mArgs[1]->SetAsUInt(newIdx);
    }

    if (call->mCallName == "glObjectPtrLabelKHR" || call->mCallName == "glObjectPtrLabel")
    {
        if(call->mArgs[0]->IsClientSideBufferReference())
        {
            unsigned int name = 0;
            name = call->mArgs[0]->mOpaqueIns->mClientSideBufferName;
            call->mArgs[0]->mOpaqueIns->mClientSideBufferName = remap_csb_handle(idx, name);
        }
    }
    if (call->mCallName == "glDrawArraysIndirect" || call->mCallName == "glMapBufferOES" )
    {
        if(call->mArgs[1]->IsClientSideBufferReference())
        {
            unsigned int name = 0;
            name = call->mArgs[1]->mOpaqueIns->mClientSideBufferName;
            call->mArgs[1]->mOpaqueIns->mClientSideBufferName = remap_csb_handle(idx, name);
        }
    }
    if (call->mCallName == "glNormalPointer" || call->mCallName == "glPointSizePointerOES" || call->mCallName == "glDrawElementsIndirect")
    {
        if(call->mArgs[2]->IsClientSideBufferReference())
        {
            unsigned int name = 0;
            name = call->mArgs[2]->mOpaqueIns->mClientSideBufferName;
            call->mArgs[2]->mOpaqueIns->mClientSideBufferName = remap_csb_handle(idx, name);
        }
    }
    if (call->mCallName == "glDrawElements" || call->mCallName == "glTexCoordPointer" || call->mCallName == "glVertexPointer" || call->mCallName == "glMatrixIndexPointerOES" || call->mCallName == "glWeightPointerOES"
    || call->mCallName == "glDrawElementsBaseVertex" || call->mCallName == "glDrawElementsInstancedBaseVertex" || call->mCallName == "glDrawElementsBaseVertexOES" || call->mCallName == "glDrawElementsInstancedBaseVertexOES"
    || call->mCallName == "glDrawElementsBaseVertexEXT" || call->mCallName == "glDrawElementsInstancedBaseVertexEXT" || call->mCallName == "glDrawElementsInstancedBaseInstanceEXT"
    || call->mCallName == "glDrawElementsInstancedBaseVertexBaseInstanceEXT" || call->mCallName == "glDrawElementsInstanced" || call->mCallName == "glMapBufferRange" || call->mCallName == "glColorPointer")
    {
        if(call->mArgs[3]->IsClientSideBufferReference())
        {
            unsigned int name = 0;
            name = call->mArgs[3]->mOpaqueIns->mClientSideBufferName;
            call->mArgs[3]->mOpaqueIns->mClientSideBufferName = remap_csb_handle(idx, name);
        }
    }
    if (call->mCallName ==  "glVertexAttribIPointer")
    {
        if(call->mArgs[4]->IsClientSideBufferReference())
        {
            unsigned int name = 0;
            name = call->mArgs[4]->mOpaqueIns->mClientSideBufferName;
            call->mArgs[4]->mOpaqueIns->mClientSideBufferName = remap_csb_handle(idx, name);
        }
    }
    if (call->mCallName == "glVertexAttribPointer" || call->mCallName == "glDrawRangeElementsBaseVertex" || call->mCallName == "glDrawRangeElementsBaseVertexOES" || call->mCallName == "glDrawRangeElementsBaseVertexEXT"
    || call->mCallName == "glDrawRangeElements")
    {
        if(call->mArgs[5]->IsClientSideBufferReference())
        {
            unsigned int name = 0;
            name = call->mArgs[5]->mOpaqueIns->mClientSideBufferName;
            call->mArgs[5]->mOpaqueIns->mClientSideBufferName = remap_csb_handle(idx, name);
        }
    }
}

unsigned int remap_csb_handle(unsigned int idx, unsigned int name)
{
    if(name >= CSB_THREAD_REMAP_BASE)
    {
        DBG_LOG("ERROR: The ClientSideBuffer index is bigger than max number!\n");
        exit(1);
    }
    else
    {
        return name + idx*CSB_THREAD_REMAP_BASE;
    }
}
