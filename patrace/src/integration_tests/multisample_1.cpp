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
	0.35f,  0.5f, 0.0f,
	-0.5f, -0.25f, 0.0f,
	0.5f, -0.25f, 0.0f,
	-0.35f,  0.5f, 0.0f,
	0.0f, -0.75f, 0.0f,
};

const float triangleColors[] =
{
	1.0, 0.2, 0.2, 1.0,
	0.0, 1.0, 0.0, 1.0,
	0.2, 0.2, 1.0, 1.0,
	0.5, 1.0, 0.0, 1.0,
	0.0, 1.0, 0.5, 1.0,
};

static GLfloat unoptimizer = 0.0f;
static GLuint vpos_obj, vcol_obj, vao, fb, tex, draw_program, vs, fs, rb, width, height;
const int positions = 5;

static int setupGraphics(PADEMO *handle, int w, int h, void *user_data)
{
	width = w;
	height = h;

	// setup space
	glViewport(0, 0, w, h);
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
	glBufferData(GL_ARRAY_BUFFER, positions * 4 * sizeof(GLfloat), triangleColors, GL_STATIC_DRAW);
	glEnableVertexAttribArray(iLocFillColor);
	glVertexAttribPointer(iLocFillColor, 4, GL_FLOAT, GL_FALSE, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glGenBuffers(1, &vpos_obj);
	glBindBuffer(GL_ARRAY_BUFFER, vpos_obj);
	glBufferData(GL_ARRAY_BUFFER, positions * 3 * sizeof(GLfloat), triangleVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(iLocPosition);
	glVertexAttribPointer(iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// how many samples does GPU support? we use as many as we can!
	GLint max_samples;
	glGetIntegerv(GL_MAX_SAMPLES, &max_samples);

	// setup multisampling
	GLenum internalformat = fb_internalformat();
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, tex);
	glObjectLabel(GL_TEXTURE, tex, -1, "Multisample TEX");
	glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, internalformat, w, h, GL_TRUE);
	glGenFramebuffers(1, &fb);
	glBindFramebuffer(GL_FRAMEBUFFER, fb);
	glObjectLabel(GL_FRAMEBUFFER, fb, -1, "Multisample FB");
	glGenRenderbuffers(1, &rb);
	glBindRenderbuffer(GL_RENDERBUFFER, rb);
	glObjectLabel(GL_RENDERBUFFER, rb, -1, "Multisample RB");
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, max_samples, internalformat, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rb);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, tex, 0);
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		PALOGE("GL_FRAMEBUFFER_COMPLETE failed\n");
	}

	// check glGetMultisamplefv()
	GLint samples;
	glGetIntegerv(GL_SAMPLES, &samples);
	for (int i = 0; i < samples; i++)
	{
		GLfloat values[2];
		glGetMultisamplefv(GL_SAMPLE_POSITION, i, values);
		unoptimizer += values[0]; // prevent compiler from optimizing away the above call
	}

	return 0;
}

// first frame render something, second frame verify it
static void callback_draw(PADEMO *handle, void *user_data)
{
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.0f, 0.0f, 0.5f, 1.0f);
	glClearDepthf(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindVertexArray(vao);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, tex);
	glBindFramebuffer(GL_FRAMEBUFFER, fb);
	glBindRenderbuffer(GL_RENDERBUFFER, rb);
	glUseProgram(draw_program);

	// draw with multisampling to our new framebuffer
	glDrawArrays(GL_LINE_LOOP, 0, positions);

	// blit custom framebuffer to front buffer
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fb);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	glStateDump_ARM();

	// verify in retracer
	assert_fb(width, height);
}

static void test_cleanup(PADEMO *handle, void *user_data)
{
	// make sure defaults are reset (important for android)
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	glBindVertexArray(0);

	// clean up
	glDeleteRenderbuffers(1, &rb);
	glDeleteFramebuffers(1, &fb);
	glDeleteVertexArrays(1, &vao);
	glDeleteTextures(1, &tex);
	glDeleteShader(vs);
	glDeleteShader(fs);
	glDeleteProgram(draw_program);
	glDeleteBuffers(1, &vcol_obj);
	glDeleteBuffers(1, &vpos_obj);
}

int main()
{
	return init("multisample_1", callback_draw, setupGraphics, test_cleanup);
}
