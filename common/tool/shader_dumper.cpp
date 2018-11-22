#include "eglstate/context.hpp"
#include "tool/trace_interface.hpp"
#include "tool/config.hpp"

using namespace pat;

extern "C"
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Please specify a trace file or -h for help. Exit.\n");
        exit(1);
    }

    if(!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
        fprintf(stderr, "Usage: %s <trace file>\n\n", argv[0]);
        fprintf(stderr, " -h output this help text\n");
        fprintf(stderr, " -v output the version\n");
        fprintf(stderr, "\nDumps the shaders in a trace file.\n");
        exit(0);
    }

    if(!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version"))
    {
        fprintf(stderr, "Version: " PATRACE_VERSION "\n");
        exit(0);
    }

    const char *trace_filename = argv[1];

    std::shared_ptr<InputFileInterface> inputFile(GenerateInputFile());
    if (!inputFile->open(trace_filename))
    {
        PAT_DEBUG_LOG("Error : failed to open %s\n", trace_filename);
        return 1;
    }

    CallInterface *call = NULL;
    while ((call = inputFile->next_call()))
    {
        const unsigned int threadID = call->GetThreadID();
        pat::ContextPtr context = pat::GetStateMangerForThread(threadID);
        context->SetCurrentCallNumber(call->GetNumber());

        if (strcmp(call->GetName(), "glShaderSource") == 0)
        {
            const unsigned int name = call->arg_to_uint(0);
            const unsigned int n = call->arg_to_uint(1);
            const unsigned int numLengths = call->array_size(3);
            std::string source;

            for (unsigned int i = 0; i < n; i++)
            {
                const char *str = call->array_arg_to_str(2, i);
                if (i < numLengths)
                {
                    const unsigned int len = call->array_arg_to_uint(3, i);
                    source += std::string(str, len);
                }
                else
                {
                    source += std::string(str);
                }
            }
            context->ShaderSource(name, source);
        }
        else if (strcmp(call->GetName(), "glCreateShader") == 0)
        {
            const unsigned int name = call->ret_to_uint();
            const unsigned int type = call->arg_to_uint(0);
            context->CreateShader(type, name);
        }

        delete call;
    }
    inputFile->close();

    const pat::ContextPtrList contexts = pat::GetAllContexts();
    pat::ContextPtrList::const_iterator citer = contexts.begin();
    for (; citer != contexts.end(); ++citer)
    {
        (*citer)->DumpShaderObjects();
    }
}
