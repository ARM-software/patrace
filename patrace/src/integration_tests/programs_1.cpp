#include "pa_demo.h"

const char *update_buf_cs_source[] =
{
	"#version 310 es\n"
	"layout (local_size_x = 128) in;\n"
	"layout(binding = 0, std140) buffer block\n"
	"{\n"
	"        vec4 pos_out[];\n"
	"};\n"
	"uniform float value;\n"
	"void main(void)\n"
	"{\n"
	"        uint gid = gl_GlobalInvocationID.x;\n"
	"        pos_out[gid] = vec4(value);\n"
	"}"
};

const int size = 1024;
static GLuint update_buf_cs, result_buffer, ppipeline;

static int setupGraphics(PAFW_HANDLE pafw_handle, int w, int h, void *user_data)
{
	glUseProgram(0); // work around bug in paframework
	setup();
	update_buf_cs = glCreateShaderProgramv(GL_COMPUTE_SHADER, 1, update_buf_cs_source);
	glGenBuffers(1, &result_buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, result_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLfloat) * size * 4, NULL, GL_DYNAMIC_DRAW);
	GLfloat *ptr = (GLfloat *)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLfloat) * size * 4, GL_MAP_WRITE_BIT);
	memset(ptr, 0, sizeof(GLfloat) * size * 4);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glGenProgramPipelines(1, &ppipeline); // pipeline object registered here
	glBindProgramPipeline(ppipeline); // pipeline object actually created here
	glUseProgramStages(ppipeline, GL_COMPUTE_SHADER_BIT, update_buf_cs);
	assert(glIsProgramPipeline(ppipeline));
	glValidateProgramPipeline(ppipeline);
	GLint len;
	glGetProgramPipelineiv(ppipeline, GL_INFO_LOG_LENGTH, &len);
	if (len > 0)
	{
		char *buf = (char*)malloc(len);
		GLsizei written;
		glGetProgramPipelineInfoLog(ppipeline, len, &written, buf);
		PALOGE("PIPELINE LOG: %s\n", buf);
		free(buf);
	}


	GLint program = 0;
	GLint pipe = 0;
	GLint count = 0;

	glGetIntegerv(GL_CURRENT_PROGRAM, &program);
	assert(program == 0);

	glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipe);
	assert(pipe == (int)ppipeline);

	glGetProgramPipelineiv(ppipeline, GL_COMPUTE_SHADER, &program);

	glGetProgramInterfaceiv(program, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &count);
	PALOGI("uniform blocks: %d\n", count);
	assert(count == 0);

	glGetProgramInterfaceiv(program, GL_UNIFORM, GL_ACTIVE_RESOURCES, &count);
	PALOGI("uniforms: %d\n", count);
	assert(count == 1);

	glGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &count);
	PALOGI("inputs: %d (expected 0)\n", count);
	//assert(count == 0);

	glGetProgramInterfaceiv(program, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, &count);
	PALOGI("outputs: %d\n", count);
	assert(count == 0);

	glGetProgramInterfaceiv(program, GL_BUFFER_VARIABLE, GL_ACTIVE_RESOURCES, &count);
	PALOGI("variables: %d\n", count);
	assert(count == 1);

	glGetProgramInterfaceiv(program, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &count);
	PALOGI("buffer blocks: %d\n", count);
	assert(count == 1);

	return 0;
}

// first frame render something, second frame verify it
static void callback_draw(PAFW_HANDLE pafw_handle, void *user_data)
{
	// compute
	GLfloat value = 0.0f;
	glUseProgram(0); // work around bug in paframework
	glBindProgramPipeline(ppipeline);
	GLint location = glGetProgramResourceLocation(update_buf_cs, GL_UNIFORM, "value");
	assert(location != -1);
	glProgramUniform1f(update_buf_cs, location, 1.5f);
	glGetUniformfv(update_buf_cs, location, &value);
	assert(value == 1.5f);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, result_buffer);
	glDispatchCompute(size / 128, 1, 1);
	glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
	glStateDump_ARM();
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
	glUseProgramStages(ppipeline, GL_ALL_SHADER_BITS, 0);

	// verify
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, result_buffer);
	GLfloat *ptr = (GLfloat *)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLfloat) * size * 4, GL_MAP_READ_BIT);
	for (int i = 0; i < size * 4; i++)
	{
		assert(ptr[i] == 1.5f);
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	// verify in retracer
	glAssertBuffer_ARM(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLfloat) * size * 4, "0123456789abcdef");
}

static void test_cleanup(PAFW_HANDLE pafw_handle, void *user_data)
{
	glDeleteProgramPipelines(1, &ppipeline);
	glDeleteProgram(update_buf_cs);
	glDeleteBuffers(1, &result_buffer);
}

#include "paframework_android_glue.h"

int PAFW_Entry_Point(PAFW_HANDLE pafw_handle)
{
	return init("programs_1", pafw_handle, callback_draw, setupGraphics, test_cleanup);
}
