#ifndef PA_DEMO_H
#define PA_DEMO_H

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#ifdef _WIN32
#define CALLCONV __stdcall
#else
#define CALLCONV
#endif

#ifndef GLAPIENTRY
#define GLAPIENTRY CALLCONV
#endif

#ifndef PAFRAMEWORK_OPENGL
#define GL_GLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <GLES2/gl2ext.h>
#include "pa_gl31.h"
#else
#define GLEW_STATIC 1
#include <GL/glew.h>
#endif

#include "paframework.h"

// Cross-platform compatibility macros. The shader macros are convenient
// for writing inline shaders, and also allow us to redefine them if we
// want to compile on native desktop GL one day.
#define GLSL_VS(src) { "#version 310 es\n" #src }
#define GLSL_FS(src) { "#version 310 es\nprecision mediump float;\n" #src }
#define GLSL_FS_5(src) { "#version 310 es\n#extension GL_EXT_gpu_shader5 : require\nprecision mediump float;\n" #src }
#define GLSL_CS(src) { "#version 310 es\n" #src }
#define GLSL_CONTROL(src) { "#version 310 es\n" #src }
#define GLSL_EVALUATE(src) { "#version 310 es\n" #src }
#ifndef PAFRAMEWORK_OPENGL
#define GLSL_GS(src) { "#version 310 es\n#extension GL_EXT_geometry_shader : enable\n" #src }
#else
#define GLSL_VS(src) { "#version 420 core\n" #src }
#define GLSL_FS(src) { "#version 420 core\nprecision mediump float;\n" #src }
#define GLSL_GS(src) { "#version 310 es\n" #src }
#define GLSL_CONTROL(src) { "#version 420 core\n" #src }
#define GLSL_EVALUATE(src) { "#version 420 core\n" #src }
#endif

#ifndef _WIN32
#define PACKED(definition) definition __attribute__((__packed__))
#else
#define PACKED(definition) __pragma(pack(push, 1)) definition __pragma(pack(pop))
#endif

// our fake GL calls
typedef void (GLAPIENTRY *PA_PFNGLASSERTBUFFERARMPROC)(GLenum target, GLsizei offset, GLsizei size, const char *md5);
typedef void (GLAPIENTRY *PA_PFNGLSTATEDUMPARMPROC)(void);
extern PA_PFNGLASSERTBUFFERARMPROC glAssertBuffer_ARM;
extern PA_PFNGLSTATEDUMPARMPROC glStateDump_ARM;

void setup();
int init(const char *name, PAFW_HANDLE pafw_handle, PAFW_CALLBACK_DRAW drawcall, PAFW_CALLBACK_INIT_AFTER_WINDOW_SETUP setupcall, PAFW_CALLBACK_CLEANUP_BEFORE_WINDOW_CLOSE cleanupcall, int frames = 1);
void assert_fb(int width, int height);
void checkError(const char *msg);
void compile(const char *name, GLint shader);
void link_shader(const char *name, GLint program);
GLenum fb_internalformat();

#endif
