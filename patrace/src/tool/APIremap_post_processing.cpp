#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "common/out_file.hpp"
#include "common/parse_api.hpp"
#include "common/os.hpp"
#include "tool/config.hpp"
#include "base/base.hpp"

static void printHelp()
{
    std::cout <<
        "Usage : APIremap_post_processing source_trace target_trace\n"
        "glMapBufferOES() -> glMapBufferRange()"
        "Options:\n"
        "  -h            print help\n"
        "  -v            print version\n"
        ;
}

static void printVersion()
{
    std::cout << PATRACE_VERSION << std::endl;
}

static common::FrameTM* _curFrame = NULL;
static unsigned _curFrameIndex = 0;
static unsigned _curCallIndexInFrame = 0;
std::map<unsigned int, unsigned int> curBufferIdx;
std::map<unsigned int, unsigned int> bufferLength;

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

int main(int argc, char **argv)
{
    int argIndex = 1;
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
            return 0;
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

    Json::FastWriter writer;
    const std::string json_header = writer.write(header);
    outputFile.mHeader.jsonLength = json_header.size();
    outputFile.WriteHeader(json_header.c_str(), json_header.size());
    common::CallTM *call = NULL;

    while ((call = next_call(inputFile)))
    {

        bool skip = false;

        if(call->mCallName == "glBindBuffer")
        {
            unsigned int bufferType = call->mArgs[0]->GetAsUInt();
            unsigned int bufferIdx = call->mArgs[1]->GetAsUInt();

            std::map<unsigned int, unsigned int>::iterator it0 = curBufferIdx.find(bufferType);
            if(it0== curBufferIdx.end())
            {
                curBufferIdx.insert(std::pair<unsigned int, unsigned int>(bufferType, bufferIdx));
            }
            else
            {
                it0->second = bufferIdx;
            }
        }

        if(call->mCallName == "glBufferData")
        {
            unsigned int type = call->mArgs[0]->GetAsUInt();
            std::map<unsigned int, unsigned int>::iterator it1 = curBufferIdx.find(type);
            if(it1== curBufferIdx.end())
            {
                printf("No glBindBuffer before glBufferData\n");
                exit(1);
            }

            std::map<unsigned int, unsigned int>::iterator it2 = bufferLength.find(it1->second);
            unsigned int length = call->mArgs[1]->GetAsUInt();
            if(it2==bufferLength.end())
            {
                bufferLength.insert(std::pair<unsigned int, unsigned int>(it1->second, length));
            }
            else
            {
                it2->second = length;
            }
        }

        if(call->mCallName == "glMapBufferOES")
        {
            unsigned int type = call->mArgs[0]->GetAsUInt();
            std::map<unsigned int, unsigned int>::iterator it3 = curBufferIdx.find(type);
            if(it3== curBufferIdx.end())
            {
                printf("No glBindBuffer before glMapBufferOES\n");
                exit(1);
            }

            std::map<unsigned int, unsigned int>::iterator it4 = bufferLength.find(it3->second);
            if(it4==bufferLength.end())
            {
                printf("No glBufferData before glMapBufferOES\n");
                exit(1);
            }

            common::CallTM glMapBufferRange("glMapBufferRange");
            glMapBufferRange.mArgs.push_back(new common::ValueTM(call->mArgs[0]->GetAsUInt()));//target
            glMapBufferRange.mArgs.push_back(new common::ValueTM(0));//offset
            glMapBufferRange.mArgs.push_back(new common::ValueTM(it4->second));//length
            glMapBufferRange.mArgs.push_back(new common::ValueTM(2));//access= GL_MAP_WRITE_BIT
            glMapBufferRange.mRet = common::ValueTM(call->mRet.mOpaqueIns->GetAsUInt());
            glMapBufferRange.mTid = call->mTid;
            writeout(outputFile, &glMapBufferRange);

            skip = true;
        }

        if(call->mCallName == "glUnmapBufferOES")
        {
            common::CallTM glUnmapBuffer("glUnmapBuffer");
            glUnmapBuffer.mArgs.push_back(new common::ValueTM(call->mArgs[0]->GetAsUInt()));
            glUnmapBuffer.mRet = common::ValueTM(call->mRet.GetAsUByte());
            glUnmapBuffer.mTid = call->mTid;
            writeout(outputFile, &glUnmapBuffer);

            skip = true;
        }

        if(skip == false)
        {
            writeout(outputFile, call);
        }
    }


    inputFile.Close();
    outputFile.Close();

    return 0;
}

