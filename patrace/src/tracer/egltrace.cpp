#include <tracer/egltrace.hpp>

#include <tracer/tracerparams.hpp>
#include <tracer/sig_enum.hpp>
#include <tracer/interactivecmd.hpp>
#include <tracer/glstate.hpp>
#include <tracer/config.hpp>

#include <helper/eglsize.hpp>
#include <helper/eglstring.hpp>

#include <common/os_string.hpp>
#include <common/os_thread.hpp>
#include <common/os_time.hpp>
#include <common/trace_model.hpp>
#include <common/trace_limits.hpp>
#include <common/image.hpp>
#include <common/gl_extension_supported.hpp>

#include "jsoncpp/include/json/writer.h"
#include "jsoncpp/include/json/reader.h"

#include <stdio.h>
#include <unistd.h>
#include <fstream>
#include <string>
#include <unordered_map>
#include <sys/stat.h>
#include <libgen.h>

using namespace common;
using namespace std;

TraceOut* gTraceOut = NULL;

/// Global to keep track of if PBOs have ever been created by the application.
bool isUsingPBO = false;

struct MyEGLSurface
{
    EGLint x;
    EGLint y;
    EGLint width;
    EGLint height;
    int index;
    uint64_t id;
};

struct MyEGLContext
{
    MyEGLAttribs config;
    int profile;
    int index;
    uint64_t id;
};

static std::map<EGLContext, TraceContext*> gCtxMap;
static unsigned int next_thread_id = 0;
static os::thread_specific_ptr<unsigned int> thread_id_specific_ptr;
static std::vector<std::unordered_map<int, int>> timesEGLConfigIdUsed;
static std::unordered_map<int, MyEGLAttribs> configIdToConfigAttribsMap;
std::vector<TraceThread> gTraceThread(PATRACE_THREAD_LIMIT);

static std::vector<MyEGLSurface> surfaces;
static std::vector<MyEGLContext> contexts;

struct MyEGLAttribArray
{
    MyEGLAttribArray() : config(PATRACE_THREAD_LIMIT) {}

    std::vector<MyEGLAttribs> config;
    unsigned defaultTid = 0;
};

static void callback(unsigned int source, unsigned int type, unsigned int id, unsigned int severity, int length, const char* message, const void* userParam)
{
    DBG_LOG("%s::%s::%s (call=%u): %s\n", cbsource(source), cbtype(type), cbseverity(severity), gTraceOut->callNo, message);
}

static MyEGLAttribArray GetBestConfigPerThread()
{
    int highestCntOverall = 0;
    MyEGLAttribArray attribArray;
    attribArray.defaultTid = 0;

    // 1. Find the Best (most used) EGL config for each thread
    for (unsigned tid = 0; tid < timesEGLConfigIdUsed.size(); tid++)
    {
        EGLint highestUseCfgId = -1;
        int highestCnt = 0;

        // Find  most used for cfgId for this tid
        for (auto it : timesEGLConfigIdUsed[tid])
        {
            if (it.second > highestCnt)
            {
                highestCnt = it.second;
                highestUseCfgId = it.first;
            }
        }

        attribArray.config.at(tid).timesUsed = 0;

        // If we found a cfg for this tid, store it
        // According to the EGL spec 1.5, EGL_CONFIG_ID must be small positive integers starting at 1.
        // But when tracing some apps on Android P, for example, calculator or clock, we found that
        // EGL_CONFIG_ID is zero. So here we detect if highestUseCfgId is greater than or EUQAL TO 0.
        if (highestUseCfgId >= 0)
        {
            MyEGLAttribs &e = configIdToConfigAttribsMap[highestUseCfgId];
            e.timesUsed = highestCnt;
            e.traceConfigId = highestUseCfgId;

            attribArray.config[tid] = e;
        }

        // find best overall
        if (highestCnt > highestCntOverall)
        {
            highestCntOverall = highestCnt;
            attribArray.defaultTid = tid;
        }
    }

    return attribArray;
}

BinAndMeta::BinAndMeta()
{
    Path path;
    os::String binName = path.getTraceFilePath();
    DBG_LOG("The trace file name is : %s\n", binName.str());

    // Try to create dir
    char *bn = strdup(binName.str()); // dirname() might modify the passed path
    mkdir(dirname(bn), 0777);
    free(bn);

    traceFile = new OutFile;
    traceFile->Open(binName.str());

    // Reset per thread counters
    timesEGLConfigIdUsed.clear();
    timesEGLConfigIdUsed.resize(PATRACE_THREAD_LIMIT);
}

BinAndMeta::~BinAndMeta()
{
    writeHeader(true);
    traceFile->Close();
}

static void addEGLConfigToJSON(Json::Value& v, const MyEGLAttribs& config)
{
    v["EGLConfig"] = Json::Value(Json::objectValue);
    v["EGLConfig"]["red"] = config.redSize;
    v["EGLConfig"]["green"] = config.greenSize;
    v["EGLConfig"]["blue"] = config.blueSize;
    v["EGLConfig"]["alpha"] = config.alphaSize;
    v["EGLConfig"]["depth"] = config.depthSize;
    v["EGLConfig"]["stencil"] = config.stencilSize;
    v["EGLConfig"]["msaaSamples"] = config.msaaSamples;
}

void BinAndMeta::writeHeader(bool cleanExit)
{
    std::lock_guard<std::recursive_mutex> guard(gTraceOut->callMutex); // need to lock against both file access and global EGL config access here

    MyEGLAttribArray perThreadEGLConfigs = GetBestConfigPerThread();

    // 1. write trace file header
    Json::Value jsonRoot;
    jsonRoot["defaultTid"] = perThreadEGLConfigs.defaultTid;
    jsonRoot["glesVersion"] = glesVersion;
    jsonRoot["texCompress"] = Json::Value(Json::arrayValue);
    jsonRoot["contexts"] = Json::Value(Json::arrayValue);
    jsonRoot["surfaces"] = Json::Value(Json::arrayValue);
    jsonRoot["callCnt"] = callCnt;
    jsonRoot["frameCnt"] = frameCnt;
    jsonRoot["threads"] = Json::Value(Json::arrayValue);
    jsonRoot["capture_device_extensions"] = captureInfo.extensions;
    jsonRoot["capture_device_vendor"] = captureInfo.vendor;
    jsonRoot["capture_device_renderer"] = captureInfo.renderer;
    jsonRoot["capture_device_version"] = captureInfo.version;
    if (!captureInfo.shaderversion.empty())
    {
        jsonRoot["capture_device_shading_language_version"] = captureInfo.shaderversion;
    }
    jsonRoot["tracer"] = PATRACE_VERSION;
    jsonRoot["tracer_extensions"] = tracerParams.SupportedExtensionsString;

    Json::Value jsonThreadArray;
    int index = 0;
    for (const MyEGLAttribs& config : perThreadEGLConfigs.config)
    {
        if (config.timesUsed > 0)
        {
            Json::Value jsThread;
            jsThread["id"] = index;
            jsThread["clientSideBufferSize"] = (Json::UInt64)gTraceOut->mCSBufferSet.total_size(index);
            // Same window resolution for all threads at capture.
            // We can change this with an editor before playback
            jsThread["winW"] = winSurWidth;
            jsThread["winH"] = winSurHeight;

            addEGLConfigToJSON(jsThread, config);

            // Convert the attributes from STL container to JSON
            Json::Value attributes(Json::arrayValue);

            const StringListList_t& aa = gTraceThread.at(index).mActiveAttributes;
            for (const StringList_t& strList : aa)
            {
                Json::Value attributesNames(Json::arrayValue);
                for (const std::string& attributeName : strList)
                {
                    attributesNames.append(attributeName);
                }
                attributes.append(attributesNames);
            }

            jsThread["attributes"] = attributes;
            jsonRoot["threads"].append(jsThread);
        }
        index++;
    }

    for (const MyEGLContext& a : contexts)
    {
        Json::Value v;
        v["index"] = a.index;
        v["id"] = (Json::Value::UInt64)a.id;
        v["glesVersion"] = a.profile;
        addEGLConfigToJSON(v, a.config);
        jsonRoot["contexts"].append(v);
    }

    for (const MyEGLSurface& s : surfaces)
    {
        Json::Value v;
        v["x"] = s.x;
        v["y"] = s.y;
        v["width"] = s.width;
        v["height"] = s.height;
        v["index"] = s.index;
        v["id"] = (Json::Value::UInt64)s.id;
        jsonRoot["surfaces"].append(v);
    }

    for (const auto& pair : texCompressFormats)
    {
        Json::Value jsonTexCompressFormat;
        jsonTexCompressFormat["internalformat"] = pair.first;
        jsonTexCompressFormat["callCnt"] = pair.second;
        jsonRoot["texCompress"].append(jsonTexCompressFormat);
    }

    jsonRoot["cleanExit"] = cleanExit;

    Json::FastWriter writer;
    std::string jsonData = writer.write(jsonRoot);

    // Now that we have all header data written to JSON, write it to reserved header-area
    if (0 != jsonData.length())
    {
        traceFile->WriteHeader(jsonData.c_str(), jsonData.length(), !tracerParams.FlushTraceFileEveryFrame);
    }
    else
    {
        DBG_LOG("Error: no jsonData to write\n");
        traceFile->mHeader.jsonLength = 0;
    }
}

void BinAndMeta::updateWinSurfSize(EGLint width, EGLint height)
{
    winSurWidth = width;
    winSurHeight = height;
    DBG_LOG("Width: %d, Height: %d\n", winSurWidth, winSurHeight);
}

MyEGLAttribs GetEGLConfigValues(EGLDisplay dpy, EGLConfig config)
{
    MyEGLAttribs ret;
    if (_eglGetConfigAttrib(dpy, config, EGL_RED_SIZE, (EGLint*)&ret.redSize) == EGL_FALSE ||
        _eglGetConfigAttrib(dpy, config, EGL_GREEN_SIZE, (EGLint*)&ret.greenSize) == EGL_FALSE ||
        _eglGetConfigAttrib(dpy, config, EGL_BLUE_SIZE, (EGLint*)&ret.blueSize) == EGL_FALSE ||
        _eglGetConfigAttrib(dpy, config, EGL_ALPHA_SIZE, (EGLint*)&ret.alphaSize) == EGL_FALSE ||
        _eglGetConfigAttrib(dpy, config, EGL_DEPTH_SIZE, (EGLint*)&ret.depthSize) == EGL_FALSE ||
        _eglGetConfigAttrib(dpy, config, EGL_STENCIL_SIZE, (EGLint*)&ret.stencilSize) == EGL_FALSE ||
        _eglGetConfigAttrib(dpy, config, EGL_SAMPLES, (EGLint*)&ret.msaaSamples) == EGL_FALSE
        )
        DBG_LOG("Failed to query the attributes of the config\n");
    return ret;
}

void BinAndMeta::saveExtensions()
{
    if (captureInfo.valid) return;
    const char* exts = (const char*)_glGetString(GL_EXTENSIONS);
    const char* vendor = (const char*)_glGetString(GL_VENDOR);
    const char* renderer = (const char*)_glGetString(GL_RENDERER);
    const char* version = (const char*)_glGetString(GL_VERSION);
    const char* shaderversion = (const char*)_glGetString(GL_SHADING_LANGUAGE_VERSION);
    captureInfo.extensions = std::string(exts);
    captureInfo.vendor = std::string(vendor);
    captureInfo.renderer = std::string(renderer);
    captureInfo.version = std::string(version);
    if (shaderversion)
    {
        captureInfo.shaderversion = std::string(shaderversion);
    }
    captureInfo.valid = true;
}

// Save all EGL configs beforehand so we know what EGL_CONFIG_ID maps to a EGL_CONFIG
// also, make a map between EGL attribs and config id's
void BinAndMeta::saveAllEGLConfigs(EGLDisplay dpy)
{
    int cfgCnt = 0;

    _eglGetConfigs(dpy, NULL, 0, &cfgCnt);
    EGLConfig* cfgArray = new(nothrow) EGLConfig[cfgCnt];
    if (cfgArray == NULL)
    {
        DBG_LOG("malloc failed\n");
        os::abort();
    }
    else
    {
        EGLint numCfgsReturned = 0;
        _eglGetConfigs(dpy, cfgArray, cfgCnt, &numCfgsReturned);
        std::lock_guard<std::recursive_mutex> guard(gTraceOut->callMutex); // changing global EGL config state
        configIdToConfigAttribsMap.clear();
        for (EGLint i = 0; i < cfgCnt; i++)
        {
            EGLint cfgId;
            _eglGetConfigAttrib(dpy, cfgArray[i], EGL_CONFIG_ID, &cfgId);
            configIdToConfigAttribsMap[cfgId] = GetEGLConfigValues(dpy,cfgArray[i]);
        }
        delete[] cfgArray;
    }
}

std::string BinAndMeta::getFileName() const
{
    return traceFile->getFileName();
}

TraceOut::TraceOut() : writebuf(new char[WRITE_BUF_LEN])
{
}

TraceOut::~TraceOut()
{
    Close();
    delete [] writebuf;
}

unsigned char GetThreadId()
{
    unsigned int* thread_id_ptr = thread_id_specific_ptr.get(); //ptr = pthread_getspecific(key); is NULL the first time
    if (thread_id_ptr == NULL) {
        thread_id_ptr = new unsigned int;
        *thread_id_ptr = next_thread_id++;
        thread_id_specific_ptr.reset(thread_id_ptr); //pthread_setspecific(key, new_value);
    }

    return (unsigned char) *thread_id_ptr;
}

// This function is called from each traced EGL/GL call, see trace.py and its output egltrace_auto.cpp
void UpdateTimesEGLConfigUsed(int threadid)
{
    const TraceContext* traceCtxPtr = GetCurTraceContext(threadid);
    if (traceCtxPtr && traceCtxPtr->mEGLCtx)
    {
        timesEGLConfigIdUsed[threadid][traceCtxPtr->mEGLConfigId]++;
    }
}

TraceContext* GetCurTraceContext(unsigned char tid)
{
    return gTraceThread.at(tid).mCurCtx;
}

TraceContext::TraceContext(EGLContext ctx, EGLint configId): mEGLCtx(ctx), mEGLConfigId(configId)
{
    if (gCtxMap.find(mEGLCtx) != gCtxMap.end())
    {
        DBG_LOG("Error: EGLContext %p already exists!\n", ctx);
        return;
    }
    gCtxMap.insert(std::pair<EGLContext, TraceContext*>(ctx, this));
}

TraceContext::~TraceContext()
{
    std::map<EGLContext, TraceContext*>::iterator it = gCtxMap.find(mEGLCtx);
    if (it == gCtxMap.end())
    {
        DBG_LOG("Error: EGLContext %p doesn't exist!\n", mEGLCtx);
        return;
    }
    gCtxMap.erase(it);
}

static int GetGLESVersion(const EGLint *attrib_list)
{
    int glesVer = 1;

    if (attrib_list)
    {
        const EGLint *attr = attrib_list;
        while (*attr != EGL_NONE)
        {
            if (*attr == EGL_CONTEXT_CLIENT_VERSION)
                glesVer = *(attr+1);

            attr += 2;
        }
    }

    return glesVer;
}

// Returns the number of locations the attribute type occupies
int numLocations(GLenum type)
{
    switch (type)
    {
    case GL_FLOAT_MAT2:
    case GL_FLOAT_MAT2x3:
    case GL_FLOAT_MAT2x4:
        return 2;
    case GL_FLOAT_MAT3x2:
    case GL_FLOAT_MAT3:
    case GL_FLOAT_MAT3x4:
        return 3;
    case GL_FLOAT_MAT4x2:
    case GL_FLOAT_MAT4x3:
    case GL_FLOAT_MAT4:
        return 4;
    default:
        return 1;
    }
}

// Sets num bits to 1.
// E.g. if num == 4, then return 1111b, i.e. 15
int numBits(int num)
{
    return (0x1 << num) - 1;
}


void after_glBindAttribLocation(unsigned char tid, GLuint program, GLuint index)
{
    std::map<unsigned int, unsigned int>& mapPrgToBoundAttribs = GetCurTraceContext(tid)->mapPrgToBoundAttribs;
    std::map<unsigned int, unsigned int>::iterator it = mapPrgToBoundAttribs.find(program);
    if (it == mapPrgToBoundAttribs.end())
        DBG_LOG("Error: The program name is not found in mapPrgToBoundAttribs!\n");

    // mark the location "index" is bound with bitmask.
    it->second |= (0x1 << index);
}

void after_glMapBufferRange(GLenum target, GLsizeiptr length, GLbitfield access, GLvoid* base)
{
    BufferRangeData data;
    data.length = length;
    data.base = base;
    data.access = access;

    unsigned char tid = GetThreadId();
    GLuint currentlyBoundBuffer = getBoundBuffer(target);
    if (currentlyBoundBuffer == 0)
    {
        DBG_LOG("No buffer currently bound to target %s for glMapBufferRange!\n", bufferName(target));
    }

    GetCurTraceContext(tid)->bufferToClientPointerMap[currentlyBoundBuffer] = data;

    if (data.access == GL_WRITE_ONLY)
    {
        std::vector<unsigned char>& contents = GetCurTraceContext(tid)->bufferToClientPointerMap[currentlyBoundBuffer].contents;
        BufferInitializedSet_t &bufInitSet = GetCurTraceContext(tid)->bufferInitializedSet;
        if (bufInitSet.find(currentlyBoundBuffer) != bufInitSet.end())
        {
            unsigned char* bufdata = static_cast<unsigned char*>(base);
            contents.assign(bufdata, bufdata + length);
        }
        else
        {
            contents.resize(length);
        }
    }
}

void pre_glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length)
{
    unsigned char tid = GetThreadId();
    BufferToClientPointerMap_t& map = GetCurTraceContext(tid)->bufferToClientPointerMap;

    GLuint currentlyBoundBuffer = getBoundBuffer(target);
    BufferToClientPointerMap_t::iterator it = map.find(currentlyBoundBuffer);
    if (it != map.end())
    {
        BufferRangeData& data = it->second;

        bool created = false;
        void* offsettedPointer = static_cast<char*>(data.base) + offset;
        ClientSideBufferObjectName name = _getOrCreateClientSideBuffer(offsettedPointer, length, created);

        if (created)
        {
            // Store data for newly created CSB
            _glClientSideBufferData(name, data.length, data.base);
        }

        _glCopyClientSideBuffer(target, name);
    }
}

static const unsigned int CSB_PATCH_MIN_BUFFER_SIZE = 0x8000; // 32kB
static const unsigned int CSB_PATCH_PAGE_SIZE = 0x400; // 1kB
static const float CSB_PATCH_UP_THRESHOLD = 0.8;
static bool genCSBPatchList(GLenum target, const void* old_data, const void* new_data, unsigned int length)
{
    const unsigned char* old_ptr = static_cast<const unsigned char*>(old_data);
    const unsigned char* new_ptr = static_cast<const unsigned char*>(new_data);
    unsigned int page_num = 0;
    unsigned int remain_data_length = length;
    std::vector<unsigned int> dirty_page_list;

    if (length < CSB_PATCH_MIN_BUFFER_SIZE)
    {
        // skip for small buffers
        //DBG_LOG("INFO: The length of buffer is %d that is too small then skip it\n", length);
        return false;
    }

    while(remain_data_length > 0)
    {
        unsigned int cmp_size = remain_data_length > CSB_PATCH_PAGE_SIZE ? CSB_PATCH_PAGE_SIZE : remain_data_length;
        if (memcmp(old_ptr, new_ptr, cmp_size))
        {
            dirty_page_list.push_back(page_num);
        }
        remain_data_length -= cmp_size;
        page_num++;
        old_ptr += cmp_size;
        new_ptr += cmp_size;
    }

    if ((dirty_page_list.size() * CSB_PATCH_PAGE_SIZE) > (length * CSB_PATCH_UP_THRESHOLD))
    {
        // too many dirty area so fall back on full copy
        //DBG_LOG("INFO: %d dirty area are found for buffer length %d, that is too many dirty area for patch so skip it\n", dirty_area_count, length);
        return false;
    }

    //DBG_LOG("INFO: PatchCSB is enabled with buffer size %d, dirty area count %d!\n", length, dirty_area_count);

    // calc patch list buffer size
    unsigned int patch_buf_size = sizeof(CSBPatchList) + (sizeof(CSBPatch) + CSB_PATCH_PAGE_SIZE) * dirty_page_list.size();
    unsigned char* patch_buf = new unsigned char[patch_buf_size];
    unsigned char* patch_buf_ptr = patch_buf;
    CSBPatchList pl;

    pl.count = dirty_page_list.size();
    memcpy(patch_buf_ptr, &pl, sizeof(pl));

    patch_buf_ptr += sizeof(pl);

    for (size_t i = 0; i < dirty_page_list.size(); i++)
    {
        CSBPatch patch;
        patch.offset = CSB_PATCH_PAGE_SIZE * dirty_page_list[i];
        patch.length = (patch.offset + CSB_PATCH_PAGE_SIZE) > length ? (length - patch.offset) : (CSB_PATCH_PAGE_SIZE);

        memcpy(patch_buf_ptr, &patch, sizeof(patch));
        patch_buf_ptr += sizeof(patch);
        memcpy(patch_buf_ptr, ((unsigned char*)new_data + patch.offset), patch.length);
        patch_buf_ptr += patch.length;
    }

    _glPatchClientSideBuffer(target, patch_buf_size, patch_buf);

    delete[] patch_buf;

    return true;
}

void pre_glUnmapBuffer(GLenum target)
{
    unsigned char tid = GetThreadId();
    BufferToClientPointerMap_t& map = GetCurTraceContext(tid)->bufferToClientPointerMap;

    GLuint currentlyBoundBuffer = getBoundBuffer(target);
    BufferToClientPointerMap_t::iterator it = map.find(currentlyBoundBuffer);
    if (it != map.end())
    {
        BufferRangeData& data = it->second;

        if ((data.access & GL_MAP_WRITE_BIT) ||
            data.access == GL_WRITE_ONLY)
        {
            bool hasPatch = false;

            BufferInitializedSet_t &bufInitSet = GetCurTraceContext(tid)->bufferInitializedSet;
            if (data.access == GL_WRITE_ONLY && bufInitSet.find(currentlyBoundBuffer) != bufInitSet.end())
            {
                hasPatch = genCSBPatchList(target, data.contents.data(), data.base, data.length);
            }

            if (!hasPatch)
            {
                bool created = false;
                ClientSideBufferObjectName name = _getOrCreateClientSideBuffer(data.base, data.length, created);

                if (created)
                {
                    // Store data for newly created CSB
                    _glClientSideBufferData(name, data.length, data.base);
                }

                _glCopyClientSideBuffer(target, name);
            }
        }
    }
    if (GetCurTraceContext(tid)->isFullMapping)
    {
        GetCurTraceContext(tid)->bufferInitializedSet.insert(currentlyBoundBuffer);
    }
}

void after_glUnmapBuffer(GLenum target)
{
    unsigned char tid = GetThreadId();
    BufferToClientPointerMap_t& map = GetCurTraceContext(tid)->bufferToClientPointerMap;

    GLuint currentlyBoundBuffer = getBoundBuffer(target);
    BufferToClientPointerMap_t::iterator it = map.find(currentlyBoundBuffer);
    if (it != map.end())
    {
        map.erase(it);
    }
}

void after_glCreateProgram(unsigned char tid, GLuint program)
{
    // attributes
    std::map<unsigned int, unsigned int>& mapPrgToBoundAttribs = GetCurTraceContext(tid)->mapPrgToBoundAttribs;
    std::map<unsigned int, unsigned int>::iterator it = mapPrgToBoundAttribs.find(program);
    if (it != mapPrgToBoundAttribs.end())
        DBG_LOG("Error: The name conflicts with an existing program!\n");
    mapPrgToBoundAttribs.insert(std::pair<unsigned int, unsigned int>(program, 0));
}

void _reportLinkErrors(GLuint program)
{
    GLint success = GL_TRUE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if(success == GL_FALSE)
    {
        DBG_LOG("PROGRAM LINK ERROR !!!\n");
        GLsizei logLength = 0;
        _glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength );
        if(logLength)
        {
            char* log = new char[logLength];
            _glGetProgramInfoLog(program, logLength, NULL, log);
            DBG_LOG("Log:\n%s\n",log);
            delete[] log;
        }
    }
}

GLuint replace_glCreateShaderProgramv(unsigned char tid, GLenum type, GLsizei count, const GLchar * const * strings)
{
    // For vertex shaders, we need to replace it with our own, so that we can inject vertex attribute location calls
    // into the trace file.
    if (type == GL_VERTEX_SHADER)
    {
        const GLuint shader = _glCreateShader(type);
        if (shader)
        {
            _glShaderSource(shader, count, strings, NULL);
            _glCompileShader(shader);
            const GLuint program = _glCreateProgram();
            if (program)
            {
                GLint compiled = GL_FALSE;
                _glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
                _glProgramParameteri(program, GL_PROGRAM_SEPARABLE, GL_TRUE);
                if (compiled)
                {
                    _glAttachShader(program, shader);
                    pre_glLinkProgram(tid, program);
                    after_glLinkProgram(program);
                    _glDetachShader(program, shader);
                }
            }
            _glDeleteShader(shader);
            return program;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        GLuint program = _glCreateShaderProgramv(type, count, strings);
        after_glLinkProgram(program);
        return program;
    }
}

void pre_glLinkProgram(unsigned char tid, unsigned int program)
{
    std::map<unsigned int, unsigned int>& mapPrgToBoundAttribs = GetCurTraceContext(tid)->mapPrgToBoundAttribs;
    std::map<unsigned int, unsigned int>::iterator it = mapPrgToBoundAttribs.find(program);
    if (it == mapPrgToBoundAttribs.end()) {
        DBG_LOG("Error: The program name is not found in mapPrgToBoundAttribs!\n");
        return;
    }

    // 1. link the 'program' in order to figure out its active vertex attributes
    _glLinkProgram(program);
    //_reportLinkErrors(program);

    // 2. figure out the number of the active vertex attributes
    GLint attribCnt = 0;
    _glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &attribCnt);

    GLint maxVertexAttribs = 0;
    _glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);

    // 3. Go through each active vertex attributes, to see if it has already been explicitly given an index by the
    //    application. Otherwise, we'll assign one index for it explicitly!
    for (int itAttrib = 0; itAttrib < attribCnt; ++itAttrib)
    {
        GLchar  attribName[128];
        GLint   attribSize = 0;
        GLenum  attribType = 0;
        _glGetActiveAttrib(program, itAttrib, 128, 0, &attribSize, &attribType, attribName);
        GLint attribLoc = _glGetAttribLocation(program, attribName);

        // glGetAttribLocation returns locations >= maxVertexAttribs for built-in
        // attributes, such that gl_VertexID. According to spec they should be -1.
        // Is this a ddk bug?
        // That is why we need to check that attribLocation < maxVertexAttribs
        if (attribLoc >= 0 && attribLoc < maxVertexAttribs)
        {
            glBindAttribLocation(program, attribLoc, attribName);
        }
    }
}

void after_glDeleteProgram(unsigned char tid, GLuint program)
{
    // attributes
    if (program == 0) // silly AnTuTu
    {
         return;
    }
    std::map<unsigned int, unsigned int>& mapPrgToBoundAttribs = GetCurTraceContext(tid)->mapPrgToBoundAttribs;
    std::map<unsigned int, unsigned int>::iterator it = mapPrgToBoundAttribs.find(program);
    if (it == mapPrgToBoundAttribs.end()) {
        DBG_LOG("Error: The program to be deleted (%u) is not found!\n", program);
    }
    else {
         mapPrgToBoundAttribs.erase(it);
    }
}

static bool TakeSnapshot(int frNoOverride = -1)
{
    image::Image *src = glstate::getDrawBufferImage();
    if (src == NULL)
    {
        DBG_LOG("Failed to take snapshot for frame %u, call no: %u\n", gTraceOut->frameNo, gTraceOut->callNo);
        return false;
    }

    char filename[128];
#ifdef ANDROID
    const char* snapPath = "/data/apitrace/snap/";
#else
    const char* snapPath = "snap/";
#endif
    int frNo;
    if (frNoOverride != -1) {
        frNo = frNoOverride;
    } else {
        frNo = gTraceOut->frameNo;
    }
    sprintf(filename, "%sf%05u_c%010u.png", snapPath, frNo, gTraceOut->callNo);

    if (src->writePNG(filename))
        DBG_LOG("Snapshot : %s\n", filename);
    else
        DBG_LOG("Failed to write snapshot : %s\n", filename);

    if (tracerParams.StateDumpAfterSnapshot)
    {
        gTraceOut->getStateLogger().logState(GetThreadId());
    }

    delete src;
    return true;
}

void after_glDraw()
{
    if (gTraceOut->snapDraw)
    {
        TakeSnapshot();
    }
}

void after_glLinkProgram(unsigned int program)
{
    ProgramInfo pi(program);
    unsigned char tid = GetThreadId();
    StringListList_t& aa = gTraceThread.at(tid).mActiveAttributes;

    // 1. Add empty list
    aa.push_back(StringList_t());
    // 2. Get its reference
    StringList_t& aaList = aa.back();
    for (int i = 0; i < pi.activeAttributes; ++i)
    {
        VertexArrayInfo vai = pi.getActiveAttribute(i);
        // 3. Add stuff to it
        aaList.push_back(vai.name);
    }

    // Inject glGetUniformBlockIndex calls to be sure we can intercept and remap block indices on retrace
    GLint linkStatus = GL_FALSE;
    _glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE)
    {
        DBG_LOG("Link failure, cannot inject block lookups!\n");
        return; // skip rest
    }
    GLint activeUniformBlocks = 0;
    _glGetProgramiv(program, GL_ACTIVE_UNIFORM_BLOCKS, &activeUniformBlocks);
    for (int i = 0; i < activeUniformBlocks; ++i)
    {
        char cname[128];
        _glGetActiveUniformBlockName(program, i, sizeof(cname), NULL, cname);
        GLuint retval = glGetUniformBlockIndex(program, cname); // injected call
        if (retval == GL_INVALID_INDEX)
        {
            DBG_LOG("INVALID INDEX injecting block lookup for %s in program %u!\n", cname, program);
        }
    }

    // Inject glGetUniformLocation calls to be sure we can intercept and remap uniform indices on retrace
    GLint activeUniforms = 0;
    _glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &activeUniforms);
    for (GLuint i = 0; activeUniforms > 0 && i < static_cast<GLuint>(activeUniforms); ++i)
    {
        char cname[128];
        GLint size = 0;
        GLenum type = 0;
        GLint block = 0;
        _glGetActiveUniformsiv(program, 1, &i, GL_UNIFORM_BLOCK_INDEX, &block);
        if (block != -1)
        {
            continue; // is inside uniform block, handled above
        }
        _glGetActiveUniform(program, i, sizeof(cname), NULL, &size, &type, cname);
        if (type == GL_UNSIGNED_INT_ATOMIC_COUNTER)
        {
            continue; // is atomic counter, cannot be location'ed since it must be explicitly bound
        }
        GLint retval = glGetUniformLocation(program, cname); // injected call
        if (retval == -1)
        {
            DBG_LOG("Error injecting location lookup for %s in program %u!\n", cname, program);
        }
        if (size > 1) { // This is an array, we need to inject glGetUniformLocation calls for all its elements
            string s(cname);
            int first_digit_id = s.find_first_of('[');
            s = s.substr(0, first_digit_id + 1);
            for (int i = 1; i < size; ++i) {
                stringstream istring;
                istring << i;
                GLint retval = glGetUniformLocation(program, (s + istring.str() + ']').c_str()); // injected call
                if (retval == -1)
                {
                    DBG_LOG("Error injecting location lookup for %s in program %u!\n", cname, program);
                }
            }
        }
    }
}

void after_eglInitialize(EGLDisplay dpy)
{
    gTraceOut->mpBinAndMeta->saveAllEGLConfigs(dpy);
    gTraceOut->mpBinAndMeta->writeHeader(false);
}

void after_eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLSurface surf, EGLint x, EGLint y, EGLint width, EGLint height)
{
    gTraceOut->mpBinAndMeta->updateWinSurfSize(width, height);
    const int idx = surfaces.size();
    const uint64_t id = reinterpret_cast<uint64_t>(surf);
    surfaces.push_back({x, y, width, height, idx, id});
}

void after_eglCreateContext(EGLContext ctx, EGLDisplay dpy, EGLConfig config, const EGLint * attrib_list)
{
    EGLint configId; _eglQueryContext(dpy, ctx, EGL_CONFIG_ID, &configId);
    TraceContext* newTraceCtx = new TraceContext(ctx, configId);
    newTraceCtx->profile = GetGLESVersion(attrib_list);
    const MyEGLAttribs &e = configIdToConfigAttribsMap.at(configId);
    const int idx = contexts.size();
    const uint64_t id = reinterpret_cast<uint64_t>(ctx);
    contexts.push_back({e, newTraceCtx->profile, idx, id});
}

void after_eglMakeCurrent(EGLDisplay dpy, EGLSurface drawSurf, EGLContext ctx)
{
    unsigned char tid = GetThreadId();

    // 1. Check the previous context
    if (gTraceThread.at(tid).mCurCtx != NULL && gTraceThread.at(tid).mCurCtx->isEglDestroyContextInvoked)
    {
        delete gTraceThread.at(tid).mCurCtx;
        gTraceThread.at(tid).mCurCtx = NULL;
    }

    // 2. Check the coming context
    if (ctx == EGL_NO_CONTEXT)
    {
        gTraceThread.at(tid).mCurCtx = NULL;
        return;
    }

    std::map<EGLContext, TraceContext*>::iterator it = gCtxMap.find(ctx);
    if (it == gCtxMap.end())
    {
        DBG_LOG("Error: EGLContext %p doesn't exist!\n", ctx);
        return;
    }

    TraceContext* traceCtx = it->second;
    gTraceThread.at(tid).mCurCtx = traceCtx;

    if (gTraceOut->mpBinAndMeta == NULL)
    {
        DBG_LOG("gTraceOut->mpBinAndMeta should not be NULL right now!\n");
    }
    gTraceOut->mpBinAndMeta->glesVersion = traceCtx->profile;

    // 3. Set the gles version, so that the dispatcher will know which DLL(gles1.so or gles2.so) to forward these gl function calls
    SetGLESVersion(traceCtx->profile);
    gTraceOut->mpBinAndMeta->saveExtensions();

    _glDebugMessageCallback(callback, 0);
    _glEnable(GL_DEBUG_OUTPUT_KHR);
    // disable notifications -- they generate too much spam on some systems
    _glDebugMessageControlKHR(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION_KHR, 0, NULL, GL_FALSE);

    int gles_version_major = gGlesFeatures.glesVersion() / 100;
    if (gles_version_major >= 2)
        _glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &traceCtx->mMaxVertexAttribs);
}

void after_eglDestroyContext(EGLContext ctx)
{
    std::map<EGLContext, TraceContext*>::iterator it = gCtxMap.find(ctx);
    if (it == gCtxMap.end())
    {
        DBG_LOG("Error: EGLContext %p doesn't exist!\n", ctx);
        return;
    }

    TraceContext* traceCtx = it->second;

    bool isCurrent = false;
    for (auto& thread : gTraceThread)
    {
        if (thread.mCurCtx == traceCtx)
        {
            isCurrent = true;
            traceCtx->isEglDestroyContextInvoked = true;
        }
    }
    if (!isCurrent)
    {
        delete traceCtx;
    }
}

void pre_eglSwapBuffers()
{
    if (!tracerParams.InteractiveIntercept)
        return;

    while (1) {
        gTraceOut->snapDraw = false;

        InteractiveCmd cmd = GetInteractiveCmd();

        if (cmd.cmd == SNAP_ALL_FRAMES_NOWAIT) {
            unsigned int curTid = GetThreadId();
            if (curTid == cmd.requiredTid) {
                DBG_LOG("Cmd:%s, tid:%d, frameRate:%d, curTid:%d, curFrame:%d\n", cmd.nameCString, cmd.requiredTid, cmd.frameRate, curTid, gTraceOut->frameNo );
                static unsigned int threadSpecificFrameCount = 0;
                bool isRequiredFrame = threadSpecificFrameCount % cmd.frameRate == 0;
                if ( isRequiredFrame ) {
                    TakeSnapshot(threadSpecificFrameCount);
                }
                threadSpecificFrameCount++;
            }
            // Always return from snap all frames
            return;

        } else {
            DBG_LOG("Cmd:%s, fr:%d\n", cmd.nameCString, cmd.frameNo);
            // Check if its time to apply command
            if (gTraceOut->frameNo < cmd.frameNo) {
                DBG_LOG("Frame no:%d < %d\n", gTraceOut->frameNo, cmd.frameNo);
                return;
            }

            if (cmd.cmd == UNKNOWN_CMD)
                return;
            else if (cmd.cmd == SNAP_DRAW_FRAME) {
                if (gTraceOut->frameNo == cmd.frameNo) {
                    gTraceOut->snapDraw = true;
                    return;
                }
            }
        }

        usleep(3000000);
    }
}

void insert_glBindTexture(GLenum target, GLuint texture, int tid)
{
    char* dest = gTraceOut->writebuf;
    BCall *pCall = (BCall*)dest;
    pCall->funcId = glBindTexture_id;
    pCall->tid = tid; pCall->reserved = 0;
    dest += sizeof(*pCall);

    dest = WriteFixed<int>(dest, target); // enum
    dest = WriteFixed<unsigned int>(dest, texture); // literal
    pCall->errNo = GetCallErrorNo("glBindTexture", tid);
    gTraceOut->WriteBuf(dest);
    gTraceOut->callNo++;
}

void insert_glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * pixels, int tid)
{
    char* dest = gTraceOut->writebuf;
    BCall_vlen *pCall2 = (BCall_vlen*)dest;
    pCall2->funcId = glTexImage2D_id;
    pCall2->tid = tid; pCall2->reserved = 0;
    dest += sizeof(*pCall2);

    dest = WriteFixed<int>(dest, target); // enum
    dest = WriteFixed<int>(dest, level); // literal
    dest = WriteFixed<int>(dest, internalformat); // enum
    dest = WriteFixed<int>(dest, width); // literal
    dest = WriteFixed<int>(dest, height); // literal
    dest = WriteFixed<int>(dest, border); // literal
    dest = WriteFixed<int>(dest, format); // enum
    dest = WriteFixed<int>(dest, type); // enum
    dest = WriteFixed<unsigned int>(dest, BlobType);
    dest = Write1DArray<char>(dest, (unsigned int)_glTexImage2D_size(format, type, width, height), (const char*)pixels);
    pCall2->errNo = GetCallErrorNo("glTexImage2D", tid);
    pCall2->toNext = dest-gTraceOut->writebuf;
    gTraceOut->WriteBuf(dest);
    gTraceOut->callNo++;
}

GLuint pre_eglCreateImageKHR(EGLImageKHR image, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list)
{
    // Fetch EGLImage as a texture
    GLint level = 0;
    GLint internalformat = 0;
    GLsizei width = 0;
    GLsizei height = 0;
    GLint border = 0;
    GLenum format = 0;
    GLenum type = 0;
    const GLvoid* pixels = 0;

    int w = 0;
    int h = 0;
    if (target == EGL_NATIVE_BUFFER_ANDROID)
    {
        // hack into ANativeWindowBuffer to get the width and height
        char *ptr_char = (char *)buffer;
        ptr_char += sizeof(int) * 2;
        ptr_char += sizeof(void *) * 6;
        int *ptr_int = (int *)ptr_char;
        w = ptr_int[0];
        h = ptr_int[1];
    }
    image_info* info = _EGLImageKHR_get_image_info(image, target, w, h);
    if (info)
    {
        internalformat = info->internalformat;
        width = info->width;
        height = info->height;
        format = info->format;
        type = info->type;
        pixels = info->pixels;
    }

    // Create new Texture2D that represent the content of the EGLImage
    GLint oldBoundTexture = 0;
    _glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldBoundTexture);
    GLuint textureId = 0;
    glGenTextures(1, &textureId);

    unsigned char tid = GetThreadId();
    gTraceOut->callMutex.lock();

    insert_glBindTexture(GL_TEXTURE_2D, textureId, tid);
    insert_glTexImage2D(GL_TEXTURE_2D, level, internalformat, width, height, border, format, type, pixels, tid);
    insert_glBindTexture(GL_TEXTURE_2D, oldBoundTexture, tid);
    gTraceOut->callMutex.unlock();

    // Delete image data
    _EGLImageKHR_free_image_info(info);

    // Add to map
    EGLImageToTextureIdMap_t& map = gTraceThread.at(tid).mEglImageToTextureIdMap;
    map[image] = textureId;
    EGLImageInfoMap_t &map2 = gTraceThread.at(tid).mEglImageInfoMap;
    map2[image] = SizeTargetAndAttrib(width, height, target, attrib_list);

    return textureId;
}

GLuint pre_glEGLImageTargetTexture2DOES(EGLImageKHR image, EGLint *&attrib_list)
{
    // Fetch EGLImage as a texture
    GLint level = 0;
    GLint internalformat = 0;
    GLsizei width = 0;
    GLsizei height = 0;
    GLint border = 0;
    GLenum format = 0;
    GLenum type = 0;
    const GLvoid* pixels = 0;

    unsigned char tid = GetThreadId();
    auto iter0 = gTraceThread.at(tid).mEglImageInfoMap.find(image);
    int w = 0, h = 0;
    EGLenum target = 0;
    if (iter0 == gTraceThread.at(tid).mEglImageInfoMap.end()) {
        DBG_LOG("Used a non-exist EGLImageKHR %p, maybe there is something wrong!\n", image);
        attrib_list = new EGLint[1];
        attrib_list[0] = EGL_NONE;
        EGLImageInfoMap_t &map = gTraceThread.at(tid).mEglImageInfoMap;
        map[image] = SizeTargetAndAttrib(w, h, target, attrib_list);
    }
    else {
        w = iter0->second.width;
        h = iter0->second.height;
        target = iter0->second.target;
        attrib_list = iter0->second.attrib_list;
    }
    image_info* info = _EGLImageKHR_get_image_info(image, target, w, h);
    if (info)
    {
        internalformat = info->internalformat;
        width = info->width;
        height = info->height;
        format = info->format;
        type = info->type;
        pixels = info->pixels;
    }

    // Create new Texture2D that represent the content of the EGLImage
    GLint oldBoundTexture = 0;
    _glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldBoundTexture);
    GLuint textureId = 0;
    auto iter = gTraceThread.at(tid).mEglImageToTextureIdMap.find(image);
    if (iter == gTraceThread.at(tid).mEglImageToTextureIdMap.end())
    {
        DBG_LOG("Used a non-exist EGLImageKHR %p, maybe there is something wrong!\n", image);
        glGenTextures(1, &textureId);
        unsigned char tid = GetThreadId();
        EGLImageToTextureIdMap_t& map = gTraceThread.at(tid).mEglImageToTextureIdMap;
        map[image] = textureId;
    }
    else {
        textureId = iter->second;
    }
    gTraceOut->callMutex.lock();
    insert_glBindTexture(GL_TEXTURE_2D, textureId, tid);
    insert_glTexImage2D(GL_TEXTURE_2D, level, internalformat, width, height, border, format, type, pixels, tid);
    insert_glBindTexture(GL_TEXTURE_2D, oldBoundTexture, tid);
    gTraceOut->callMutex.unlock();

    // Delete image data
    _EGLImageKHR_free_image_info(info);

    return textureId;
}

void after_eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR image)
{
    unsigned char tid = GetThreadId();
    EGLImageToTextureIdMap_t& map = gTraceThread.at(tid).mEglImageToTextureIdMap;
    EGLImageToTextureIdMap_t::iterator it = map.find(image);
    if (it == map.end())
    {
        return;
    }

    GLuint textureId = it->second;
    if (_eglGetCurrentContext())
    {
        glDeleteTextures(1, &textureId);
    }
    map.erase(it);
}

void updateUsage(GLenum target)
{
    if (target == GL_PIXEL_UNPACK_BUFFER || target == GL_PIXEL_PACK_BUFFER)
    {
        isUsingPBO = true;
    }
}

void after_eglSwapBuffers()
{
    if (tracerParams.FlushTraceFileEveryFrame)
    {
        gTraceOut->mpBinAndMeta->writeHeader(true);
    }
    gTraceOut->frameNo++;
}

void after_eglDestroySurface()
{
    gTraceOut->frameNo++;
}

void _glVertexPointer_fake(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer, unsigned int _size)
{
    unsigned char tid = GetThreadId();
    std::lock_guard<std::recursive_mutex> guard(gTraceOut->callMutex);
    char* dest = gTraceOut->writebuf;
    BCall_vlen *pCall = (BCall_vlen*)dest;
    pCall->funcId = glVertexPointer_id;
    pCall->tid = tid; pCall->reserved = 0;
    dest += sizeof(*pCall);

    //_glVertexPointer(size, type, stride, pointer);
    dest = WriteFixed<int>(dest, size); // literal
    dest = WriteFixed<int>(dest, type); // enum
    dest = WriteFixed<int>(dest, stride); // literal
    dest = WriteFixed<unsigned int>(dest, 1); // IS *BLOB*
    dest = Write1DArray<char>(dest, _size, (char*)pointer); // opaque -> blob
    pCall->errNo = GetCallErrorNo("glVertexPointer", tid);
    pCall->toNext = dest-gTraceOut->writebuf;
    gTraceOut->Write(gTraceOut->writebuf, dest-gTraceOut->writebuf);
    gTraceOut->callNo++;
}

void _glNormalPointer_fake(GLenum type, GLsizei stride, const GLvoid * pointer, unsigned int _size)
{
    unsigned char tid = GetThreadId();
    std::lock_guard<std::recursive_mutex> guard(gTraceOut->callMutex);
    char* dest = gTraceOut->writebuf;
    BCall_vlen *pCall = (BCall_vlen*)dest;
    pCall->funcId = glNormalPointer_id;
    pCall->tid = tid; pCall->reserved = 0;
    dest += sizeof(*pCall);

    //_glNormalPointer(type, stride, pointer);
    dest = WriteFixed<int>(dest, type); // enum
    dest = WriteFixed<int>(dest, stride); // literal
    dest = WriteFixed<unsigned int>(dest, 1); // IS *BLOB*
    dest = Write1DArray<char>(dest, _size, (char*)pointer); // opaque -> blob
    pCall->errNo = GetCallErrorNo("glNormalPointer", tid);
    pCall->toNext = dest-gTraceOut->writebuf;
    gTraceOut->Write(gTraceOut->writebuf, dest-gTraceOut->writebuf);
    gTraceOut->callNo++;
}

void _glColorPointer_fake(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer, unsigned int _size)
{
    unsigned char tid = GetThreadId();
    std::lock_guard<std::recursive_mutex> guard(gTraceOut->callMutex);
    char* dest = gTraceOut->writebuf;
    BCall_vlen *pCall = (BCall_vlen*)dest;
    pCall->funcId = glColorPointer_id;
    pCall->tid = tid; pCall->reserved = 0;
    dest += sizeof(*pCall);

    //_glColorPointer(size, type, stride, pointer);
    dest = WriteFixed<int>(dest, size); // literal
    dest = WriteFixed<int>(dest, type); // enum
    dest = WriteFixed<int>(dest, stride); // literal
    dest = WriteFixed<unsigned int>(dest, 1); // IS *BLOB*
    dest = Write1DArray<char>(dest, _size, (char*)pointer); // opaque -> blob
    pCall->errNo = GetCallErrorNo("glColorPointer", tid);
    pCall->toNext = dest-gTraceOut->writebuf;
    gTraceOut->Write(gTraceOut->writebuf, dest-gTraceOut->writebuf);
    gTraceOut->callNo++;
}

void _glTexCoordPointer_fake(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer, unsigned int _size){
    unsigned char tid = GetThreadId();
    std::lock_guard<std::recursive_mutex> guard(gTraceOut->callMutex);
    char* dest = gTraceOut->writebuf;
    BCall_vlen *pCall = (BCall_vlen*)dest;
    pCall->funcId = glTexCoordPointer_id;
    pCall->tid = tid; pCall->reserved = 0;
    dest += sizeof(*pCall);

    //_glTexCoordPointer(size, type, stride, pointer);
    dest = WriteFixed<int>(dest, size); // literal
    dest = WriteFixed<int>(dest, type); // enum
    dest = WriteFixed<int>(dest, stride); // literal
    dest = WriteFixed<unsigned int>(dest, 1); // IS *BLOB*
    dest = Write1DArray<char>(dest, _size, (char*)pointer); // opaque -> blob
    pCall->errNo = GetCallErrorNo("glTexCoordPointer", tid);
    pCall->toNext = dest-gTraceOut->writebuf;
    gTraceOut->Write(gTraceOut->writebuf, dest-gTraceOut->writebuf);
    gTraceOut->callNo++;
}

#if ENABLE_CLIENT_SIDE_BUFFER
void _glVertexAttribPointer_fake(const VertexAttributeMemoryMerger::AttributeInfo *ai, unsigned int name)
{
    unsigned char tid = GetThreadId();
    std::lock_guard<std::recursive_mutex> guard(gTraceOut->callMutex);
    char* dest = gTraceOut->writebuf;
    BCall_vlen *pCall = (BCall_vlen*)dest;
    pCall->funcId = glVertexAttribPointer_id;
    pCall->tid = tid; pCall->reserved = 0;
    dest += sizeof(*pCall);

    //_glVertexAttribPointer(index, size, type, normalized, stride, pointer);
    dest = WriteFixed<unsigned int>(dest, ai->index); // literal
    dest = WriteFixed<int>(dest, ai->size); // literal
    dest = WriteFixed<int>(dest, ai->type); // enum
    dest = WriteFixed<int>(dest, ai->normalized); // enum
    dest = WriteFixed<int>(dest, ai->stride); // literal
    dest = WriteFixed<unsigned int>(dest, ClientSideBufferObjectReferenceType);
    dest = WriteFixed<unsigned int>(dest, (unsigned int)(name));
    dest = WriteFixed<unsigned int>(dest, (unsigned int)(ai->offset));
    pCall->errNo = GetCallErrorNo("glVertexAttribPointer", tid);
    pCall->toNext = dest-gTraceOut->writebuf;
    gTraceOut->Write(gTraceOut->writebuf, dest-gTraceOut->writebuf);
    gTraceOut->callNo++;
}
#else
void _glVertexAttribPointer_fake(GLuint index, GLint size, GLenum type, GLboolean normalized,
    GLsizei stride, const GLvoid *pointer, GLint _size)
{
    unsigned char tid = GetThreadId();
    std::lock_guard<std::recursive_mutex> guard(gTraceOut->callMutex);
    char* dest = gTraceOut->writebuf;
    BCall_vlen *pCall = (BCall_vlen*)dest;
    pCall->funcId = glVertexAttribPointer_id;
    pCall->tid = tid; pCall->reserved = 0;
    dest += sizeof(*pCall);

    //_glVertexAttribPointer(index, size, type, normalized, stride, pointer);
    dest = WriteFixed<unsigned int>(dest, index); // literal
    dest = WriteFixed<int>(dest, size); // literal
    dest = WriteFixed<int>(dest, type); // enum
    dest = WriteFixed<int>(dest, normalized); // enum
    dest = WriteFixed<int>(dest, stride); // literal
    dest = Write1DArray<char>(dest, (unsigned int)_size, (const char*)pointer); // blob
    pCall->errNo = GetCallErrorNo("glVertexAttribPointer", tid);
    pCall->toNext = dest-gTraceOut->writebuf;
    gTraceOut->Write(gTraceOut->writebuf, dest-gTraceOut->writebuf);
    gTraceOut->callNo++;
}
#endif

void _glClientActiveTexture_fake(GLenum texture){
    unsigned char tid = GetThreadId();
    std::lock_guard<std::recursive_mutex> guard(gTraceOut->callMutex);
    char* dest = gTraceOut->writebuf;
    BCall *pCall = (BCall*)dest;
    pCall->funcId = glClientActiveTexture_id;
    pCall->tid = tid; pCall->reserved = 0;
    dest += sizeof(*pCall);

    // _glClientActiveTexture(texture);
    dest = WriteFixed<int>(dest, texture); // enum
    pCall->errNo = GetCallErrorNo("glClientActiveTexture", tid);
    gTraceOut->Write(gTraceOut->writebuf, dest-gTraceOut->writebuf);
    gTraceOut->callNo++;
}

#if ENABLE_CLIENT_SIDE_BUFFER

ClientSideBufferObjectName _glCreateClientSideBuffer()
{
    const unsigned char tid = GetThreadId();
    const ClientSideBufferObjectName name = gTraceOut->mCSBufferSet.create_object(tid);

    std::lock_guard<std::recursive_mutex> guard(gTraceOut->callMutex);
    char* dest = gTraceOut->writebuf;
    BCall *pCall = (BCall*)dest;
    pCall->funcId = glCreateClientSideBuffer_id;
    pCall->tid = tid; pCall->reserved = 0;
    dest += sizeof(*pCall);

    dest = WriteFixed<unsigned int>(dest, name); // literal
    pCall->errNo = GetCallErrorNo("glCreateClientSideBuffer", tid);
    gTraceOut->Write(gTraceOut->writebuf, dest-gTraceOut->writebuf);
    gTraceOut->callNo++;
    return name;
}

void _glDeleteClientSideBuffer(ClientSideBufferObjectName name)
{
    const unsigned char tid = GetThreadId();
    gTraceOut->mCSBufferSet.delete_object(tid, name);

    std::lock_guard<std::recursive_mutex> guard(gTraceOut->callMutex);
    char* dest = gTraceOut->writebuf;
    BCall *pCall = (BCall*)dest;
    pCall->funcId = glDeleteClientSideBuffer_id;
    pCall->tid = tid; pCall->reserved = 0;
    dest += sizeof(*pCall);

    dest = WriteFixed<unsigned int>(dest, name); // literal
    pCall->errNo = GetCallErrorNo("glDeleteClientSideBuffer", tid);
    gTraceOut->Write(gTraceOut->writebuf, dest-gTraceOut->writebuf);
    gTraceOut->callNo++;
}

void _glCopyClientSideBuffer(GLenum target, ClientSideBufferObjectName name)
{
    const unsigned char tid = GetThreadId();

    std::lock_guard<std::recursive_mutex> guard(gTraceOut->callMutex);
    char* dest = gTraceOut->writebuf;
    BCall *pCall = (BCall*)dest;
    pCall->funcId = glCopyClientSideBuffer_id;
    pCall->tid = tid; pCall->reserved = 0;
    dest += sizeof(*pCall);

    dest = WriteFixed<int>(dest, target); // enum
    dest = WriteFixed<unsigned int>(dest, name); // literal
    pCall->errNo = GetCallErrorNo("glCopyClientSideBuffer", tid);
    gTraceOut->Write(gTraceOut->writebuf, dest - gTraceOut->writebuf);
    gTraceOut->callNo++;
}

void _glPatchClientSideBuffer(GLenum target, int length, const void *data)
{
    const unsigned char tid = GetThreadId();

    std::lock_guard<std::recursive_mutex> guard(gTraceOut->callMutex);
    char* dest = gTraceOut->writebuf;
    BCall_vlen *pCall = (BCall_vlen*)dest;
    pCall->funcId = glPatchClientSideBuffer_id;
    pCall->tid = tid; pCall->reserved = 0;
    dest += sizeof(*pCall);

    dest = WriteFixed<int>(dest, target); // enum
    dest = WriteFixed<int>(dest, length); // literal
    dest = Write1DArray<GLubyte>(dest, (unsigned int)(length), (const GLubyte *)(data)); // array
    pCall->errNo = GetCallErrorNo("glPatchClientSideBuffer", tid);
    pCall->toNext = dest-gTraceOut->writebuf;
    gTraceOut->Write(gTraceOut->writebuf, dest - gTraceOut->writebuf);
    gTraceOut->callNo++;
}

void _glClientSideBufferData(ClientSideBufferObjectName name,
    int length, const void *data)
{
    const unsigned char tid = GetThreadId();
    gTraceOut->mCSBufferSet.object_data(tid, name, length, data);

    std::lock_guard<std::recursive_mutex> guard(gTraceOut->callMutex);
    char* dest = gTraceOut->writebuf;
    BCall_vlen *pCall = (BCall_vlen*)dest;
    pCall->funcId = glClientSideBufferData_id;
    pCall->tid = tid; pCall->reserved = 0;
    dest += sizeof(*pCall);

    dest = WriteFixed<unsigned int>(dest, name); // literal
    dest = WriteFixed<int>(dest, length); // literal
    dest = Write1DArray<GLubyte>(dest, (unsigned int)(length), (const GLubyte *)(data)); // array
    pCall->errNo = GetCallErrorNo("glClientSideBufferData", tid);
    pCall->toNext = dest-gTraceOut->writebuf;
    gTraceOut->Write(gTraceOut->writebuf, dest-gTraceOut->writebuf);
    gTraceOut->callNo++;
}

void _glClientSideBufferSubData(ClientSideBufferObjectName name,
    int offset, int length, const char *data)
{
    const unsigned char tid = GetThreadId();
    gTraceOut->mCSBufferSet.object_subdata(tid, name, offset, length, data);

    std::lock_guard<std::recursive_mutex> guard(gTraceOut->callMutex);
    char* dest = gTraceOut->writebuf;
    BCall_vlen *pCall = (BCall_vlen*)dest;
    pCall->funcId = glClientSideBufferSubData_id;
    pCall->tid = tid; pCall->reserved = 0;
    dest += sizeof(*pCall);

    dest = WriteFixed<unsigned int>(dest, name); // literal
    dest = WriteFixed<int>(dest, offset); // literal
    dest = WriteFixed<int>(dest, length); // literal
    dest = Write1DArray<GLubyte>(dest, (unsigned int)(length), (const GLubyte *)(data)); // array
    pCall->errNo = GetCallErrorNo("glClientSideBufferSubData", tid);
    pCall->toNext = dest-gTraceOut->writebuf;
    gTraceOut->Write(gTraceOut->writebuf, dest-gTraceOut->writebuf);
    gTraceOut->callNo++;
}

ClientSideBufferObjectName _getOrCreateClientSideBuffer(const ClientSideBufferObject& obj, bool& created)
{
    const unsigned char tid = GetThreadId();
    ClientSideBufferObjectName name = 0;

    if (gTraceOut->mCSBufferSet.find(tid, obj, name))
    {
        created = false;
        return name;
    }

    // create a new object
    ClientSideBufferObjectName newName = _glCreateClientSideBuffer();
    created = true;
    return newName;
}

ClientSideBufferObjectName _getOrCreateClientSideBuffer(const void *p, ptrdiff_t size, bool& created)
{
    ClientSideBufferObject obj(p, size, false);
    return _getOrCreateClientSideBuffer(obj, created);
}

ClientSideBufferObjectName _glClientSideBufferData(const ClientSideBufferObject *obj)
{
    bool created = false;
    ClientSideBufferObjectName name = _getOrCreateClientSideBuffer(*obj, created);

    if (created)
    {
        _glClientSideBufferData(name, obj->size, obj->base_address);
    }

    return name;
}

ClientSideBufferObjectName _glClientSideBufferData(const void *p, ptrdiff_t size)
{
    ClientSideBufferObject obj(p, size, false);
    return _glClientSideBufferData(&obj);
}

#endif

void GetActiveAttribIdx(GLint prg, unsigned int &flagArray)
{
    const int MAX_VERTEX_ATTRIB_COUNT = 32;
    flagArray = 0x00000000;

    GLint attribCnt = 0;
    _glGetProgramiv(prg, GL_ACTIVE_ATTRIBUTES, &attribCnt);

    if (attribCnt == 0)
        return;

    for (int i = 0; i < attribCnt; ++i)
    {
        GLchar  attribName[128];
        GLsizei attribNameLen = 0;
        GLint   attribSize = 0;
        GLenum  attribType = 0;

        _glGetActiveAttrib(prg, i, 128,
            &attribNameLen, &attribSize, &attribType, attribName);

        GLint attribLoc = _glGetAttribLocation(prg, attribName);
        if (attribLoc >= 0 && attribLoc < MAX_VERTEX_ATTRIB_COUNT)
        {
            int locationBits = numBits(numLocations(attribType));
            flagArray |= (locationBits << attribLoc);
        }
    }
}

bool _need_user_arrays()
{
    unsigned char tid = GetThreadId();

    if (GetCurTraceContext(tid)->profile == 1) {
        // glVertexPointer
        if (_glIsEnabled(GL_VERTEX_ARRAY)) {
            GLint _binding = 0;
            _glGetIntegerv(GL_VERTEX_ARRAY_BUFFER_BINDING, &_binding);
            if (!_binding) {
                return true;
            }
        }
        // glNormalPointer
        if (_glIsEnabled(GL_NORMAL_ARRAY)) {
            GLint _binding = 0;
            _glGetIntegerv(GL_NORMAL_ARRAY_BUFFER_BINDING, &_binding);
            if (!_binding) {
                return true;
            }
        }
        // glColorPointer
        if (_glIsEnabled(GL_COLOR_ARRAY)) {
            GLint _binding = 0;
            _glGetIntegerv(GL_COLOR_ARRAY_BUFFER_BINDING, &_binding);
            if (!_binding) {
                return true;
            }
        }
        // glTexCoordPointer
        GLint client_active_texture = 0;
        _glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &client_active_texture);
        GLint max_texture_coords = 0;
        _glGetIntegerv(GL_MAX_TEXTURE_UNITS, &max_texture_coords);
        for (GLint unit = 0; unit < max_texture_coords; ++unit) {
            GLint texture = GL_TEXTURE0 + unit;
            _glClientActiveTexture(texture);
            if (_glIsEnabled(GL_TEXTURE_COORD_ARRAY)) {
                GLint _binding = 0;
                _glGetIntegerv(GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING, &_binding);
                if (!_binding) {
                    _glClientActiveTexture(client_active_texture);
                    return true;
                }
            }
        }
        _glClientActiveTexture(client_active_texture);

        return false;
    }

    GLint maxVertexAttribs = GetCurTraceContext(tid)->mMaxVertexAttribs;
    for (GLint index = 0; index < maxVertexAttribs; ++index)
    {
        GLint _enabled = 0;
        _glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &_enabled);
        if (_enabled)
        {
            GLint _binding = 0;
            _glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &_binding);
            if (!_binding)
            {
                return true;
            }
        }
    }

    return false;
}

void _trace_user_arrays(int count, int instancecount)
{
    unsigned char tid = GetThreadId();

    if (GetCurTraceContext(tid)->profile == 1) {
        // void GLES_CALLCONVENTION glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
        if(_glIsEnabled(GL_VERTEX_ARRAY)) {
            GLint _binding = 0;
            _glGetIntegerv(GL_VERTEX_ARRAY_BUFFER_BINDING, &_binding);
            if (!_binding) {
                GLint size = 0;
                _glGetIntegerv(GL_VERTEX_ARRAY_SIZE, &size);
                GLint type = 0;
                _glGetIntegerv(GL_VERTEX_ARRAY_TYPE, &type);
                GLint stride = 0;
                _glGetIntegerv(GL_VERTEX_ARRAY_STRIDE, &stride);
                GLvoid * pointer = 0;
                _glGetPointerv(GL_VERTEX_ARRAY_POINTER, &pointer);
                size_t _size = _glVertexPointer_size(size, type, stride, count);
                _glVertexPointer_fake(size, type, stride, pointer, _size);
            }
        }
        // void GLES_CALLCONVENTION glNormalPointer(GLenum type, GLsizei stride, const GLvoid * pointer)
        if(_glIsEnabled(GL_NORMAL_ARRAY)) {
            GLint _binding = 0;
            _glGetIntegerv(GL_NORMAL_ARRAY_BUFFER_BINDING, &_binding);
            if (!_binding) {
                GLint type = 0;
                _glGetIntegerv(GL_NORMAL_ARRAY_TYPE, &type);
                GLint stride = 0;
                _glGetIntegerv(GL_NORMAL_ARRAY_STRIDE, &stride);
                GLvoid * pointer = 0;
                _glGetPointerv(GL_NORMAL_ARRAY_POINTER, &pointer);
                size_t _size = _glNormalPointer_size(type, stride, count);
                _glNormalPointer_fake(type, stride, pointer, _size);
            }
        }
        // void GLES_CALLCONVENTION glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
        if(_glIsEnabled(GL_COLOR_ARRAY)) {
            GLint _binding = 0;
            _glGetIntegerv(GL_COLOR_ARRAY_BUFFER_BINDING, &_binding);
            if (!_binding) {
                GLint size = 0;
                _glGetIntegerv(GL_COLOR_ARRAY_SIZE, &size);
                GLint type = 0;
                _glGetIntegerv(GL_COLOR_ARRAY_TYPE, &type);
                GLint stride = 0;
                _glGetIntegerv(GL_COLOR_ARRAY_STRIDE, &stride);
                GLvoid * pointer = 0;
                _glGetPointerv(GL_COLOR_ARRAY_POINTER, &pointer);
                size_t _size = _glColorPointer_size(size, type, stride, count);
                _glColorPointer_fake(size, type, stride, pointer, _size);
            }
        }
        // void GLES_CALLCONVENTION glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
        bool client_active_texture_dirty = false;
        GLint client_active_texture = 0;
        _glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &client_active_texture);
        GLint active_texture = 0;
        _glGetIntegerv(GL_ACTIVE_TEXTURE, &active_texture);
        GLint max_texture_coords = 0;
        _glGetIntegerv(GL_MAX_TEXTURE_UNITS, &max_texture_coords);
        for (GLint unit = 0; unit < max_texture_coords; ++unit) {
            GLint texture = GL_TEXTURE0 + unit;
            _glActiveTexture(texture);

            if(_glIsEnabled(GL_TEXTURE_2D) || _glIsEnabled(GL_TEXTURE_3D) || _glIsEnabled(GL_TEXTURE_CUBE_MAP)) {
                GLint _binding = 0;
                _glGetIntegerv(GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING, &_binding);
                if (!_binding) {
                    GLint size = 0;
                    _glGetIntegerv(GL_TEXTURE_COORD_ARRAY_SIZE, &size);
                    GLint type = 0;
                    _glGetIntegerv(GL_TEXTURE_COORD_ARRAY_TYPE, &type);
                    GLint stride = 0;
                    _glGetIntegerv(GL_TEXTURE_COORD_ARRAY_STRIDE, &stride);
                    GLvoid * pointer = 0;
                    _glGetPointerv(GL_TEXTURE_COORD_ARRAY_POINTER, &pointer);
                    size_t _size = _glTexCoordPointer_size(size, type, stride, count);

                    if (texture != client_active_texture || client_active_texture_dirty) {
                        client_active_texture_dirty = true;
                        _glClientActiveTexture_fake(texture);
                    }

                    _glTexCoordPointer_fake(size, type, stride, pointer, _size);
                }
            }
        }
        _glActiveTexture(active_texture);
        _glClientActiveTexture(client_active_texture);
        if (client_active_texture_dirty) {
            _glClientActiveTexture_fake(client_active_texture);
        }

        return;
    }

    // void GLES_CALLCONVENTION glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer)
    unsigned int activeAttribArray = 0xffffffff;
    if (tracerParams.EnableActiveAttribCheck == true) {
        GLint currentPrg = 0;
        _glGetIntegerv(GL_CURRENT_PROGRAM, &currentPrg);
            if (currentPrg != 0)
                GetActiveAttribIdx(currentPrg, activeAttribArray);
    }

#if ENABLE_CLIENT_SIDE_BUFFER
    VertexAttributeMemoryMerger mbc;
#endif
    GLint _max_vertex_attribs = 0;
    _glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &_max_vertex_attribs);
    for (GLint index = 0; index < _max_vertex_attribs; ++index)
    {
        GLint _enabled = 0;
        _glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &_enabled);

        if (!_enabled)
        {
            continue;
        }

        if (!(activeAttribArray & (0x1 << index)))
        {
            continue;
        }

        GLint _binding = 0;
        _glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &_binding);

        if (_binding)
        {
            continue;
        }

        GLint size = 0;
        _glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_SIZE, &size);

        GLint type = 0;
        _glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_TYPE, &type);

        GLint normalized = 0;
        _glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &normalized);

        GLint stride = 0;
        _glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &stride);

        GLvoid * pointer = 0;
        _glGetVertexAttribPointerv(index, GL_VERTEX_ATTRIB_ARRAY_POINTER, &pointer);

        GLint divisor = 0;
        if (instancecount)
        {
            // No need to check if in a GLES3 profile, since instancecount
            // will always be 0 in GLES2. GL_VERTEX_ATTRIB_ARRAY_DIVISOR
            // would have resulted in GL_INVALID_ENUM if queried in a
            // GLES2 profile.
            _glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_DIVISOR, &divisor);
        }

        size_t _size = 0;
        if (!divisor)
        {
            // In GLES2 we will always perform this clause, since divisor will
            // always be 0 in GLES2.
            _size = _glVertexAttribPointer_size(size, type, normalized, stride, count);
        }
        else
        {
            _size = _instanced_index_array_size(size, type, stride, instancecount, divisor);
        }

#if ENABLE_CLIENT_SIDE_BUFFER
        mbc.add_attribute(index, pointer, _size, size, type, normalized, stride);
#else
        _glVertexAttribPointer_fake(index, size, type, normalized, stride, pointer, _size);
#endif
    }

#if ENABLE_CLIENT_SIDE_BUFFER
    // If necessary, inject glCreateClientSideBuffer and glClientSideBufferSubData
    const unsigned int mb_count = mbc.memory_range_count();
    std::map<const void *, unsigned int> address_to_name;
    for (unsigned int i = 0; i < mb_count; ++i)
    {
        const ClientSideBufferObject* mb = mbc.memory_range(i);
        unsigned int name = _glClientSideBufferData(mb);
        address_to_name[mb->base_address] = name;
    }

    // If necessary, inject glVertexAttribPointer calls for client side vertex attributes
    const unsigned int attr_count = mbc.attribute_count();
    if (attr_count)
    {
        GLint originallyBoundBuffer = 0;
        _glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &originallyBoundBuffer);

        // Make sure that 0 is bound to GL_ARRAY_BUFFER when injecting
        // glVertexAttribPointer below.
        if (originallyBoundBuffer)
        {
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        for (unsigned int i = 0; i < attr_count; ++i)
        {
            const VertexAttributeMemoryMerger::AttributeInfo *ai = mbc.attribute(i);
            std::map<const void *, unsigned int>::iterator found = address_to_name.find(ai->base_address);
            _glVertexAttribPointer_fake(ai, found->second);
        }

        if (originallyBoundBuffer)
        {
            glBindBuffer(GL_ARRAY_BUFFER, originallyBoundBuffer);
        }
    }
#endif
}

namespace {

static void cleanup()
{
    delete gTraceOut;
    gTraceOut = NULL;
    isUsingPBO = false;
}

static void ExceptionCallback()
{
    cleanup();
}

class DllLoadUnloadHandler
{
public:
    DllLoadUnloadHandler()
    {
        const char* blueOnBlack = "\033[34;40m";
        const char* resetColor = "\033[00;00m";
        DBG_LOG("%sPaTrace init version: " PATRACE_VERSION "%s\n", blueOnBlack, resetColor);
        gTraceOut = new TraceOut;

        os::setExceptionCallback(ExceptionCallback);
    }

    ~DllLoadUnloadHandler()
    {
        const char* blueOnBlack = "\033[33;40m";
        const char* resetColor = "\033[00;00m";
        DBG_LOG("%sPaTrace de-init%s\n", blueOnBlack, resetColor);
        cleanup();

        os::resetExceptionCallback();
    }
};

/// Singleton class
static DllLoadUnloadHandler instance;

}
