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
static GLuint update_buf_cs, cs, result_buffer;

static int setupGraphics(PADEMO *handle, int w, int h, void *user_data)
{
	update_buf_cs = glCreateProgram();
	cs = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(cs, 1, update_buf_cs_source, NULL);
	compile("update_buf compute", cs);
	glAttachShader(update_buf_cs, cs);
	link_shader("update_buf link", update_buf_cs);
	glGenBuffers(1, &result_buffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, result_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLfloat) * size * 4, NULL, GL_DYNAMIC_DRAW);
	GLfloat *ptr = (GLfloat *)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLfloat) * size * 4, GL_MAP_WRITE_BIT);
	memset(ptr, 0, sizeof(GLfloat) * size * 4);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	return 0;
}

// first frame render something, second frame verify it
static void callback_draw(PADEMO *handle, void *user_data)
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, result_buffer);
	glUseProgram(update_buf_cs);
	glDispatchCompute(size / 128, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	glStateDump_ARM();
	glUseProgram(0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);

	// verify
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, result_buffer);
	GLfloat *ptr = (GLfloat *)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLfloat) * size * 4, GL_MAP_READ_BIT);
	for (int i = 0; i < size * 4; i++)
	{
		if (!is_null_run()) assert(ptr[i] == 1.0f);
	}
	(void)ptr; // silence compiler warnings if asserts disabled
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	// verify in retracer
	glAssertBuffer_ARM(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLfloat) * size * 4, "0123456789abcdef");
}

static void test_cleanup(PADEMO *handle, void *user_data)
{
	glDeleteShader(cs);
	glDeleteProgram(update_buf_cs);
	glDeleteBuffers(1, &result_buffer);
}

int main()
{
	return init("compute_1", callback_draw, setupGraphics, test_cleanup);
}
