#include "tool/parse_interface_retracing.hpp"

#include "common/gl_utility.hpp"
#include "helper/eglsize.hpp"

#include <assert.h>
#include <set>
#include <limits>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <sys/types.h>

using namespace retracer;

const int cache_size = 512;
static int mPerfFD = -1;
static bool perf_initialized = false;

static bool perf_init()
{
    struct perf_event_attr pe;

    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.type = PERF_TYPE_HARDWARE;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = PERF_COUNT_HW_CPU_CYCLES;
    pe.disabled = 1;
    pe.inherit = 1;
    pe.exclude_kernel = 0;
    pe.exclude_hv = 0;

    mPerfFD = syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
    if (mPerfFD < 0)
    {
        DBG_LOG("Error opening perf: %s\n", strerror(errno));
        return false;
    }
    perf_initialized = true;

    return true;
}

static bool perf_start()
{
    if (!perf_initialized) return false;

    if (ioctl(mPerfFD, PERF_EVENT_IOC_RESET, 0) == -1)
    {
        DBG_LOG("ioctl PERF_EVENT_IOC_RESET: %s\n", strerror(errno));
        return false;
    }

    if (ioctl(mPerfFD, PERF_EVENT_IOC_ENABLE, 0) == -1)
    {
        DBG_LOG("ioctl PERF_EVENT_IOC_ENABLE: %s\n", strerror(errno));
        return false;
    }

    return true;
}

static int64_t perf_stop()
{
    long long count = 0;

    if (!perf_initialized) return 0;

    if (read(mPerfFD, &count, sizeof(long long)) == -1)
    {
        perror("read");
        return false;
    }

    if (ioctl(mPerfFD, PERF_EVENT_IOC_DISABLE, 0) == -1)
    {
        DBG_LOG("ioctl PERF_EVENT_IOC_DISABLE: %s\n", strerror(errno));
        return false;
    }

    if (ioctl(mPerfFD, PERF_EVENT_IOC_RESET, 0) == -1)
    {
        DBG_LOG("ioctl PERF_EVENT_IOC_RESET: %s\n", strerror(errno));
        return false;
    }

    return count;
}

template<class T>
static void analyzeIndexBuffer(const void *ptr, intptr_t offset, DrawParams& ret)
{
    GLboolean primitive_restart = 0;
    glGetBooleanv(GL_PRIMITIVE_RESTART_FIXED_INDEX, &primitive_restart);
    std::map<T, long> cache;
    long timestamp = 0;
    const unsigned stride = (unsigned)sizeof(T);
    const T* buffer = (const T*)ptr;
    offset /= stride;
    std::set<T> seen;
    int sum_age = 0;
    for (int idx = offset; idx < ret.vertices + offset; idx++)
    {
        const T element = buffer[idx];
        long age;

        if (cache.count(element) > 0)
        {
            age = std::min<long>(timestamp - cache[element], cache_size);
        }
        else
        {
            age = cache_size;
        }

        if (primitive_restart && element == std::numeric_limits<T>::max())
        {
            ret.primitives++;
        }
        seen.insert(element);
        cache[element] = timestamp;
        timestamp++;
        sum_age += age;
    }
    ret.temporal_locality = 1.0 - (double)(sum_age / ret.vertices) / (double)cache_size;
    ret.unique_vertices = seen.size();
    ret.min_value = *seen.begin();
    ret.max_value = *seen.rbegin();
    ret.avg_sparseness = 0;
    T prev = *seen.begin();
    int shaded_verts = 4;
    for (const T v : seen)
    {
        if (v - (prev / 4) * 4 >= 4)
        {
            shaded_verts += 4;
        }
        T distance = std::max<T>(1, v - prev);
        ret.max_sparseness = std::max<T>(distance, ret.max_sparseness);
        ret.avg_sparseness += distance;
        prev = v;
    }
    ret.vec4_locality = (double)seen.size() / (double)shaded_verts;
    ret.avg_sparseness /= seen.size();
    ret.spatial_locality = 1.0 / ret.avg_sparseness;
}

DrawParams ParseInterfaceRetracing::getDrawCallCount(common::CallTM *call)
{
    DrawParams ret = ParseInterfaceBase::getDrawCallCount(call);

    if (call->mCallName == "glDrawElementsIndirect")
    {
        IndirectDrawElements params;
        GLvoid *bufptr = reinterpret_cast<GLvoid*>(call->mArgs[2]->GetAsUInt64());
        if (getIndirectBuffer(&params, sizeof(params), bufptr))
        {
            ret.vertices = params.count;
            ret.instances = params.instanceCount;
        }
    }
    else if (call->mCallName == "glDrawArraysIndirect")
    {
        IndirectDrawArrays params;
        GLvoid *bufptr = reinterpret_cast<GLvoid*>(call->mArgs[1]->GetAsUInt64());
        if (getIndirectBuffer(&params, sizeof(params), bufptr))
        {
            ret.vertices = params.count;
            ret.instances = params.primCount;
        }
    }

    if (ret.vertices == 0) // which is fully legal
    {
        return ret;
    }

    ret.primitives = calculate_primitives(ret.mode, ret.vertices, contexts[context_index].patchSize);

    // Peek at indices
    ret.unique_vertices = ret.vertices; // for non-indexed case
    if (call->mCallName.find("Elements") != std::string::npos && !mQuickMode)
    {
        GLvoid *indices = drawCallIndexPtr(call);
        ret.value_type = drawCallIndexType(call);
        const unsigned bufferSize = _gl_type_size(ret.value_type, ret.vertices);
        GLint bufferId = 0;
        GLint oldCopybufferId = 0;
        _glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &bufferId);
        _glGetIntegerv(GL_COPY_READ_BUFFER_BINDING, &oldCopybufferId);
        const char *ptr;
        if (bufferId == 0) // buffer is in client memory
        {
            ptr = reinterpret_cast<const char *>(indices);
            indices = NULL;
            if (!ptr) // this seems very very broken, but does happen in some traces
            {
                return ret;
            }
        }
        else
        {
            GLint bufSize = 0;
            _glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufSize);
            if (bufferSize > (unsigned)bufSize)
            {
                DBG_LOG("Buffer too small: Calculated to %ld, was %ld\n", (long)bufferSize, (long)bufSize);
                assert(false);
                return ret;
            }
            _glBindBuffer(GL_COPY_READ_BUFFER, bufferId);
            ptr = (const char *)_glMapBufferRange(GL_COPY_READ_BUFFER, 0, bufferSize, GL_MAP_READ_BIT);
            if (!ptr)
            {
                DBG_LOG("Failed to bind buffer %d for draw call analysis at call number %ld\n", bufferId, (long)call->mCallNo);
                return ret;
            }
        }
        switch (ret.value_type)
        {
        case GL_UNSIGNED_BYTE:
            analyzeIndexBuffer<GLubyte>(ptr, reinterpret_cast<intptr_t>(indices), ret);
            break;
        case GL_UNSIGNED_SHORT:
            analyzeIndexBuffer<GLushort>(ptr, reinterpret_cast<intptr_t>(indices), ret);
            break;
        case GL_UNSIGNED_INT:
            analyzeIndexBuffer<GLuint>(ptr, reinterpret_cast<intptr_t>(indices), ret);
            break;
        default:
            DBG_LOG("Unknown index value type: %04x\n", (unsigned)ret.value_type);
            break;
        }
        if (bufferId != 0)
        {
            _glUnmapBuffer(GL_COPY_READ_BUFFER);
            _glBindBuffer(GL_COPY_READ_BUFFER, oldCopybufferId);
        }
    }

    if (ret.instances > 0)
    {
        ret.vertices *= ret.instances;
        ret.unique_vertices *= ret.instances;
        ret.primitives *= ret.instances;
    }

    return ret;
}

bool ParseInterfaceRetracing::open(const std::string& input, const std::string& output)
{
    filename = input;
    common::gApiInfo.RegisterEntries(gles_callbacks);
    common::gApiInfo.RegisterEntries(egl_callbacks);
    gRetracer.mOptions.mPbufferRendering = !mDisplayMode;
    if (!gRetracer.OpenTraceFile(input.c_str()))
    {
        DBG_LOG("Failed to open %s\n", input.c_str());
        return 1;
    }
    GLWS::instance().Init(gRetracer.mOptions.mApiVersion);
    header = gRetracer.mFile.getJSONHeader();
    threadArray = header["threads"];
    defaultTid = header["defaultTid"].asUInt();
    highest_gles_version = header["glesVersion"].asInt() * 10;
    numThreads = threadArray.size();
    if (numThreads == 0)
    {
        DBG_LOG("Bad number of threads: %d\n", numThreads);
        return false;
    }
    if (!perf_init())
    {
        DBG_LOG("Could not initialize perf subsystem\n");
    }
    return true;
}

void ParseInterfaceRetracing::close()
{
    GLWS::instance().Cleanup();
}

common::CallTM* ParseInterfaceRetracing::next_call()
{
    void *fptr = NULL;
    char *src = NULL;
    // Find first call on active thread
    do
    {
        if (!gRetracer.mFile.GetNextCall(fptr, gRetracer.mCurCall, src) || gRetracer.mFinish)
        {
            GLWS::instance().Cleanup();
            gRetracer.CloseTraceFile();
            return NULL;
        }
        gRetracer.IncCurCallId();
    } while (gRetracer.getCurTid() != gRetracer.mOptions.mRetraceTid);
    // Interpret function
    delete mCall;
    mCall = new common::CallTM(gRetracer.mFile, gRetracer.GetCurCallId(), gRetracer.mCurCall);
    interpret_call(mCall);
    if (mDumpFramebuffers && (mCall->mCallName == "glBindFramebuffer" || mCall->mCallName == "glBindFramebufferOES"
                              || mCall->mCallName.compare(0, 14, "eglSwapBuffers") == 0))
    {
        if (contexts[context_index].render_passes.size() > 0 && mScreenshots)
        {
            StateTracker::RenderPass &rp = contexts[context_index].render_passes.back();
            if (!rp.snapshot_filename.empty())
            {
                DBG_LOG("Making duplicate snapshots for FBOs!\n");
            }
            rp.snapshot_filename = "f" + std::to_string(gRetracer.GetCurFrameId()) + "_c" + std::to_string(gRetracer.GetCurCallId()) + ".png";
            gRetracer.TakeSnapshot(gRetracer.GetCurCallId(), gRetracer.GetCurFrameId(), rp.snapshot_filename.c_str());
        }
    }
    // Call function
    mCpuCycles = 0;
    if (fptr)
    {
        perf_start();
        (*(RetraceFunc)fptr)(src); // increments src to point to end of parameter block
        mCpuCycles = perf_stop();
    }
    // Additional special handling
    if (mCall->mCallName == "glCompileShader")
    {
        GLuint shader = mCall->mArgs[0]->GetAsUInt();
        int target_shader_index = contexts[context_index].shaders.remap(shader);
        if (contexts[context_index].shaders.all().size() > (unsigned)target_shader_index) // this is for Egypt...
        {
            GLint compiled = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
            contexts[context_index].shaders[target_shader_index].compile_status = compiled;
        }
    }
    else if (mCall->mCallName == "glLinkProgram")
    {
        GLuint program = mCall->mArgs[0]->GetAsUInt();
        int target_program_index = contexts[context_index].programs.remap(program);
        GLint linked = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        contexts[context_index].programs[target_program_index].link_status = linked;
    }
    // Return function
    return mCall;
}
