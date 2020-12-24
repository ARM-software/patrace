#ifndef _OFFSCREENMANAGER_H_
#define _OFFSCREENMANAGER_H_

#include "quad.h"

typedef int             GLint;
typedef unsigned int    GLuint;
typedef int             GLsizei;
typedef unsigned int    GLenum;

struct offscreen_config
{
    int offscreen_resolution_width;
    int offscreen_resolution_height;
    int offscreen_red_size;
    int offscreen_green_size;
    int offscreen_blue_size;
    int offscreen_alpha_size;
    int offscreen_depth_size;
    int offscreen_stencil_size;
};

class OffscreenManager
{
public:
    OffscreenManager(
            int fboWidth, int fboHeight,
            int onscrSampleW, int onscrSampleH,
            int onscrSampleNumX, int onscrSampleNumY,
            int colorBitsRed, int colorBitsGreen, int colorBitsBlue, int colorBitsAlpha,
            int depthBits, int stencilBits, int msaaSamples,
            int glesVer);
    ~OffscreenManager();

    void Init();
    bool BindOffscreenFBO(GLenum target);
    bool BindOffscreenReadFBO();
    void OffscreenToMosaic();
    bool MosaicToScreenIfNeeded(bool forceFlush = false);
    void ReleaseOwnershipOfGLObjects();
    void framebufferTexture(int w, int h);
    int last_non_zero_draw;
    int last_non_zero_ctx;
    int last_tid;

private:
    void CreateFBOs();
    void DeleteFBOs();

    void glGenFramebuffers12(GLsizei n, GLuint* framebuffers);
    void glGenRenderbuffers12(GLsizei n, GLuint* renderbuffers);
    void glBindFramebuffer12(GLenum target, GLuint framebuffer);
    void glBindRenderbuffer12(GLenum target, GLuint renderbuffer);
    void glDeleteFramebuffers12(GLsizei n, const GLuint* framebuffers);
    void glDeleteRenderbuffers12(GLsizei n, const GLuint* renderbuffers);
    void glFramebufferTexture2D12(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    void glRenderbufferStorage12(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
    void glFramebufferRenderbuffer12(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
    GLenum glCheckFramebufferStatus12(GLenum target);

    offscreen_config mConfig;
    int             mOffscrFboW;
    int             mOffscrFboH;
    bool            mDepth;
    int             mGlesVer;
    GLenum          mTexImage2D_format;
    GLenum          mTexImage2D_internal_format;
    GLenum          mTexImage2D_type;
    GLenum          mRenderbufferStorage_depth_format;
    bool            mStencil;
    GLenum          mRenderbufferStorage_stencil_format;
    bool            mDepthStencil;
    bool            mFramebufferTexture;

    Quad            mQuad;
    int             mMosaicIdx;
    int             mMosaicX;
    int             mMosaicY;

    bool         mOwnsGLObjects;
    unsigned int mStencilBuffer[2];
    unsigned int mOffscreenTex[2];
    unsigned int mDepthBuffer[2];
    unsigned int mOffscreenFBO[2];
    unsigned int mMosaicFBO;
    unsigned int mMosaicTex;
    unsigned int mOnscrSampleW;
    unsigned int mOnscrSampleH;
    unsigned int mOnscrSampleNumX;
    unsigned int mOnscrSampleNumY;
    unsigned int mOnscrMosaicWidth;
    unsigned int mOnscrMosaicHeight;
    unsigned int mOnscrSampleC;
    unsigned int mMsaaSamples;
};

// int GetTextureIdFromCurrentFbo();

#endif
