#ifndef PA_DEMO_H
#define PA_DEMO_H

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <vector>

#define EGL_NO_X11
#define GL_GLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <GLES3/gl32.h>
#include <GLES2/gl2ext.h>

#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif

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

#define PACKED(definition) definition __attribute__((__packed__))

// our fake GL calls
typedef void (GLAPIENTRY *PA_PFNGLASSERTBUFFERARMPROC)(GLenum target, GLsizei offset, GLsizei size, const char *md5);
typedef void (GLAPIENTRY *PA_PFNGLSTATEDUMPARMPROC)(void);
extern PA_PFNGLASSERTBUFFERARMPROC glAssertBuffer_ARM;
extern PA_PFNGLSTATEDUMPARMPROC glStateDump_ARM;

struct PADEMO;
typedef int (GLAPIENTRY *PADEMO_CALLBACK_INIT)(PADEMO *handle, int w, int h, void *user_data);
typedef void (GLAPIENTRY *PADEMO_CALLBACK_SWAP)(PADEMO *handle, void *user_data);
typedef void (GLAPIENTRY *PADEMO_CALLBACK_FREE)(PADEMO *handle, void *user_data);

struct PADEMO
{
	std::string name;
	PADEMO_CALLBACK_SWAP swap;
	PADEMO_CALLBACK_INIT init;
	PADEMO_CALLBACK_FREE done;
	int frames = 10;
	EGLint width = 640;
	EGLint height = 480;
	void *user_data = nullptr;
	EGLDisplay display = 0;
	std::vector<EGLContext> context;
	std::vector<EGLSurface> surface;
	int current_frame = 0;
};

int init(const char *name, PADEMO_CALLBACK_SWAP swap, PADEMO_CALLBACK_INIT setup, PADEMO_CALLBACK_FREE cleanup, void *user_data = nullptr, EGLint *attribs = nullptr, int surfaces = 1);
void assert_fb(int width, int height);
void checkError(const char *msg);
void compile(const char *name, GLint shader);
void link_shader(const char *name, GLint program);
GLenum fb_internalformat();
bool is_null_run();
void annotate(const char *annotation);

#define SHORT_FILE (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define PALOGE(format, ...) do { printf("%s,%d: " format, SHORT_FILE, __LINE__, ##__VA_ARGS__); } while(0)
#define PALOGI(format, ...) do { printf("%s,%d: " format, SHORT_FILE, __LINE__, ##__VA_ARGS__); } while(0)
#define PALOGW(format, ...) do { printf("%s,%d: " format, SHORT_FILE, __LINE__, ##__VA_ARGS__); } while(0)

#endif
