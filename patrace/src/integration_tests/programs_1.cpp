#include "pa_demo.h"

const char *vertex_shader_source[] = GLSL_VS(
	uniform float offset;
	in vec4 a_v4Position;
	in vec4 a_v4FillColor;
	highp out vec4 v_v4FillColor;
	void main()
	{
		v_v4FillColor = a_v4FillColor;
		gl_Position = a_v4Position;
		gl_Position.x += offset;
	}
);

const char *fragment_shader_source[] = GLSL_FS(
	highp in vec4 v_v4FillColor;
	out vec4 fragColor;
	void main()
	{
	        fragColor = v_v4FillColor;
	}
);

const float triangleVertices[] =
{
	0.0f,  0.5f, 0.0f,
	-0.5f, -0.5f, 0.0f,
	0.5f, -0.5f, 0.0f,
};

const float triangleColors[] =
{
	1.0, 0.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0,
};


static int width = 1024;
static int height = 600;
static GLuint vpos_obj, vcol_obj, vertex_program, fragment_program, vao, ppipeline;
static GLint upos;

static int setupGraphics(PADEMO *handle, int w, int h, void *user_data)
{
	width = w;
	height = h;

	// setup space
	glViewport(0, 0, width, height);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.5f, 1.0f);
	glClearDepthf(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// setup draw program
	vertex_program = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, vertex_shader_source);
	fragment_program = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, fragment_shader_source);
	glGenProgramPipelines(1, &ppipeline); // pipeline object registered here
	glBindProgramPipeline(ppipeline); // pipeline object actually created here
	glUseProgramStages(ppipeline, GL_VERTEX_SHADER_BIT, vertex_program);
	glUseProgramStages(ppipeline, GL_FRAGMENT_SHADER_BIT, fragment_program);
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
	PALOGI("program pipeline: %d\n", program);
	assert(pipe == (int)ppipeline);

	glGetProgramPipelineiv(ppipeline, GL_VERTEX_SHADER, &program);
	PALOGI("vertex program: %d\n", program);

	glGetProgramInterfaceiv(program, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &count);
	PALOGI("uniform blocks: %d\n", count);
	assert(count == 0);

	glGetProgramInterfaceiv(program, GL_UNIFORM, GL_ACTIVE_RESOURCES, &count);
	PALOGI("uniforms: %d\n", count);
	assert(count == 1);

	glGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &count);
	PALOGI("inputs: %d\n", count);
	assert(count == 2);

	glGetProgramInterfaceiv(program, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, &count);
	PALOGI("outputs: %d\n", count);
	assert(count == 2);

	glGetProgramInterfaceiv(program, GL_BUFFER_VARIABLE, GL_ACTIVE_RESOURCES, &count);
	PALOGI("variables: %d\n", count);
	assert(count == 0);

	glGetProgramInterfaceiv(program, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &count);
	PALOGI("buffer blocks: %d\n", count);
	assert(count == 0);

	// setup buffers
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	GLint iLocPosition = glGetProgramResourceLocation(vertex_program, GL_PROGRAM_INPUT, "a_v4Position");
	GLint iLocFillColor = glGetProgramResourceLocation(vertex_program, GL_PROGRAM_INPUT, "a_v4FillColor");
	upos = glGetProgramResourceLocation(vertex_program, GL_UNIFORM, "offset");
	glGenBuffers(1, &vcol_obj);
	glBindBuffer(GL_ARRAY_BUFFER, vcol_obj);
	glBufferData(GL_ARRAY_BUFFER, 3 * 4 * sizeof(GLfloat), triangleColors, GL_STATIC_DRAW);
	glEnableVertexAttribArray(iLocFillColor);
	glVertexAttribPointer(iLocFillColor, 4, GL_FLOAT, GL_FALSE, 0, NULL);//triangleColors);
	glGenBuffers(1, &vpos_obj);
	glBindBuffer(GL_ARRAY_BUFFER, vpos_obj);
	glBufferData(GL_ARRAY_BUFFER, 3 * 3 * sizeof(GLfloat), triangleVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(iLocPosition);
	glVertexAttribPointer(iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, NULL);//triangleVertices);

	return 0;
}

// first frame render something, second frame verify it
static void callback_draw(PADEMO *handle, void *user_data)
{
	glProgramUniform1f(vertex_program, upos, handle->current_frame * 0.1f);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glStateDump_ARM();

	// verify in retracer
	assert_fb(width, height);
}

static void test_cleanup(PADEMO *handle, void *user_data)
{
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vcol_obj);
	glDeleteBuffers(1, &vpos_obj);
	glDeleteProgramPipelines(1, &ppipeline);
	glDeleteProgram(vertex_program);
	glDeleteProgram(fragment_program);
}

int main()
{
	return init("programs_1", callback_draw, setupGraphics, test_cleanup);
}
