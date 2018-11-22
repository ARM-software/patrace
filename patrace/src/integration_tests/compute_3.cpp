#include "pa_demo.h"

const char *update_buf_cs_source[] = GLSL_CS(
	layout (local_size_x = 128) in;
	layout(binding = 0, std140) buffer block
	{
	        vec4 pos_out[];
	};
	void main(void)
	{
	        uint gid = gl_GlobalInvocationID.x;
	        pos_out[gid] = vec4(1.0f);
	}
);

const int size = 1024;
static GLuint update_buf_cs, result_buffer;

static int setupGraphics(PAFW_HANDLE pafw_handle, int w, int h, void *user_data)
{
	setup();
	update_buf_cs = glCreateShaderProgramv(GL_COMPUTE_SHADER, 1, update_buf_cs_source);
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
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, result_buffer);
	glUseProgram(update_buf_cs);
	glDispatchCompute(size / 128, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	glStateDump_ARM();
	glUseProgram(0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);

	// verify
	glBindBuffer(GL_UNIFORM_BUFFER, result_buffer);
	GLfloat *ptr = (GLfloat *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(GLfloat) * size * 4, GL_MAP_READ_BIT);
	for (int i = 0; i < size * 4; i++)
	{
		assert(ptr[i] == 1.0f);
	}
	(void)ptr; // silence compiler warning when asserts are disabled
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	// verify in retracer
	glAssertBuffer_ARM(GL_UNIFORM_BUFFER, 0, sizeof(GLfloat) * size * 4, "0123456789abcdef");
}

static void test_cleanup(PAFW_HANDLE pafw_handle, void *user_data)
{
	glDeleteProgram(update_buf_cs);
	glDeleteBuffers(1, &result_buffer);
}

#include "paframework_android_glue.h"

int PAFW_Entry_Point(PAFW_HANDLE pafw_handle)
{
	return init("compute_3", pafw_handle, callback_draw, setupGraphics, test_cleanup);
}
