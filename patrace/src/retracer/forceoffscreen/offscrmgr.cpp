#include "offscrmgr.h"

#include "retracer/retracer.hpp"
#include "os.hpp"
#include "common/gl_extension_supported.hpp"

// #define _DEBUG_EGLRETRACE_
#ifdef _DEBUG_EGLRETRACE_
#include "glretrace.hpp"
#endif

#include <dispatch/eglproc_auto.hpp>

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
const unsigned int ON_SCREEN_FBO = 1;
#else
const unsigned int ON_SCREEN_FBO = 0;
#endif

using namespace retracer;

namespace
{
    void setGL(GLenum e, bool state)
    {
        if (state)
        {
            glEnable(e);
        }
        else
        {
            glDisable(e);
        }
    }
}

OffscreenManager::OffscreenManager(
        int fboWidth, int fboHeight,
        int onscrSampleW, int onscrSampleH,
        int onscrSampleNumX, int onscrSampleNumY,
        int colorBitsRed, int colorBitsGreen, int colorBitsBlue, int colorBitsAlpha,
        int depthBits, int stencilBits, int msaaSamples,
        int glesVer)
 : mConfig()
 , mOffscrFboW(fboWidth)
 , mOffscrFboH(fboHeight)
 , mDepth(false)
 , mGlesVer(glesVer)
 , mTexImage2D_format(0)
 , mTexImage2D_type(0)
 , mRenderbufferStorage_depth_format(0)
 , mStencil(false)
 , mRenderbufferStorage_stencil_format(0)
 , mDepthStencil(false)
 , mFramebufferTexture(false)
 , mQuad(glesVer)
 , mMosaicIdx(0)
 , mMosaicX(0)
 , mMosaicY(0)
 , mOwnsGLObjects(true)
 , mStencilBuffer()
 , mOffscreenTex()
 , mDepthBuffer()
 , mOffscreenFBO()
 , mMosaicFBO(0)
 , mMosaicTex(0)
 , mOnscrSampleW(onscrSampleW)
 , mOnscrSampleH(onscrSampleH)
 , mOnscrSampleNumX(onscrSampleNumX)
 , mOnscrSampleNumY(onscrSampleNumY)
 , mOnscrMosaicWidth(mOnscrSampleNumX * mOnscrSampleW)
 , mOnscrMosaicHeight(mOnscrSampleNumY * mOnscrSampleH)
 , mOnscrSampleC(mOnscrSampleNumX * mOnscrSampleNumY)
 , mMsaaSamples(msaaSamples)
{
    mConfig.offscreen_resolution_width = 0;
    mConfig.offscreen_resolution_height = 0;
    mConfig.offscreen_red_size = colorBitsRed;
    mConfig.offscreen_green_size = colorBitsGreen;
    mConfig.offscreen_blue_size = colorBitsBlue;
    mConfig.offscreen_alpha_size = colorBitsAlpha;
    mConfig.offscreen_depth_size = depthBits;
    mConfig.offscreen_stencil_size = stencilBits;

    mOffscreenTex[0] = 0;
    mOffscreenTex[1] = 0;
    mDepthBuffer[0] = 0;
    mDepthBuffer[1] = 0;
    mOffscreenFBO[0] = 0;
    mOffscreenFBO[1] = 0;

    last_non_zero_draw = 0;
    last_non_zero_ctx = 0;
    last_tid = -1;

    std::string extension((const char*)glGetString(GL_EXTENSIONS));
    if (mMsaaSamples > 0 && extension.find("GL_EXT_multisampled_render_to_texture") == std::string::npos) {
        gRetracer.reportAndAbort("The pat file requires MSAA = %d. However, the current GPU (%s %s) doesn't support "
                                 "GL_EXT_multisampled_render_to_texture extension. "
                                 "So it cannot be retraced under offscreen mode.\n",
                                 mMsaaSamples, _glGetString(GL_VENDOR), _glGetString(GL_RENDERER));
    }
}

void OffscreenManager::Init()
{
    mConfig.offscreen_resolution_width = mOffscrFboW;
    mConfig.offscreen_resolution_height = mOffscrFboH;

    if (
            mConfig.offscreen_red_size == 8 &&
            mConfig.offscreen_green_size == 8 &&
            mConfig.offscreen_blue_size == 8 &&
            mConfig.offscreen_alpha_size == 0)
    {
        mTexImage2D_format = GL_RGB;
        mTexImage2D_type = GL_UNSIGNED_BYTE;
    }
    else if (
            mConfig.offscreen_red_size == 5 &&
            mConfig.offscreen_green_size == 6 &&
            mConfig.offscreen_blue_size == 5 &&
            mConfig.offscreen_alpha_size == 0)
    {
        mTexImage2D_format = GL_RGB;
        mTexImage2D_type = GL_UNSIGNED_SHORT_5_6_5;
    }
    else if (
            mConfig.offscreen_red_size == 8 &&
            mConfig.offscreen_green_size == 8 &&
            mConfig.offscreen_blue_size == 8 &&
            mConfig.offscreen_alpha_size == 8)
    {
        mTexImage2D_format = GL_RGBA;
        mTexImage2D_type = GL_UNSIGNED_BYTE;
    }
    else if (
            mConfig.offscreen_red_size == 4 &&
            mConfig.offscreen_green_size == 4 &&
            mConfig.offscreen_blue_size == 4 &&
            mConfig.offscreen_alpha_size == 4)
    {
        mTexImage2D_format = GL_RGBA;
        mTexImage2D_type = GL_UNSIGNED_SHORT_4_4_4_4;
    }
    else if (
            mConfig.offscreen_red_size == 5 &&
            mConfig.offscreen_green_size == 5 &&
            mConfig.offscreen_blue_size == 5 &&
            mConfig.offscreen_alpha_size == 1)
    {
        mTexImage2D_format = GL_RGBA;
        mTexImage2D_type = GL_UNSIGNED_SHORT_5_5_5_1;
    }
    else
    {
        gRetracer.reportAndAbort("Unsupported offscreen buffer configuration (red=%d, green=%d, blue=%d, alpha=%d)",
                mConfig.offscreen_red_size, mConfig.offscreen_green_size, mConfig.offscreen_blue_size, mConfig.offscreen_alpha_size);
    }

    mDepth = (mConfig.offscreen_depth_size != 0);
    if (mConfig.offscreen_depth_size == 0)
    {
        // Don't create RBO for depth
    }
    else if (mConfig.offscreen_depth_size == 16)
    {
        mRenderbufferStorage_depth_format = GL_DEPTH_COMPONENT16;
    }
    else if (mConfig.offscreen_depth_size == 24)
    {
        if (mGlesVer < 3 && !isGlesExtensionSupported("GL_OES_depth24"))
        {
            DBG_LOG("GL_OES_depth24 is not supported in runtime environment\n");
        }
        mRenderbufferStorage_depth_format = GL_DEPTH_COMPONENT24;
    }
    else
    {
        gRetracer.reportAndAbort("Unsupported offscreen buffer configuration (depth=%d)", mConfig.offscreen_depth_size);
    }

    mStencil = (mConfig.offscreen_stencil_size != 0);
    if (mConfig.offscreen_stencil_size == 0)
    {
        // Don't create RBO for stencil
    }
    else if (mConfig.offscreen_stencil_size == 8)
    {
        mRenderbufferStorage_stencil_format = GL_STENCIL_INDEX8;
    }
    else
    {
        gRetracer.reportAndAbort("Unsupported offscreen buffer configuration (stencil=%d)", mConfig.offscreen_stencil_size);
    }

    if (mDepth && mStencil &&
        (mConfig.offscreen_depth_size == 24 ||
        mConfig.offscreen_depth_size == 16) &&
        mConfig.offscreen_stencil_size == 8)
    {
        // Use single D24S8 buffer for this case
        mDepthStencil = true;
        mRenderbufferStorage_depth_format = GL_DEPTH24_STENCIL8_OES;
    }

    DBG_LOG("Offscreen FBO using %d%d%d%d color, %d depth, %d stencil.\n",
            mConfig.offscreen_red_size, mConfig.offscreen_green_size,
            mConfig.offscreen_blue_size, mConfig.offscreen_alpha_size,
            mConfig.offscreen_depth_size, mConfig.offscreen_stencil_size);

    CreateFBOs();
}

OffscreenManager::~OffscreenManager()
{
    DeleteFBOs();
}

bool OffscreenManager::BindOffscreenFBO()
{
    //DBG_LOG("BindOffscreenFBO %d", (mMosaicIdx%2)==0 ? mOffscreenFBO[0] : mOffscreenFBO[1]);
    glBindFramebuffer12(GL_FRAMEBUFFER,
        (mMosaicIdx%2)==0 ? mOffscreenFBO[0] : mOffscreenFBO[1]);
    return true;
}

bool OffscreenManager::BindOffscreenReadFBO()
{
    //DBG_LOG("BindOffscreenReadFBO %d", (mMosaicIdx%2)==0 ? mOffscreenFBO[0] : mOffscreenFBO[1]);
    glBindFramebuffer12(GL_READ_FRAMEBUFFER,
        (mMosaicIdx%2)==0 ? mOffscreenFBO[0] : mOffscreenFBO[1]);
    return true;
}

int GetTextureIdFromCurrentFbo()
{
    GLint draw_framebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &draw_framebuffer);
    GLint type, name;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &name);
    DBG_LOG("framebuffer id=%d, type: %d, name: %d", draw_framebuffer, type, name);

    if (type == GL_TEXTURE)
        return name;
    else
        return 0;
}

void OffscreenManager::OffscreenToMosaic()
{
#ifdef _DEBUG_EGLRETRACE_
    static int no = 0;
    no++;
    if (no < 10)
    {
        char filename[128];
        sprintf(filename, "/data/apitrace/fbo%06u.png", glretrace::gFrameNo);
        glretrace::Snapshot(filename);
    }
#endif

    // 1. Save current render state
    // We don't need to restore fbo as another fbo (the offscreen target)
    // will be bound again after this function (see retrace_eglSwapBuffers)
    GLint oVp[4];
    _glGetIntegerv(GL_VIEWPORT, oVp);
    GLboolean oScissorTestState = _glIsEnabled(GL_SCISSOR_TEST);
    GLboolean oBlendState = _glIsEnabled(GL_BLEND);
    GLboolean oStencilTest = _glIsEnabled(GL_STENCIL_TEST);
    GLboolean oCullFace = _glIsEnabled(GL_CULL_FACE);

    // 2. Draw a texture into the fbo
    glBindFramebuffer12(GL_FRAMEBUFFER, mMosaicFBO);

    int x = mMosaicIdx % mOnscrSampleNumX;
    int y = mMosaicIdx / mOnscrSampleNumX;

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);

    if (mMosaicIdx == 0)
    {
        _glClear(GL_COLOR_BUFFER_BIT);
    }

    _glViewport(x * mOnscrSampleW, y * mOnscrSampleH, mOnscrSampleW, mOnscrSampleH);

    int texId = (mMosaicIdx%2)==0 ? mOffscreenTex[0] : mOffscreenTex[1];
    mQuad.DrawTex(texId);

#ifdef _DEBUG_EGLRETRACE_
    if (no < 10)
    {
        char filename[128];
        sprintf(filename, "/data/apitrace/mosaic%06u.png", glretrace::gFrameNo);
        glretrace::Snapshot(filename);
    }
#endif

    // 3. Restore the original render state
    _glViewport(oVp[0], oVp[1], oVp[2], oVp[3]);

    setGL(GL_SCISSOR_TEST, oScissorTestState);
    setGL(GL_CULL_FACE, oCullFace);
    setGL(GL_BLEND, oBlendState);
    setGL(GL_STENCIL_TEST, oStencilTest);

    ++mMosaicIdx;
    mMosaicIdx %= mOnscrSampleC;
}

bool OffscreenManager::MosaicToScreenIfNeeded(bool forceFlush)
{
    if (mMosaicIdx != 0 && !forceFlush)
    {
        return false;
    }

    // 1. Save current render state
    GLint oVp[4];
    _glGetIntegerv(GL_VIEWPORT, oVp);
    float oClearColor[4];
    _glGetFloatv(GL_COLOR_CLEAR_VALUE, oClearColor);
    GLboolean oScissorTestState = _glIsEnabled(GL_SCISSOR_TEST);
    GLboolean oBlendState = _glIsEnabled(GL_BLEND);
    GLboolean oStencilTest = _glIsEnabled(GL_STENCIL_TEST);
    GLboolean oCullFace = _glIsEnabled(GL_CULL_FACE);

    // 2. Render mosaic picture to the screen
    glBindFramebuffer12(GL_FRAMEBUFFER, ON_SCREEN_FBO);
    _glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);

    _glViewport(mMosaicX, mMosaicY, mOnscrMosaicWidth, mOnscrMosaicHeight);
    _glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    mQuad.DrawTex(mMosaicTex);

    // 3. Restore the original render state
    _glViewport(oVp[0], oVp[1], oVp[2], oVp[3]);
    _glClearColor(oClearColor[0], oClearColor[1], oClearColor[2], oClearColor[3]);

    setGL(GL_SCISSOR_TEST, oScissorTestState);
    setGL(GL_CULL_FACE, oCullFace);
    setGL(GL_BLEND, oBlendState);
    setGL(GL_STENCIL_TEST, oStencilTest);

    return true;
}

void OffscreenManager::CreateFBOs()
{
    mOwnsGLObjects = true;

    // 1. offscreen texture/depth/framebuffer
    glGenFramebuffers12(2, mOffscreenFBO);
    _glGenTextures(2, mOffscreenTex);
    if (mDepth)
    {
        glGenRenderbuffers12(2, mDepthBuffer);
    }
    if (mStencil && !mDepthStencil)
    {
        glGenRenderbuffers12(2, mStencilBuffer);
    }

    framebufferTexture(mOffscrFboW, mOffscrFboH);

    // 2. mosaic texture/framebuffer
    glGenFramebuffers12(1, &mMosaicFBO);
    _glGenTextures(1, &mMosaicTex);
    glBindFramebuffer12(GL_FRAMEBUFFER, mMosaicFBO);
    _glBindTexture(GL_TEXTURE_2D, mMosaicTex);
    _glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    _glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    _glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    _glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    _glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mOnscrMosaicWidth, mOnscrMosaicHeight, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
    glFramebufferTexture2D12(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mMosaicTex, 0);
    int status = glCheckFramebufferStatus12(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        gRetracer.reportAndAbort("Framebuffer incomplete (%d = %d x %d) color_mode = %d depth_mode = %d, status=%04X\n",
                                 mMosaicFBO, mOnscrMosaicWidth, mOnscrMosaicHeight, GL_UNSIGNED_SHORT_5_6_5, 0, status);
    }
    glBindFramebuffer12(GL_FRAMEBUFFER, ON_SCREEN_FBO);
}

void OffscreenManager::framebufferTexture(int w, int h)
{
    if (w == mOffscrFboW && h == mOffscrFboH && mFramebufferTexture)
        return;

    mOffscrFboW = w;
    mOffscrFboH = h;
    mFramebufferTexture = true;

    for (int i = 0; i < 2; ++i)
    {
        glBindFramebuffer12(GL_FRAMEBUFFER, mOffscreenFBO[i]);

        _glBindTexture(GL_TEXTURE_2D, mOffscreenTex[i]);
        _glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        _glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        _glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        _glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        _glTexImage2D(GL_TEXTURE_2D, 0, mTexImage2D_format, mOffscrFboW, mOffscrFboH, 0, mTexImage2D_format, mTexImage2D_type, NULL);
        if (mMsaaSamples == 0)
            glFramebufferTexture2D12(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mOffscreenTex[i], 0);
        else
            glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mOffscreenTex[i], 0, mMsaaSamples);

        if (mDepth)
        {
            glBindRenderbuffer12(GL_RENDERBUFFER, mDepthBuffer[i]);
            if (mMsaaSamples == 0)
                glRenderbufferStorage12(GL_RENDERBUFFER, mRenderbufferStorage_depth_format, mOffscrFboW, mOffscrFboH);
            else
                glRenderbufferStorageMultisample23(GL_RENDERBUFFER, mMsaaSamples, mRenderbufferStorage_depth_format, mOffscrFboW, mOffscrFboH);
            glFramebufferRenderbuffer12(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffer[i]);
            if (mDepthStencil)
            {
                glFramebufferRenderbuffer12(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffer[i]);
            }
            glBindRenderbuffer12(GL_RENDERBUFFER, ON_SCREEN_FBO);
        }

        if (mStencil && !mDepthStencil)
        {
            glBindRenderbuffer12(GL_RENDERBUFFER, mStencilBuffer[i]);
            if (mMsaaSamples == 0)
                glRenderbufferStorage12(GL_RENDERBUFFER, mRenderbufferStorage_stencil_format, mOffscrFboW, mOffscrFboH);
            else
                glRenderbufferStorageMultisample23(GL_RENDERBUFFER, mMsaaSamples, mRenderbufferStorage_stencil_format, mOffscrFboW, mOffscrFboH);
            glFramebufferRenderbuffer12(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mStencilBuffer[i]);
            glBindRenderbuffer12(GL_RENDERBUFFER, ON_SCREEN_FBO);
        }

        int status = glCheckFramebufferStatus12(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            gRetracer.reportAndAbort("Framebuffer incomplete (FBOid=%d, resolution=%d x %d, tex_format=%X, tex_type=%X, depth_rbo_format=%X, stencil_rbo_format=%X, status=%04X",
                                     mOffscreenFBO[i], mOffscrFboW, mOffscrFboH, mTexImage2D_format, mTexImage2D_type, mRenderbufferStorage_depth_format, mRenderbufferStorage_stencil_format, status);
        }
    }
}

void OffscreenManager::ReleaseOwnershipOfGLObjects()
{
    // Caller takes ownership of all GL objects created by this class, so they no longer need to be deleted
    mOwnsGLObjects = false;
    mQuad.ReleaseOwnershipOfGLObjects();
}

void OffscreenManager::DeleteFBOs()
{
    if (mOwnsGLObjects)
    {
        glDeleteFramebuffers12(2, mOffscreenFBO);
        _glDeleteTextures(2, mOffscreenTex);
        if (mDepth)
            glDeleteRenderbuffers12(2, mDepthBuffer);
        if (mStencil && !mDepthStencil)
            glDeleteRenderbuffers12(2, mStencilBuffer);

        glDeleteFramebuffers12(1, &mMosaicFBO);
        _glDeleteTextures(1, &mMosaicTex);
    }
}

void OffscreenManager::glGenFramebuffers12(GLsizei n, GLuint* framebuffers)
{
    mGlesVer == 1 ?
        _glGenFramebuffersOES(n, framebuffers) :
        _glGenFramebuffers(n, framebuffers);
}

void OffscreenManager::glGenRenderbuffers12(GLsizei n, GLuint* renderbuffers)
{
    mGlesVer == 1 ?
        _glGenRenderbuffersOES(n, renderbuffers) :
        _glGenRenderbuffers(n, renderbuffers);
}

void OffscreenManager::glBindFramebuffer12(GLenum target, GLuint framebuffer)
{
    mGlesVer == 1 ?
        _glBindFramebufferOES(target, framebuffer) :
        _glBindFramebuffer(target, framebuffer);
}

void OffscreenManager::glBindRenderbuffer12(GLenum target, GLuint renderbuffer)
{
    mGlesVer == 1 ?
        _glBindRenderbufferOES(target, renderbuffer) :
        _glBindRenderbuffer(target, renderbuffer);
}

void OffscreenManager::glDeleteFramebuffers12(GLsizei n, const GLuint* framebuffers)
{
    mGlesVer == 1 ?
        _glDeleteFramebuffersOES(n, framebuffers) :
        _glDeleteFramebuffers(n, framebuffers);
}

void OffscreenManager::glDeleteRenderbuffers12(GLsizei n, const GLuint* renderbuffers)
{
    mGlesVer == 1 ?
        _glDeleteRenderbuffersOES(n, renderbuffers) :
        _glDeleteRenderbuffers(n, renderbuffers);
}

void OffscreenManager::glFramebufferTexture2D12(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    mGlesVer == 1 ?
        _glFramebufferTexture2DOES(target, attachment, textarget, texture, level) :
        _glFramebufferTexture2D(target, attachment, textarget, texture, level);
}

void OffscreenManager::glRenderbufferStorage12(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
    mGlesVer == 1 ?
        _glRenderbufferStorageOES(target, internalformat, width, height) :
        _glRenderbufferStorage(target, internalformat, width, height);
}

void OffscreenManager::glRenderbufferStorageMultisample23(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{
    mGlesVer <= 2 ?
        _glRenderbufferStorageMultisampleEXT(target, samples, internalformat, width, height) :
        _glRenderbufferStorageMultisample(target, samples, internalformat, width, height);
}

void OffscreenManager::glFramebufferRenderbuffer12(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
    mGlesVer == 1 ?
        _glFramebufferRenderbufferOES(target, attachment, renderbuffertarget, renderbuffer) :
        _glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
}

GLenum OffscreenManager::glCheckFramebufferStatus12(GLenum target)
{
    return (mGlesVer == 1) ?
        _glCheckFramebufferStatusOES(target) :
        _glCheckFramebufferStatus(target);
}
