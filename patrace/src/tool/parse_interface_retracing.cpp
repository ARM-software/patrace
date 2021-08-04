#include "tool/parse_interface_retracing.hpp"

#include "common/gl_utility.hpp"
#include "common/memory.hpp"
#include "helper/eglsize.hpp"
#include "tool/utils.hpp"
#include "tool/glsl_parser.h"
#include "helper/eglstring.hpp"
#include "tool/glsl_utils.h"

#include <sys/stat.h>
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

#pragma GCC diagnostic ignored "-Wunused-variable"

using namespace retracer;

const int cache_size = 512;
static int mPerfFD = -1;
static bool perf_initialized = false;

void validate_value(Json::Value &val)
{
    if (val.isDouble())
    {
        if (std::isinf(val.asDouble()))
        {
            gRetracer.reportAndAbort("Inf value detected in renderpass json file.");
        }
        else if (std::isnan(val.asDouble()))
        {
            gRetracer.reportAndAbort("NaN value detected in renderpass json file.");
        }
    }
}

void validate_json(Json::Value &root)
{
    if (root.size() > 0) {
        for (auto it = root.begin(); it != root.end(); ++it) {
            validate_json(*it);
        }
    } else {
        validate_value(root);
    }
}

static std::string write_or_reuse(const std::string& dirname, const std::string& filename, const std::string& blob, cache_type& cache)
{
    const std::string md5 = common::MD5Digest(blob).text();
    if (cache.count(md5) > 0)
    {
        return cache.at(md5);
    }
    else
    {
        FILE *fp = fopen(std::string(dirname + "/" + filename).c_str(), "w");
        fwrite(blob.c_str(), blob.size(), 1, fp);
        fclose(fp);
        cache[md5] = filename;
        return filename;
    }
}

static std::string shader_filename(const StateTracker::Shader &shader, int context_index, int program_index)
{
    return "shader_" + std::to_string(shader.id) + "_p" + std::to_string(program_index) + "_c" + std::to_string(context_index) + shader_extension(shader.shader_type);
}

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
    _glGetBooleanv(GL_PRIMITIVE_RESTART_FIXED_INDEX, &primitive_restart);
    std::map<T, long> cache;
    long timestamp = 0;
    const unsigned stride = (unsigned)sizeof(T);
    const T* buffer = (const T*)ptr;
    offset /= stride;
    std::set<T> seen;
    int sum_age = 0;
    for (int idx = offset; idx < ret.count + offset; idx++)
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

    if (call->mCallName == "glDrawElements" || call->mCallName == "glDrawElementsInstanced" || call->mCallName == "glDrawElementsBaseVertex"
        || call->mCallName == "glDrawElementsInstancedBaseVertex")
    {
        ret.vertices = call->mArgs[1]->GetAsUInt();
        if (call->mArgs[3]->mOpaqueType == common::ClientSideBufferObjectReferenceType)
        {
            ret.index_buffer = (void*)gRetracer.mCSBuffers.translate_address(gRetracer.getCurTid(), call->mArgs[3]->mOpaqueIns->mClientSideBufferName,
                                                                             (ptrdiff_t)call->mArgs[3]->mOpaqueIns->mClientSideBufferOffset);
        }
        else
        {
            ret.index_buffer = (void*)call->mArgs[3]->GetAsUInt64();
        }
        ret.value_type = call->mArgs[2]->GetAsUInt();
    }
    else if (call->mCallName == "glDrawRangeElements" || call->mCallName == "glDrawRangeElementsBaseVertex")
    {
        if (call->mArgs[5]->mOpaqueType == common::ClientSideBufferObjectReferenceType)
        {
            ret.index_buffer = (void*)gRetracer.mCSBuffers.translate_address(gRetracer.getCurTid(), call->mArgs[5]->mOpaqueIns->mClientSideBufferName,
                                                                             (ptrdiff_t)call->mArgs[5]->mOpaqueIns->mClientSideBufferOffset);
        }
        else
        {
             ret.index_buffer = (void*)call->mArgs[5]->GetAsUInt64();
        }
    }
    else if (call->mCallName == "glDrawElementsIndirect")
    {
        IndirectDrawElements params = {};
        GLvoid *bufptr = reinterpret_cast<GLvoid*>(call->mArgs[2]->GetAsUInt64());
        if (getIndirectBuffer(&params, sizeof(params), bufptr))
        {
            ret.count = ret.vertices = params.count;
            ret.instances = params.instanceCount;
            ret.first_index = params.firstIndex;
            ret.base_vertex = params.baseVertex;
        }
        if (ret.instances < 0 || ret.first_index < 0 || ret.count < 0) gRetracer.reportAndAbort("Bad indirect buffer");
        ret.value_type = call->mArgs[1]->GetAsUInt();
    }
    else if (call->mCallName == "glDrawArraysIndirect")
    {
        IndirectDrawArrays params = {};
        GLvoid *bufptr = reinterpret_cast<GLvoid*>(call->mArgs[1]->GetAsUInt64());
        if (getIndirectBuffer(&params, sizeof(params), bufptr))
        {
            ret.count = ret.vertices = params.count;
            ret.instances = params.primCount;
            ret.first_index = params.first;
        }
        if (ret.instances < 0 || ret.first_index < 0 || ret.count < 0) gRetracer.reportAndAbort("Bad indirect buffer");
    }
    assert(ret.instances >= 0);

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
                DBG_LOG("Failed to bind buffer %d for draw call analysis at call number %ld: 0x%04x\n", bufferId, (long)call->mCallNo, (unsigned)_glGetError());
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
        return false;
    }
    if (gRetracer.mOptions.mPbufferRendering)
    {
        gRetracer.mOptions.mOnscreenConfig = EglConfigInfo(8, 8, 8, 0, 0, 0, 0, 0);
    }
    GLWS::instance().Init(gRetracer.mOptions.mApiVersion);
    header = gRetracer.mFile.getJSONHeader();
    threadArray = header["threads"];
    defaultTid = header["defaultTid"].asUInt();
    gRetracer.mOptions.mDebug = mDebug;
    if (mForceMultithread)
    {
        gRetracer.mOptions.mMultiThread = true;
        only_default = false;
    }
    else
    {
        gRetracer.mOptions.mMultiThread = header.get("multiThread", false).asBool();
        only_default = header.get("multiThread", false).asBool();
    }
    DBG_LOG("Multi-threading is %s, selected thread is %d\n", gRetracer.mOptions.mMultiThread ? "ON" : "OFF", (int)defaultTid);
    highest_gles_version = header["glesVersion"].asInt() * 10;
    DBG_LOG("Initial GLES version set to %d based on JSON header\n", highest_gles_version);
    numThreads = threadArray.size();
    if (numThreads == 0)
    {
        DBG_LOG("Bad number of threads: %d\n", numThreads);
        return false;
    }
    const Json::Value eglconfig = gRetracer.mFile.getJSONThreadById(defaultTid)["EGLConfig"];
    jsonConfig.red = eglconfig["red"].asInt();
    jsonConfig.green = eglconfig["green"].asInt();
    jsonConfig.blue = eglconfig["blue"].asInt();
    jsonConfig.alpha = eglconfig["alpha"].asInt();
    jsonConfig.depth = eglconfig["depth"].asInt();
    jsonConfig.stencil = eglconfig["stencil"].asInt();
    jsonConfig.samples = eglconfig.get("msaaSamples", -1).asInt();
    if (jsonConfig.red <= 0) DBG_LOG("Zero red bits! This trace likely has a bad header!\n");
    if (!perf_init()) DBG_LOG("Could not initialize perf subsystem\n");
    return true;
}

void ParseInterfaceRetracing::close()
{
    GLWS::instance().Cleanup();
}

common::CallTM* ParseInterfaceRetracing::next_call()
{
    // Get function
    delete mCall;
    mCall = new common::CallTM(gRetracer.mFile, gRetracer.GetCurCallId(), gRetracer.mCurCall);
    // Verification checks (check that previous state is correct before overwriting)
    if (mCall->mCallName == "glEnable" || mCall->mCallName == "glDisable")
    {
        const GLenum target = mCall->mArgs[0]->GetAsInt();
        if (context_index != UNBOUND)
        {
            const bool value = _glIsEnabled(target);
            // Need to hack a check for GL_TEXTURE_* because some content set these for GLES2+ contexts even though it is invalid.
            // 0x0C53 is GL_POLYGON_SMOOTH_HINT which is to get TiatianFeiche working, and yes, it is not a valid parameter here.
            assert(contexts[context_index].enabled.count(target) == 0 || value == contexts[context_index].enabled.at(target)
                   || target == GL_TEXTURE_2D || target == GL_TEXTURE_3D || target == 0x0C53);
        }
    }
    else if (mDumpFramebuffers && (mCall->mCallName == "glBindFramebuffer" || mCall->mCallName == "glBindFramebufferOES"
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
            if (mRenderpassJSON) rp.snapshot_filename = mRenderpass.dirname + "/after.png";
            gRetracer.TakeSnapshot(gRetracer.GetCurCallId(), gRetracer.GetCurFrameId(), rp.snapshot_filename.c_str());
        }
    }
    // Interpret function
    interpret_call(mCall);

    // Perform transform feedback
    if (!mQuickMode && mCall->mCallName.compare(0, 6, "glDraw") == 0 && mCall->mCallName != "glDrawBuffers" && contexts[context_index].transform_feedback_binding == 0
        && (contexts[context_index].program_index != UNBOUND || contexts[context_index].program_pipeline_index != UNBOUND))
    {
        if (mDebug) DBG_LOG("Capturing geometry for %s\n", mCall->mCallName.c_str());
        StateTracker::VertexArrayObject& vao = contexts[context_index].vaos.at(contexts[context_index].vao_index);
        int program_index = 0;
        if (contexts[context_index].program_index != UNBOUND)
        {
            program_index = contexts[context_index].program_index;
        }
        else
        {
            program_index = contexts[context_index].program_pipelines.at(contexts[context_index].program_pipeline_index).program_stages.at(GL_VERTEX_SHADER);
            gRetracer.reportAndAbort("draw call TF from separate program");
        }

        const DrawParams params = getDrawCallCount(mCall);
        const StateTracker::Program& program = contexts[context_index].programs.at(program_index);
        const StateTracker::Shader& vshader = contexts[context_index].shaders.at(program.shaders.at(GL_VERTEX_SHADER));

        std::unordered_map<std::string, int> varying_sizes_bytes;
        std::unordered_map<std::string, int> varying_offsets_bytes;
        int vertex_size_in_bytes = 0;
        int offset = 0;

        std::map<std::string, StateTracker::GLSLVarying> varyings;
        for (auto& var : vshader.varyings) {
            varyings[var.first] = var.second;
        }

        for (auto& var : varyings) {
            GLenum elem_type;
            GLint num_elems;
            _gl_uniform_size(var.second.type, elem_type, num_elems);
            int var_size = num_elems * _gl_type_size(elem_type);
            varying_sizes_bytes[var.first] = var_size;
            varying_offsets_bytes[var.first] = offset;

            offset += var_size;
            vertex_size_in_bytes += var_size;

            if (mDebug) DBG_LOG("feedback for varying binding=%s, size=%d\n", var.first.c_str(), var_size);
        }

        // Transform feedback draw
        int vertex_size_in_floats = vertex_size_in_bytes / sizeof(GLfloat);
        int out_data_num_floats = vertex_size_in_floats * params.num_vertices_out;
        std::vector<GLfloat> feedback(out_data_num_floats);

        GLint oldCopyBufferId = 0;
        _glGetIntegerv(GL_COPY_READ_BUFFER_BINDING, &oldCopyBufferId);

        GLuint tbo;
        _glGenBuffers(1, &tbo);
        _glBindBuffer(GL_COPY_READ_BUFFER, tbo);
        _glBufferData(GL_COPY_READ_BUFFER, feedback.size() * sizeof(GLfloat), feedback.data(), GL_STATIC_READ);
        _glBindBuffer(GL_COPY_READ_BUFFER, oldCopyBufferId);
        _glEnable(GL_RASTERIZER_DISCARD);
        _glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, tbo);
        _glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, tbo);

        const GLenum orig_mode = mCall->mArgs[0]->GetAsUInt();
        GLenum primitive_mode = GL_TRIANGLES; // default
        switch (orig_mode)
        {
        case GL_POINTS:
            primitive_mode = GL_POINTS;
            break;
        case GL_LINE_STRIP:
        case GL_LINE_LOOP:
        case GL_LINES:
            primitive_mode = GL_LINES;
            break;
        default:
            break;
        }

        _glBeginTransformFeedback(primitive_mode);
        char *src = gRetracer.src; // to avoid incrementing original pointer
        (*(RetraceFunc)gRetracer.fptr)(src);
        _glEndTransformFeedback();
        _glFinish();

        void* tf_mem = _glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, feedback.size() * sizeof(GLfloat), GL_MAP_READ_BIT);
        if (tf_mem) {
            memcpy(feedback.data(), tf_mem, feedback.size() * sizeof(GLfloat));
            _glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
        } else {
            gRetracer.reportAndAbort("Failed to map transform feedback buffer.");
        }

        if (mRenderpassJSON) // save it to file?
        {
            StateTracker::RenderPass &rp = contexts[context_index].render_passes.back();
            for (auto& var : varyings) {
                GLenum elem_type;
                GLint num_elems;
                _gl_uniform_size(var.second.type, elem_type, num_elems);

                Json::Value output;
                output["stride"] = varying_sizes_bytes[var.first];
                output["num_instances"] = 1;
                output["components"] = num_elems;
                output["type"] = "float";
                output["abstract_type"] = var.second.abstract_type;
                output["binding"] = var.first;
                output["precision"] = "mediump";
                if (var.second.precision == StateTracker::HIGHP) {
                    output["precision"] = "highp";
                } else if (var.second.precision == StateTracker::LOWP) {
                    output["precision"] = "lowp";
                }
                output["num_vertices"] = params.num_vertices_out;
                output["filenames"] = Json::Value(Json::arrayValue);

                int var_size_floats = varying_sizes_bytes[var.first] / sizeof(GLfloat);
                int var_offset_in_floats = varying_offsets_bytes[var.first] / sizeof(GLfloat);

                std::vector<GLfloat> varying_data(var_size_floats * params.num_vertices_out);

                for (int i = 0; i < params.num_vertices_out; ++i) {
                    for (int y = 0; y < var_size_floats; ++y) {
                        varying_data[i * var_size_floats + y] = feedback[i * vertex_size_in_floats + var_offset_in_floats + y];
                    }
                }

                int geometry_index = mRenderpass.data["resources"]["geometry"].size() - 1;
                std::string filename = "vertex_output_dc" + std::to_string(geometry_index) + "_g" + std::to_string(geometry_index) + "_" + var.first + ".bin";
                output["filenames"].append(filename);
                std::string filepath = mRenderpass.dirname + "/" + filename;
                FILE *fp = fopen(filepath.c_str(), "w");
                if (fp)
                {
                    size_t written = fwrite(static_cast<void*>(varying_data.data()), varying_data.size() * sizeof(GLfloat), 1, fp);
                    if (written != 1)
                    {
                        gRetracer.reportAndAbort("Failed to write out feedback data! wrote = %d", (int)written);
                    }
                    fclose(fp);
                } else {
                    perror("Failed to open file");
                    gRetracer.reportAndAbort("Failed to open file");
                }

                mRenderpass.data["resources"]["geometry"][geometry_index]["outputs"].append(output);
            }
        }

        // Restore state
        _glDisable(GL_RASTERIZER_DISCARD);
        _glDeleteBuffers(1, &tbo);
    }

    // Enable transform feedback in the shader
    if (mCall->mCallName == "glLinkProgram" && mRenderpassJSON)
    {
        const GLuint id = mCall->mArgs[0]->GetAsUInt();
        const int target_program_index = contexts[context_index].programs.remap(id);
        StateTracker::Program& p = contexts[context_index].programs[target_program_index];
        if (p.shaders.count(GL_COMPUTE_SHADER) == 0)
        {
            const StateTracker::Shader& vshader = contexts[context_index].shaders.at(p.shaders.at(GL_VERTEX_SHADER));
            const StateTracker::Shader& fshader = contexts[context_index].shaders.at(p.shaders.at(GL_FRAGMENT_SHADER));

            // Need to read this before we start to mess with it
            _glGetProgramInterfaceiv(id, GL_TRANSFORM_FEEDBACK_VARYING, GL_ACTIVE_RESOURCES, &p.activeTransformFeedbackVaryings);

            std::map<std::string, StateTracker::GLSLVarying> varyings;
            for (auto& var : vshader.varyings)
            {
                varyings[var.first] = var.second;
            }

            std::vector<GLchar*> feedbackVaryings;
            for (auto& var : varyings)
            {
                feedbackVaryings.push_back((GLchar*)(var.first.c_str()));
                if (mDebug) DBG_LOG("Enabling varying: %s\n", (GLchar*)(var.first.c_str()));
            }

            _glTransformFeedbackVaryings(id, feedbackVaryings.size(), feedbackVaryings.data(), GL_INTERLEAVED_ATTRIBS);
        }
    }

    // Call function
    mCpuCycles = 0;
    if (gRetracer.fptr)
    {
        perf_start();
        (*(RetraceFunc)gRetracer.fptr)(gRetracer.src); // increments src to point to end of parameter block
        mCpuCycles = perf_stop();
    }

    // Additional special handling
    if (mCall->mCallName == "glCompileShader")
    {
        GLuint shader = mCall->mArgs[0]->GetAsUInt();
        if (contexts[context_index].shaders.contains(shader))
        {
            int target_shader_index = contexts[context_index].shaders.remap(shader);
            if (contexts[context_index].shaders.all().size() > (unsigned)target_shader_index) // this is for Egypt...
            {
                GLint compiled = 0;
                glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
                contexts[context_index].shaders[target_shader_index].compile_status = compiled;
            }
        }
    }
    else if (mCall->mCallName == "glVertexAttribPointer" || mCall->mCallName == "glVertexAttribIPointer")
    {
        StateTracker::VertexArrayObject& vao = contexts[context_index].vaos.at(contexts[context_index].vao_index);
        const GLuint buffer_id = vao.boundBufferIds[GL_ARRAY_BUFFER][0].buffer;
        if (buffer_id == 0) // user will pass in pointer
        {
            const int buffer_index = contexts[context_index].buffers.remap(buffer_id); // store GL_ARRAY_BUFFER default states in buffer zero
            if (mCall->mCallName == "glVertexAttribIPointer") contexts[context_index].buffers[buffer_index].ptr = getBufferPointer(mCall->mArgs[4]);
            else contexts[context_index].buffers[buffer_index].ptr = getBufferPointer(mCall->mArgs[5]);
        }
        else if (buffer_id != 0 && contexts[context_index].buffers.contains(buffer_id)) // user may pass in offset
        {
            const int buffer_index = contexts[context_index].buffers.remap(buffer_id);
            contexts[context_index].buffers[buffer_index].ptr = contexts[context_index].buffers.at(buffer_index).ptr;
            if (mCall->mCallName == "glVertexAttribIPointer") contexts[context_index].buffers[buffer_index].offset = mCall->mArgs[4]->GetAsUInt64();
            else contexts[context_index].buffers[buffer_index].offset = mCall->mArgs[5]->GetAsUInt64();
        }
    }
    else if (mCall->mCallName == "glLinkProgram")
    {
        const GLuint id = mCall->mArgs[0]->GetAsUInt();
        const int target_program_index = contexts[context_index].programs.remap(id);
        StateTracker::Program& p = contexts[context_index].programs[target_program_index];
        GLint linked = 0;
        _glGetProgramiv(id, GL_LINK_STATUS, &linked);
        p.link_status = linked;
        _glGetProgramiv(id, GL_ACTIVE_ATTRIBUTES, &p.activeAttributes);
        _glGetProgramInterfaceiv(id, GL_UNIFORM, GL_ACTIVE_RESOURCES, &p.activeUniforms);
        _glGetProgramInterfaceiv(id, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &p.activeUniformBlocks);
        _glGetProgramInterfaceiv(id, GL_ATOMIC_COUNTER_BUFFER, GL_ACTIVE_RESOURCES, &p.activeAtomicCounterBuffers);
        _glGetProgramInterfaceiv(id, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &p.activeInputs);
        _glGetProgramInterfaceiv(id, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, &p.activeOutputs);
        _glGetProgramInterfaceiv(id, GL_TRANSFORM_FEEDBACK_BUFFER, GL_ACTIVE_RESOURCES, &p.activeTransformFeedbackBuffers);
        _glGetProgramInterfaceiv(id, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &p.activeSSBOs);
        for (int i = 0; i < p.activeUniforms; ++i)
        {
            GLsizei length = 0;
            GLint size = 0;
            GLenum type = GL_NONE;
            GLchar tmp[128];
            memset(tmp, 0, sizeof(tmp));
            _glGetActiveUniform(id, i, sizeof(tmp) - 1, &length, &size, &type, tmp);
            GLint location = _glGetUniformLocation(id, tmp);
            if (location >= 0) p.uniformNames[location] = tmp;
            for (int j = 0; location >= 0 && j < size; ++j)
            {
                if (isUniformSamplerType(type))
                {
                    GLint param = 0;
                    _glGetUniformiv(id, location + j, &param); // arrays are guaranteed to be in sequential location
                    p.texture_bindings[tmp] = { type, param };
                }
            }
        }
    }
    // Return function
    return mCall;
}

void ParseInterfaceRetracing::completed_renderpass(const StateTracker::RenderPass &rp)
{
    if (!mRenderpassJSON) return;

    // -- Renderpasses --
    assert(rp.width != 0);
    mRenderpass.data["renderpass"]["width"] = rp.width;
    mRenderpass.data["renderpass"]["height"] = rp.height;
    mRenderpass.data["renderpass"]["depth"] = rp.depth;
    for (const auto& rpa : rp.attachments)
    {
        Json::Value v;

        // We need to re-couple the components of combined depth-stencil here
        if (rpa.format == GL_DEPTH24_STENCIL8 && rpa.slot == GL_STENCIL_ATTACHMENT) continue;
        else if (rpa.slot == GL_BACK) v["display"] = true; // is backbuffer / swapchain image
        else if (rpa.slot == GL_NONE) continue; // is unused part of backbuffer

        switch (rpa.load_op)
        {
        case StateTracker::RenderPass::LOAD_OP_LOAD: v["load_op"] = "LOAD"; break;
        case StateTracker::RenderPass::LOAD_OP_CLEAR: v["load_op"] = "CLEAR"; break;
        case StateTracker::RenderPass::LOAD_OP_DONT_CARE: v["load_op"] = "DONT_CARE"; break;
        default: assert(false); break;
        }

        if (rpa.store_op == StateTracker::RenderPass::STORE_OP_DONT_CARE) v["store_op"] = "DONT_CARE";
        v["format"] = texEnum(rpa.format).replace(0, 3, std::string());
        v["type"] = texEnum(rpa.type).replace(0, 3, std::string());

        mRenderpass.data["renderpass"]["config"].append(v);
    }
    if (mScreenshots) mRenderpass.data["renderpass"]["snapshot"] = rp.snapshot_filename;

    // Check for nan or inf values
    validate_json(mRenderpass.data);

    std::fstream fs;
    fs.open(mRenderpass.filename, std::fstream::out |  std::fstream::trunc);
    fs << mRenderpass.data.toStyledString();
    fs.close();

    mRenderpass.started = false;
    mRenderpass.shader_cache.clear();
    mRenderpass.index_cache.clear();
    mRenderpass.vertex_cache.clear();
    mRenderpass.stored_programs.clear();
    mRenderpass.data = Json::Value();
}

static bool my_glGetBool(GLenum target)
{
    GLboolean value;
    _glGetBooleanv(target, &value);
    return value;
}

static GLint my_glGetInt(GLenum target)
{
    GLint value;
    _glGetIntegerv(target, &value);
    return value;
}

static GLfloat my_glGetFloat(GLenum target)
{
    GLfloat value;
    _glGetFloatv(target, &value);
    return value;
}

static std::string my_glGetString(GLenum target)
{
    GLint value;
    _glGetIntegerv(target, &value);
    return texEnum(value).replace(0, 3, std::string());;
}

static Json::Value fillFaceJson(const StateTracker::Context& context, GLenum face)
{
    Json::Value v;
    v["write_mask"] = context.fillstate.stencilwritemask.at(face);
    switch (context.fillstate.stencilfunc.at(face))
    {
    case GL_NEVER: v["function"] = "NEVER"; break;
    case GL_LESS: v["function"] = "LESS"; break;
    case GL_EQUAL: v["function"] = "EQUAL"; break;
    case GL_LEQUAL: v["function"] = "LEQUAL"; break;
    case GL_GREATER: v["function"] = "GREATER"; break;
    case GL_NOTEQUAL: v["function"] = "NOTEQUAL"; break;
    case GL_GEQUAL: v["function"] = "GEQUAL"; break;
    case GL_ALWAYS: v["function"] = "ALWAYS"; break;
    default: assert(false); break;
    }
    v["reference"] = context.fillstate.stencilref.at(face);;
    v["compare_mask"] = context.fillstate.stencilcomparemask.at(face);
    // missing pass_operation, fail_operation and depth_fail_operation - see glStencilOpSeparate
    return v;
}

static Json::Value get_rasterization_state(const StateTracker::Context& context)
{
    Json::Value raster;
    switch (my_glGetInt(GL_CULL_FACE_MODE))
    {
    case GL_BACK: raster["cull_mode"] = "BACK"; break;
    case GL_FRONT: raster["cull_mode"] = "BACK"; break;
    case GL_FRONT_AND_BACK: raster["cull_mode"] = "FRONT_AND_BACK"; break;
    default: assert(false); break;
    }
    if (my_glGetInt(GL_CULL_FACE) == GL_FALSE) raster["cull_mode"] = "NONE";
    raster["winding_order"] = my_glGetString(GL_FRONT_FACE);
    raster["line_width"] = my_glGetInt(GL_LINE_WIDTH);
    raster["dither"] = my_glGetBool(GL_DITHER);
    raster["polygon_offset_factor"] = my_glGetFloat(GL_POLYGON_OFFSET_FACTOR);
    raster["polygon_offset_units"] = my_glGetFloat(GL_POLYGON_OFFSET_UNITS);
    raster["polygon_offset_fill"] = my_glGetBool(GL_POLYGON_OFFSET_FILL);
    raster["discard"] = my_glGetBool(GL_RASTERIZER_DISCARD);
    raster["sample_coverage"] = my_glGetBool(GL_SAMPLE_COVERAGE);
    raster["sample_coverage_value"] = my_glGetFloat(GL_SAMPLE_COVERAGE_VALUE);
    raster["sample_coverage_invert"] = my_glGetBool(GL_SAMPLE_COVERAGE_INVERT);
    raster["sample_alpha_to_coverage"] = my_glGetBool(GL_SAMPLE_ALPHA_TO_COVERAGE);
    return raster;
}

static Json::Value get_scissor_state(const StateTracker::Context& context)
{
    Json::Value scissor;
    GLint box[4];
    glGetIntegerv(GL_SCISSOR_BOX, box);
    scissor["enabled"] = my_glGetBool(GL_SCISSOR_TEST);
    scissor["x"] = box[0];
    scissor["y"] = box[1];
    scissor["width"] = box[2];
    scissor["height"] = box[3];
    return scissor;
}

static Json::Value get_viewport_state(const StateTracker::Context& context)
{
    GLfloat depth[2];
    glGetFloatv(GL_DEPTH_RANGE, depth);
    GLint box[4];
    glGetIntegerv(GL_VIEWPORT, box);
    Json::Value viewport;
    viewport["x"] = box[0]; assert(box[0] == context.viewport.x);
    viewport["y"] = box[1]; assert(box[1] == context.viewport.y);
    viewport["width"] = box[2]; assert(box[2] == context.viewport.width);
    viewport["height"] = box[3]; assert(box[3] == context.viewport.height);
    viewport["min_depth"] = depth[0]; assert(depth[0] == context.viewport.near);
    viewport["max_depth"] = depth[1]; assert(depth[1] == context.viewport.far);
    return viewport;
}

static Json::Value get_blend_state(const StateTracker::Context& context)
{
    Json::Value blend;
    blend["enabled"] = (bool)_glIsEnabled(GL_BLEND);
    blend["alpha_blend"] = Json::Value();
    blend["color_blend"] = Json::Value();
    blend["alpha_blend"]["destination"] = blendEnum(context.fillstate.blend_alpha.destination).replace(0, 3, std::string());
    blend["alpha_blend"]["source"] = blendEnum(context.fillstate.blend_alpha.source).replace(0, 3, std::string());
    blend["alpha_blend"]["operation"] = blendEnum(context.fillstate.blend_alpha.operation).replace(0, 3, std::string());
    blend["color_blend"]["destination"] = blendEnum(context.fillstate.blend_rgb.destination).replace(0, 3, std::string());
    blend["color_blend"]["source"] = blendEnum(context.fillstate.blend_rgb.source).replace(0, 3, std::string());
    blend["color_blend"]["operation"] = blendEnum(context.fillstate.blend_rgb.operation).replace(0, 3, std::string());
    blend["blend_factor"] = Json::arrayValue;
    for (const GLfloat f : context.fillstate.blendFactor) blend["blend_factor"].append(f);
    // TBD the rest...
    return blend;
}

static Json::Value get_depth_stencil_state(const StateTracker::Context& context)
{
    Json::Value depthstencil;
    depthstencil["back_face"] = fillFaceJson(context, GL_BACK);
    depthstencil["front_face"] = fillFaceJson(context, GL_FRONT);
    depthstencil["depth_enable"] = (bool)_glIsEnabled(GL_DEPTH_TEST);
    const int depthfunc = my_glGetInt(GL_DEPTH_FUNC);
    switch (depthfunc)
    {
    case GL_NEVER: depthstencil["depth_function"] = "NEVER"; break;
    case GL_LESS: depthstencil["depth_function"] = "LESS"; break;
    case GL_EQUAL: depthstencil["depth_function"] = "EQUAL"; break;
    case GL_LEQUAL: depthstencil["depth_function"] = "LEQUAL"; break;
    case GL_GREATER: depthstencil["depth_function"] = "GREATER"; break;
    case GL_NOTEQUAL: depthstencil["depth_function"] = "NOTEQUAL"; break;
    case GL_GEQUAL: depthstencil["depth_function"] = "GEQUAL"; break;
    case GL_ALWAYS: depthstencil["depth_function"] = "ALWAYS"; break;
    default : assert(false); break;
    }
    depthstencil["depth_writes"] = my_glGetBool(GL_DEPTH_WRITEMASK);
    depthstencil["stencil_enable"] = (bool)_glIsEnabled(GL_STENCIL_TEST);
    return depthstencil;
}

static int json_reuse_or_add(Json::Value& data, const std::string& key, const Json::Value& newval)
{
    const int size = data[key].size();
    int idx = size;
    int i = 0;
    for (const auto& state : data[key]) // compare and try to reuse
    {
        if (state.compare(newval) == 0) { idx = i; break; }
        i++;
    }
    if (idx == size) data[key].append(newval);
    return idx;
}

static Json::Value write_index_buffer(const std::string& dirname, const std::string& filename, int geomidx, const DrawParams& params, const common::CallTM* call, cache_type& index_cache)
{
    Json::Value geometry;
    int width = 1;
    geometry["offset"] = 0;
    switch (params.value_type)
    {
    case GL_UNSIGNED_BYTE: width = 1; break;
    case GL_UNSIGNED_SHORT: width = 2; break;
    case GL_UNSIGNED_INT: width = 4; break;
    default: assert(false); break;
    }
    geometry["index_byte_width"] = width;
    GLvoid *indices = drawCallIndexPtr(call);
    const unsigned bufferSize = _gl_type_size(params.value_type, params.count);
    geometry["bytes"] = bufferSize;
    GLint bufferId = 0;
    GLint oldCopybufferId = 0;
    _glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &bufferId);
    _glGetIntegerv(GL_COPY_READ_BUFFER_BINDING, &oldCopybufferId);
    const char *ptr = nullptr;
    if (bufferId == 0) // buffer is in client memory
    {
        ptr = reinterpret_cast<const char *>(indices);
        indices = NULL;
    }
    else
    {
        GLint bufSize = 0;
        _glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufSize);
        if (bufferSize > (unsigned)bufSize)
        {
            DBG_LOG("Buffer too small: Calculated to %ld, was %ld\n", (long)bufferSize, (long)bufSize);
            assert(false);
            return Json::Value();
        }
        _glBindBuffer(GL_COPY_READ_BUFFER, bufferId);
        ptr = (const char *)_glMapBufferRange(GL_COPY_READ_BUFFER, reinterpret_cast<intptr_t>(indices), bufferSize, GL_MAP_READ_BIT);
        if (!ptr)
        {
            DBG_LOG("Failed to bind buffer %d for draw call analysis at call number %ld: 0x%04x\n", bufferId, (long)call->mCallNo, (unsigned)_glGetError());
            return Json::Value();
        }
    }
    geometry["filename"] = write_or_reuse(dirname, filename, std::string(ptr, bufferSize), index_cache);
    if (bufferId != 0)
    {
        _glUnmapBuffer(GL_COPY_READ_BUFFER);
        _glBindBuffer(GL_COPY_READ_BUFFER, oldCopybufferId);
    }
    return geometry;
}

static std::string gl_to_json_type(GLenum type)
{
    switch(type)
    {
    case GL_BYTE: return "INT8";
    case GL_UNSIGNED_BYTE: return "UINT8";
    case GL_SHORT: return "INT16";
    case GL_UNSIGNED_SHORT: return "UINT16";
    case GL_INT: return "INT32";
    case GL_UNSIGNED_INT: return "UINT32";
    case GL_HALF_FLOAT: return "HALF";
    case GL_FLOAT: return "FLOAT";
    case GL_FIXED: return "FIXED";
    case GL_INT_2_10_10_10_REV: return "INT_2_10_10_10_REV";
    case GL_UNSIGNED_INT_2_10_10_10_REV: return "UNSIGNED_INT_2_10_10_10_REV";
    default: return "UNKNOWN"; abort(); break;
    }
}

static Json::Value get_vertex_attributes(GLuint program, const std::string& dirname, const std::string& filename, int geomidx, const DrawParams& params, GLint index, cache_type& vertex_cache)
{
    GLsizei length = 0;
    GLint size2 = 0;
    GLenum type2 = GL_NONE;
    GLchar tmp[128];
    memset(tmp, 0, sizeof(tmp));
    _glGetActiveAttrib(program, index, sizeof(tmp) - 1, &length, &size2, &type2, tmp);

    GLint binding = 0;
    GLint components = 0;
    GLint type = 0;
    GLint normalized = 0;
    GLint stride = 0;
    GLvoid *pointer = nullptr;
    GLint divisor = 0;
    Json::Value v;

    _glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &binding);
    _glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_SIZE, &components);
    _glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_TYPE, &type);
    _glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &normalized);
    _glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &stride);
    _glGetVertexAttribPointerv(index, GL_VERTEX_ATTRIB_ARRAY_POINTER, &pointer);
    _glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_DIVISOR, &divisor);
    GLint size;
    if (divisor) size = _instanced_index_array_size(components, type, stride, params.instances, divisor); // instanced
    else if (params.value_type != GL_NONE) size = _glVertexAttribPointer_size(components, type, normalized, stride, params.max_value + params.base_vertex + 1); // indexed
    else size = _glVertexAttribPointer_size(components, type, normalized, stride, params.count); // non-indexed
    assert(size > 0);

    char *ptr = nullptr;
    GLint oldCopybufferId = 0;
    if (binding != 0) // buffer
    {
        _glGetIntegerv(GL_COPY_READ_BUFFER_BINDING, &oldCopybufferId);
        GLint bufSize = 0;
        _glBindBuffer(GL_COPY_READ_BUFFER, binding);
        _glGetBufferParameteriv(GL_COPY_READ_BUFFER, GL_BUFFER_SIZE, &bufSize);
        if (bufSize >= size + reinterpret_cast<intptr_t>(pointer))
        {
            ptr = (char *)_glMapBufferRange(GL_COPY_READ_BUFFER, reinterpret_cast<intptr_t>(pointer), size, GL_MAP_READ_BIT);
            if (!ptr) DBG_LOG("Failed to map buffer %d glMapBufferRange(GL_COPY_READ_BUFFER, %lu, %lu, GL_MAP_READ_BIT), err=0x%04x\n", (int)binding,
                              (unsigned long)pointer, (unsigned long)size, (unsigned)_glGetError());
        }
    }
    else // client side buffer
    {
        ptr = (char*)pointer;
    }

    if (ptr && size)
    {
        v["filename"] = write_or_reuse(dirname, filename, std::string(ptr, size), vertex_cache);
    }
    else v["broken"] = true;

    if (binding != 0) // buffer
    {
        _glUnmapBuffer(GL_COPY_READ_BUFFER);
        _glBindBuffer(GL_COPY_READ_BUFFER, oldCopybufferId);
    }

    v["normalized"] = (bool)normalized;
    v["binding"] = tmp;
    v["stride"] = (unsigned)stride;
    v["slot"] = index;
    v["components"] = (unsigned)components;
    v["type"] = gl_to_json_type(type);
    v["max_index_value"] = params.max_value;
    v["base_vertex"] = params.base_vertex;
    v["size"] = size;
    return v;
}

static void add_shader(Json::Value& v, GLenum type, const std::string& key, const StateTracker::Context& context, const StateTracker::Program& program, const std::string& dirname, int context_index, int rp_index, cache_type& cache)
{
    if (program.shaders.count(type) > 0)
    {
        const StateTracker::Shader& shader = context.shaders.at(program.shaders.at(type));
        const std::string filename = shader_filename(shader, context_index, rp_index);
        v[key] = write_or_reuse(dirname, filename, shader.source_code, cache);
        v[key + "_preprocessed"] = write_or_reuse(dirname, "preprocessed_" + filename, shader.source_preprocessed, cache);
        if (shader.contains_invariants) v["contains_invariants"] = true;
        v["varying_count"] = shader.varying_locations_used;
    }
}

void ParseInterfaceRetracing::completed_drawcall(int frame, const DrawParams& params, const StateTracker::RenderPass &rp)
{
    if (!mRenderpassJSON) return;

    if (!mRenderpass.started)
    {
        // Clear old data
        mRenderpass.data = Json::Value();
        mRenderpass.stored_programs.clear();

        // Initialize values
        Json::Value content;
        content["content_name"] = mOutputName;
        content["content_source"] = filename;
        content["content_frame"] = frame;
        content["content_api_version"] = floor(((float)highest_gles_version / 10.0f) * 10.f) / 10.f;
        content["source_api"] = "GLES";
        content["generation"] = Json::arrayValue;
        Json::Value generated;
        generated["tool"] = "analyze_trace";
        generated["user"] = getUserName();
        generated["time"] = getTimeStamp();
        generated["gpu"] = std::string((char*)_glGetString(GL_RENDERER));
        content["generation"].append(generated);
        mRenderpass.data["content_info"] = content;
        mRenderpass.data["renderdump_version"] = 0.01;
        Json::Value renderpass;
        renderpass["commands"] = Json::arrayValue;
        renderpass["config"] = Json::arrayValue;
        mRenderpass.data["renderpass"] = renderpass;
        Json::Value statevectors;
        statevectors["blend_states"] = Json::arrayValue;
        statevectors["depthstencil_states"] = Json::arrayValue;
        statevectors["rasterization_states"] = Json::arrayValue;
        statevectors["scissor_states"] = Json::arrayValue;
        statevectors["viewport_states"] = Json::arrayValue;
        mRenderpass.data["statevectors"] = statevectors;
        Json::Value resources;
        resources["geometry"] = Json::arrayValue;
        resources["programs"] = Json::arrayValue;
        mRenderpass.data["resources"] = resources;
        mRenderpass.dirname = mOutputName + "_f" + std::to_string(frame) + "_rp" + std::to_string(rp.index);
        mRenderpass.filename = mRenderpass.dirname + "/renderpass.json";
        mRenderpass.started = true;
        int result = mkdir(mRenderpass.dirname.c_str(), 0777);
        if (result != 0 && errno != EEXIST)
        {
            DBG_LOG("Failed to create directory \"%s\": %s\n", mRenderpass.dirname.c_str(), strerror(errno));
            abort();
        }
    }

    Json::Value command;

    // -- Program --
    const int program_index = contexts[context_index].program_index;
    int programidx = 0;
    if (mRenderpass.stored_programs.count(program_index) == 0)
    {
        const StateTracker::Program& program = contexts[context_index].programs.at(program_index);
        mRenderpass.stored_programs.insert(program_index);
        programidx = mRenderpass.data["resources"]["programs"].size();
        Json::Value s;
        add_shader(s, GL_VERTEX_SHADER, "vertex_glsl", contexts[context_index], program, mRenderpass.dirname, context_index, rp.index, mRenderpass.shader_cache);
        add_shader(s, GL_FRAGMENT_SHADER, "fragment_glsl", contexts[context_index], program, mRenderpass.dirname, context_index, rp.index, mRenderpass.shader_cache);
        add_shader(s, GL_GEOMETRY_SHADER, "geometry_glsl", contexts[context_index], program, mRenderpass.dirname, context_index, rp.index, mRenderpass.shader_cache);
        add_shader(s, GL_TESS_CONTROL_SHADER, "tess_control_glsl", contexts[context_index], program, mRenderpass.dirname, context_index, rp.index, mRenderpass.shader_cache);
        add_shader(s, GL_TESS_EVALUATION_SHADER, "tess_evaluation_glsl", contexts[context_index], program, mRenderpass.dirname, context_index, rp.index, mRenderpass.shader_cache);
        Json::Value p;
        p["shaders"] = s;
        mRenderpass.data["resources"]["programs"].append(p);
    }
    command["program"] = programidx;

    // -- Reusable states --
    command["blend_state"] = json_reuse_or_add(mRenderpass.data["statevectors"], "blend_states", get_blend_state(contexts[context_index]));
    command["depthstencil_state"] = json_reuse_or_add(mRenderpass.data["statevectors"], "depthstencil_states", get_depth_stencil_state(contexts[context_index]));
    command["rasterization_state"] = json_reuse_or_add(mRenderpass.data["statevectors"], "rasterization_states", get_rasterization_state(contexts[context_index]));
    command["viewport_state"] = json_reuse_or_add(mRenderpass.data["statevectors"], "viewport_states", get_viewport_state(contexts[context_index]));
    command["scissor_state"] = json_reuse_or_add(mRenderpass.data["statevectors"], "scissor_states", get_scissor_state(contexts[context_index]));

    // -- Draw parameters --
    command["draw_params"] = Json::Value();
    command["draw_params"]["element_count"] = params.vertices;
    if (params.first_index) command["draw_params"]["first_element"] = params.first_index;
    if (params.first_instance) command["draw_params"]["first_instance"] = params.first_instance;
    if (params.instances) command["draw_params"]["instance_count"] = params.instances;
    if (params.base_vertex) command["draw_params"]["base_vertex"] = params.base_vertex;
    if (contexts[context_index].enabled.count(GL_PRIMITIVE_RESTART_FIXED_INDEX) > 0 && contexts[context_index].enabled.at(GL_PRIMITIVE_RESTART_FIXED_INDEX))
    {
        command["draw_params"]["primitive_restart"] = true;
    }
    if (params.count == 0) return; // this actually happens...
    const GLuint tf_id = contexts[context_index].transform_feedback_binding;
    const int tf_index = contexts[context_index].transform_feedbacks.remap(tf_id);
    if (contexts[context_index].transform_feedbacks[tf_index].active)
    {
        command["draw_params"]["transform_feedback"] = true;
    }

    // -- Geometry --
    const int geomidx = mRenderpass.data["resources"]["geometry"].size(); // always add another
    command["geometry"] = geomidx;
    Json::Value geometry;
    GLint activeUniforms = 0;
    GLuint program_id = contexts[context_index].programs.at(program_index).id;
    _glGetProgramInterfaceiv(program_id, GL_UNIFORM, GL_ACTIVE_RESOURCES, &activeUniforms);
    geometry["uniforms"] = saveProgramUniforms(program_id, activeUniforms);
    //GLint activeUniformBlocks = 0;
    //_glGetProgramInterfaceiv(id, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &activeUniformBlocks);
    geometry["vertex_buffers"] = Json::arrayValue;
    if (params.value_type != GL_NONE)
    {
        std::string filename = "index_buffer_g" + std::to_string(geomidx) + ".bin";
        command["draw_params"]["unique_indices"] = params.unique_vertices;
        command["draw_params"]["largest_index_hole"] = params.max_sparseness - 1;
        command["draw_params"]["average_index_hole"] = params.avg_sparseness - 1;
        command["draw_params"]["max_index_value"] = params.max_value;
        command["draw_params"]["min_index_value"] = params.min_value;
        geometry["index_buffer"] = write_index_buffer(mRenderpass.dirname, filename, geomidx, params, mCall, mRenderpass.index_cache);
    }
    // For each enabled vertex attribute binding
    GLint max_vertex_attribs = 0;
    _glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attribs);
    for (GLint index = 0; index < max_vertex_attribs; ++index)
    {
        GLint enabled = 0;
        _glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
        if (!enabled) continue;
        const std::string vfilename = "vertex_buffer_g" + std::to_string(geomidx) + "_b" + std::to_string(index) + ".bin";
        Json::Value v = get_vertex_attributes(program_id, mRenderpass.dirname, vfilename, geomidx, params, index, mRenderpass.vertex_cache);
        geometry["vertex_buffers"].append(v);
    }

    geometry["outputs"] = Json::Value(Json::arrayValue);

    mRenderpass.data["resources"]["geometry"].append(geometry);

    command["indexed"] = (params.value_type != GL_NONE);
    Json::Value analyze_trace;
    analyze_trace["renderpass_draw_index"] = rp.draw_calls - 1; // relative to start of renderpass
    analyze_trace["call_number"] = mCall->mCallNo;
    analyze_trace["name"] = mCall->mCallName;
    Json::Value metadata;
    metadata["analyze_trace"] = analyze_trace;
    command["metadata"] = metadata;
    switch (params.mode)
    {
    case GL_TRIANGLES: command["topology"] = "TRIANGLES"; break;
    case GL_TRIANGLE_STRIP: command["topology"] = "TRIANGLE_STRIP"; break;
    case GL_TRIANGLE_FAN: command["topology"] = "TRIANGLE_FAN"; break;
    case GL_POINTS: command["topology"] = "POINTS"; break;
    case GL_LINES: command["topology"] = "LINES"; break;
    case GL_LINE_LOOP: command["topology"] = "LINE_LOOP"; break;
    case GL_LINE_STRIP: command["topology"] = "LINE_STRIP"; break;
    case GL_LINES_ADJACENCY: command["topology"] = "LINES_ADJACENCY"; break;
    case GL_PATCHES: command["topology"] = "PATCHES"; break;
    case GL_TRIANGLES_ADJACENCY: command["topology"] = "TRIANGLES_ADJACENCY"; break;
    case GL_TRIANGLE_STRIP_ADJACENCY: command["topology"] = "TRIANGLE_STRIP_ADJACENCY"; break;
    default: assert(false); break;
    }
    mRenderpass.data["renderpass"]["commands"].append(command);
}

void ParseInterfaceRetracing::thread(const int threadidx, const int our_tid, Callback c, void *data)
{
    std::unique_lock<std::mutex> lk(gRetracer.mConditionMutex);
    thread_result r;
    r.our_tid = our_tid;
    while (!gRetracer.mFinish)
    {
        mCall = next_call();
        if (!mCall || !c(*this, mCall, data))
        {
            gRetracer.mFinish = true;
            for (auto &cv : gRetracer.conditions) cv.notify_one(); // Wake up all other threads
            break;
        }

        // ---------------------------------------------------------------------------
        // Get next packet
skip_call:
        gRetracer.curCallNo++;
        current_pos.call = gRetracer.curCallNo;
        if (!gRetracer.mFile.GetNextCall(gRetracer.fptr, gRetracer.mCurCall, gRetracer.src))
        {
            gRetracer.mFinish = true;
            for (auto &cv : gRetracer.conditions) cv.notify_one(); // Wake up all other threads
            break;
        }
        // Skip call because it is on an ignored thread?
        if (!gRetracer.mOptions.mMultiThread && gRetracer.mCurCall.tid != gRetracer.mOptions.mRetraceTid)
        {
            goto skip_call;
        }
        // Need to switch active thread?
        if (our_tid != gRetracer.mCurCall.tid)
        {
            gRetracer.latest_call_tid = gRetracer.mCurCall.tid; // need to use an atomic member copy of this here
            // Do we need to make this thread?
            if (gRetracer.thread_remapping.count(gRetracer.mCurCall.tid) == 0)
            {
                gRetracer.thread_remapping[gRetracer.mCurCall.tid] = gRetracer.threads.size();
                int newthreadidx = gRetracer.threads.size();
                gRetracer.conditions.emplace_back();
                gRetracer.threads.emplace_back(&ParseInterfaceRetracing::thread, this, (int)newthreadidx, (int)gRetracer.mCurCall.tid, c, data);
            }
            else // Wake up existing thread
            {
                const int otheridx = gRetracer.thread_remapping.at(gRetracer.mCurCall.tid);
                gRetracer.conditions.at(otheridx).notify_one();
            }
            // Now we go to sleep. However, there is a race condition, since the thread we woke up above might already have told us
            // to wake up again, so keep waking up to check if that is indeed the case.
            bool success = false;
            do {
                success = gRetracer.conditions.at(threadidx).wait_for(lk, std::chrono::milliseconds(50), [&]{ return our_tid == gRetracer.latest_call_tid || gRetracer.mFinish; });
            } while (!success);
            // Set internal tracking variables correctly
            if (current_surface.count(our_tid) > 0) surface_index = current_surface.at(our_tid);
            if (current_context.count(our_tid) > 0) context_index = current_context.at(our_tid);
        }
    }
}

void ParseInterfaceRetracing::loop(Callback c, void *data)
{
    if (!gRetracer.mFile.GetNextCall(gRetracer.fptr, gRetracer.mCurCall, gRetracer.src))
    {
        gRetracer.reportAndAbort("Empty trace file!");
    }
    mCall = next_call();
    gRetracer.threads.resize(1);
    gRetracer.conditions.resize(1);
    thread(0, gRetracer.mCurCall.tid, c, data);
    for (std::thread &t : gRetracer.threads)
    {
        if (t.joinable()) t.join();
    }
    GLWS::instance().Cleanup();
    gRetracer.CloseTraceFile();
}
