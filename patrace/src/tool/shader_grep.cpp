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

static bool debug = false;

#define DEBUG_LOG(...) if (debug) DBG_LOG(__VA_ARGS__)

static void printHelp()
{
    std::cout <<
        "Usage : shader_grep [OPTIONS] keyword trace_file.pat\n"
        "Options:\n"
        "  -h            Print help\n"
        "  -v            Print version\n"
        "  -t TYPE       Restrict search to shader type [VERT|FRAG|COMP|GEOM|TESE|TESC]"
        "  -D            Add debug information to stderr\n"
        ;
}

static void printVersion()
{
    std::cout << PATRACE_VERSION << std::endl;
}

void grep_shader(ParseInterface& input, const std::string& trace_filename, const std::string& match, GLenum match_shader_type)
{
    common::CallTM *call = nullptr;

    // Go through entire trace file
    while ((call = input.next_call())) {}

    // Check what we found
    for (const auto& context : input.contexts)
    {
        for (const auto& shader : context.shaders.all())
        {
            if (match_shader_type == GL_NONE || shader.shader_type == match_shader_type)
            {
                const char* stype = "VERT";
                switch (shader.shader_type)
                {
                case GL_FRAGMENT_SHADER: stype = "FRAG"; break;
                case GL_VERTEX_SHADER: ; stype = "VERT"; break;
                case GL_GEOMETRY_SHADER: stype = "GEOM"; break;
                case GL_TESS_EVALUATION_SHADER: stype = "TESE"; break;
                case GL_TESS_CONTROL_SHADER: stype = "TESC"; break;
                case GL_COMPUTE_SHADER: stype = "COMP"; break;
                default: assert(false); break;
                }
                std::istringstream stream(shader.source_code);
                std::string line;
                while (std::getline(stream, line))
                {
                    if (line.find(match) != std::string::npos)
                    {
                        fprintf(stdout, "%s : %s : %03u : %s\n", trace_filename.c_str(), stype, shader.id, line.c_str());
                    }
                }
            }
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
        else if (arg == "-v")
        {
            printVersion();
            return 0;
        }
        else if (arg == "-D")
        {
            debug = true;
        }
        else if (arg == "-t" && argIndex + 1 < argc)
        {
            std::string arg = argv[argIndex + 1];
            if (arg == "VERT")
            {
                shader_type = GL_VERTEX_SHADER;
            }
            else if (arg == "FRAG")
            {
                shader_type = GL_FRAGMENT_SHADER;
            }
            else if (arg == "COMP")
            {
                shader_type = GL_COMPUTE_SHADER;
            }
            else if (arg == "GEOM")
            {
                shader_type = GL_VERTEX_SHADER;
            }
            else if (arg == "TESE")
            {
                shader_type = GL_TESS_EVALUATION_SHADER;
            }
            else if (arg == "TESC")
            {
                shader_type = GL_TESS_CONTROL_SHADER;
            }
            else
            {
                std::cerr << "Unknown shader type: " << arg << std::endl;
                exit(1);
            }
            argIndex++;
        }
        else
        {
            std::cerr << "Error: Unknown option " << arg << std::endl;
            printHelp();
            return 1;
        }
    }

    if (argIndex + 2 > argc)
    {
        printHelp();
        return 1;
    }
    std::string match = argv[argIndex++];
    std::string source_trace_filename = argv[argIndex++];
    ParseInterface inputFile;
    if (!inputFile.open(source_trace_filename))
    {
        std::cerr << "Failed to open for reading: " << source_trace_filename << std::endl;
        return 1;
    }
    grep_shader(inputFile, source_trace_filename, match, shader_type);
    inputFile.close();
    return 0;
}
