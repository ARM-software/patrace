#include <vector>
#include <list>
#include <map>
#include <set>
#include <utility>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl31.h>

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

/// Large negative index number to encourage crashing if used improperly.
const int UNBOUND = -1000;
const unsigned int THREAD_REMAP_BASE = 10000000;
const unsigned int MAX_THREAD_IDX = 428;

static bool dump = false;
#define dumpstream if (!dump) {} else std::cout

static void printHelp()
{
    std::cout <<
        "Usage : single_surface [OPTIONS] source_trace target_trace\n"
        "Options:\n"
        "  -s <index>    which surface to retrace (by default tries to pick automatically)\n"
        "  -n            do not reindex clientside buffers\n"
        "  -l            list surfaces present in the trace file and exit\n"
        "  -h            print help\n"
        "  -v            print version\n"
        "  -d            dump lots of debug information\n"
        "  -D            dump call log\n"
        ;
}

static void printVersion()
{
    std::cout << PATRACE_VERSION << std::endl;
}

void clientSideBufferRemap(unsigned int idx, common::CallTM * call);
unsigned int remap_handle(unsigned int idx, unsigned int name);

struct thread_state
{
    int display;
    int draw;
    int read;
    int context;
    int context_index; // as below
    int surface_index; // generated unique ID for a surface, since real ID can be recycled
    int fb; // framebuffer bound

    thread_state() : display(-1), draw(-1), read(-1), context(-1), surface_index(-1), fb(0) {}
};

typedef std::list<common::CallTM *> call_list;

static common::FrameTM* _curFrame = NULL;
static unsigned _curFrameIndex = 0;
static unsigned _curCallIndexInFrame = 0;
static bool debug = false;
static bool no_reindex = false;

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

static void list_surfaces(common::TraceFileTM &inputFile)
{
    int frames = 0;
    int calls = 0;
    std::map<GLenum, int> shaders; // type and count
    struct thread {
        int id = 0;
        int index = 0;
        int glcount = 0;
        int eglcount = 0;
        std::set<int> contexts_used;
        std::set<int> surfaces_used;
    };
    std::map<int, thread> thread_data; // indexed by tid
    struct cont {
        int id;
        int index;
        int display;
        int share_context;
        int shaders;
        int textures;
        std::map<GLenum, bool> enabled;
        cont(int _id, int _display, int _share, int _index) : id(_id), index(_index), display(_display), share_context(_share), shaders(0), textures(0) { enabled[GL_DITHER] = true; }
    };
    std::vector<cont> contexts;
    struct surf {
        int id;
        int index;
        int display;
        unsigned call_created;
        unsigned call_destroyed;
        unsigned frame_created;
        unsigned frame_destroyed;
        int glcount;
        int eglcount;
        int activations;
        int swaps;
        surf(int _id, int _display, unsigned _call_created, unsigned _frame_created, int _index)
             : id(_id), index(_index), display(_display), call_created(_call_created), call_destroyed(0),
               frame_created(_frame_created), frame_destroyed(0), glcount(0), eglcount(0), activations(0), swaps(0) {}
        std::map<int, bool> threads_used;
        std::map<int, bool> contexts_used; // by idx
    };
    std::vector<surf> surfaces;
    std::map<int, int> threads; // map threads to surfaces
    int threadcount = 0;
    std::vector<int> context_tracking;
    std::map<int, int> context_remapping; // from id to index in the original file
    std::map<int, int> current_context; // for each thread
    std::map<int, int> surface_remapping; // from id to index
    common::CallTM *call = NULL;
    const std::string uncomprtexfunc = "glTexImage";
    const std::string uncomprtexfunc2 = "glTexStorage";
    const std::string comprtexfunc = "glCompressedTexImage";
    while ((call = next_call(inputFile)))
    {
        const int surface = threads[call->mTid];
        const int context_index = current_context[call->mTid];

        if (thread_data.count(call->mTid) == 0)
        {
            thread& tdata = thread_data[call->mTid];
            tdata.id = call->mTid;
            tdata.index = threadcount;
            threadcount++;
        }
        thread& tdata = thread_data[call->mTid];

        if (call->mCallName.compare(0, 2, "gl") == 0)
        {
            tdata.glcount++;
        }
        else if (call->mCallName.compare(0, 3, "egl") == 0)
        {
            tdata.eglcount++;
        }
        tdata.contexts_used.insert(context_index);
        if (surface != 0)
        {
            tdata.surfaces_used.insert(surface_remapping.at(surface));
        }

        calls++;
        if (call->mCallName == "eglCreateWindowSurface" || call->mCallName == "eglCreateWindowSurface2")
        {
            int mret = call->mRet.GetAsInt();
            int display = call->mArgs[0]->GetAsInt();
            surface_remapping[mret] = surfaces.size(); // generate id<->idx table
            surfaces.push_back(surf(mret, display, call->mCallNo, frames, surfaces.size()));
        }
        else if (call->mCallName.compare(0, uncomprtexfunc.size(), uncomprtexfunc) == 0
                 || call->mCallName.compare(0, uncomprtexfunc2.size(), uncomprtexfunc2) == 0
                 || call->mCallName.compare(0, comprtexfunc.size(), comprtexfunc) == 0)
        {
            contexts[context_index].textures++;
        }
        else if (call->mCallName == "glCreateShader" || call->mCallName == "glCreateShaderProgram")
        {
            contexts[context_index].shaders++;
        }
        else if (call->mCallName == "eglCreateContext")
        {
            int mret = call->mRet.GetAsInt();
            int display = call->mArgs[0]->GetAsInt();
            int share = call->mArgs[2]->GetAsInt();
            context_remapping[mret] = context_tracking.size(); // generate id<->idx table
            context_tracking.push_back(mret); // generate index list
            contexts.push_back(cont(mret, display, share, contexts.size()));
        }
        else if (call->mCallName == "eglDestroySurface")
        {
            int surface = call->mArgs[1]->GetAsInt();
            for (auto &s : surfaces)
            {
                if (s.id == surface && s.call_destroyed == 0) // handle ID recycling
                {
                    s.call_destroyed = call->mCallNo;
                    s.frame_destroyed = frames;
                }
            }
        }
        else if (call->mCallName == "eglMakeCurrent")
        {
            int surface = call->mArgs[1]->GetAsInt();
            int context = call->mArgs[3]->GetAsInt();
            int context_index = context_remapping[context];
            for (auto &s : surfaces)
            {
                if (s.id == surface && s.call_destroyed == 0) // skip destroyed surfaces with recycled IDs
                {
                    s.activations++;
                    s.eglcount++;
                    s.threads_used[call->mTid] = true;
                    s.contexts_used[context_index] = true;
                    threads[call->mTid] = s.id;
                }
            }
            current_context[call->mTid] = context_index;
        }
        else if (call->mCallName.compare(0, 14, "eglSwapBuffers") == 0)
        {
            int surface = call->mArgs[1]->GetAsInt();
            for (auto &s : surfaces)
            {
                if (s.id == surface && s.call_destroyed == 0) // skip destroyed surfaces with recycled IDs
                {
                    s.swaps++;
                }
            }
            frames++;
        }
        for (auto &s : surfaces)
        {
            if (s.id == surface && s.call_destroyed == 0) // skip destroyed surfaces with recycled IDs
            {
                s.threads_used[call->mTid] = true;
                if (call->mCallName.compare(0, 2, "gl") == 0)
                {
                    s.glcount++;
                }
                else if (call->mCallName.compare(0, 3, "egl") == 0)
                {
                    s.eglcount++;
                }
                else
                {
                    DBG_LOG("What kind of call is %s?!\n", call->mCallName.c_str());
                }
            }
        }
    }
    std::cout << "Contexts:" << std::endl;
    for (auto &c : contexts)
    {
        std::cout << "    IDX=" << c.index << " Name=" << c.id << " Display=" << c.display << " Share=" << c.share_context << " Shaders=" << c.shaders 
                  << " Textures=" << c.textures << std::endl;
    }
    std::cout << "Surfaces:" << std::endl;
    for (auto &s : surfaces)
    {
        std::cout << "    IDX=" << s.index << " Name=" << s.id << " Display=" << s.display << " Lifetime=[" << s.call_created << " - " << s.call_destroyed
                  << ", " << s.frame_created << " - " << s.frame_destroyed << "] Activations=" << s.activations << " GLCalls=" << s.glcount << " EGLCalls="
                  << s.eglcount << " Swaps=" << s.swaps << " Threads=[";
        for (auto &c : s.threads_used)
        {
            std::cout << c.first << " ";
        }
        std::cout << "] Contexts=[";
        for (auto &c : s.contexts_used)
        {
            std::cout << c.first << " ";
        }
        std::cout << "]" << std::endl;
    }
    std::cout << "Threads: " << std::endl;
    for (auto &t : thread_data)
    {
        std::cout << "    IDX=" << t.second.index << " Name=" << t.second.id << " GLCalls=" << t.second.glcount << " EGLCalls=" << t.second.eglcount << " Contexts=[";
        for (auto &c : t.second.contexts_used)
        {
            std::cout << c << " ";
        }
        std::cout << "] Surfaces=[";
        for (auto &s : t.second.surfaces_used)
        {
            std::cout << s << " ";
        }
        std::cout << "]" << std::endl;
    }
    std::cout << "Frames: " << frames << std::endl;
    std::cout << "Calls: " << calls << " (" << calls/frames << " calls/frame)" << std::endl;
}

int main(int argc, char **argv)
{
    int surface = -1; // default surface, means pick one by automation
    int argIndex = 1;
    bool do_list_surfaces = false;
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
        else if (!strcmp(arg, "-l"))
        {
            do_list_surfaces = true;
        }
        else if (!strcmp(arg, "-n"))
        {
            no_reindex = true;
            printf("No longer reindexing clientside buffers!\n");
        }
        else if (!strcmp(arg, "-d"))
        {
            debug = true;
        }
        else if (!strcmp(arg, "-D"))
        {
            dump = true;
        }
        else if (!strcmp(arg, "-s") && argIndex + 1 < argc)
        {
            surface = atoi(argv[argIndex + 1]);
            argIndex++;
        }
        else
        {
            printf("Error: Unknow option %s\n", arg);
            printHelp();
            return 1;
        }
    }

    if (argIndex + 1 > argc)
    {
        printHelp();
        return 1;
    }
    const char* source_trace_filename = argv[argIndex++];
    common::TraceFileTM inputFile;
    common::gApiInfo.RegisterEntries(common::parse_callbacks);
    if (!inputFile.Open(source_trace_filename))
    {
        DBG_LOG("Failed to open for reading: %s\n", source_trace_filename);
        return 1;
    }
    _curFrame = inputFile.mFrames[0];
    _curFrame->LoadCalls(inputFile.mpInFileRA);
    if (do_list_surfaces)
    {
        list_surfaces(inputFile);
        inputFile.Close();
        return 0;
    }
    if (argIndex + 1 > argc)
    {
        printHelp();
        return 1;
    }
    const char* target_trace_filename = argv[argIndex++];

    common::OutFile outputFile;
    if (!outputFile.Open(target_trace_filename))
    {
        DBG_LOG("Failed to open for writing: %s\n", target_trace_filename);
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
    for (unsigned i = 0; i < numThreads; i++)
    {
        unsigned tid = threadArray[i]["id"].asUInt();
        DBG_LOG("remapping tid %u => idx %u\n", tid, i);
        tid_remapping[tid] = i;
        contexts[tid] = thread_state();
    }
    Json::Value newThreadArray = Json::Value(Json::arrayValue); // make empty array
    for (auto &a : threadArray) // only use defaultTid's thread info
    {
        if (a["id"] == header["defaultTid"])
        {
            DBG_LOG("  found thread %d - appending\n", a["id"].asInt());
            a["id"] = 0;
            newThreadArray.append(a);
        }
    }
    const unsigned defaultTid = header["defaultTid"].asUInt();
    header["defaultTid"] = 0;
    header["threads"] = newThreadArray;
    Json::FastWriter writer;
    int surface_id = -1; // GL id of chosen surface
    std::map<int, int> surface_remapping; // from id to index in the original file
    std::vector<int> surface_tracking; // from index to id in the original file
    std::map<int, int> context_remapping; // from id to index in the original file
    std::vector<int> context_tracking; // from index to id
    typedef std::pair<int, int> surface_context_pair;
    std::set<surface_context_pair> used_contexts; // list of indexes used on the desired surface
    common::CallTM *call = NULL;
    // Find the surface creation and initialize calls
    int newCallNo = 0;
    while ((call = next_call(inputFile)))
    {
        if (call->mCallName == "eglInitialize") // need this first
        {
            call->mTid = 0;
            writeout(outputFile, call);
            newCallNo++;
        }
        else if (call->mCallName == "eglMakeCurrent") // find contexts that are used
        {
            int draw = call->mArgs[1]->GetAsInt();
            int read = call->mArgs[2]->GetAsInt();
            int context = call->mArgs[3]->GetAsInt();
            int write_index = surface_remapping[draw];
            int read_index = surface_remapping[read];
            int context_index = context_remapping[context];
            used_contexts.insert(surface_context_pair(write_index, context_index));
            used_contexts.insert(surface_context_pair(read_index, context_index));
        }
        else if (call->mCallName == "eglCreateContext")
        {
            int mret = call->mRet.GetAsInt();
            context_remapping[mret] = context_tracking.size(); // generate id<->idx table
            context_tracking.push_back(mret); // generate index list
        }
        else if (call->mCallName == "eglCreateWindowSurface" || call->mCallName == "eglCreateWindowSurface2")
        {
            int mret = call->mRet.GetAsInt();
            // if no surface chosen by user, assume that the first surface on default thread is what we want
            if ((unsigned)surface == surface_tracking.size() || (surface == -1 && call->mTid == defaultTid))
            {
                surface = surface_tracking.size();
                surface_id = mret;
                DBG_LOG("=== Picking surface idx %d, id %d ===\n", surface, surface_id);
                call->mTid = 0;
                writeout(outputFile, call);
                newCallNo++;
            }
            else
            {
                DBG_LOG("skipping creation of surface idx=%d\n", (int)surface_tracking.size());
            }
            surface_remapping[mret] = surface_tracking.size(); // generate id<->idx table
            surface_tracking.push_back(mret); // generate index list
        }
    }
    if (surface == -1 || surface_id == -1)
    {
        DBG_LOG("ERROR: Default surface not set!\n");
        exit(1);
    }
    // Now deal the rest
    bool done = false;
    inputFile.Close();
    // Write out header
    Json::Value info;
    info["surface"] = surface;
    addConversionEntry(header, "single_surface", source_trace_filename, info);
    const std::string json_header = writer.write(header);
    outputFile.mHeader.jsonLength = json_header.size();
    outputFile.WriteHeader(json_header.c_str(), json_header.size());
    // Reopen infile, easier than resetting... ugh.
    inputFile.Open(source_trace_filename);
    _curFrameIndex = 0;
    _curCallIndexInFrame = 0;
    _curFrame = inputFile.mFrames[0];
    _curFrame->LoadCalls(inputFile.mpInFileRA);
    surface_remapping.clear();
    surface_tracking.clear();
    context_remapping.clear();
    context_tracking.clear();
    int current_output_context = 0;
    std::vector<std::pair<int, int>> non_injected_ranges;
    int prev_injected = -1;
    int total_injected = 0;
    while (!done && (call = next_call(inputFile)))
    {
        // We need to remember the context of the latest eglMakeCurrent not in the current frame.
        // This is because we need to know the 'default' context state to set at the start of each
        // frame+thread, valid until we hit another, app-supplied eglMakeCurrent.
        std::map<unsigned, thread_state> initial_contexts = contexts;
        unsigned tid = call->mTid;
        unsigned idx = tid_remapping[tid];
        bool skip = false;
        bool any_injected = false;
        if (idx >= numThreads)
        {
            DBG_LOG("Call has higher thread idx %u than max tid %u\n", idx, numThreads);
            return 1;
        }

        clientSideBufferRemap(idx, call);

        if (call->mCallName == "eglMakeCurrent")
        {
            auto &context = contexts[call->mTid];
            context.display = call->mArgs[0]->GetAsInt();
            context.draw = call->mArgs[1]->GetAsInt();
            context.read = call->mArgs[2]->GetAsInt();
            context.context = call->mArgs[3]->GetAsInt();
            context.surface_index = surface_remapping[context.draw];
            context.context_index = context_remapping[context.context];
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
            if (context.read != context.draw)
            {
                DBG_LOG("ERROR: Reading and drawing to different surfaces not supported!\n");
                exit(1);
            }
            display = context.display;
            if (context.context != 0)
            {
                call->mArgs[1] = new common::ValueTM(surface_id); // draw surface
                call->mArgs[2] = new common::ValueTM(surface_id); // read surface
            }
        }
        else if (call->mCallName[0] != 'e') // special case EGL calls below
        {
            auto &context = contexts[call->mTid];
            if (context.display == -1)
            {
                DBG_LOG("ERROR: Bad display -- eglMakeCurrent() not called before GL calls!\n");
                exit(1);
            }
            if (used_contexts.count(surface_context_pair(context.surface_index, context.context_index)) == 0)
            {
                DBG_LOG("skipped call %d due to unused context (surface=%d context=%d)\n", (int)call->mCallNo, context.surface_index, context.context_index);
                skip = true; // skip all calls in contexts never used by relevant surface
            }
        }

        if (call->mCallName == "eglTerminate") // make sure it gets called last
        {
            skip = true;
        }
        else if (call->mCallName == "eglCreateContext")
        {
            int mret = call->mRet.GetAsInt();
            context_remapping[mret] = context_tracking.size(); // generate id<->idx table
            context_tracking.push_back(mret); // generate index list
        }
        else if (call->mCallName == "eglCreatePbufferSurface")
        {
            // TBD : doing fancy surface tricks, don't omit them
        }
        else if (call->mCallName == "eglGetError" || call->mCallName == "eglGetProcAddress") // noise
        {
            skip = true;
        }
        else if (call->mCallName == "eglInitialize") // injected at the beginning, so don't repeat it
        {
            skip = true;
        }
        else if (call->mCallName == "eglCreateWindowSurface" || call->mCallName == "eglCreateWindowSurface2")
        {
            skip = true; // handled above in previous loop
            int mret = call->mRet.GetAsInt();
            surface_remapping[mret] = surface_tracking.size(); // generate id<->idx table
            surface_tracking.push_back(mret); // generate index list
        }
        else if (call->mCallName.compare(0, 14, "eglSwapBuffers") == 0 || call->mCallName == "eglSurfaceAttrib" || call->mCallName == "eglQuerySurface")
        {
            // these calls all have surface as their second argument, but since it could be recycled, we need to double check against idx
            int s = call->mArgs[1]->GetAsInt();
            if (surface_remapping[s] != surface)
            {
                skip = true;
                if (debug) DBG_LOG("  skipping swap at call %u since it is in surface %d, and we want only %d\n", call->mCallNo, surface_remapping[s], surface);
            }
        }
        else if (call->mCallName == "eglCopyBuffers" || call->mCallName == "eglBindTexImage" || call->mCallName == "eglReleaseTexImage")
        {
            // these calls all have surface as their second argument, so swap it out with warning
            DBG_LOG("call %u : %s : might not be handled correctly!\n", call->mCallNo, call->mCallName.c_str());
            call->mArgs[1] = new common::ValueTM(surface_id); // swapped surface
        }
        else if (call->mCallName == "eglDestroySurface")
        {
            int s = call->mArgs[1]->GetAsInt();
            if (surface_remapping[s] == surface)
            {
                done = true; // done extracting this surface, skip rest of file
            }
            else
            {
                skip = true; // not created now, so should not be destroyed either
            }
        }
        else if (call->mCallName == "glBindFramebuffers")
        {
            // If a framebuffer other than the default is bound, then this might affect global state that we need to preserve
            // for later rendering. Target is irrelevant here, and all we really care about is whether it is zero or not.
            int fb = call->mArgs[1]->GetAsInt();
            auto &context = contexts[call->mTid];
            context.fb = fb;
            DBG_LOG("FB for TID=%u -> %d\n", call->mTid, fb);
        }
        else if (call->mCallName != "glDrawBuffers")
        {
            // not trigger the below check for this call, since it is not a draw call, but modifies state
        }
        else if (call->mCallName.compare(0, 6, "glDraw") == 0 || call->mCallName.compare(0, 10, "glDispatch") == 0
                 || call->mCallName == "glClear")
        {
            // Draw calls. If they apply to framebuffer of ignored surface, skip them.
            if (contexts[call->mTid].surface_index != surface && contexts[call->mTid].fb == 0 && surface != -1)
            {
                if (debug) DBG_LOG("  skipping draw call %u : %s\n", call->mCallNo, call->mCallName.c_str());
                skip = true;
            }
        }

        if (!skip)
        {
            calls[idx].push_back(call);
        }
        else
        {
            auto &context = contexts[call->mTid];
            dumpstream << "SKIPPED [f" << _curFrameIndex << ":t" << call->mTid << ":c" << ((context.context_index != UNBOUND) ? std::to_string(context.context_index) : std::string("-"))
                       << ":s" << ((context.surface_index != UNBOUND) ? std::to_string(context.surface_index) : std::string("-")) << "] "
                       << call->mCallNo << ": " << call->ToStr(false) << std::endl;
        }
        if (done || _curCallIndexInFrame >= _curFrame->GetLoadedCallCount()) // flush out calls in orderly manner
        {
            unsigned active_threads = 0; // count active threads this frame
            for (auto &thread : calls)
            {
                if (thread.size() > 0)
                {
                    active_threads++;
                }
            }
            if (debug) DBG_LOG("-- writeout! %u / %u threads active --\n", active_threads, (unsigned)calls.size());
            for (auto &thread : calls)
            {
                if (thread.size() == 0)
                {
                    continue;
                }
                auto &context = initial_contexts[thread.front()->mTid];
                // If necessary, inject an eglMakeCurrent to set context and display for each
                // flattened thread section, to reproduce original behaviour.
                // However, do not put it in front of EGL calls, since they do
                // not need it (and may break, eg eglInitialize).
                common::CallTM makeCurrent("eglMakeCurrent");
                makeCurrent.mArgs.push_back(new common::ValueTM(context.display));
                makeCurrent.mArgs.push_back(new common::ValueTM(surface_id));
                makeCurrent.mArgs.push_back(new common::ValueTM(surface_id));
                makeCurrent.mArgs.push_back(new common::ValueTM(context.context));
                makeCurrent.mRet = common::ValueTM((int)EGL_TRUE);
                if (debug) DBG_LOG("  writing (%u calls)\n", (unsigned)thread.size());
                bool injected = false;
                if (active_threads <= 1 && current_output_context != 0)
                {
                    // set this to skip injecting eglMakeCurrent for the case where only one
                    // thread is active - we don't need to inject anything then
                    injected = true;
                }
                for (auto &out : thread)
                {
                    if (!injected && out->mCallName == "eglMakeCurrent") // special case
                    {
                        // Special case. The app supplied eglMakeCurrent takes priority.
                        injected = true;
                    }
                    else if (out->mCallName[0] != 'e' && !injected)
                    {
                        dumpstream << "INSERTED [f" << _curFrameIndex << "t0:c" << ((context.context_index != UNBOUND) ? std::to_string(context.context_index) : std::string("-"))
                                   << ":s" << ((context.surface_index != UNBOUND) ? std::to_string(context.surface_index) : std::string("-")) << "] "
                                   << "{}->" << newCallNo << ": " << makeCurrent.ToStr(false) << std::endl;
                        newCallNo++;
                        writeout(outputFile, &makeCurrent);
                        injected = true;
                        any_injected = true;
                        total_injected++;
                    }
                    dumpstream << "[f" << _curFrameIndex << ":t" << out->mTid << ":c" << ((context.context_index != UNBOUND) ? std::to_string(context.context_index) : std::string("-"))
                               << ":s" << ((context.surface_index != UNBOUND) ? std::to_string(context.surface_index) : std::string("-")) << "] "
                               << out->mCallNo << "->" << newCallNo << ": " << out->ToStr(false) << std::endl;
                    newCallNo++;
                    out->mTid = 0;
                    if (out->mCallName == "eglMakeCurrent")
                    {
                        current_output_context = out->mArgs[3]->GetAsInt();
                    }
                    writeout(outputFile, out);
                }
                thread.clear();
            }
        }

        // Keep track of non-injected franges. If we had a number of frames without injections, track those.
        // Ignore such ranges with less than 100 frames.
        if (any_injected && prev_injected != -1 && static_cast<int>(_curFrameIndex) - prev_injected > 100)
        {
            non_injected_ranges.push_back(std::make_pair(prev_injected, static_cast<int>(_curFrameIndex) - 1));
        }
        if (any_injected)
        {
            prev_injected = _curFrameIndex;
        }
    }

    prev_injected = std::max(0, prev_injected);
    if (static_cast<int>(_curFrameIndex) - prev_injected > 100)
    {
        non_injected_ranges.push_back(std::make_pair(prev_injected, static_cast<int>(_curFrameIndex) - 1));
    }

    DBG_LOG("Total number of injected eglMakeCurrent() calls: %d\n", total_injected);
    DBG_LOG("Wide ranges of frames without any eglMakeCurrent() injections:\n");
    for (const auto& pair : non_injected_ranges)
    {
        DBG_LOG("\t%d - %d\n", pair.first, pair.second);
    }

    common::CallTM eglTerminate("eglTerminate");
    eglTerminate.mArgs.push_back(new common::ValueTM(display));
    eglTerminate.mRet = common::ValueTM((int)EGL_TRUE);
    writeout(outputFile, &eglTerminate);

    inputFile.Close();
    outputFile.Close();

    return 0;
}

void clientSideBufferRemap(unsigned int idx, common::CallTM * call)
{
    if (no_reindex)
    {
        return;
    }
    else if (idx > MAX_THREAD_IDX)
    {
        DBG_LOG("ERROR: The thread index is bigger than max number!\n");
        exit(1);
    }
    else if (call->mCallName == "glCreateClientSideBuffer")
    {
        unsigned int ret = call->mRet.GetAsUInt();
        unsigned int newIdx = remap_handle(idx, ret);
        call->mRet.SetAsUInt(newIdx);
    }
    else if (call->mCallName == "glDeleteClientSideBuffer" || call->mCallName == "glClientSideBufferData" || call->mCallName == "glClientSideBufferSubData")
    {
        unsigned int name = call->mArgs[0]->GetAsUInt();
        unsigned int newIdx = remap_handle(idx, name);
        call->mArgs[0]->SetAsUInt(newIdx);
    }
    else if (call->mCallName == "glCopyClientSideBuffer")
    {
        unsigned int name = call->mArgs[1]->GetAsUInt();
        unsigned int newIdx = remap_handle(idx, name);
        call->mArgs[1]->SetAsUInt(newIdx);
    }
    else if (call->mCallName == "glObjectPtrLabelKHR" || call->mCallName == "glObjectPtrLabel")
    {
        if(call->mArgs[0]->IsClientSideBufferReference())
        {
            unsigned int name = 0;
            name = call->mArgs[0]->mOpaqueIns->mClientSideBufferName;
            call->mArgs[0]->mOpaqueIns->mClientSideBufferName = remap_handle(idx, name);
        }
    }
    else if (call->mCallName == "glDrawArraysIndirect" || call->mCallName == "glMapBufferOES" )
    {
        if(call->mArgs[1]->IsClientSideBufferReference())
        {
            unsigned int name = 0;
            name = call->mArgs[1]->mOpaqueIns->mClientSideBufferName;
            call->mArgs[1]->mOpaqueIns->mClientSideBufferName = remap_handle(idx, name);
        }
    }
    else if (call->mCallName == "glNormalPointer" || call->mCallName == "glPointSizePointerOES" || call->mCallName == "glDrawElementsIndirect")
    {
        if(call->mArgs[2]->IsClientSideBufferReference())
        {
            unsigned int name = 0;
            name = call->mArgs[2]->mOpaqueIns->mClientSideBufferName;
            call->mArgs[2]->mOpaqueIns->mClientSideBufferName = remap_handle(idx, name);
        }
    }
    else if (call->mCallName == "glDrawElements" || call->mCallName == "glTexCoordPointer" || call->mCallName == "glVertexPointer" || call->mCallName == "glMatrixIndexPointerOES" || call->mCallName == "glWeightPointerOES"
             || call->mCallName == "glDrawElementsBaseVertex" || call->mCallName == "glDrawElementsInstancedBaseVertex" || call->mCallName == "glDrawElementsBaseVertexOES" || call->mCallName == "glDrawElementsInstancedBaseVertexOES"
             || call->mCallName == "glDrawElementsBaseVertexEXT" || call->mCallName == "glDrawElementsInstancedBaseVertexEXT" || call->mCallName == "glDrawElementsInstancedBaseInstanceEXT"
             || call->mCallName == "glDrawElementsInstancedBaseVertexBaseInstanceEXT" || call->mCallName == "glDrawElementsInstanced" || call->mCallName == "glMapBufferRange" || call->mCallName == "glColorPointer")
    {
        if(call->mArgs[3]->IsClientSideBufferReference())
        {
            unsigned int name = 0;
            name = call->mArgs[3]->mOpaqueIns->mClientSideBufferName;
            call->mArgs[3]->mOpaqueIns->mClientSideBufferName = remap_handle(idx, name);
        }
    }
    else if (call->mCallName ==  "glVertexAttribIPointer")
    {
        if(call->mArgs[4]->IsClientSideBufferReference())
        {
            unsigned int name = 0;
            name = call->mArgs[4]->mOpaqueIns->mClientSideBufferName;
            call->mArgs[4]->mOpaqueIns->mClientSideBufferName = remap_handle(idx, name);
        }
    }
    else if (call->mCallName == "glVertexAttribPointer" || call->mCallName == "glDrawRangeElementsBaseVertex" || call->mCallName == "glDrawRangeElementsBaseVertexOES" || call->mCallName == "glDrawRangeElementsBaseVertexEXT"
             || call->mCallName == "glDrawRangeElements")
    {
        if(call->mArgs[5]->IsClientSideBufferReference())
        {
            unsigned int name = 0;
            name = call->mArgs[5]->mOpaqueIns->mClientSideBufferName;
            call->mArgs[5]->mOpaqueIns->mClientSideBufferName = remap_handle(idx, name);
        }
    }
}

unsigned int remap_handle(unsigned int idx, unsigned int name)
{
    if(name >= THREAD_REMAP_BASE)
    {
        DBG_LOG("ERROR: The ClientSideBuffer index is bigger than max number!\n");
        exit(1);
    }
    else
    {
        return (name == 0) ? 0 : (name + idx*THREAD_REMAP_BASE);
    }
}
