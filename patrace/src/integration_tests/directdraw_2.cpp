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
	precision mediump float;
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

static int width = 1024;
static int height = 600;
static GLuint vpos_obj, vcol_obj, vs, fs, draw_program, vao, index_obj;

static int setupGraphics(PAFW_HANDLE pafw_handle, int w, int h, void *user_data)
{
	setup();

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
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	GLuint iLocPosition = glGetAttribLocation(draw_program, "a_v4Position");
	GLuint iLocFillColor = glGetAttribLocation(draw_program, "a_v4FillColor");
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
	glGenBuffers(1, &index_obj);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_obj);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * sizeof(GLuint), indices, GL_STATIC_DRAW);

	return 0;
}

static void callback_draw(PAFW_HANDLE pafw_handle, void *user_data)
{
	glUseProgram(draw_program);
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, NULL);
	glStateDump_ARM();

	// verify in retracer
	assert_fb(width, height);
}

static void test_cleanup(PAFW_HANDLE pafw_handle, void *user_data)
{
	glDeleteVertexArrays(1, &vao);
	glDeleteShader(vs);
	glDeleteShader(fs);
	glDeleteProgram(draw_program);
	glDeleteBuffers(1, &vcol_obj);
	glDeleteBuffers(1, &vpos_obj);
	glDeleteBuffers(1, &index_obj);
}

#include "paframework_android_glue.h"

int PAFW_Entry_Point(PAFW_HANDLE pafw_handle)
{
	return init("directdraw_2", pafw_handle, callback_draw, setupGraphics, test_cleanup);
}
