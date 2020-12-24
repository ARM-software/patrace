#include "pa_demo.h"

const char *vertex_shader_source[] = GLSL_VS(
	in vec4 a_v4Position;
	in vec4 a_v4FillColor;
	out vec4 v_v4FillColor;
	void main()
	{
		v_v4FillColor = a_v4FillColor;
		gl_Position = a_v4Position;
	}
);

const char *fragment_shader_source[] = GLSL_FS(
	in vec4 v_v4FillColor;
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

const GLuint indices[] =
{
	0, 1, 2
};

static GLuint vs, fs, draw_program;

static int setupGraphics(PADEMO *handle, int w, int h, void *user_data)
{
	// setup space
	glViewport(0, 0, handle->width, handle->height);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.5f, 1.0f);
	glClearDepthf(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// setup draw program
	draw_program = glCreateProgram();
	vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, vertex_shader_source, NULL);
	compile("vertex_shader_source", vs);
	fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, fragment_shader_source, NULL);
	compile("fragment_shader_source", fs);
	glAttachShader(draw_program, vs);
	glAttachShader(draw_program, fs);
	link_shader("draw_program", draw_program);
	glUseProgram(draw_program);

	// setup buffers
	GLuint iLocPosition = glGetAttribLocation(draw_program, "a_v4Position");
	GLuint iLocFillColor = glGetAttribLocation(draw_program, "a_v4FillColor");
	glEnableVertexAttribArray(iLocFillColor);
	glEnableVertexAttribArray(iLocPosition);
	glVertexAttribPointer(iLocFillColor, 4, GL_FLOAT, GL_FALSE, 0, triangleColors);
	glVertexAttribPointer(iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, triangleVertices);

	return 0;
}

// first frame render something, second frame verify it
static void callback_draw(PADEMO *handle, void *user_data)
{
	glUseProgram(draw_program);
	glDrawRangeElements(GL_TRIANGLES, 0, 2, 3, GL_UNSIGNED_INT, indices);
	glStateDump_ARM();

	// verify in retracer
	assert_fb(handle->width, handle->height);
}

static void test_cleanup(PADEMO *handle, void *user_data)
{
	glDeleteShader(vs);
	glDeleteShader(fs);
	glDeleteProgram(draw_program);
}

int main()
{
	return init("drawrange_1", callback_draw, setupGraphics, test_cleanup);
}
