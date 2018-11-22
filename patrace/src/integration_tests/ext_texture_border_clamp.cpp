#include "pa_demo.h"
#include "paframework_gl.h"

#ifdef PAFRAMEWORK_OPENGL
// Rename GLES extension functions to OpenGL functions
#define GL_CLAMP_TO_BORDER_EXT GL_CLAMP_TO_BORDER
#define GL_TEXTURE_BORDER_COLOR_EXT GL_TEXTURE_BORDER_COLOR
#endif

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
	uniform sampler2D s_texture;
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


bool check_feature_availability()
{
#ifndef PAFRAMEWORK_OPENGL
    if (!PAFW_GL_Is_GLES_Extension_Supported("GL_EXT_texture_border_clamp"))
    {
        PALOGE("The extention EXT_texture_border_clamp not found -- this may not work\n");
    }
    return true; // try anyway, works on Nvidia desktop EGL
#else
    GLint major_version;
    GLint minor_version;
    int version;

    glGetIntegerv(GL_MAJOR_VERSION, &major_version);
    glGetIntegerv(GL_MINOR_VERSION, &minor_version);
    version = 100 * major_version + 10 * minor_version;

    if (version >= 430)
    {
        return true;
    }
    PALOGE("The OpenGLversion (currently %d) must be 430 or higher\n", version);
    return false;
#endif
}

static int setupGraphics(PAFW_HANDLE pafw_handle, int w, int h, void *user_data)
{
    setup();

    width = w;
    height = h;

    if (!check_feature_availability())
    {
        PALOGE("The extension EXT_texture_border_clamp is not available\n");
        return 1;
    }

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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER_EXT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER_EXT);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR_EXT, borderColor);

    glGenSamplers(1, &g_samplerId);
    glBindSampler(0, g_samplerId);
    glSamplerParameteri(g_samplerId, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(g_samplerId, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glSamplerParameteri(g_samplerId, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER_EXT);
    glSamplerParameteri(g_samplerId, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER_EXT);
    glSamplerParameterfv(g_samplerId, GL_TEXTURE_BORDER_COLOR_EXT, borderColor);

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

static void callback_draw(PAFW_HANDLE pafw_handle, void *user_data)
{
    PAGL(glClearColor(0.0f, 0.5f, 0.5f, 1.0f));
    PAGL(glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT));

    for (int i = 0; i < triangle_num; i++)
    {
        PAGL(glUseProgram(draw_program));

        PAGL(glVertexAttribPointer(gvPositionHandle, 3, GL_FLOAT, GL_FALSE, 0, gTriangleVertices[i]));
        PAGL(glEnableVertexAttribArray(gvPositionHandle));

        PAGL(glDrawArrays(GL_TRIANGLES, 0, 3));
    }

    glStateDump_ARM();
    assert_fb(width, height);
}

static void test_cleanup(PAFW_HANDLE pafw_handle, void *user_data)
{
    glDeleteShader(vs);
    glDeleteShader(fs);
    glDeleteProgram(draw_program);
}

#include "paframework_android_glue.h"

int PAFW_Entry_Point(PAFW_HANDLE pafw_handle)
{
    return init("ext_texture_border_clamp", pafw_handle, callback_draw, setupGraphics, test_cleanup);
}
