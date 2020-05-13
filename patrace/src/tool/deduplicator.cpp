// Warning: This tool is a huge hack, use with care!

#include <utility>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>
#include <limits.h>
#include <unordered_map>

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

#define DEDUP_BUFFERS 1
#define DEDUP_UNIFORMS 2
#define DEDUP_TEXTURES 4
#define DEDUP_SCISSORS 8
#define DEDUP_BLENDFUNC 16
#define DEDUP_ENABLE 32
#define DEDUP_DEPTHFUNC 64
#define DEDUP_VERTEXATTRIB 64

static bool replace = false;
static long dedups = 0;

static void printHelp()
{
    std::cout <<
        "Usage : deduplicator [OPTIONS] trace_file.pat new_file.pat\n"
        "Options:\n"
        "  --buffers     Deduplicate glBindBuffer calls\n"
        "  --textures    Deduplicate glBindTexture and glBindSampler calls\n"
        "  --uniforms    Deduplicate glUniform2/3f calls\n"
        "  --scissors    Deduplicate glScissor calls\n"
        "  --blendfunc   Deduplicate glBlendFunc and glBlendColor calls\n"
        "  --depthfunc   Deduplicate glDepthFunc calls\n"
        "  --enable      Deduplicate glEnable/glDisable calls\n"
        "  --vertexattr  Deduplicate glVertexAttribPointer calls\n"
        "  --all         Deduplicate all the above\n"
        "  --end FRAME   End frame (terminates trace here)\n"
        "  --replace     Replace calls with glEnable(GL_INVALID_INDEX) instead of removing\n"
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

static void dedup(common::OutFile& outputFile)
{
    if (replace)
    {
        common::CallTM enable("glEnable");
        enable.mArgs.push_back(new common::ValueTM((GLenum)GL_INVALID_INDEX));
        writeout(outputFile, &enable);
    }
    dedups++;
}

void deduplicate(ParseInterface& input, common::OutFile& outputFile, int endframe, int tid, int flags)
{
    common::CallTM *call = nullptr;
    std::unordered_map<GLenum, GLuint> buffers;
    std::unordered_map<GLuint, std::tuple<GLfloat, GLfloat>> uniform2f;
    std::unordered_map<GLuint, std::tuple<GLfloat, GLfloat, GLfloat>> uniform3f;
    std::unordered_map<GLenum, GLuint> textures;
    std::unordered_map<GLenum, GLuint> samplers;
    std::tuple<GLint, GLint, GLsizei, GLsizei> scissors = std::make_tuple(-1, -1, -1, -1);
    std::tuple<GLenum, GLenum> blendfunc = std::make_tuple(GL_NONE, GL_NONE);
    GLenum depthfunc = GL_NONE;
    std::unordered_map<GLenum, bool> enabled;
    std::tuple<GLfloat, GLfloat, GLfloat, GLfloat> blendcolor = std::make_tuple(0.0f, 0.0f, 0.0f, 0.0f);
    std::tuple<GLuint, GLint, GLenum, GLboolean, GLsizei, uint64_t> vertexattrib;

    // Go through entire trace file
    while ((call = input.next_call()))
    {
        if ((int)call->mTid != tid) continue;

        if (call->mCallName == "glUseProgram")
        {
            uniform2f.clear();
            uniform3f.clear();
        }
        else if (call->mCallName == "eglMakeCurrent")
        {
            uniform2f.clear();
            uniform3f.clear();
            buffers.clear();
            textures.clear();
            samplers.clear();
            scissors = std::make_tuple(-1, -1, -1, -1);
            blendfunc = std::make_tuple(GL_NONE, GL_NONE);
            depthfunc = GL_NONE;
            enabled.clear();
            blendcolor = std::make_tuple(0.0f, 0.0f, 0.0f, 0.0f);
        }

        if (call->mCallName == "glBindBuffer" && (flags & DEDUP_BUFFERS))
        {
            const GLenum target = call->mArgs[0]->GetAsUInt();
            const GLuint id = call->mArgs[1]->GetAsUInt();
            if (buffers.count(target) == 0 || buffers.at(target) != id)
            {
                writeout(outputFile, call);
                buffers[target] = id;
            }
            else dedup(outputFile);
        }
        else if (call->mCallName == "glEnable" && (flags & DEDUP_ENABLE))
        {
            const GLenum key = call->mArgs[0]->GetAsUInt();
            if (enabled.count(key) == 0 || !enabled.at(key))
            {
                writeout(outputFile, call);
                enabled[key] = true;
            }
            else dedup(outputFile);
        }
        else if (call->mCallName == "glDisable" && (flags & DEDUP_ENABLE))
        {
            const GLenum key = call->mArgs[0]->GetAsUInt();
            if (enabled.count(key) == 0 || enabled.at(key))
            {
                writeout(outputFile, call);
                enabled[key] = false;
            }
            else dedup(outputFile);
        }
        else if (call->mCallName == "glScissor" && (flags & DEDUP_SCISSORS))
        {
            const GLint x = call->mArgs[0]->GetAsInt();
            const GLint y = call->mArgs[1]->GetAsInt();
            const GLsizei width = call->mArgs[2]->GetAsInt();
            const GLsizei height = call->mArgs[3]->GetAsInt();
            auto val = std::make_tuple(x, y, width, height);
            if (scissors != val)
            {
                writeout(outputFile, call);
                scissors = val;
            }
            else dedup(outputFile);
        }
        else if (call->mCallName == "glDepthFunc" && (flags & DEDUP_DEPTHFUNC))
        {
            const GLenum func = call->mArgs[0]->GetAsUInt();
            if (depthfunc != func)
            {
                writeout(outputFile, call);
                depthfunc = func;
            }
            else dedup(outputFile);
        }
        else if (call->mCallName == "glBlendFunc" && (flags & DEDUP_BLENDFUNC))
        {
            const GLenum sfactor = call->mArgs[0]->GetAsUInt();
            const GLenum dfactor = call->mArgs[1]->GetAsUInt();
            auto val = std::make_tuple(sfactor, dfactor);
            if (blendfunc != val)
            {
                writeout(outputFile, call);
                blendfunc = val;
            }
            else dedup(outputFile);
        }
        else if (call->mCallName == "glVertexAttribPointer" && (flags & DEDUP_VERTEXATTRIB))
        {
            const GLuint index = call->mArgs[0]->GetAsUInt();
            const GLint size = call->mArgs[1]->GetAsInt();
            const GLenum type = call->mArgs[2]->GetAsUInt();
            const GLboolean normalized = (GLboolean)call->mArgs[3]->GetAsUInt();
            const GLsizei stride = call->mArgs[4]->GetAsInt();
            const uint64_t pointer = call->mArgs[5]->GetAsUInt64();
            auto val = std::make_tuple(index, size, type, normalized, stride, pointer);
            if (vertexattrib != val)
            {
                writeout(outputFile, call);
                vertexattrib = val;
            }
            else dedup(outputFile);
        }
        else if (call->mCallName == "glBlendColor" && (flags & DEDUP_BLENDFUNC))
        {
            const GLfloat r = call->mArgs[0]->GetAsFloat();
            const GLfloat g = call->mArgs[1]->GetAsFloat();
            const GLfloat b = call->mArgs[2]->GetAsFloat();
            const GLfloat a = call->mArgs[3]->GetAsFloat();
            auto val = std::make_tuple(r, g, b, a);
            if (blendcolor != val)
            {
                writeout(outputFile, call);
                blendcolor = val;
            }
            else dedup(outputFile);
        }
        else if (call->mCallName == "glBindTexture" && (flags & DEDUP_TEXTURES))
        {
            const GLenum target = call->mArgs[0]->GetAsUInt();
            const GLuint id = call->mArgs[1]->GetAsUInt();
            if (textures.count(target) == 0 || textures.at(target) != id)
            {
                writeout(outputFile, call);
                textures[target] = id;
                samplers.clear();
            }
            else dedup(outputFile);
        }
        else if (call->mCallName == "glBindSampler" && (flags & DEDUP_TEXTURES))
        {
            const GLenum target = call->mArgs[0]->GetAsUInt();
            const GLuint id = call->mArgs[1]->GetAsUInt();
            if (samplers.count(target) == 0 || samplers.at(target) != id)
            {
                writeout(outputFile, call);
                samplers[target] = id;
            }
            else dedup(outputFile);
        }
        else if (call->mCallName == "glUniform2f" && (flags & DEDUP_UNIFORMS))
        {
            const GLuint location = call->mArgs[0]->GetAsUInt();
            const GLfloat v1 = call->mArgs[1]->GetAsFloat();
            const GLfloat v2 = call->mArgs[2]->GetAsFloat();
            if (uniform2f.count(location) == 0 || std::get<0>(uniform2f.at(location)) != v1 || std::get<1>(uniform2f.at(location)) != v2)
            {
                writeout(outputFile, call);
                uniform2f[location] = std::make_tuple(v1, v2);
            }
            else dedup(outputFile);
        }
        else if (call->mCallName == "glUniform3f" && (flags & DEDUP_UNIFORMS))
        {
            const GLuint location = call->mArgs[0]->GetAsUInt();
            const GLfloat v1 = call->mArgs[1]->GetAsFloat();
            const GLfloat v2 = call->mArgs[2]->GetAsFloat();
            const GLfloat v3 = call->mArgs[3]->GetAsFloat();
            if (uniform3f.count(location) == 0 || std::get<0>(uniform3f.at(location)) != v1 || std::get<1>(uniform3f.at(location)) != v2 || std::get<2>(uniform3f.at(location)) != v3)
            {
                writeout(outputFile, call);
                uniform3f[location] = std::make_tuple(v1, v2, v3);
            }
            else dedup(outputFile);
        }
        else if (call->mCallName == "eglSwapBuffers" && input.frames != endframe) // log (slow) progress
        {
            DBG_LOG("Frame %d / %d\n", (int)input.frames, endframe);
            writeout(outputFile, call);
        }
        else if (call->mCallName == "eglSwapBuffers" && input.frames == endframe) // terminate here?
        {
            writeout(outputFile, call);
            DBG_LOG("Ending!\n");
            break;
        }
        else
        {
            writeout(outputFile, call);
        }
    }
    DBG_LOG("Removed %ld calls\n", dedups);
}

int main(int argc, char **argv)
{
    int endframe = -1;
    int argIndex = 1;
    int flags = 0;
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
        else if (arg == "--end")
        {
            argIndex++;
            endframe = atoi(argv[argIndex]);
        }
        else if (arg == "--buffers")
        {
            flags |= DEDUP_BUFFERS;
        }
        else if (arg == "--textures")
        {
            flags |= DEDUP_TEXTURES;
        }
        else if (arg == "--uniforms")
        {
            flags |= DEDUP_UNIFORMS;
        }
        else if (arg == "--scissors")
        {
            flags |= DEDUP_SCISSORS;
        }
        else if (arg == "--blendfunc")
        {
            flags |= DEDUP_BLENDFUNC;
        }
        else if (arg == "--depthfunc")
        {
            flags |= DEDUP_DEPTHFUNC;
        }
        else if (arg == "--vertexattr")
        {
            flags |= DEDUP_VERTEXATTRIB;
        }
        else if (arg == "--all")
        {
            flags |= INT32_MAX;
        }
        else if (arg == "--enable")
        {
            flags |= DEDUP_ENABLE;
        }
        else if (arg == "--replace")
        {
            replace = true;
        }
        else if (arg == "-v")
        {
            printVersion();
            return 1;
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
    std::string source_trace_filename = argv[argIndex++];
    ParseInterface inputFile(true);
    inputFile.setQuickMode(true);
    inputFile.setScreenshots(false);
    if (!inputFile.open(source_trace_filename))
    {
        std::cerr << "Failed to open for reading: " << source_trace_filename << std::endl;
        return 1;
    }

    common::OutFile outputFile;
    std::string target_trace_filename = argv[argIndex++];
    if (!outputFile.Open(target_trace_filename.c_str()))
    {
        std::cerr << "Failed to open for writing: " << target_trace_filename << std::endl;
        return 1;
    }
    Json::Value header = inputFile.header;
    Json::Value info;
    addConversionEntry(header, "deduplicate", source_trace_filename, info);
    Json::FastWriter writer;
    const std::string json_header = writer.write(header);
    outputFile.mHeader.jsonLength = json_header.size();
    outputFile.WriteHeader(json_header.c_str(), json_header.size());
    deduplicate(inputFile, outputFile, endframe, header["defaultTid"].asInt(), flags);
    inputFile.close();
    outputFile.Close();
    return 0;
}
