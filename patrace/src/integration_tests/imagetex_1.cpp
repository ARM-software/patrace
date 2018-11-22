#include "pa_demo.h"

const char *update_buf_cs_source[] =
{
	"#version 310 es\n"
	"layout (local_size_x = 128) in;\n"
	"layout (binding = 0, std140) buffer block\n"
	"{\n"
	"       vec4 pos_out[];\n"
	"};\n"
	"layout (binding = 0, r32f) highp writeonly restrict uniform image2D img;\n"
	"void main(void)\n"
	"{\n"
	"       uint gid = gl_GlobalInvocationID.x;\n"
	"       imageStore(img, ivec2(gid, 1), vec4(1.0f, 1.0f, 0.0f, 1.0f));\n"
	"       pos_out[gid] = vec4(1.0f);\n"
	"}"
};

const int size = 1024;
static GLuint tex;
static GLuint update_buf_cs, cs, result_buffer;

static int setupGraphics(PAFW_HANDLE pafw_handle, int w, int h, void *user_data)
{
	setup();
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, size, 1);
	GLfloat *data = (GLfloat*)malloc(size * 4 * sizeof(GLfloat));
	memset(data, 0, sizeof(GLfloat) * 4 * size);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size, 1, GL_RED, GL_FLOAT, data);
	free(data);
	update_buf_cs = glCreateProgram();
	cs = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(cs, 1, update_buf_cs_source, NULL);
	compile("update_buf compute", cs);
	glAttachShader(update_buf_cs, cs);
	link_shader("update_buf link", update_buf_cs);
	glGenBuffers(1, &result_buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, result_buffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(GLfloat) * size * 4, NULL, GL_DYNAMIC_DRAW);
	GLfloat *ptr = (GLfloat *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(GLfloat) * size * 4, GL_MAP_WRITE_BIT);
	memset(ptr, 0, sizeof(GLfloat) * size * 4);
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	return 0;
}

// first frame render something, second frame verify it
static void callback_draw(PAFW_HANDLE pafw_handle, void *user_data)
{
	GLint units;
	glGetIntegerv(GL_MAX_IMAGE_UNITS, &units);
	assert(units > 0);
	glBindImageTexture(0, tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, result_buffer);
	glUseProgram(update_buf_cs);
	glDispatchCompute(size / 128, 1, 1);
	glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
	// If line below crashes, see bug report in MIDGLES-3776, fixed in trunk f8f2b77a5d7a66ce4bfcecdbd132a7bd9989971e
	glStateDump_ARM();
	glUseProgram(0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
	glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	// verify
	GLfloat *ptr = (GLfloat *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(GLfloat) * size * 4, GL_MAP_READ_BIT);
	for (int i = 0; i < size * 4; i++)
	{
		assert(ptr[i] == 1.0f);
	}
	(void)ptr; // silence compiler warning if asserts are disabled
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	// verify in retracer
	glAssertBuffer_ARM(GL_UNIFORM_BUFFER, 0, sizeof(GLfloat) * size * 4, "0123456789abcdef");
}

static void test_cleanup(PAFW_HANDLE pafw_handle, void *user_data)
{
	glDeleteShader(cs);
	glDeleteProgram(update_buf_cs);
	glDeleteBuffers(1, &result_buffer);
}

int PAFW_Entry_Point(PAFW_HANDLE pafw_handle)
{
	return init("imagetex_1", pafw_handle, callback_draw, setupGraphics, test_cleanup);
}

