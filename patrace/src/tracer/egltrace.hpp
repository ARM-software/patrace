#ifndef _TRACER_EGLTRACE_H_
#define _TRACER_EGLTRACE_H_

#include <tracer/tracerparams.hpp>
#include "tracer/path.hpp"

#include <dispatch/eglproc_auto.hpp>

#include <common/out_file.hpp>
#include <common/os_string.hpp>
#include "common/trace_limits.hpp"
#include <common/my_egl_attribs.hpp>
#include "common/memory.hpp"
#include "helper/states.h"

#include <mutex>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <fstream>

#define ENABLE_CLIENT_SIDE_BUFFER 1

extern bool isUsingPBO;

class BinAndMeta {
public:

    BinAndMeta();
    ~BinAndMeta();

    void writeHeader(bool cleanExit);

    inline void write(const void* buf, unsigned int len)
    {
        traceFile->Write(buf, len);
    }

    void saveExtensions();
    void saveAllEGLConfigs(EGLDisplay dpy);
    void updateWinSurfSize(EGLint width, EGLint height);
    std::string getFileName() const;

    unsigned int winSurWidth = 0;
    unsigned int winSurHeight = 0;
    std::map<unsigned int, unsigned int> texCompressFormats;
    unsigned int glesVersion = 0;

    // config that was last made current
    MyEGLAttribs            currentEGLConfig;

    unsigned callCnt = 0;
    unsigned frameCnt = 0;

    struct CaptureInfo {
        std::string extensions;
        std::string vendor;
        std::string renderer;
        std::string version;
        std::string shaderversion;
        bool valid;
        CaptureInfo () : valid (false) { }
    };
    CaptureInfo captureInfo;

private:
    // The binary trace file
    common::OutFile*        traceFile;
};

class TraceOut {
public:
    BinAndMeta* mpBinAndMeta = nullptr;
    common::ClientSideBufferObjectSet mCSBufferSet;

    // The buffer and mutex used during capturing
    const static int WRITE_BUF_LEN = (150*1024*1024);
    char* writebuf = nullptr;
    std::recursive_mutex callMutex;

    unsigned callNo = 0;
    unsigned frameNo = 0;
    bool snapDraw = false;
    long long mFrameBegTime = 0;

    TraceOut();
    ~TraceOut();

    inline void Write(const void* buf, unsigned int len)
    {
        if (mpBinAndMeta == NULL)
        {
            mpBinAndMeta = new BinAndMeta();
            mStateLogger.open(mpBinAndMeta->getFileName() + ".tracelog");
        }
        mpBinAndMeta->write(buf, len);
    }

    inline void WriteBuf(const char *endPointer)
    {
        const int size = endPointer - writebuf;
        if (size > WRITE_BUF_LEN)
        {
            DBG_LOG("Write buffer overflow (%d > %d)\n", size, WRITE_BUF_LEN);
            abort(); // we've already overwritten memory, no way to recover
        }
        Write(writebuf, size);
    }

    void Close()
    {
        if (mpBinAndMeta)
        {
            mpBinAndMeta->callCnt = callNo;
            mpBinAndMeta->frameCnt = frameNo;
            mStateLogger.close();
            delete mpBinAndMeta;
            mpBinAndMeta = NULL;
        }
        callNo = 0;
        frameNo = 0;
        snapDraw = false;
    }

    StateLogger& getStateLogger() { return mStateLogger; }

private:
    StateLogger mStateLogger;
};

extern TraceOut* gTraceOut;


struct BufferRangeData
{
    unsigned int length;
    void* base;
    GLbitfield access;
    std::vector<unsigned char> contents;
};

typedef std::unordered_map<GLuint, BufferRangeData> BufferToClientPointerMap_t;
typedef std::unordered_set<GLuint> BufferInitializedSet_t;
typedef std::vector<std::string> StringList_t;
typedef std::vector<StringList_t> StringListList_t;

struct TraceContext
{
    /// GLES version set for this context
    int profile = 0;
    std::map<unsigned int, unsigned int> mapPrgToBoundAttribs;
    BufferToClientPointerMap_t bufferToClientPointerMap;
    BufferInitializedSet_t bufferInitializedSet;
    /// Register a pending/lazy deletion of context
    bool isEglDestroyContextInvoked = false;
    /// Whether the currently mapped buffer is the whole buffer or just a range
    bool isFullMapping = false;
    GLenum lastGlError = GL_NO_ERROR;
    EGLContext mEGLCtx;
    EGLint mEGLConfigId;
    GLint mMaxVertexAttribs = 0;

    TraceContext(EGLContext ctx, EGLint configId);
    ~TraceContext();
};

struct TraceSurface
{
    EGLSurface mEGLSurf;
    EGLint mEGLConfigId;
    bool isEglDestroySurfaceInvoked = false;

    TraceSurface(EGLSurface surf, EGLint configId);
    ~TraceSurface();
};

typedef std::map<EGLImageKHR, GLuint> EGLImageToTextureIdMap_t;

struct SizeTargetAndAttrib {
    int width, height;
    EGLenum target;
    EGLint attrib_list[1024];
    SizeTargetAndAttrib() : width(0), height(0), target(0) {
        attrib_list[0] = 0;
    }
    SizeTargetAndAttrib(int w, int h, EGLenum t, const EGLint *al) : width(w), height(h), target(t) {
        int i = 0;
        if (al != NULL) {
            for (; al[i] != EGL_NONE; ++i)
                attrib_list[i] = al[i];
        }
        attrib_list[i] = EGL_NONE;
    }
};
typedef std::unordered_map<EGLImageKHR, SizeTargetAndAttrib> EGLImageInfoMap_t;
struct TraceThread {
    TraceThread()
        : mCurCtx(NULL)
        , mCurSurf(NULL)
        , mCallDepth(0)
        , mActiveAttributes()
        , mEglImageToTextureIdMap()
    {}

    TraceContext *mCurCtx;
    TraceSurface *mCurSurf;
    int mCallDepth;
    StringListList_t mActiveAttributes;
    EGLImageToTextureIdMap_t mEglImageToTextureIdMap;
    EGLImageInfoMap_t mEglImageInfoMap;
};

extern std::vector<TraceThread> gTraceThread;

unsigned char GetThreadId();
void UpdateTimesEGLConfigUsed(int threadid);
TraceContext* GetCurTraceContext(unsigned char tid);
TraceSurface* GetCurTraceSurface(unsigned char tid);

void after_glBindAttribLocation(unsigned char tid, GLuint program, GLuint index);
void after_glMapBufferRange(GLenum target, GLsizeiptr length, GLbitfield access, void* base);
void after_glCreateProgram(unsigned char tid, GLuint program);
void pre_glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length);
void pre_glUnmapBuffer(GLenum target);
void after_glUnmapBuffer(GLenum target);
bool pre_glLinkProgram(unsigned char tid, unsigned int program);
GLuint replace_glCreateShaderProgramv(unsigned char tid, GLenum type, GLsizei count, const GLchar * const * strings);
void after_glDeleteProgram(unsigned char tid, GLuint program);
void after_glDraw();
void after_glLinkProgram(unsigned int program);

void after_eglInitialize(EGLDisplay dpy);
void after_eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLSurface surf, EGLint x, EGLint y, EGLint width, EGLint height);
void after_eglCreateContext(EGLContext ctx, EGLDisplay dpy, EGLConfig config, const EGLint * attrib_list);
void after_eglMakeCurrent(EGLDisplay dpy, EGLSurface drawSurf, EGLContext ctx);
void after_eglDestroyContext(EGLContext ctx);
void pre_eglSwapBuffers();
void after_eglSwapBuffers();
void after_eglDestroySurface(EGLSurface surf);
GLuint pre_eglCreateImageKHR(EGLImageKHR image, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list);
GLuint pre_eglCreateImage(EGLImageKHR image, EGLenum target, EGLClientBuffer buffer, const EGLAttrib *attrib_list);
GLuint pre_glEGLImageTargetTexture2DOES(EGLImageKHR image, EGLint *&attrib_list);
void after_eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR image);
void after_eglDestroyImage(EGLDisplay dpy, EGLImageKHR image);

// The purpose of this funciton is to keep track of what functionality
// that the application uses. This is a better way than testing for GLES
// version inside functions.
void updateUsage(GLenum target);

static inline common::CALL_ERROR_NO GetCallErrorNo(const char *funcname, unsigned char tid)
{
    if (!tracerParams.EnableErrorCheck)
        return common::CALL_GL_NO_ERROR;

    GLenum glErr = _glGetError();
    GetCurTraceContext(tid)->lastGlError = glErr;

#if defined(REPORT_GL_ERRORS)
    if (glErr != GL_NO_ERROR)
    {
        DBG_LOG("GLError from call to %s: 0x%x\n", funcname, glErr);
    }
#endif

    switch (glErr) {
    case GL_NO_ERROR:
        return common::CALL_GL_NO_ERROR;
    case GL_INVALID_ENUM:
        return common::CALL_GL_INVALID_ENUM;
    case GL_INVALID_VALUE:
        return common::CALL_GL_INVALID_VALUE;
    case GL_INVALID_OPERATION:
        return common::CALL_GL_INVALID_OPERATION;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return common::CALL_GL_INVALID_FRAMEBUFFER_OPERATION;
    case GL_OUT_OF_MEMORY:
        return common::CALL_GL_OUT_OF_MEMORY;
    }
    DBG_LOG("Error: unknown error no from glGetError: %x\n", glErr);
    return common::CALL_GL_NO_ERROR;
}

__eglMustCastToProperFunctionPointerType _wrapProcAddress(
    const char * procName, __eglMustCastToProperFunctionPointerType procPtr);
bool _need_user_arrays();
void _trace_user_arrays(int maxindex, int instancecount = 0);
#if ENABLE_CLIENT_SIDE_BUFFER
common::ClientSideBufferObjectName _getOrCreateClientSideBuffer(const common::ClientSideBufferObject& obj, bool& created);
common::ClientSideBufferObjectName _getOrCreateClientSideBuffer(const void *p, ptrdiff_t size, bool& created);
unsigned int _glClientSideBufferData(const void *p, ptrdiff_t size);
void _glClientSideBufferData(common::ClientSideBufferObjectName name, int length, const void *data);
void _glCopyClientSideBuffer(GLenum target, common::ClientSideBufferObjectName name);
void _glPatchClientSideBuffer(GLenum target, int length, const void *data);
common::ClientSideBufferObjectName _glCreateClientSideBuffer();
void _glDeleteClientSideBuffer(common::ClientSideBufferObjectName name);
#endif
void _pa_MandatoryExtensions();

#endif
