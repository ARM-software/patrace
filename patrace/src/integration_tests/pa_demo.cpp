#include "pa_demo.h"

#include <vector>

static bool null_run = false;
static bool inject_asserts = false;

struct fbdev_window
{
	unsigned short width;
	unsigned short height;
};

static void dummy_glStateDump_ARM()
{
	// nothing happens here
}

static void dummy_glAssertBuffer_ARM(GLenum target, GLsizei offset, GLsizei size, const char *md5)
{
	(void)target;
	(void)offset;
	(void)size;
	(void)md5;
	// nothing happens here
}

PA_PFNGLASSERTBUFFERARMPROC glAssertBuffer_ARM = dummy_glAssertBuffer_ARM;
PA_PFNGLSTATEDUMPARMPROC glStateDump_ARM = dummy_glStateDump_ARM;

static void debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
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

static int get_env_int(const char* name, int fallback)
{
	int v = fallback;
	const char* tmpstr = getenv(name);
	if (tmpstr)
	{
		v = atoi(tmpstr);
	}
	return v;
}

bool is_null_run()
{
	return null_run;
}

int init(const char *name, PADEMO_CALLBACK_SWAP swap, PADEMO_CALLBACK_INIT setup, PADEMO_CALLBACK_FREE cleanup, void *user_data, EGLint *attribs)
{
	PADEMO handle;
	handle.name = name;
	handle.swap = swap;
	handle.init = setup;
	handle.done = cleanup;
	handle.frames = get_env_int("PADEMO_FRAMES", 10);
	handle.user_data = user_data;
	handle.current_frame = 0;

	inject_asserts = (bool)get_env_int("PADEMO_SANITY", 0);
	null_run = (bool)get_env_int("PADEMO_NULL_RUN", 0);
	if (null_run)
	{
		PALOGI("Doing a null run - not checking results!\n");
	}

	EGLint contextAttribs[] = {
		EGL_CONTEXT_MAJOR_VERSION, 3,
		EGL_CONTEXT_MINOR_VERSION, 2,
		EGL_NONE, EGL_NONE,
	};
	EGLint surfaceAttribs[] = {
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_ALPHA_SIZE, EGL_DONT_CARE,
		EGL_STENCIL_SIZE, EGL_DONT_CARE,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_NONE, EGL_NONE,
	};
	handle.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (handle.display == EGL_NO_DISPLAY)
	{
		PALOGE("eglGetDisplay() failed\n");
		return EGL_FALSE;
	}
	EGLint majorVersion;
	EGLint minorVersion;
	if (!eglInitialize(handle.display, &majorVersion, &minorVersion))
	{
		PALOGE("eglInitialize() failed\n");
		return EGL_FALSE;
	}
	PALOGI("EGL version is %d.%d\n", majorVersion, minorVersion);

	EGLint numConfigs = 0;
	if (!eglChooseConfig(handle.display, attribs, nullptr, 0, &numConfigs))
	{
		PALOGE("eglChooseConfig(null) failed\n");
		eglTerminate(handle.display);
		return EGL_FALSE;
	}
	std::vector<EGLConfig> configs(numConfigs);
	if (attribs == nullptr) attribs = surfaceAttribs; // use defaults
	if (!eglChooseConfig(handle.display, attribs, configs.data(), configs.size(), &numConfigs))
	{
		PALOGE("eglChooseConfig() failed\n");
		eglTerminate(handle.display);
		return EGL_FALSE;
	}

	fbdev_window window = { 1024, 640 };
	EGLint selected = -1;
	for (EGLint i = 0; i < (EGLint)configs.size(); i++)
	{
		EGLConfig config = configs[i];
		EGLint value = -1;
		eglGetConfigAttrib(handle.display, config, EGL_RED_SIZE, &value);
		if (value == 8) // make sure we avoid the 10bit formats
		{
			selected = i;
			break;
		}
	}
	PALOGI("Found %d EGL configs, selected %d\n", (int)configs.size(), selected);
	assert(selected != -1);
	handle.surface = eglCreateWindowSurface(handle.display, configs[selected], (intptr_t)(&window), NULL);
	if (handle.surface == EGL_NO_SURFACE)
	{
		PALOGE("eglCreateWindowSurface() failed\n");
		return EGL_FALSE;
	}
	handle.context = eglCreateContext(handle.display, configs[0], EGL_NO_CONTEXT, contextAttribs);
	if (handle.context == EGL_NO_CONTEXT)
	{
		PALOGE("eglCreateContext() failed\n");
		return EGL_FALSE;
	}
	if (!eglMakeCurrent(handle.display, handle.surface, handle.surface, handle.context))
 	{
		PALOGE("eglMakeCurrent() failed\n");
		return EGL_FALSE;
	}

	EGLint egl_context_client_version;
	eglQueryContext(handle.display, handle.context, EGL_CONTEXT_CLIENT_VERSION, &egl_context_client_version);
	PALOGI("EGL client version %d\n", egl_context_client_version);
	eglQuerySurface(handle.display, handle.surface, EGL_WIDTH, &handle.width);
	eglQuerySurface(handle.display, handle.surface, EGL_HEIGHT, &handle.height);
	PALOGI("Surface resolution (%d, %d)\n", handle.width, handle.height);

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
	glEnable(GL_DEBUG_OUTPUT_KHR); // use GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR for serious debugging
	glDebugMessageCallback(debug_callback, NULL); // if we crash here on a qualcomm device, it is their driver's fault...

	int ret = setup(&handle, handle.width, handle.height, handle.user_data);
	if (ret != 0)
	{
		PALOGE("Setup failed\n");
		return ret;
	}
	for (int i = 0; i < handle.frames; i++)
	{
		handle.current_frame = i;
		swap(&handle, handle.user_data);
		eglSwapBuffers(handle.display, handle.surface);
	}
	cleanup(&handle, handle.user_data);

	eglMakeCurrent(handle.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(handle.display, handle.context);
	eglDestroySurface(handle.display, handle.surface);
	eglTerminate(handle.display);

	return 0;
}

// before calling this, add appropriate memory barriers
void assert_fb(int width, int height)
{
	if (!inject_asserts) return;

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
