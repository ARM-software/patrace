#ifndef _RETRACER_STATE_HPP_
#define _RETRACER_STATE_HPP_

#include <unordered_map>
#include <vector>
#include <string>
#include <set>

#include <common/trace_limits.hpp>
#include <common/gl_utility.hpp>
#include <retracer/value_map.hpp>
#include <retracer/retrace_options.hpp> // enum Profile
#include "dispatch/eglimports.hpp"
#include "graphic_buffer/GraphicBuffer.hpp"
#include <map>
#include <stdint.h>
#include "dma_buffer/dma_buffer.hpp"

class OffscreenManager;

namespace retracer {

typedef std::unordered_map<GLuint, void*> BufferIdToData_t;

struct Rectangle
{
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;

    inline Rectangle& operator=(const Rectangle& b) {
        x = b.x;
        y = b.y;
        w = b.w;
        h = b.h;
        return *this;
    }

    inline Rectangle Stretch(float sX, float sY) const {
        Rectangle tmp;

        tmp.x = static_cast<int>(x * sX);
        tmp.w = static_cast<int>(w * sX);

        tmp.y = static_cast<int>(y * sY);
        tmp.h = static_cast<int>(h * sY);

        return tmp;
    }

    inline bool operator==(const Rectangle& b) const {
        return
            x == b.x &&
            y == b.y &&
            w == b.w &&
            h == b.h;
    }
    inline bool operator!=(const Rectangle& b) const {
        return !(*this == b);
    }
};

class Drawable
{
public:
    int width;
    int height;
    int winWidth;
    int winHeight;
    bool visible;
    float mOverrideResRatioW = 0.0f;
    float mOverrideResRatioH = 0.0f;

    Drawable(int w, int h) :
        width(w),
        height(h),
        winWidth(w),
        winHeight(h),
        visible(false)
    {
        refcnt = 1;
    }

    virtual ~Drawable() {}

    void retain()
    {
        refcnt++;
    }
    void release()
    {
        int left = 0;
        left = --refcnt;
        if (left <= 0)
            delete this;
    }

    virtual void resize(int w, int h)
    {
        width = w;
        height = h;
    }

    virtual void show(void)
    {
        visible = true;
    }

    virtual void close(void)
    {
        visible = false;
    }

    virtual void swapBuffers(void) = 0;
    virtual void swapBuffersWithDamage(int *rects, int n_rects) = 0;
    virtual void setDamage(int* array, int length) {}
    virtual void querySurface(int attribute, int* value) {}

private:
    int refcnt;
};

class PbufferDrawable : public Drawable
{
public:
    PbufferDrawable(EGLint const* attribList)
        : Drawable(0, 0)
        , largestPbuffer(EGL_FALSE)
        , mipmapTexture(EGL_FALSE)
        , textureTarget(EGL_NO_TEXTURE)
        , vgAlphaFormat(EGL_VG_ALPHA_FORMAT_NONPRE)
        , texFormat(EGL_NO_TEXTURE)
    {
    }

    int largestPbuffer;
    int mipmapTexture;
    int textureTarget;
    int vgAlphaFormat;
    int texFormat;
};

class Context
{
public:
    Context(Profile prof, Context* shareContext)
        : _profile(prof)
        , _current_program(0)
        , _current_framebuffer(0)
        , _firstTimeMakeCurrent(true)
        , _offscrMgr(0)
        , _shareContext(shareContext)
    {
        _texture_map.LValue(0) = 0;
        _buffer_map.LValue(0) = 0;
        _program_map.LValue(0) = 0;
        _shader_map.LValue(0) = 0;
        _renderbuffer_map.LValue(0) = 0;
        _sampler_map.LValue(0) = 0;
        _query_map.LValue(0) = 0;
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        _framebuffer_map.LValue(0) = 1;
#else
        _framebuffer_map.LValue(0) = 0;
#endif
        _array_map.LValue(0) = 0;
        _feedback_map.LValue(0) = 0;
        _graphicbuffer_map.LValue(0) = 0;

        if (shareContext != NULL)
        {
            shareContext->retain();
        }

        refcnt = 1;
        mAndroidToLinuxPixelMap[HAL_PIXEL_FORMAT_YV12] = DRM_FORMAT_YVU420;
        mAndroidToLinuxPixelMap[MALI_GRALLOC_FORMAT_INTERNAL_NV12] = DRM_FORMAT_NV12;
    }

    virtual ~Context();

    void retain()
    {
        refcnt++;
    }
    void release()
    {
        if (--refcnt <= 0)
            delete this;
    }

    std::unordered_map<unsigned int, locationmap >& getUniformlocationMap();
    hmap<unsigned int>& getTextureMap();
    hmap<unsigned int>& getTextureRevMap();

    hmap<unsigned int>& getBufferMap();
    hmap<unsigned int>& getBufferRevMap();

    hmap<unsigned int>& getProgramMap();
    hmap<unsigned int>& getProgramRevMap();

    hmap<unsigned int>& getShaderMap();
    hmap<unsigned int>& getShaderRevMap();

    hmap<unsigned int>& getFramebufferMap();
    hmap<unsigned int>& getFramebufferRevMap();

    hmap<unsigned int>& getRenderbufferMap();
    hmap<unsigned int>& getRenderbufferRevMap();

    hmap<unsigned int>& getSamplerMap();
    hmap<unsigned int>& getSamplerRevMap();

    stdmap<unsigned long long, GLsync>& getSyncMap();

    hmap<unsigned int>& getGraphicBufferMap();

    Profile _profile;

    hmap<unsigned int> _list_map;
    hmap<unsigned int> _query_map;
    std::unordered_map<unsigned int, locationmap > _uniformLocation_map;
    std::unordered_map<unsigned int, stdmap<unsigned int, unsigned int> > _uniformBlockIndex_map;
    hmap<unsigned int> _framebuffer_map;
    hmap<unsigned int> _array_map;
    hmap<unsigned int> _feedback_map;
    hmap<unsigned int> _pipeline_map;
    hmap<unsigned int> _region_map;
    // Reverse mappings - used for debugging/analyzing traces
    hmap<unsigned int> _query_rev_map;
    hmap<unsigned int> _framebuffer_rev_map;
    hmap<unsigned int> _array_rev_map;
    hmap<unsigned int> _feedback_rev_map;
    hmap<unsigned int> _pipeline_rev_map;

    BufferIdToData_t _bufferToData_map;

    unsigned int      _current_program;
    unsigned int      _current_framebuffer;
    bool              _firstTimeMakeCurrent;
    OffscreenManager* _offscrMgr;
#ifdef ANDROID
    std::vector<GraphicBuffer *> mGraphicBuffers;
    std::vector<HardwareBuffer *> mHardwareBuffers;
#else
    std::vector<egl_image_fixture *> mGraphicBuffers;
#endif
    std::unordered_map<PixelFormat, unsigned int, std::hash<int> > mAndroidToLinuxPixelMap;

    inline const std::string& getShaderSource(GLuint shader) const
    {
        if (_shareContext) return _shareContext->getShaderSource(shader);
        else return mShaderSources.at(shader);
    }
    inline void setShaderSource(GLuint shader, const std::string& source)
    {
        if (_shareContext) _shareContext->setShaderSource(shader, source);
        else mShaderSources[shader] = source;
    }
    inline void deleteShader(GLuint shader)
    {
        if (_shareContext) _shareContext->deleteShader(shader);
        else mShaderSources.erase(shader);
    }

    inline const std::vector<GLuint>& getShaderIDs(GLuint program) const
    {
        if (_shareContext) return _shareContext->getShaderIDs(program);
        else return mProgramShaders.at(program);
    }
    inline void addShaderID(GLuint program, GLuint shader)
    {
        if (_shareContext) _shareContext->addShaderID(program, shader);
        else mProgramShaders[program].push_back(shader);
    }
    inline void deleteShaderIDs(GLuint program)
    {
        if (_shareContext) _shareContext->deleteShaderIDs(program);
        else mProgramShaders.erase(program);
    }

private:
    Context* _shareContext;
    std::unordered_map<GLuint, std::string> mShaderSources; // shader id to string; shared
    std::unordered_map<GLuint, std::vector<GLuint>> mProgramShaders; // program id to list of shader ids; shared
    int refcnt;
    hmap<unsigned int> _texture_map; // shared
    hmap<unsigned int> _buffer_map; // shared
    hmap<unsigned int> _program_map; // shared
    hmap<unsigned int> _shader_map; // shared
    hmap<unsigned int> _renderbuffer_map; // shared
    hmap<unsigned int> _sampler_map; // shared
    stdmap<unsigned long long, GLsync> _sync_map; // shared
    // Reverse mappings - used for debugging/analyzing traces
    hmap<unsigned int> _texture_rev_map; // shared
    hmap<unsigned int> _buffer_rev_map; // shared
    hmap<unsigned int> _program_rev_map; // shared
    hmap<unsigned int> _shader_rev_map; // shared
    hmap<unsigned int> _renderbuffer_rev_map; // shared
    hmap<unsigned int> _sampler_rev_map; // shared
    hmap<unsigned int> _graphicbuffer_map; // shared
};

inline std::unordered_map<unsigned int, locationmap >& Context::getUniformlocationMap()
{
    if (_shareContext)
    {
        return _shareContext->getUniformlocationMap();
    }
    else
    {
        return _uniformLocation_map;
    }
}

inline hmap<unsigned int>& Context::getTextureMap()
{
    if (_shareContext)
    {
        return _shareContext->getTextureMap();
    }
    else
    {
        return _texture_map;
    }
}

inline hmap<unsigned int>& Context::getTextureRevMap()
{
    if (_shareContext)
    {
        return _shareContext->getTextureRevMap();
    }
    else
    {
        return _texture_rev_map;
    }
}

inline hmap<unsigned int>& Context::getBufferMap()
{
    if (_shareContext)
    {
        return _shareContext->getBufferMap();
    }
    else
    {
        return _buffer_map;
    }
}

inline hmap<unsigned int>& Context::getBufferRevMap()
{
    if (_shareContext)
    {
        return _shareContext->getBufferRevMap();
    }
    else
    {
        return _buffer_rev_map;
    }
}

inline hmap<unsigned int>& Context::getProgramMap()
{
    if (_shareContext)
    {
        return _shareContext->getProgramMap();
    }
    else
    {
        return _program_map;
    }
}

inline hmap<unsigned int>& Context::getProgramRevMap()
{
    if (_shareContext)
    {
        return _shareContext->getProgramRevMap();
    }
    else
    {
        return _program_rev_map;
    }
}

inline hmap<unsigned int>& Context::getShaderMap()
{
    if (_shareContext)
    {
        return _shareContext->getShaderMap();
    }
    else
    {
        return _shader_map;
    }
}

inline hmap<unsigned int>& Context::getShaderRevMap()
{
    if (_shareContext)
    {
        return _shareContext->getShaderRevMap();
    }
    else
    {
        return _shader_rev_map;
    }
}

inline hmap<unsigned int>& Context::getRenderbufferMap()
{
    if (_shareContext)
    {
        return _shareContext->getRenderbufferMap();
    }
    else
    {
        return _renderbuffer_map;
    }
}

inline hmap<unsigned int>& Context::getRenderbufferRevMap()
{
    if (_shareContext)
    {
        return _shareContext->getRenderbufferRevMap();
    }
    else
    {
        return _renderbuffer_rev_map;
    }
}

inline hmap<unsigned int>& Context::getFramebufferMap()
{
    if (_shareContext)
    {
        return _shareContext->getFramebufferMap();
    }
    else
    {
        return _framebuffer_map;
    }
}

inline hmap<unsigned int>& Context::getFramebufferRevMap()
{
    if (_shareContext)
    {
        return _shareContext->getFramebufferRevMap();
    }
    else
    {
        return _framebuffer_rev_map;
    }
}

inline hmap<unsigned int>& Context::getSamplerMap()
{
    if (_shareContext)
    {
        return _shareContext->getSamplerMap();
    }
    else
    {
        return _sampler_map;
    }
}

inline hmap<unsigned int>& Context::getSamplerRevMap()
{
    if (_shareContext)
    {
        return _shareContext->getSamplerRevMap();
    }
    else
    {
        return _sampler_rev_map;
    }
}

inline hmap<unsigned int>& Context::getGraphicBufferMap()
{
    if (_shareContext)
    {
        return _shareContext->getGraphicBufferMap();
    }
    else
    {
        return _graphicbuffer_map;
    }
}

inline stdmap<unsigned long long, GLsync>& Context::getSyncMap()
{
    if (_shareContext)
    {
        return _shareContext->getSyncMap();
    }
    else
    {
        return _sync_map;
    }
}

class GLESThread
{
public:
    void                *mpJavaEnv;

    // for override resolution
    Rectangle           mCurAppVP; // viewport
    Rectangle           mCurAppSR; // scissor rect

    Rectangle           mCurDrvVP; // override viewport
    Rectangle           mCurDrvSR; // override scissor

    GLESThread():
        mpJavaEnv(NULL),
        mpCurrentContext(NULL),
        mpCurrentDrawable(NULL)
    {}

    void Reset() {
        mpJavaEnv = NULL;
        mpCurrentContext = NULL;
        mpCurrentDrawable = NULL;
    }

    void setContext(Context *ctx)
    {
        if (ctx != mpCurrentContext)
        {
            if (mpCurrentContext != NULL)
                mpCurrentContext->release();

            if (ctx != NULL)
                ctx->retain();

            mpCurrentContext = ctx;
        }
    }

    inline Context* getContext() const
    {
        return mpCurrentContext;
    }

    void setDrawable(Drawable* d)
    {
        if (d != mpCurrentDrawable)
        {
            if (mpCurrentDrawable != NULL)
                mpCurrentDrawable->release();

            if (d != NULL)
                d->retain();

            mpCurrentDrawable = d;
        }
    }

    inline Drawable* getDrawable() const
    {
        return mpCurrentDrawable;
    }

private:
    Context             *mpCurrentContext;
    Drawable            *mpCurrentDrawable;
};


class StateMgr
{
public:
    StateMgr();
    ~StateMgr() {}
    void        Reset();

    void        InsertContextMap(int oldVal, Context* ctx);
    Context*    GetContext(int oldVal) const;
    void        RemoveContextMap(int oldVal);
    int         GetCtx(Context *) const;

    void        InsertDrawableMap(int oldVal, Drawable* drawable);
    Drawable*   GetDrawable(int oldVal) const;
    void        RemoveDrawableMap(int oldVal);
    bool        IsInDrawableMap(Drawable* drawable) const;
    int         GetDraw(Drawable *) const;

    void        InsertEGLImageMap(int oldVal, EGLImageKHR image);
    EGLImageKHR GetEGLImage(int oldVal, bool &found) const;
    void        RemoveEGLImageMap(int oldVal);
    bool        IsInEGLImageMap(EGLImageKHR image) const;

    void        InsertDrawableToWinMap(int drawableVal, int winVal);
    int         GetWin(int draableVal) const;

    std::vector<GLESThread> mThreadArr;
    Drawable* mSingleSurface;
    bool mForceSingleWindow;
    EGLDisplay mEglDisplay;
    EGLConfig  mEglConfig;

    stdmap<unsigned long long, EGLSyncKHR> mEGLSyncMap;
    std::set<Drawable*> pDrawableSet;

private:
    std::unordered_map<int, Drawable*>    mDrawableMap;
    std::unordered_map<int, Context*>     mContextMap;
    std::unordered_map<int, EGLImageKHR>  mEGLImageKHRMap;
    std::unordered_map<int, int>          mDrawableToWinMap;

};

}

#endif
