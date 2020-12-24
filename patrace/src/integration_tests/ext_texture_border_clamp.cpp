#include "pa_demo.h"

static GLuint gvPositionHandle;

const char *vertex_shader_source[] = GLSL_VS(
	in vec4 vPosition;
	out vec4 c;
	void main()
	{
		gl_Position = vPosition;
		c = vec4(vPosition.xy, 0.0f, 1.0f);
	}
);

const char *fragment_shader_source[] = GLSL_FS(
	precision mediump float;
	in vec4 c;
	out vec4 fragmentColor;
	uniform highp sampler2D s_texture;
	void main()
	{
		fragmentColor = texture(s_texture, c.xy);
	}
);

static int width = 1024;
static int height = 600;
static GLuint vs, fs, draw_program;
static GLuint g_textureId, g_samplerId;

static GLubyte pixels[4 * 4] =
{
    0, 0, 0, 255,
    255, 0, 0, 255,
    0, 255, 0, 255,
    0, 0, 255, 255
};

const GLfloat borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };


static int setupGraphics(PADEMO *handle, int w, int h, void *user_data)
{
    width = w;
    height = h;

    // setup space
    glViewport(0, 0, width, height);

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

    glGenTextures(1, &g_textureId);
    glBindTexture(GL_TEXTURE_2D, g_textureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glGenSamplers(1, &g_samplerId);
    glBindSampler(0, g_samplerId);
    glSamplerParameteri(g_samplerId, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(g_samplerId, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glSamplerParameteri(g_samplerId, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glSamplerParameteri(g_samplerId, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glSamplerParameterfv(g_samplerId, GL_TEXTURE_BORDER_COLOR, borderColor);

    GLint binding;
    glGetIntegerv(GL_SAMPLER_BINDING, &binding);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_textureId);
    glUniform1i(glGetUniformLocation(draw_program, "s_texture"), 0);

    return 0;
}

static const int triangle_num = 4;
static const GLfloat gTriangleVertices[triangle_num][9] =
{
    { -0.5f, 0.875f, 0, -0.875f, 0.125f, 0, -0.125f, 0.125f, 0 },
    { 0.5f, 0.875f, 0, 0.125f, 0.125f, 0, 0.875f, 0.125f, 0 },
    { -0.5f, -0.125f, 0, -0.875f, -0.875f, 0, -0.125f, -0.875f, 0 },
    { 0.5f, -0.125f, 0, 0.125f, -0.875f, 0, 0.875f, -0.875f, 0 }
};

static const int multisample_mask[triangle_num]
{
    0x1, 0x3, 0x7, 0xf
};

static void callback_draw(PADEMO *handle, void *user_data)
{
    glClearColor(0.0f, 0.5f, 0.5f, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    for (int i = 0; i < triangle_num; i++)
    {
        glUseProgram(draw_program);

        glVertexAttribPointer(gvPositionHandle, 3, GL_FLOAT, GL_FALSE, 0, gTriangleVertices[i]);
        glEnableVertexAttribArray(gvPositionHandle);

        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    glStateDump_ARM();
    assert_fb(width, height);
}

static void test_cleanup(PADEMO *handle, void *user_data)
{
    glDeleteShader(vs);
    glDeleteShader(fs);
    glDeleteProgram(draw_program);
}

int main()
{
    return init("ext_texture_border_clamp", callback_draw, setupGraphics, test_cleanup);
}
