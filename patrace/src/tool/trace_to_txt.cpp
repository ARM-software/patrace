#include <errno.h>
#include <stdio.h>

#include <common/api_info.hpp>
#include <common/parse_api.hpp>
#include <common/trace_model.hpp>
#include <tool/config.hpp>

const unsigned int MAX_CYCLE = 1000000;

bool fileExists(const char* filename) {
    std::ifstream file(filename);
    return file.good();
}

void usage(const char *argv0)
{
    DBG_LOG(
        "Usage: %s [OPTION] <path_to_trace_file> [outfile]\n"
        "Version: " PATRACE_VERSION "\n"
        "dump trace as text\n"
        "\n"
        "  -help Display this message\n"
        "  -d Print draw call number\n"
        "  -r Print frame number\n"
        "  -f <f> <l> Define frame interval, inclusive\n"
        "  -tid <thread_id> The function calls invoked by thread <thread_id> will be printed\n"
        "\n"
        , argv0);
}

int readValidValue(const char* v)
{
    char* endptr;
    errno = 0;
    int val = strtol(v, &endptr, 10);
    if(errno) {
        perror("strtol");
        exit(1);
    }
    if(endptr == v || *endptr != '\0') {
        fprintf(stderr, "Invalid parameter value: %s\n", v);
        exit(1);
    }

    return val;
}

bool isDrawCall(const std::string &funcName)
{
    static const std::string strDraw = "glDraw";
    static const size_t strDrawLen = strDraw.length();

    if (funcName == "glDrawBuffers")    // glDrawBuffers is actually not a draw call
        return false;
    if (strDraw.compare(0, strDrawLen, funcName, 0, strDrawLen) == 0)   // funcName starts with "glDraw"
        return true;
    else
        return false;
}

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        usage(argv[0]);
        return -1;
    }
    const char* filename = NULL;
    const char* out = NULL;
    int startDumpFrame = -1;
    int lastDumpFrame = -1;
    bool printDrawCallNum = false;
    bool printFrameNum = false;
    int tid = -1;
    for (int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];
        if (arg[0] != '-' && !filename)
        {
            filename = arg;
        }
        else if (arg[0] != '-' && !out)
        {
            out = arg;
        }
        else if (!strcmp(arg, "-help"))
        {
            usage(argv[0]);
            return 0;
        }
        else if (!strcmp(arg, "-f"))
        {
            startDumpFrame = readValidValue(argv[++i]);
            lastDumpFrame = readValidValue(argv[++i]);
            if (lastDumpFrame < startDumpFrame)
            {
                DBG_LOG("Error: LastFrameNum is less than StartFrameNum.\n");
                return -1;
            }
        }
        else if (!strcmp(arg, "-d"))
        {
            printDrawCallNum = true;
        }
        else if (!strcmp(arg, "-r"))
        {
            printFrameNum = true;
        }
        else if (!strcmp(arg, "-tid"))
        {
            tid = readValidValue(argv[++i]);
            if (tid < 0)
            {
                DBG_LOG("Error: thread id must be greater than or equal to zero.\n");
                return -1;
            }
        }
        else
        {
            DBG_LOG("Error: Unknown option %s\n", arg);
            return -1;
        }
    }
    if (!fileExists(filename))
    {
        DBG_LOG("Error: Did not find file: %s\n", filename);
        return -1;
    }
    FILE *fp = stdout;
    if (out)
    {
        fp = fopen(out, "w");
        if (!fp)
        {
            DBG_LOG("Error: %s could not be created!\n", out);
            return -1;
        }
    }
    common::gApiInfo.RegisterEntries(common::parse_callbacks);
    common::TraceFileTM inputFile;
    inputFile.Open(filename, false);
    int beginFrame = 0;
    int endFrame = inputFile.mFrames.size();
    int drawCallNum = 0;


    for (int fr = beginFrame; fr != endFrame; ++fr)
    {
        if (startDumpFrame >= 0 && lastDumpFrame >= 0 &&
            (startDumpFrame > fr || fr > lastDumpFrame))
        {
            // skip to dump this frame.
            continue;
        }
        common::FrameTM& curFrame = *(inputFile.mFrames[fr]);

        unsigned int frameCallNum = curFrame.GetCallCount();
        int cycleNum = 0;
        for (unsigned int ca = 0; ca < frameCallNum; ++ca)
        {
            if(ca % MAX_CYCLE == 0)
            {
                if(ca != 0)
                {
                    curFrame.UnloadCalls();
                }

                if(frameCallNum - ca > MAX_CYCLE)
                {
                    curFrame.LoadCallsForTraceToTxt(inputFile.mpInFileRA, MAX_CYCLE, cycleNum, MAX_CYCLE);
                    cycleNum++;
                }
                else
                {
                    curFrame.LoadCallsForTraceToTxt(inputFile.mpInFileRA, frameCallNum - ca, cycleNum, MAX_CYCLE);
                }
            }

            common::CallTM& curCall = *curFrame.mCalls[ca % MAX_CYCLE];
            if (tid >= 0 && (int)curCall.mTid != tid)
                continue;
            fprintf(fp, "[%d]", curCall.mTid);
            if (printFrameNum)
            {
                fprintf(fp, " [f:%d]", fr);
            }
            if (printDrawCallNum && isDrawCall(curCall.Name()))
            {
                fprintf(fp, " [d:%d]", drawCallNum++);
            }
            fprintf(fp, " %d : %s\n", curCall.mCallNo, curCall.ToStr(false).c_str());
        }
        curFrame.UnloadCalls();
    }
    fclose(fp);
}
