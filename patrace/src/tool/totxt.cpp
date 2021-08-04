#include <errno.h>
#include <stdio.h>

#include "tool/parse_interface.h"
#include <common/api_info.hpp>
#include <common/parse_api.hpp>
#include <common/trace_model.hpp>
#include <common/gl_utility.hpp>
#include <tool/config.hpp>

static int start_frame = 0;
static int end_frame = INT32_MAX;
static int our_tid = -1;
static bool verbose = false;
static bool colours = false;
static bool bare = false;

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

void usage(const char *argv0)
{
    DBG_LOG(
        "Usage: %s [OPTION] <path_to_trace_file> [outfile]\n"
        "Version: " PATRACE_VERSION "\n"
        "dump trace as text\n"
        "\n"
        "  -help  Display this message\n"
        "  -f <f> <l> Define frame interval, inclusive\n"
        "  -tid   <thread_id> Only the function calls invoked by thread <thread_id> will be printed\n"
        "  -v     Verbose output\n"
        "  -c     Add colours\n"
        "  -b     Bare mode (useful for making diffs between two output files)\n"
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

static bool callback(ParseInterfaceBase& input, common::CallTM *call, void *fpp)
{
    FILE *fp = (FILE*)fpp;

    if (input.frames > end_frame) return false;
    if (!(input.frames >= start_frame && input.frames <= end_frame && (our_tid == -1 || our_tid == (int)call->mTid))) return true;

    const char *injected = call->mInjected ? "INJECTED " : "";
    if (call->mInjected && colours) injected = BLU "INJECTED " RESET;
    const char *mark = (colours && call->mCallName.compare(0, 14, "eglSwapBuffers") == 0) ? GRN : "";
    if (colours && call->mCallName == "glInsertEventMarkerEXT") mark = YEL;
    if (colours && call->mCallName == "eglMakeCurrent") mark = CYN;
    const char *reset = (colours) ? RESET : "";
    if (!bare) fprintf(fp, "[t%d, f%d, c%d] %d : %s%s%s%s%s\n", call->mTid, input.frames, input.context_index, call->mCallNo, injected, reset, mark, call->ToStr(false).c_str(), reset);
    else fprintf(fp, "%s%s%s%s%s\n", injected, reset, mark, call->ToStr(false).c_str(), reset);
    if (verbose)
    {
        const int context_index = input.context_index;
        std::string c;
        if (call->mCallName.find("glTex") != std::string::npos
            || call->mCallName.find("glCompressedTex") != std::string::npos
            || call->mCallName.find("glCopyTex") != std::string::npos
            || call->mCallName == "glEGLImageTargetTexture2DOES"
            || call->mCallName == "glFramebufferTexture2D"
            || call->mCallName == "glFramebufferTexture2DOES"
            || call->mCallName == "glFramebufferTextureLayerOES"
            || call->mCallName == "glActiveTexture"
            || call->mCallName == "glBindTexture" || call->mCallName == "glBindImageTexture")
        {
            GLenum target = 0;
            GLuint tex_id = 0;
            int tex_index = -1;
            if (call->mCallName == "glFramebufferTexture2D" || call->mCallName == "glFramebufferTexture2DOES") target = call->mArgs[2]->GetAsUInt();
            else if (call->mCallName != "glFramebufferTextureLayer" && call->mCallName != "glFramebufferTextureLayerOES") target = call->mArgs[0]->GetAsUInt();
            else tex_id = call->mArgs[2]->GetAsUInt();
            const GLuint unit = input.contexts[context_index].activeTextureUnit;
            if (call->mCallName == "glBindTexture" || call->mCallName == "glBindImageTexture") tex_id = call->mArgs[1]->GetAsUInt();
            else if (target != 0) tex_id = input.contexts[context_index].textureUnits[unit][target];
            if (input.contexts[context_index].textures.contains(tex_id)) tex_index = input.contexts[context_index].textures.remap(tex_id);
            c += " tex_unit=" + std::to_string(unit) + " tex=" + std::to_string(tex_index) + "|" + std::to_string(tex_id);
        }
        else if (call->mCallName == "glBindVertexArray" || call->mCallName == "glBindVertexArrayOES")
        {
            const GLuint vao_id = call->mArgs[0]->GetAsUInt();
            if (vao_id != 0)
            {
                input.contexts[context_index].vao_index = input.contexts[context_index].vaos.remap(vao_id);
                StateTracker::VertexArrayObject& vao = input.contexts[context_index].vaos.at(input.contexts[context_index].vao_index);
                for (const auto& target : vao.boundBufferIds)
                {
                    for (auto& buffer : target.second)
                    {
                        c += " " + texEnum(target.first) + "=" + std::to_string(buffer.second.buffer);
                    }
                }
            }
        }
        if (call->mCallName.find("glUniform") != std::string::npos)
        {
            const int program_index = input.contexts[context_index].program_index;
            if (program_index != UNBOUND)
            {
                const StateTracker::Program &p = input.contexts[context_index].programs.at(program_index);
                const GLint location = call->mArgs[0]->GetAsInt();
                c += " program=" + std::to_string(program_index) + "|" + std::to_string(p.id) + " name=" + (p.uniformNames.count(location) ? p.uniformNames.at(location) : std::string("INVALID"));
            }
        }
        if (call->mCallName.compare(0, 6, "glDraw") == 0 && call->mCallName != "glDrawBuffers")
        {
            const int program_index = input.contexts[context_index].program_index;
            c += " drawfb=" + std::to_string(input.contexts[context_index].drawframebuffer);
            if (program_index > 0)
            {
                const StateTracker::Program& p = input.contexts[context_index].programs[program_index];
                c += " program=" + std::to_string(p.id);
#if 0
                for (const auto& pair : p.samplers)
                {
                    const StateTracker::GLSLSampler& s = pair.second;
                    const GLenum binding = samplerTypeToBindingType(s.type);
                    if (s.value >= 0 && input.contexts[context_index].textureUnits.count(s.value) && input.contexts[context_index].textureUnits.at(s.value).count(binding))
                    {
                        const GLuint texture_id = input.contexts[context_index].textureUnits.at(s.value).at(binding);
                        if (texture_id != 0)
                        {
                            const int tex_idx = input.contexts[context_index].textures.remap(texture_id);
                            c += " tu" + std::to_string(s.value) + ":" + texEnum(binding) + ":" + std::to_string(s.value) + ":" + pair.first + "=" + std::to_string(tex_idx) + "|" + std::to_string(texture_id);
                        }
                    }
                    if (input.contexts[context_index].sampler_binding.count(s.value))
                    {
                        const GLuint samplerObject = input.contexts[context_index].sampler_binding.at(s.value);
                        const int sampler_idx = input.contexts[context_index].samplers.remap(samplerObject);
                        const StateTracker::Sampler& sampler = input.contexts[context_index].samplers.at(sampler_idx);
                        c += " sampler=" + std::to_string(sampler.index) + "|" + std::to_string(sampler.id);
                    }
                }
#endif
            }
            c += " renderpass=" + std::to_string(input.contexts[context_index].render_passes.back().index);
        }
        if (c.length() > 0) fprintf(fp, "\t%s\n", c.c_str());
    }
    return true;
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
        else if (!strcmp(arg, "-help") || !strcmp(arg, "-h"))
        {
            usage(argv[0]);
            return 0;
        }
        else if (!strcmp(arg, "-f"))
        {
            start_frame = readValidValue(argv[++i]);
            end_frame = readValidValue(argv[++i]);
            if (start_frame > end_frame)
            {
                DBG_LOG("Error: last frame is less than start frame.\n");
                return -1;
            }
        }
        else if (!strcmp(arg, "-tid"))
        {
            our_tid = readValidValue(argv[++i]);
        }
        else if (!strcmp(arg, "-c"))
        {
            colours = true;
        }
        else if (!strcmp(arg, "-b"))
        {
            bare = true;
        }
        else if (!strcmp(arg, "-v"))
        {
            verbose = true;
        }
        else
        {
            usage(argv[0]);
            return -1;
        }
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
    ParseInterface inputFile;
    if (!inputFile.open(filename))
    {
        std::cerr << "Failed to open for reading: " << filename << std::endl;
        return 1;
    }
    common::CallTM *call = nullptr;
    while ((call = inputFile.next_call()) && callback(inputFile, call, fp)) {}
    fclose(fp);
    return 0;
}
