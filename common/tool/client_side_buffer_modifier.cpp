#include <map>
#include "eglstate/context.hpp"
#include "tool/trace_interface.hpp"
#include "tool/config.hpp"
using namespace pat;

namespace
{

// API calls which may reference to client-side buffers
// And the argument which may reference to client-side buffers is always the last one
const char *CALL_NAMES[] = {
    "glDrawElements",
    "glVertexAttribPointer",
    "glColorPointer",
    "glNormalPointer",
    "glVertexPointer",
    "glTexCoordPointer",
    "glSecondaryColorPointer",
    "glFogCoordPointer",
    "glIndexPointer",
};
const unsigned int CALL_NAME_COUNT = sizeof(CALL_NAMES) / sizeof(const char *);

CallInterface *DeleteBuffer(unsigned int bufferObjName, unsigned int tid)
{
    CallInterface *call = GenerateCall();
    call->set_signature("glDeleteClientSideBuffer");
    call->SetThreadID(tid);
    call->push_back_sint_arg(bufferObjName);
    return call;
}

void printHelp()
{
    std::cout <<
        "Add glDeleteClientSideBuffer when client-side buffers will\n"
        "not be used any more.\n"
        "When you trace an application heavily using CPU-side memory\n"
        "with glVertexAttribPointer calls. The content of the CPU-side\n"
        "memory will be copied and saved into the trace file multiple times.\n"
        "This will enlarge the size of trace file and hurt the replaying\n"
        "performance. So client-side buffers are added into trace file to\n"
        "manage the CPU-side memory.\n"
        "But we can't know when is the last time of the client-side buffers\n"
        "be used and keeping all client-side buffers in memory will enlarge\n"
        "the runtime memory footprint of replaying. So this tool is implemented\n"
        "to free the memory of the client-side buffers when they are not used\n"
        "any more.\n"
        "\n"
        "Usage : client_side_buffer_modifier source_trace target_trace\n\n"
        " -h    print help\n"
        " -v    print version\n"
        ;
}

void printVersion()
{
    std::cout << PATRACE_VERSION << std::endl;
}

}  // unnamed namespace

extern "C"
int main(int argc, char **argv)
{
    int count = 0;
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
            return 1;
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
    const char * source_trace_filename = argv[argIndex++];
    const char * target_trace_filename = argv[argIndex++];

    std::shared_ptr<InputFileInterface> inputFile(GenerateInputFile());
    if (!inputFile->open(source_trace_filename))
    {
        PAT_DEBUG_LOG("Error : failed to open %s\n", source_trace_filename);
        return 1;
    }

    // The call number of the last call which reference to a client-side buffer object
    std::map<unsigned int, unsigned int> lastReferenceCallNum;

    // Client-side buffer objects can only be deleted after draw calls.
    // The array records all the client-side buffer objects to be deleted between draw calls
    std::vector<unsigned int> buffersToBeDeleted;

    CallInterface *call = NULL;
    while ((call = inputFile->next_call()))
    {
        const char *callName = call->GetName();
        const unsigned int callNo = call->GetNumber();

        for (unsigned int i = 0; i < CALL_NAME_COUNT; ++i)
        {
            if (strcmp(callName, CALL_NAMES[i]) == 0)
            {
                const UInt32 lastArgIndex = call->arg_num() - 1;
                if (call->arg_is_client_side_buffer(lastArgIndex))
                {
                    const UInt32 bufferName = call->arg_to_client_side_buffer(lastArgIndex);
                    lastReferenceCallNum[bufferName] = callNo;
                }
                break;
            }
        }

        delete call;
    }

    inputFile->reset();

    std::shared_ptr<OutputFileInterface> outputFile(GenerateOutputFile());
    if (!outputFile->open(target_trace_filename))
    {
        PAT_DEBUG_LOG("Error : failed to open %s\n", target_trace_filename);
        return 1;
    }

    // copy the json string of the header from the input file to the output file
    const std::string json_header = inputFile->json_header();
    outputFile->write_json_header(json_header);

    while ((call = inputFile->next_call()))
    {
        const char *callName = call->GetName();
        const unsigned int callNo = call->GetNumber();

        for (unsigned int i = 0; i < CALL_NAME_COUNT; ++i)
        {
            if (strcmp(callName, CALL_NAMES[i]) == 0)
            {
                const UInt32 lastArgIndex = call->arg_num() - 1;
                if (call->arg_is_client_side_buffer(lastArgIndex))
                {
                    const UInt32 bufferName = call->arg_to_client_side_buffer(lastArgIndex);
                    if (callNo == lastReferenceCallNum[bufferName])
                    {
                        buffersToBeDeleted.push_back(bufferName);
                    }
                }
                break;
            }
        }

        outputFile->write(call);

        if (strcmp(callName, "glDrawElements") == 0 || strcmp(callName, "glDrawArrays") == 0
            || strcmp(callName, "glDrawRangeElements") == 0)
        {
            std::vector<unsigned int>::const_iterator citer = buffersToBeDeleted.begin();
            for (; citer != buffersToBeDeleted.end(); ++citer)
            {
                CallInterface * newCall = DeleteBuffer(*citer, call->GetThreadID());
                outputFile->write(newCall);
                delete newCall;
                count++;
            }

            buffersToBeDeleted.clear();
        }

        delete call;
    }

    inputFile->close();
    outputFile->close();
    PAT_DEBUG_LOG("Injected %d delete calls\n", count);

    return 0;
}
