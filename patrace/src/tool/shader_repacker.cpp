#include <utility>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>
#include <limits.h>

#include "tool/parse_interface.h"

#include "common/in_file.hpp"
#include "common/file_format.hpp"
#include "common/api_info.hpp"
#include "common/parse_api.hpp"
#include "common/trace_model.hpp"
#include "common/gl_utility.hpp"
#include "common/os.hpp"
#include "eglstate/context.hpp"
#include "tool/config.hpp"
#include "base/base.hpp"
#include "tool/utils.hpp"

#define DEBUG_LOG(...) if (debug) DBG_LOG(__VA_ARGS__)

static bool split = false;
static bool repack = false;

static void printHelp()
{
    std::cout <<
        "Usage : shader_dumper [OPTIONS] keyword trace_file.pat [new_file.pat]\n"
        "Options:\n"
        "  --split       Dump out shaders\n"
        "  --repack      Pack shaders back in again\n"
        "  -h            Print help\n"
        "  -v            Print version\n"
        ;
}

static void printVersion()
{
    std::cout << PATRACE_VERSION << std::endl;
}

static void writeout(common::OutFile &outputFile, common::CallTM *call)
{
    const unsigned int WRITE_BUF_LEN = 150*1024*1024;
    static char buffer[WRITE_BUF_LEN];
    char *dest = buffer;
    dest = call->Serialize(dest);
    outputFile.Write(buffer, dest-buffer);
}

static std::string shader_filename(const StateTracker::Shader &shader, int context_index, int program_index)
{
    return "shader_" + std::to_string(shader.id) + "_p" + std::to_string(program_index) + "_c" + std::to_string(context_index) + shader_extension(shader.shader_type);
}

void pack_shaders(ParseInterface& input, common::OutFile& outputFile, const std::string& keyword, GLenum match_shader_type)
{
    common::CallTM *call = nullptr;

    // Go through entire trace file
    while ((call = input.next_call()))
    {
        if (call->mCallName == "glShaderSource" && repack)
        {
            const StateTracker::Context& ctx = input.contexts.at(input.context_index);
            const GLuint shader_id = call->mArgs[0]->GetAsUInt();
            int shader_index = ctx.shaders.remap(shader_id);
            const StateTracker::Shader& shader = ctx.shaders.at(shader_index);
            std::string filename = keyword + shader_filename(shader, input.context_index, 0);
            FILE *fp = fopen(filename.c_str(), "r");
            if (!fp)
            {
                fprintf(stderr, "%s not found!\n", filename.c_str());
            }
            fseek(fp, 0, SEEK_END);
            std::string data;
            data.resize(ftell(fp));
            fseek(fp, 0, SEEK_SET);
            size_t r = fread(&data[0], data.size(), 1, fp);
            if (r != 1)
            {
                fprintf(stderr, "Failed to read \"%s\"!\n", filename.c_str());
            }
            fclose(fp);
            call->mArgs[1]->SetAsUInt(1);
            call->mArgs[2]->mArrayLen = 1;
            call->mArgs[2]->mArray[0].SetAsString(data);
            call->mArgs[3]->mArray->mArrayLen = 0;
        }

        if (repack)
        {
            writeout(outputFile, call);
        }
    }
    if (!split)
    {
        return;
    }

    // Check what we found
    for (const auto& context : input.contexts)
    {
        for (const auto& shader : context.shaders.all())
        {
            std::string filename = keyword + shader_filename(shader, context.index, 0);
            FILE *fp = fopen(filename.c_str(), "w");
            fwrite(shader.source_code.c_str(), shader.source_code.size(), 1, fp);
            fclose(fp);
        }
    }
}

int main(int argc, char **argv)
{
    GLenum shader_type = GL_NONE;
    int argIndex = 1;
    for (; argIndex < argc; ++argIndex)
    {
        std::string arg = argv[argIndex];

        if (arg[0] != '-')
        {
            break;
        }
        else if (arg == "-h")
        {
            printHelp();
            return 1;
        }
        else if (arg == "--split")
        {
            split = true;
        }
        else if (arg == "--repack")
        {
            repack = true;
        }
        else if (arg == "-v")
        {
            printVersion();
            return 0;
        }
        else
        {
            std::cerr << "Error: Unknown option " << arg << std::endl;
            printHelp();
            return 1;
        }
    }

    if ((repack && argc < argIndex + 3) || (split && argc < argIndex + 2) || (repack && split) || (!repack && !split))
    {
        printHelp();
        return 1;
    }
    std::string keyword = argv[argIndex++];
    std::string source_trace_filename = argv[argIndex++];
    ParseInterface inputFile;
    if (!inputFile.open(source_trace_filename))
    {
        std::cerr << "Failed to open for reading: " << source_trace_filename << std::endl;
        return 1;
    }

    common::OutFile outputFile;
    if (repack)
    {
        std::string target_trace_filename = argv[argIndex++];
        if (!outputFile.Open(target_trace_filename.c_str()))
        {
            std::cerr << "Failed to open for writing: " << target_trace_filename << std::endl;
            return 1;
        }
        Json::Value header = inputFile.header;
        Json::Value info;
        info["keyword"] = keyword;
        addConversionEntry(header, "shader_repack", source_trace_filename, info);
        Json::FastWriter writer;
        const std::string json_header = writer.write(header);
        outputFile.mHeader.jsonLength = json_header.size();
        outputFile.WriteHeader(json_header.c_str(), json_header.size());
    }
    pack_shaders(inputFile, outputFile, keyword, shader_type);
    inputFile.close();
    if (repack)
    {
        outputFile.Close();
    }
    return 0;
}
