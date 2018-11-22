#include "pa_demo.h"

#ifndef PAFRAMEWORK_OPENGL
#include <EGL/egl.h>
#else
static void (* eglGetProcAddress(char const * procname))()
{
	return NULL;
}
#define GL_DEBUG_TYPE_ERROR_KHR GL_DEBUG_TYPE_ERROR
#define GL_DEBUG_OUTPUT_KHR GL_DEBUG_OUTPUT
#define GL_DEBUG_TYPE_PUSH_GROUP_KHR GL_DEBUG_TYPE_PUSH_GROUP
#define GL_DEBUG_TYPE_POP_GROUP_KHR GL_DEBUG_TYPE_POP_GROUP
#endif

static void CALLCONV dummy_glStateDump_ARM()
{
	// nothing happens here
}

static void CALLCONV dummy_glAssertBuffer_ARM(GLenum target, GLsizei offset, GLsizei size, const char *md5)
{
	(void)target;
	(void)offset;
	(void)size;
	(void)md5;
	// nothing happens here
}

PA_PFNGLASSERTBUFFERARMPROC glAssertBuffer_ARM = dummy_glAssertBuffer_ARM;
PA_PFNGLSTATEDUMPARMPROC glStateDump_ARM = dummy_glStateDump_ARM;

static void CALLCONV debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
	if (type == GL_DEBUG_TYPE_PUSH_GROUP_KHR || type == GL_DEBUG_TYPE_POP_GROUP_KHR)
	{
		return; // don't care
	}
	PALOGE("OpenGL message: %s\n", message);
	if (type == GL_DEBUG_TYPE_ERROR_KHR)
	{
		abort();
	}
}

GLenum fb_internalformat()
{
	GLint red = 0;
	GLint alpha = 0;

	glGetIntegerv(GL_RED_BITS, &red);
	glGetIntegerv(GL_ALPHA_BITS, &alpha);
	if (red == 5)
	{
		PALOGI("GL_RGB565\n");
		return GL_RGB565;
	}
	else if (alpha == 0)
	{
		PALOGI("GL_RGB8\n");
		return GL_RGB8;
	}
	else
	{
		PALOGI("GL_RGBA8\n");
		return GL_RGBA8;
	}
}

void setup()
{
	glEnable(GL_DEBUG_OUTPUT_KHR); // use GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR for serious debugging
	glDebugMessageCallback(debug_callback, NULL); // if we crash here on a qualcomm device, it is their driver's fault...
}

int init(const char *name, PAFW_HANDLE pafw_handle, PAFW_CALLBACK_DRAW drawcall, PAFW_CALLBACK_INIT_AFTER_WINDOW_SETUP setupcall, PAFW_CALLBACK_CLEANUP_BEFORE_WINDOW_CLOSE cleanupcall, int frames)
{
	// if our tracer / retracer implements these functions, replace our dummies with their real implementation
	void* ptr = (void*)eglGetProcAddress("glAssertBuffer_ARM");
	if (ptr)
	{
		PALOGI("We found an implementation of glAssertBuffer_ARM (%p)\n", ptr);
		glAssertBuffer_ARM = (PA_PFNGLASSERTBUFFERARMPROC)ptr;
	}
	ptr = (void*)eglGetProcAddress("glStateDump_ARM");
	if (ptr)
	{
		PALOGI("We found an implementation of glStateDump_ARM (%p)\n", ptr);
		glStateDump_ARM = (PA_PFNGLSTATEDUMPARMPROC)ptr;
	}

	PAFW_Register_Draw(pafw_handle, drawcall);
	PAFW_Register_Init_After_Window_Setup(pafw_handle, setupcall);
	PAFW_Register_Cleanup_Before_Window_Close(pafw_handle, cleanupcall);

	PAFW_Set_Config_String_Value(pafw_handle, PAFW_PARAM_APP_CAPTION, name);
	PAFW_Set_Config_String_Value(pafw_handle, PAFW_PARAM_APP_NAME, name);
	PAFW_Set_Config_Integer_Value(pafw_handle, PAFW_PARAM_NUMBER_OF_FRAMES, frames);
	PAFW_Set_Config_Integer_Value(pafw_handle, PAFW_PARAM_ONSCREEN_RESOLUTION_WIDTH, 1024);
	PAFW_Set_Config_Integer_Value(pafw_handle, PAFW_PARAM_ONSCREEN_RESOLUTION_HEIGHT, 600);
	PAFW_Set_Config_Integer_Value(pafw_handle, PAFW_PARAM_GLES_VERSION, 3);
	PAFW_Set_Config_Integer_Value(pafw_handle, PAFW_PARAM_LOG_LEVEL, PAFW_LOG_LEVEL_WARNING);
	return 0;
}

// before calling this, add appropriate memory barriers
void assert_fb(int width, int height)
{
	GLuint pbo;
	GLenum internalformat = fb_internalformat();
	int mult;
	GLenum format;
	GLenum type;

	switch (internalformat)
	{
	case GL_RGB565: mult = 2; format = GL_RGB; type = GL_UNSIGNED_SHORT_5_6_5; break;
	case GL_RGB8: mult = 3; format = GL_RGB; type = GL_UNSIGNED_BYTE; break;
	case GL_RGBA8: mult = 4; format = GL_RGBA; type = GL_UNSIGNED_BYTE; break;
	default: PALOGE("Bad internal format\n"); abort(); break;
	}
	glGenBuffers(1, &pbo);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
	glBufferData(GL_PIXEL_PACK_BUFFER, width * height * mult, NULL, GL_DYNAMIC_READ);
	glReadPixels(0, 0, width, height, format, type, 0);
	GLsync s = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	GLenum e = glClientWaitSync(s, GL_SYNC_FLUSH_COMMANDS_BIT, 100 * 1000 * 1000);
	if (e == GL_TIMEOUT_EXPIRED) // we get this on Note3, not sure why
	{
		PALOGI("Wait for sync object timed out\n");
	}
	else if (e != GL_CONDITION_SATISFIED && e != GL_ALREADY_SIGNALED)
	{
		PALOGE("Wait for sync object failed, got %x as response\n", e);
	}
	glDeleteSync(s);
	glAssertBuffer_ARM(GL_PIXEL_PACK_BUFFER, 0, width * height * mult, "0123456789abcdef");
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	glDeleteBuffers(1, &pbo);
}

void compile(const char *name, GLint shader)
{
	GLint rvalue;
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &rvalue);
	if (!rvalue)
	{
		GLint maxLength = 0, len = -1;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
		char *infoLog = NULL;
		if (maxLength > 0)
		{
			infoLog = (char *)malloc(maxLength);
			glGetShaderInfoLog(shader, maxLength, &len, infoLog);
		}
		PALOGE("Error in compiling %s (%d): %s\n", name, len, infoLog ? infoLog : "(n/a)");
		free(infoLog);
		abort();
	}
}

void link_shader(const char *name, GLint program)
{
	GLint rvalue;
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &rvalue);
	if (!rvalue)
	{
		GLint maxLength = 0, len = -1;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
		char *infoLog = (char *)NULL;
		if (maxLength > 0)
		{
			infoLog = (char *)malloc(maxLength);
			glGetProgramInfoLog(program, maxLength, &len, infoLog);
		}
		PALOGE("Error in linking %s (%d): %s\n", name, len, infoLog ? infoLog : "(n/a)");
		free(infoLog);
		abort();
	}
}
