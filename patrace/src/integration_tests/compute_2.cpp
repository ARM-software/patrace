#include "pa_demo.h"

struct __attribute__ ((packed)) indirect_obj
{
    GLuint x, y,z;
};

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
static GLuint indirect_buffer_obj;

static int setupGraphics(PAFW_HANDLE pafw_handle, int w, int h, void *user_data)
{
	struct indirect_obj obj;

	setup();
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
	obj.x = size / 128;
	obj.y = 1;
	obj.z = 1;
	glGenBuffers(1, &indirect_buffer_obj);
	glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, indirect_buffer_obj);
	glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(obj), &obj, GL_STATIC_READ);

	return 0;
}

// first frame render something, second frame verify it
static void callback_draw(PAFW_HANDLE pafw_handle, void *user_data)
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, result_buffer);
	glUseProgram(update_buf_cs);
	glDispatchComputeIndirect(0);
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
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	// verify in retracer
	glAssertBuffer_ARM(GL_UNIFORM_BUFFER, 0, sizeof(GLfloat) * size * 4, "0123456789abcdef");
}

static void test_cleanup(PAFW_HANDLE pafw_handle, void *user_data)
{
	glDeleteShader(cs);
	glDeleteProgram(update_buf_cs);
	glDeleteBuffers(1, &result_buffer);
	glDeleteBuffers(1, &indirect_buffer_obj);
}

#include "paframework_android_glue.h"

int PAFW_Entry_Point(PAFW_HANDLE pafw_handle)
{
	return init("compute_2", pafw_handle, callback_draw, setupGraphics, test_cleanup);
}
