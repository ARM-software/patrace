// Test the KHR_debug extension

#include "pa_demo.h"

#define GL_DEBUG_OUTPUT_SYNCHRONOUS                             0x8242
#define GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH                     0x8243
#define GL_DEBUG_CALLBACK_FUNCTION                              0x8244
#define GL_DEBUG_CALLBACK_USER_PARAM                            0x8245
#define GL_DEBUG_SOURCE_API                                     0x8246
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM                           0x8247
#define GL_DEBUG_SOURCE_SHADER_COMPILER                         0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY                             0x8249
#define GL_DEBUG_SOURCE_APPLICATION                             0x824A
#define GL_DEBUG_SOURCE_OTHER                                   0x824B
#define GL_DEBUG_TYPE_ERROR                                     0x824C
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR                       0x824D
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR                        0x824E
#define GL_DEBUG_TYPE_PORTABILITY                               0x824F
#define GL_DEBUG_TYPE_PERFORMANCE                               0x8250
#define GL_DEBUG_TYPE_OTHER                                     0x8251
#define GL_DEBUG_TYPE_MARKER                                    0x8268
#define GL_DEBUG_TYPE_PUSH_GROUP                                0x8269
#define GL_DEBUG_TYPE_POP_GROUP                                 0x826A
#define GL_DEBUG_SEVERITY_NOTIFICATION                          0x826B
#define GL_MAX_DEBUG_GROUP_STACK_DEPTH                          0x826C
#define GL_DEBUG_GROUP_STACK_DEPTH                              0x826D
#define GL_BUFFER                                               0x82E0
#define GL_SHADER                                               0x82E1
#define GL_PROGRAM                                              0x82E2
#define GL_QUERY                                                0x82E3
#define GL_SAMPLER                                              0x82E6
#define GL_MAX_LABEL_LENGTH                                     0x82E8
#define GL_MAX_DEBUG_MESSAGE_LENGTH                             0x9143
#define GL_MAX_DEBUG_LOGGED_MESSAGES                            0x9144
#define GL_DEBUG_LOGGED_MESSAGES                                0x9145
#define GL_DEBUG_SEVERITY_HIGH                                  0x9146
#define GL_DEBUG_SEVERITY_MEDIUM                                0x9147
#define GL_DEBUG_SEVERITY_LOW                                   0x9148
#define GL_DEBUG_OUTPUT                                         0x92E0
#define GL_CONTEXT_FLAG_DEBUG_BIT                               0x00000002
#define GL_STACK_OVERFLOW                                       0x0503
#define GL_STACK_UNDERFLOW                                      0x0504

static int width = 1024;
static int height = 600;

static int setupGraphics(PAFW_HANDLE pafw_handle, int w, int h, void *user_data)
{
	setup();

	width = w;
	height = h;

	glViewport(0, 0, w, h);
	glClearColor(0.0f, 0.0f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	return 0;
}

static void debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
	PALOGE("Message (khr_debug test): %s\n", message);
	if (type == GL_DEBUG_TYPE_ERROR)
	{
		abort();
	}
	if (strcmp((const char *)userParam, "PASSTHROUGH") != 0)
	{
		PALOGE("User parameter string is wrong!\n");
		abort();
	}
}

// first frame render something, second frame verify it
static void callback_draw(PAFW_HANDLE pafw_handle, void *user_data)
{
	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "GROUP!");

	const char *txt = "PASSTHROUGH";
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(debug_callback, txt);
	void *param;
	glGetPointerv(GL_DEBUG_CALLBACK_FUNCTION, &param);
	// The two asserts below will fail during tracing, since we hijack the debug callback there!
	//assert(param == debug_callback);
	glGetPointerv(GL_DEBUG_CALLBACK_USER_PARAM, &param);
	//assert(param == txt);

	unsigned int ids[100];
	memset(ids, 0, sizeof(ids));
	glDebugMessageControl(GL_DEBUG_SOURCE_OTHER, GL_DEBUG_TYPE_OTHER, GL_DONT_CARE, 100, ids, GL_TRUE);
	glDebugMessageCallback(NULL, NULL); // stop callback, start queueing up messages for later retrieval
	glDebugMessageControl(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER, GL_DEBUG_SEVERITY_LOW, 0, NULL, GL_TRUE);
	glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER, 0, GL_DEBUG_SEVERITY_LOW, -1, "Injected error");
	char log[255];
	memset(log, 0, sizeof(log));
	GLenum source;
	GLenum type;
	GLenum severity;
	unsigned int id;
	GLsizei size;
	unsigned int n = glGetDebugMessageLog(1, sizeof(log) - 1, &source, &type, &id, &severity, &size, log);
	if (n > 0)
	{
		PALOGI("Retrieved message: %s\n", log);
	}
	else
	{
		PALOGE("No errors found, not even our injected one!\n");
	}

	glDebugMessageCallback(debug_callback, txt); // re-enable error repoting, in case we get an error below...

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	const char *shadertext = "TEST_SHADER";
	glObjectLabel(GL_SHADER, vs, -1, shadertext);
	memset(log, 0, sizeof(log));
	glGetObjectLabel(GL_SHADER, vs, sizeof(log), NULL, log);
	assert(*log);
	if (*log)
	{
		assert(strcmp(log, shadertext) == 0);
		PALOGI("Object label: %s\n", log);
	}
	else
	{
		PALOGE("No object label!\n");
	}
	glDeleteShader(vs);

	GLsync s = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	const char *synctext = "TEST_SYNC_OBJECT";
	glObjectPtrLabel(s, -1, synctext);
	memset(log, 0, sizeof(log));
	glGetObjectPtrLabel(s, sizeof(log) - 1, NULL, log);
	assert(*log);
	if (*log)
	{
		assert(strcmp(log, synctext) == 0);
		PALOGI("Sync object label: %s\n", log);
	}
	else
	{
		PALOGE("No syn cobject label!\n");
	}
	glDeleteSync(s);

	glPopDebugGroup();
}

static void test_cleanup(PAFW_HANDLE pafw_handle, void *user_data)
{
	// Nothing
}

#include "paframework_android_glue.h"

int PAFW_Entry_Point(PAFW_HANDLE pafw_handle)
{
	return init("khr_debug", pafw_handle, callback_draw, setupGraphics, test_cleanup);
}
