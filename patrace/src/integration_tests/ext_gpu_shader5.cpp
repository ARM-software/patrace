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

const char *fragment_shader_source[] = GLSL_FS_5(
	precision mediump float;
	in vec4 c;
	uniform int samplerIndex;
	out vec4 fragmentColor;
	uniform sampler2D s_texture[2];
	layout (std140, column_major) uniform;
	layout(binding = 2, rgba8) uniform highp writeonly image2D s_image;
	layout(binding = 0) uniform a
	{
		ivec4 indirectIndex[2];
	} A;
	void main()
	{
		precise vec4 result;
		result = texture(s_texture[samplerIndex], c.xy);
		int index = A.indirectIndex[samplerIndex].x;
		vec4 red = textureGatherOffset(s_texture[index], c.xy, ivec2(1, 0), 0);
		vec4 green = textureGatherOffset(s_texture[index], c.xy, ivec2(1, 0), 1);
		vec4 blue = textureGatherOffset(s_texture[index], c.xy, ivec2(1, 0), 2);
		result.x = red.x;
		result.y = green.x;
		result.z = blue.x;
		result.w = 1.0f;
		result = fma(result, vec4(0.5f, 0.5f, 0.5f, 0.5f), vec4(0.2f, 0.2f, 0.2f, 0.2f));
		fragmentColor = result;
	}
);

static int width = 1024;
static int height = 600;
static GLuint vs, fs, draw_program;
static GLuint g_textureId, g_textureId2, g_textureId3, g_constBufferID;

static GLubyte pixels[4 * 4] =
{
    0, 0, 0, 255,
    255, 0, 0, 255,
    0, 255, 0, 255,
    0, 0, 255, 255
};

static GLubyte pixels2[4 * 4] =
{
    127, 0, 127, 255,
    0, 127, 0, 255,
    127, 127, 0, 255,
    0, 0, 127, 255
};

static GLubyte pixels3[4 * 4] =
{
    192, 192, 0, 255,
    192, 0, 192, 255,
    192, 192, 0, 255,
    0, 192, 192, 255
};

static GLubyte pixels4[4 * 4] =
{
    32, 32, 127, 255,
    0, 127, 0, 255,
    32, 127, 0, 255,
    0, 32, 127, 255
};

static GLint g_constValue[8]
{
    0, 0, 0, 0,
    1, 1, 1, 1
};

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

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &g_textureId);
    glBindTexture(GL_TEXTURE_2D, g_textureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glActiveTexture(GL_TEXTURE1);
    glGenTextures(1, &g_textureId2);
    glBindTexture(GL_TEXTURE_2D, g_textureId2);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glActiveTexture(GL_TEXTURE2);
    glGenTextures(1, &g_textureId3);
    glBindTexture(GL_TEXTURE_2D, g_textureId3);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexStorage2D(GL_TEXTURE_2D, 2, GL_RGBA8, 2, 2);

    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0, 0,
        2,
        2,
        GL_RGBA, GL_UNSIGNED_BYTE,
        pixels3);

    glTexSubImage2D(
        GL_TEXTURE_2D,
        1,
        0, 0,
        1,
        1,
        GL_RGBA, GL_UNSIGNED_BYTE,
        pixels4);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    glBindImageTexture(2, g_textureId3, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);

    glGenBuffers(1, &g_constBufferID);
    glBindBuffer(GL_UNIFORM_BUFFER, g_constBufferID);
    glBufferData(GL_UNIFORM_BUFFER, 8 * sizeof(int), g_constValue, GL_DYNAMIC_DRAW);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, g_constBufferID);

    glUniform1i(glGetUniformLocation(draw_program, "s_texture[0]"), 0);
    glUniform1i(glGetUniformLocation(draw_program, "s_texture[1]"), 1);

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
        glUniform1i(glGetUniformLocation(draw_program, "samplerIndex"), i % 2);

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
    return init("ext_gpu_shader5", callback_draw, setupGraphics, test_cleanup);
}
