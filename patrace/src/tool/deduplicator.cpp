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
#include "common/memory.hpp"
#include "eglstate/context.hpp"
#include "tool/config.hpp"
#include "base/base.hpp"
#include "tool/utils.hpp"

#define DEDUP_BUFFERS 1
#define DEDUP_UNIFORMS 2
#define DEDUP_TEXTURES 4
#define DEDUP_SCISSORS 8
#define DEDUP_BLENDFUNC 16
#define DEDUP_ENABLE 32
#define DEDUP_DEPTHFUNC 64
#define DEDUP_VERTEXATTRIB 128
#define DEDUP_MAKECURRENT 256
#define DEDUP_PROGRAMS 512
#define DEDUP_CSB 1024

static bool replace = false;
static std::pair<int, int> dedups;
static bool onlycount = false;
static bool verbose = false;
static FILE* fp = stdout;
static int lastframe = -1;
static bool debug = false;
#define DEBUG_LOG(...) if (debug) DBG_LOG(__VA_ARGS__)

static void printHelp()
{
    std::cout <<
        "Usage : deduplicator [OPTIONS] trace_file.pat new_file.pat\n"
        "Only works for single-threaded traces for now.\n"
        "Options:\n"
        "  --buffers     Deduplicate glBindBuffer calls\n"
        "  --textures    Deduplicate glBindTexture and glBindSampler calls\n"
        "  --uniforms    Deduplicate glUniform2/3f calls\n"
        "  --scissors    Deduplicate glScissor calls\n"
        "  --blendfunc   Deduplicate glBlendFunc and glBlendColor calls\n"
        "  --depthfunc   Deduplicate glDepthFunc calls\n"
        "  --enable      Deduplicate glEnable/glDisable calls\n"
        "  --vertexattr  Deduplicate glVertexAttribPointer calls\n"
        "  --makecurrent Deduplicate eglMakeCurrent calls\n"
        "  --programs    Deduplicate glUseProgram calls\n"
        "  --csb         Deduplicate client side buffer calls\n"
        "  --all         Deduplicate all the above\n"
        "  --end FRAME   End frame (terminates trace here)\n"
        "  --last FRAME  Stop doing changes at this frame (copies the remaining trace without changes)\n"
        "  --replace     Replace calls with glEnable(GL_INVALID_INDEX) instead of removing\n"
        "  --verbose     Print more information while running\n"
        "  -c            Only count and report instances, do no changes\n"
        "  -d            Debug mode (print a lot of info)\n"
        "  -o FILE       Write results to file\n"
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
    dedups.second++;
    if (onlycount) return;
    const unsigned int WRITE_BUF_LEN = 150*1024*1024;
    static char buffer[WRITE_BUF_LEN];
    char *dest = buffer;
    dest = call->Serialize(dest);
    outputFile.Write(buffer, dest-buffer);
}

static void dedup(common::OutFile& outputFile, int &stat)
{
    if (replace && !onlycount)
    {
        common::CallTM enable("glEnable");
        enable.mArgs.push_back(new common::ValueTM((GLenum)GL_INVALID_INDEX));
        writeout(outputFile, &enable);
    }
    dedups.first++;
    stat++;
}

void deduplicate(ParseInterface& input, common::OutFile& outputFile, int endframe, int tid, int flags)
{
    common::CallTM *call = nullptr;
    std::unordered_map<GLenum, GLuint> buffers;
    std::unordered_map<GLuint, GLfloat> uniform1f;
    std::unordered_map<GLuint, std::tuple<GLfloat, GLfloat>> uniform2f;
    std::unordered_map<GLuint, std::tuple<GLfloat, GLfloat, GLfloat>> uniform3f;
    std::unordered_map<GLenum, GLuint> textures;
    std::unordered_map<GLenum, GLuint> samplers;
    std::tuple<GLint, GLint, GLsizei, GLsizei> scissors = std::make_tuple(-1, -1, -1, -1);
    std::tuple<GLenum, GLenum> blendfunc = std::make_tuple(GL_NONE, GL_NONE);
    GLenum depthfunc = GL_NONE;
    std::unordered_map<GLenum, bool> enabled;
    std::tuple<GLfloat, GLfloat, GLfloat, GLfloat> blendcolor = std::make_tuple(0.0f, 0.0f, 0.0f, 0.0f);
    std::unordered_map<GLuint, std::tuple<GLuint, GLint, GLenum, GLboolean, GLsizei, uint64_t>> vertexattrib;
    std::unordered_map<GLuint, bool> enablevertex;
    std::unordered_map<GLuint, GLuint> vertexdivisor;
    std::pair<int, int> vertexattrs;
    std::pair<int, int> bindbuffers;
    std::pair<int, int> enables; // includes disables
    std::pair<int, int> scissordupes;
    std::pair<int, int> blendcols;
    std::pair<int, int> bindtexs;
    std::pair<int, int> bindsamps;
    std::pair<int, int> uniforms;
    std::pair<int, int> depthfuncs;
    std::pair<int, int> blendfuncs;
    std::pair<int, int> useprograms;
    std::pair<int, int> makecurr;
    std::pair<int, int> csb;
    int makecurr_harmless = 0;
    int last_swap = 0;
    GLuint current_program_id = 0;
    int old_surface_id = (int64_t)EGL_NO_SURFACE;
    int old_context_id = (int64_t)EGL_NO_CONTEXT;
    std::pair<int, int> csbdedup_possible = std::make_pair(-1, -1);

    // Go through entire trace file
    while ((call = input.next_call()))
    {
        if (lastframe != -1 && input.frames >= lastframe)
        {
            writeout(outputFile, call);
            continue;
        }

        if ((int)call->mTid != tid) continue;

        if (call->mCallName == "glUseProgram")
        {
            useprograms.second++;
            const GLenum id = call->mArgs[0]->GetAsUInt();
            if (id != current_program_id || id == 0 || !(flags & DEDUP_PROGRAMS))
            {
                uniform2f.clear();
                uniform3f.clear();
                vertexattrib.clear();
                enablevertex.clear();
                current_program_id = id;
                writeout(outputFile, call);
            }
            else
            {
                dedup(outputFile, useprograms.first);
                assert(current_program_id == id);
            }
            csbdedup_possible = std::make_pair(-1, -1);
        }
        else if (call->mCallName == "eglMakeCurrent")
        {
            int surface = call->mArgs[1]->GetAsInt();
            int readsurface = call->mArgs[2]->GetAsInt();
            int context = call->mArgs[3]->GetAsInt();
            assert(readsurface == surface);

            uniform2f.clear();
            uniform3f.clear();
            buffers.clear();
            textures.clear();
            samplers.clear();
            vertexattrib.clear();
            scissors = std::make_tuple(-1, -1, -1, -1);
            blendfunc = std::make_tuple(GL_NONE, GL_NONE);
            depthfunc = GL_NONE;
            enabled.clear();
            blendcolor = std::make_tuple(0.0f, 0.0f, 0.0f, 0.0f);
            enablevertex.clear();
            current_program_id = 0;
            makecurr.second++;

            if ((context == (int64_t)EGL_NO_CONTEXT && old_context_id != (int64_t)EGL_NO_CONTEXT)
                || (surface == (int64_t)EGL_NO_SURFACE && input.surface_index != UNBOUND))
            {
                writeout(outputFile, call);
            }
            else if (context == (int64_t)EGL_NO_CONTEXT || surface == (int64_t)EGL_NO_SURFACE)
            {
                if (flags & DEDUP_MAKECURRENT) dedup(outputFile, makecurr.first);
                else writeout(outputFile, call);
            }
            else if (old_context_id == context && old_surface_id == surface && (flags & DEDUP_MAKECURRENT))
            {
                dedup(outputFile, makecurr.first);
                if (last_swap == (int)call->mCallNo - 1 || input.frames == 0) makecurr_harmless++;
            }
            else
            {
                writeout(outputFile, call);
            }

            old_surface_id = surface;
            old_context_id = context;
        }
        else if (call->mCallName == "glBindBuffer" && (flags & DEDUP_BUFFERS))
        {
            bindbuffers.second++;
            const GLenum target = call->mArgs[0]->GetAsUInt();
            const GLuint id = call->mArgs[1]->GetAsUInt();
            if ((int)target == csbdedup_possible.second) csbdedup_possible = std::make_pair(-1, -1);
            if (buffers.count(target) == 0 || buffers.at(target) != id)
            {
                writeout(outputFile, call);
                buffers[target] = id;
            }
            else dedup(outputFile, bindbuffers.first);
        }
        else if (call->mCallName == "glVertexAttribDivisor" && (flags & DEDUP_VERTEXATTRIB))
        {
            enables.second++;
            const GLuint target = call->mArgs[0]->GetAsUInt();
            const GLuint divisor = call->mArgs[1]->GetAsUInt();
            if (vertexdivisor.count(target) > 0 && vertexdivisor[target] == divisor) dedup(outputFile, enables.first);
            else writeout(outputFile, call);
            vertexdivisor[target] = divisor;
        }
        else if (call->mCallName == "glEnableVertexAttribArray" && (flags & DEDUP_VERTEXATTRIB))
        {
            enables.second++;
            const GLenum target = call->mArgs[0]->GetAsUInt();
            if (enablevertex.count(target) > 0 && enablevertex[target]) dedup(outputFile, enables.first);
            else writeout(outputFile, call);
            enablevertex[target] = true;
        }
        else if (call->mCallName == "glDisableVertexAttribArray" && (flags & DEDUP_VERTEXATTRIB))
        {
            enables.second++;
            const GLenum target = call->mArgs[0]->GetAsUInt();
            if (enablevertex.count(target) > 0 && !enablevertex[target]) dedup(outputFile, enables.first);
            else writeout(outputFile, call);
            enablevertex[target] = false;
        }
        else if (call->mCallName == "glEnable" && (flags & DEDUP_ENABLE))
        {
            enables.second++;
            const GLenum key = call->mArgs[0]->GetAsUInt();
            if (enabled.count(key) == 0 || !enabled.at(key))
            {
                writeout(outputFile, call);
                enabled[key] = true;
            }
            else dedup(outputFile, enables.first);
        }
        else if (call->mCallName == "glDisable" && (flags & DEDUP_ENABLE))
        {
            enables.second++;
            const GLenum key = call->mArgs[0]->GetAsUInt();
            if (enabled.count(key) == 0 || enabled.at(key))
            {
                writeout(outputFile, call);
                enabled[key] = false;
            }
            else dedup(outputFile, enables.first);
        }
        else if (call->mCallName == "glScissor" && (flags & DEDUP_SCISSORS))
        {
            scissordupes.second++;
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
            else dedup(outputFile, scissordupes.first);
        }
        else if (call->mCallName == "glDepthFunc" && (flags & DEDUP_DEPTHFUNC))
        {
            depthfuncs.second++;
            const GLenum func = call->mArgs[0]->GetAsUInt();
            if (depthfunc != func)
            {
                writeout(outputFile, call);
                depthfunc = func;
            }
            else dedup(outputFile, depthfuncs.first);
        }
        else if (call->mCallName == "glBlendFunc" && (flags & DEDUP_BLENDFUNC))
        {
            blendfuncs.second++;
            const GLenum sfactor = call->mArgs[0]->GetAsUInt();
            const GLenum dfactor = call->mArgs[1]->GetAsUInt();
            auto val = std::make_tuple(sfactor, dfactor);
            if (blendfunc != val)
            {
                writeout(outputFile, call);
                blendfunc = val;
            }
            else dedup(outputFile, blendfuncs.first);
        }
        else if (call->mCallName == "glVertexAttribPointer" && (flags & DEDUP_VERTEXATTRIB))
        {
            vertexattrs.second++;
            const GLuint index = call->mArgs[0]->GetAsUInt();
            const GLint size = call->mArgs[1]->GetAsInt();
            const GLenum type = call->mArgs[2]->GetAsUInt();
            const GLboolean normalized = (GLboolean)call->mArgs[3]->GetAsUInt();
            const GLsizei stride = call->mArgs[4]->GetAsInt();
            const uint64_t pointer = call->mArgs[5]->GetAsUInt64();
            auto val = std::make_tuple(index, size, type, normalized, stride, pointer);
            if (vertexattrib.count(index) == 0 || vertexattrib[index] != val)
            {
                writeout(outputFile, call);
                vertexattrib[index] = val;
            }
            else dedup(outputFile, vertexattrs.first);
        }
        else if (call->mCallName == "glBlendColor" && (flags & DEDUP_BLENDFUNC))
        {
            blendcols.second++;
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
            else dedup(outputFile, blendcols.first);
        }
        else if (call->mCallName == "glBindTexture" && (flags & DEDUP_TEXTURES))
        {
            bindtexs.second++;
            const GLenum target = call->mArgs[0]->GetAsUInt();
            const GLuint id = call->mArgs[1]->GetAsUInt();
            if (textures.count(target) == 0 || textures.at(target) != id)
            {
                writeout(outputFile, call);
                textures[target] = id;
                samplers.clear();
            }
            else dedup(outputFile, bindtexs.first);
        }
        else if (call->mCallName == "glBindSampler" && (flags & DEDUP_TEXTURES))
        {
            bindsamps.second++;
            const GLenum target = call->mArgs[0]->GetAsUInt();
            const GLuint id = call->mArgs[1]->GetAsUInt();
            if (samplers.count(target) == 0 || samplers.at(target) != id)
            {
                writeout(outputFile, call);
                samplers[target] = id;
            }
            else dedup(outputFile, bindsamps.first);
        }
        else if ((call->mCallName == "glUniform1f" || call->mCallName == "glUniform1fv") && (flags & DEDUP_UNIFORMS))
        {
            uniforms.second++;
            const GLuint location = call->mArgs[0]->GetAsUInt();
            const GLsizei count = (call->mCallName == "glUniform1f") ? 1 : call->mArgs[1]->GetAsUInt();
            const GLfloat v1 = (call->mCallName == "glUniform1f") ? call->mArgs[1]->GetAsFloat() : call->mArgs[2]->mArray[0].GetAsFloat();
            if (count != 1 || uniform1f.count(location) == 0 || uniform1f.at(location) != v1)
            {
                writeout(outputFile, call);
                uniform1f[location] = v1;
            }
            else dedup(outputFile, uniforms.first);
        }
        else if (call->mCallName == "glUniform2f" && (flags & DEDUP_UNIFORMS))
        {
            uniforms.second++;
            const GLuint location = call->mArgs[0]->GetAsUInt();
            const GLfloat v1 = call->mArgs[1]->GetAsFloat();
            const GLfloat v2 = call->mArgs[2]->GetAsFloat();
            if (uniform2f.count(location) == 0 || std::get<0>(uniform2f.at(location)) != v1 || std::get<1>(uniform2f.at(location)) != v2)
            {
                writeout(outputFile, call);
                uniform2f[location] = std::make_tuple(v1, v2);
            }
            else dedup(outputFile, uniforms.first);
        }
        else if (call->mCallName == "glUniform2fv" && (flags & DEDUP_UNIFORMS))
        {
            uniforms.second++;
            const GLuint location = call->mArgs[0]->GetAsUInt();
            const GLsizei count = call->mArgs[1]->GetAsUInt();
            const GLfloat v1 = call->mArgs[2]->mArray[0].GetAsFloat();
            const GLfloat v2 = call->mArgs[2]->mArray[1].GetAsFloat();
            if (count != 1 || uniform2f.count(location) == 0 || std::get<0>(uniform2f.at(location)) != v1 || std::get<1>(uniform2f.at(location)) != v2)
            {
                writeout(outputFile, call);
                uniform2f[location] = std::make_tuple(v1, v2);
            }
            else dedup(outputFile, uniforms.first);
        }
        else if (call->mCallName == "glUniform3f" && (flags & DEDUP_UNIFORMS))
        {
            uniforms.second++;
            const GLuint location = call->mArgs[0]->GetAsUInt();
            const GLfloat v1 = call->mArgs[1]->GetAsFloat();
            const GLfloat v2 = call->mArgs[2]->GetAsFloat();
            const GLfloat v3 = call->mArgs[3]->GetAsFloat();
            if (uniform3f.count(location) == 0 || std::get<0>(uniform3f.at(location)) != v1 || std::get<1>(uniform3f.at(location)) != v2 || std::get<2>(uniform3f.at(location)) != v3)
            {
                writeout(outputFile, call);
                uniform3f[location] = std::make_tuple(v1, v2, v3);
            }
            else dedup(outputFile, uniforms.first);
        }
        else if (call->mCallName == "glUniform3fv" && (flags & DEDUP_UNIFORMS))
        {
            uniforms.second++;
            const GLuint location = call->mArgs[0]->GetAsUInt();
            const GLsizei count = call->mArgs[1]->GetAsUInt();
            const GLfloat v1 = call->mArgs[2]->mArray[0].GetAsFloat();
            const GLfloat v2 = call->mArgs[2]->mArray[1].GetAsFloat();
            const GLfloat v3 = call->mArgs[2]->mArray[2].GetAsFloat();
            if (count != 1 || uniform3f.count(location) == 0 || std::get<0>(uniform3f.at(location)) != v1 || std::get<1>(uniform3f.at(location)) != v2 || std::get<2>(uniform3f.at(location)) != v3)
            {
                writeout(outputFile, call);
                uniform3f[location] = std::make_tuple(v1, v2, v3);
            }
            else dedup(outputFile, uniforms.first);
        }
        else if (call->mCallName == "eglGetError")
        {
            writeout(outputFile, call);
            if (last_swap == (int)call->mCallNo - 1) last_swap++; // pretend this call doesn't exist for purposes of checking if we just swapped
        }
        else if (call->mCallName == "eglSwapBuffers" && input.frames != endframe) // log (slow) progress
        {
            if (verbose) DBG_LOG("Frame %d / %d\n", (int)input.frames, endframe);
            writeout(outputFile, call);
            last_swap = call->mCallNo;
        }
        else if (call->mCallName == "eglSwapBuffers" && input.frames == endframe) // terminate here?
        {
            writeout(outputFile, call);
            if (verbose) DBG_LOG("Ending!\n");
            break;
        }
        else if (call->mCallName == "glCopyClientSideBuffer" && (flags & DEDUP_CSB))
        {
            csb.second++;
            const GLenum target = call->mArgs[0]->GetAsUInt();
            const GLuint name = call->mArgs[1]->GetAsUInt();
            if (csbdedup_possible == std::make_pair((int)name, (int)target))
            {
                dedup(outputFile, csb.first);
                continue;
            }
            csbdedup_possible = std::make_pair((int)name, (int)target);
            writeout(outputFile, call);
        }
        else if (call->mCallName == "glUnmapBuffer" && (flags & DEDUP_CSB))
        {
            const GLenum target = call->mArgs[0]->GetAsUInt();
            if ((int)target == csbdedup_possible.second) csbdedup_possible = std::make_pair(-1, -1);
            writeout(outputFile, call);
        }
        else if (call->mCallName == "glClientSideBufferData" && (flags & DEDUP_CSB))
        {
            csbdedup_possible = std::make_pair(-1, -1);
            writeout(outputFile, call);
        }
        else if (call->mCallName == "glPatchClientSideBuffer" && (flags & DEDUP_CSB))
        {
            csbdedup_possible = std::make_pair(-1, -1);
            writeout(outputFile, call);
        }
        else if (call->mCallName == "glClientSideBufferSubData" && (flags & DEDUP_CSB))
        {
            csbdedup_possible = std::make_pair(-1, -1);
            writeout(outputFile, call);
        }
        else
        {
            writeout(outputFile, call);
        }
    }
    fprintf(fp, "Removed %d / %d calls (%d%%)\n", dedups.first, dedups.second, dedups.first * 100 / dedups.second);
    if (useprograms.first) fprintf(fp, "Removed %d / %d glUseProgram calls (%d%%)\n", useprograms.first, useprograms.second, useprograms.first * 100 / useprograms.second);
    if (vertexattrs.first) fprintf(fp, "Removed %d / %d vertex attr calls (%d%%)\n", vertexattrs.first, vertexattrs.second, vertexattrs.first * 100 / vertexattrs.second);
    if (bindbuffers.first) fprintf(fp, "Removed %d / %d bindbuffer calls (%d%%)\n", bindbuffers.first, bindbuffers.second, bindbuffers.first * 100 / bindbuffers.second);
    if (enables.first) fprintf(fp, "Removed %d / %d enable/disable calls (%d%%)\n", enables.first, enables.second, enables.first * 100 / enables.second);
    if (scissordupes.first) fprintf(fp, "Removed %d / %d scissor calls (%d%%)\n", scissordupes.first, scissordupes.second, scissordupes.first * 100 / scissordupes.second);
    if (blendcols.first) fprintf(fp, "Removed %d / %d blendcol calls (%d%%)\n", blendcols.first, blendcols.second, blendcols.first * 100 / blendcols.second);
    if (bindtexs.first) fprintf(fp, "Removed %d / %d bindtexture calls (%d%%)\n", bindtexs.first, bindtexs.second, bindtexs.first * 100 / bindtexs.second);
    if (bindsamps.first) fprintf(fp, "Removed %d / %d bindsampler calls (%d%%)\n", bindsamps.first, bindsamps.second, bindsamps.first * 100 / bindsamps.second);
    if (uniforms.first) fprintf(fp, "Removed %d / %d relevant uniform calls (%d%%)\n", uniforms.first, uniforms.second, uniforms.first * 100 / uniforms.second);
    if (depthfuncs.first) fprintf(fp, "Removed %d / %d depth func calls (%d%%)\n", depthfuncs.first, depthfuncs.second, depthfuncs.first * 100 / depthfuncs.second);
    if (blendfuncs.first) fprintf(fp, "Removed %d / %d blend func calls (%d%%)\n", blendfuncs.first, blendfuncs.second, blendfuncs.first * 100 / blendfuncs.second);
    if (makecurr.first) fprintf(fp, "Removed %d / %d makecurrent calls (%d%%, at least %d were harmless on Mali)\n", makecurr.first, makecurr.second, makecurr.first * 100 / makecurr.second, makecurr_harmless);
    if (csb.first) fprintf(fp, "Removed %d / %d glCopyClientSideBuffer func calls (%d%%)\n", csb.first, csb.second, csb.first * 100 / csb.second);
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
        else if (arg == "--last")
        {
            argIndex++;
            lastframe = atoi(argv[argIndex]);
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
        else if (arg == "--programs")
        {
            flags |= DEDUP_PROGRAMS;
        }
        else if (arg == "--csb")
        {
            flags |= DEDUP_CSB;
        }
        else if (arg == "--makecurrent")
        {
            flags |= DEDUP_MAKECURRENT;
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
        else if (arg == "--verbose")
        {
            verbose = true;
        }
        else if (arg == "-c")
        {
            onlycount = true;
        }
        else if (arg == "-d")
        {
            debug = true;
        }
        else if (arg == "-o")
        {
            argIndex++;
            if (argIndex == argc) { printHelp(); return -3; }
            std::string filename = argv[argIndex];
            fp = fopen(filename.c_str(), "w");
            if (!fp) { std::cerr << "Error: Could not open file for writing: "  << filename << std::endl; return -4; };
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

    if ((argIndex + 2 > argc && !onlycount) || (onlycount && argIndex + 1 > argc))
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

    Json::Value header = inputFile.header;
    if (header.isMember("multiThread") && header.get("multiThread", false).asBool())
    {
        std::cerr << "Multi-threaded - not supported yet: " << source_trace_filename << std::endl;
        return 2;
    }

    common::OutFile outputFile;
    if (!onlycount)
    {
        std::string target_trace_filename = argv[argIndex++];
        if (!outputFile.Open(target_trace_filename.c_str()))
        {
            std::cerr << "Failed to open for writing: " << target_trace_filename << std::endl;
            return 1;
        }
        Json::Value info;
        addConversionEntry(header, "deduplicate", source_trace_filename, info);
        Json::FastWriter writer;
        const std::string json_header = writer.write(header);
        outputFile.mHeader.jsonLength = json_header.size();
        outputFile.WriteHeader(json_header.c_str(), json_header.size());
    }
    deduplicate(inputFile, outputFile, endframe, header["defaultTid"].asInt(), flags);
    inputFile.close();
    outputFile.Close();
    return 0;
}
