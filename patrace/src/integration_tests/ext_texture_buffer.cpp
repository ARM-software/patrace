#include "pa_demo.h"

static GLuint gvPositionHandle;

PACKED(struct params
{
    GLuint count;
    GLuint primCount;
    GLuint first;
    GLuint baseInstance;
});

const char *vertex_shader_source[] = {
    "#version 320 es\n"
    "in vec4 vPosition; \n"
    "flat out int texcoord;\n"
    " "
    "void main() \n"
    "{ \n"
    "    gl_Position = vPosition; \n"
    "    texcoord = int(floor(vPosition.y + 1.0f) * 2.0f + floor(vPosition.x + 1.0f)); \n"
    "}"
};

const char *fragment_shader_source[] = {
    "#version 320 es\n"
    "precision mediump float; \n"
    "flat in int texcoord; \n"
    "out vec4 fragmentColor; \n"
    "uniform highp samplerBuffer s_texture;\n"
    " \n"
    "void main() \n"
    "{ \n"
    "    fragmentColor = texelFetch( s_texture, texcoord ); \n"
    "}"
};

static int width = 1024;
static int height = 600;
static GLuint vs, fs, draw_program;
static GLuint g_bufferId;

static GLubyte pixels[4 * 4] =
{
    0, 0, 0, 255,
    255, 0, 0, 255,
    0, 255, 0, 255,
    0, 0, 255, 255
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

    glGenBuffers(1, &g_bufferId);
    glBindBuffer(GL_TEXTURE_BUFFER_EXT, g_bufferId);
    glBufferData(GL_TEXTURE_BUFFER_EXT, sizeof(pixels), pixels, GL_STATIC_READ);
    glTexBuffer(GL_TEXTURE_BUFFER_EXT, GL_RGBA8, g_bufferId);

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(draw_program, "s_texture"), 0);

    GLint value;
    glGetIntegerv(GL_TEXTURE_BUFFER, &value);
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &value);
    glGetIntegerv(GL_TEXTURE_BINDING_BUFFER, &value);
    glGetIntegerv(GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT, &value);

    GLsizei length;
    GLint size;
    GLenum type;
    GLchar name[255];
    glGetActiveUniform(draw_program, 0, 1, &length, &size, &type, name);

    glGetTexLevelParameteriv(GL_TEXTURE_BUFFER, 0, GL_TEXTURE_BUFFER_DATA_STORE_BINDING, &value);
    glGetTexLevelParameteriv(GL_TEXTURE_BUFFER, 0, GL_TEXTURE_BUFFER_OFFSET, &value);
    glGetTexLevelParameteriv(GL_TEXTURE_BUFFER, 0, GL_TEXTURE_BUFFER_SIZE, &value);
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
    return init("ext_texture_buffer", callback_draw, setupGraphics, test_cleanup);
}

