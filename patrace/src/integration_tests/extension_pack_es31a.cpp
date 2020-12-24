// Test the ANDROID_extension_pack_es31a extension

#include "pa_demo.h"

static int width = 1024;
static int height = 600;

static int setupGraphics(PADEMO *handle, int w, int h, void *user_data)
{
	width = w;
	height = h;

	glViewport(0, 0, w, h);
	glClearColor(0.0f, 0.0f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	GLint extensions;
	glGetIntegerv(GL_NUM_EXTENSIONS, &extensions);
	for (int i = 0; i < extensions; i++)
	{
		const char *ext = (const char *)glGetStringi(GL_EXTENSIONS, i);
		if (strcmp(ext, "GL_ANDROID_extension_pack_es31a") == 0)
		{
			PALOGI("GL_ANDROID_extension_pack_es31a supported\n");
			return 0; // found
		}
	}
	PALOGE("Android extension pack 1 support not found\n");
	return 1;
}

static void callback_draw(PADEMO *handle, void *user_data)
{
	GLint value;
	glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS, &value);
	assert(value >= 1);
	PALOGI("GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS = %d\n", value);
	glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTERS, &value);
	assert(value >= 8);
	PALOGI("GL_MAX_FRAGMENT_ATOMIC_COUNTERS = %d\n", value);
	glGetIntegerv(GL_MAX_FRAGMENT_IMAGE_UNIFORMS, &value);
	assert(value >= 4);
	PALOGI("GL_MAX_FRAGMENT_IMAGE_UNIFORMS = %d\n", value);
	glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, &value);
	assert(value >= 4);
	PALOGI("GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS = %d\n", value);
}

static void test_cleanup(PADEMO *handle, void *user_data)
{
	// Nothing
}

int main()
{
	return init("extension_pack_es31a", callback_draw, setupGraphics, test_cleanup);
}
