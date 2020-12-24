// Test of geometry shaders

#include "pa_demo.h"

const char* vertex_shader_source[] = GLSL_VS(
	in vec2 pos;
	in vec3 color;
	in float sides;

	out vec3 vColor; // Output to geometry (or fragment) shader
	out float vSides;

	void main()
	{
		gl_Position = vec4(pos, 0.0, 1.0);
		vColor = color;
		vSides = sides;
	}
);

// Fragment shader
const char* fragment_shader_source[] = GLSL_FS(
	in vec3 fColor;
	out vec4 outColor;

	void main()
	{
		outColor = vec4(fColor, 1.0);
	}
);

const char *geometry_shader_source[] = GLSL_GS(
	layout(points) in;
	layout(line_strip, max_vertices = 64) out;

	in vec3 vColor[1];
	in float vSides[1];

	out vec3 fColor;

	const float PI = 3.1415926;

	void main()
	{
		fColor = vColor[0];

		for (int i = 0; i <= int(vSides[0]); i++)
		{
			// Angle between each side in radians
			float ang = PI * 2.0 / vSides[0] * float(i);

			// Offset from center of point (0.3 to accomodate for aspect ratio)
			vec4 offset = vec4(cos(ang) * 0.3, -sin(ang) * 0.4, 0.0, 0.0);
			gl_Position = gl_in[0].gl_Position + offset;

			EmitVertex();
		}
		EndPrimitive();
	}
);

float points[] =
{
	//  Coordinates - Color - Sides
	-0.45f,  0.45f, 1.0f, 0.0f, 0.0f,  4.0f,
	 0.45f,  0.45f, 0.0f, 1.0f, 0.0f,  8.0f,
	 0.45f, -0.45f, 0.0f, 0.0f, 1.0f, 16.0f,
	-0.45f, -0.45f, 1.0f, 1.0f, 0.0f, 32.0f
};

static int width = 1024;
static int height = 600;
static GLuint draw_program, vpos_obj, vcol_obj, vbo, vao, vs, fs, gs;

static int setupGraphics(PADEMO *handle, int w, int h, void *user_data)
{
	width = w;
	height = h;

	// setup space
	glViewport(0, 0, w, h);

	// setup draw program
	draw_program = glCreateProgram();
	vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, vertex_shader_source, NULL);
	compile("vertex_shader_source", vs);
	fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, fragment_shader_source, NULL);
	compile("fragment_shader_source", fs);
	gs = glCreateShader(GL_GEOMETRY_SHADER_OES);
	glShaderSource(gs, 1, geometry_shader_source, NULL);
	compile("geometry_shader_source", gs);
	glAttachShader(draw_program, vs);
	glAttachShader(draw_program, fs);
	glAttachShader(draw_program, gs);
	link_shader("draw_program", draw_program);
	glUseProgram(draw_program);

	// setup buffers
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);

	GLint posAttrib = glGetAttribLocation(draw_program, "pos");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);

	GLint colAttrib = glGetAttribLocation(draw_program, "color");
	glEnableVertexAttribArray(colAttrib);
	glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) (2 * sizeof(float)));

	GLint sidesAttrib = glGetAttribLocation(draw_program, "sides");
	glEnableVertexAttribArray(sidesAttrib);
	glVertexAttribPointer(sidesAttrib, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) (5 * sizeof(float)));

	return 0;
}

// first frame render something, second frame verify it
static void callback_draw(PADEMO *handle, void *user_data)
{
	glBindVertexArray(vao);
	glUseProgram(draw_program);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_POINTS, 0, 4);

	glStateDump_ARM();

	// verify in retracer
	assert_fb(width, height);
}

static void test_cleanup(PADEMO *handle, void *user_data)
{
	glDeleteShader(vs);
	glDeleteShader(fs);
	glDeleteShader(gs);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(draw_program);
	glDeleteBuffers(1, &vcol_obj);
	glDeleteBuffers(1, &vpos_obj);
}

int main()
{
	return init("geomtry_shader_1", callback_draw, setupGraphics, test_cleanup);
}
