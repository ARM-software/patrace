#include "pa_demo.h"
#include <EGL/eglext.h>

#include <vector>

struct fbdev_window
{
	unsigned short width;
	unsigned short height;
};

static bool null_run = false;
static bool inject_asserts = false;
static std::vector<fbdev_window> windows;
static PFNGLINSERTEVENTMARKEREXTPROC my_glInsertEventMarkerEXT = nullptr;

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

void annotate(const char *annotation)
{
	if (my_glInsertEventMarkerEXT) my_glInsertEventMarkerEXT(0, annotation);
}

int init(const char *name, PADEMO_CALLBACK_SWAP swap, PADEMO_CALLBACK_INIT setup, PADEMO_CALLBACK_FREE cleanup, void *user_data, EGLint *attribs, int surfaces)
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
	bool pbuffers = (bool)get_env_int("PADEMO_PBUFFERS", 0);
	if (null_run)
	{
		PALOGI("Doing a null run - not checking results!\n");
	}

	handle.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	PFNEGLQUERYDEVICESEXTPROC eglQueryDevicesEXT = (PFNEGLQUERYDEVICESEXTPROC)eglGetProcAddress("eglQueryDevicesEXT");
	PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
	if (eglQueryDevicesEXT && eglGetPlatformDisplayEXT)
	{
		EGLint numDevices = 0;
		if (eglQueryDevicesEXT(0, nullptr, &numDevices) == EGL_FALSE)
		{
			PALOGE("Failed to poll EGL devices\n");
			return EGL_FALSE;
		}
		std::vector<EGLDeviceEXT> devices(numDevices);
		if (eglQueryDevicesEXT(devices.size(), devices.data(), &numDevices) == EGL_FALSE)
		{
			PALOGE("Failed to fetch EGL devices\n");
			return EGL_FALSE;
		}
		for (unsigned i = 0; i < devices.size(); i++)
		{
			if (handle.display == EGL_NO_DISPLAY)
			{
				handle.display = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, devices[i], 0);
				PALOGI("Using EGL device %u for our display!\n", i);
			}
			else PALOGI("EGL device found and ignored: %u\n", i);
		}
	}

	if (handle.display == EGL_NO_DISPLAY)
	{
		PALOGE("No display found\n");
		return EGL_FALSE;
	}

	EGLint surfaceAttribs[] = {
		EGL_SURFACE_TYPE, pbuffers == 0 ? EGL_WINDOW_BIT : EGL_PBUFFER_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_ALPHA_SIZE, EGL_DONT_CARE,
		EGL_STENCIL_SIZE, EGL_DONT_CARE,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_NONE, EGL_NONE,
	};
	EGLint contextAttribs[] = {
		EGL_CONTEXT_MAJOR_VERSION, 3,
		EGL_CONTEXT_MINOR_VERSION, 2,
		EGL_NONE, EGL_NONE,
	};
	if (attribs == nullptr) attribs = surfaceAttribs; // use defaults

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
	if (!eglChooseConfig(handle.display, attribs, configs.data(), configs.size(), &numConfigs))
	{
		PALOGE("eglChooseConfig() failed\n");
		eglTerminate(handle.display);
		return EGL_FALSE;
	}

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

	handle.surface.resize(surfaces);
	handle.context.resize(surfaces);
	windows.resize(surfaces);
	for (int j = 0; j < surfaces; j++)
	{
		windows[j] = { 1024, 640 };

		if (!pbuffers)
		{
			handle.surface[j] = eglCreateWindowSurface(handle.display, configs[selected], (intptr_t)(&windows[j]), NULL);
		}
		else
		{
			EGLint pAttribs[] = {
				EGL_HEIGHT, (EGLint)windows[j].height,
				EGL_WIDTH, (EGLint)windows[j].width,
				EGL_NONE, EGL_NONE,
			};
			handle.surface[j] = eglCreatePbufferSurface(handle.display, configs[selected], pAttribs);
		}
		if (handle.surface[j] == EGL_NO_SURFACE)
		{
			PALOGE("eglCreateWindowSurface() failed: 0x%04x\n", (unsigned)eglGetError());
			return EGL_FALSE;
		}
		handle.context[j] = eglCreateContext(handle.display, configs[0], (j == 0) ? EGL_NO_CONTEXT : handle.context[0], contextAttribs);
		if (handle.context[j] == EGL_NO_CONTEXT)
		{
			PALOGE("eglCreateContext() failed: 0x%04x\n", (unsigned)eglGetError());
			return EGL_FALSE;
		}
	}

	if (!eglMakeCurrent(handle.display, handle.surface[0], handle.surface[0], handle.context[0]))
 	{
		PALOGE("eglMakeCurrent() failed\n");
		return EGL_FALSE;
	}

	EGLint egl_context_client_version;
	eglQueryContext(handle.display, handle.context[0], EGL_CONTEXT_CLIENT_VERSION, &egl_context_client_version);
	PALOGI("EGL client version %d\n", egl_context_client_version);
	eglQuerySurface(handle.display, handle.surface[0], EGL_WIDTH, &handle.width);
	eglQuerySurface(handle.display, handle.surface[0], EGL_HEIGHT, &handle.height);
	PALOGI("Surface resolution %p(%d, %d)\n", handle.surface[0], handle.width, handle.height);

	my_glInsertEventMarkerEXT = (PFNGLINSERTEVENTMARKEREXTPROC)eglGetProcAddress("glInsertEventMarkerEXT");
	if (!my_glInsertEventMarkerEXT) PALOGE("No glInsertEventMarkerEXT implementation found!\n");
	else PALOGI("Marker annotation initialized\n");

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
		std::string annotation = std::string(name) + " frame " + std::to_string(handle.current_frame);
		annotate(annotation.c_str());
		swap(&handle, handle.user_data);
		eglSwapBuffers(handle.display, handle.surface[0]);
	}
	cleanup(&handle, handle.user_data);

	eglMakeCurrent(handle.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	for (unsigned j = 0; j < handle.context.size(); j++) eglDestroyContext(handle.display, handle.context[j]);
	for (unsigned j = 0; j < handle.surface.size(); j++) eglDestroySurface(handle.display, handle.surface[j]);
	handle.context.clear();
	handle.surface.clear();
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
