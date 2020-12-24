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
	0.0f,  0.5f,
	-0.5f, -0.5f,
	0.5f, -0.5f
};

const unsigned char triangleColors[] =
{
	255, 0, 0, 255,
	0, 255, 0, 255,
	0, 0, 255, 255
};

const unsigned short indices[] =
{
      0,
      1,
      2,
      2,
      1,
      3,
      4,
      5,
      6,
      6,
      5,
      7,
      8,
      9,
      10,
      10,
      9,
      11,
      12,
      13,
      14,
      14,
      13,
      15
};

static GLuint vs, fs, draw_program, iLocPosition, iLocFillColor;
static char *buffer = nullptr;

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

	// create vertex array data
	const int chunks = 17;
	const int chunksize = 12;
	buffer = (char*)calloc(chunks * chunksize, 1);
	int t = 0;
	for (int i = 0; i < chunks; i++)
	{
		const int offset = i * chunksize;
		*((float*)(buffer + offset)) = triangleVertices[t*2 + 0] + 0.05 * (i / 3);
		*((float*)(buffer + offset + 4)) = triangleVertices[t*2 + 1] + 0.05 * (i / 3);
		buffer[offset + 8 + 0] = triangleColors[t*4 + 0];
		buffer[offset + 8 + 1] = triangleColors[t*4 + 1];
		buffer[offset + 8 + 2] = triangleColors[t*4 + 2];
		buffer[offset + 8 + 3] = triangleColors[t*4 + 3];
		t = i % 2;
	}

	// setup buffers
	iLocPosition = glGetAttribLocation(draw_program, "a_v4Position");
	iLocFillColor = glGetAttribLocation(draw_program, "a_v4FillColor");
	glVertexAttribPointer(iLocPosition, 2, GL_FLOAT, GL_FALSE, 12, buffer);
	glVertexAttribPointer(iLocFillColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, 12, (buffer + 8));
	glEnableVertexAttribArray(iLocFillColor);
	glEnableVertexAttribArray(iLocPosition);

	return 0;
}

static void callback_draw(PADEMO *handle, void *user_data)
{
	// Note that the max index value is actually 15 ...
	glDrawRangeElements(GL_TRIANGLES, 0, 16, 24, GL_UNSIGNED_SHORT, indices);
	glStateDump_ARM();
	assert_fb(handle->width, handle->height);
}

static void test_cleanup(PADEMO *handle, void *user_data)
{
	free(buffer);
	glDeleteShader(vs);
	glDeleteShader(fs);
	glDeleteProgram(draw_program);
}

int main()
{
	return init("drawrange_2", callback_draw, setupGraphics, test_cleanup);
}
