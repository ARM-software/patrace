#include "pa_demo.h"

const char *vertex_shader_source[] = {
    "#version 320 es\n"
    "in vec4 vPosition;\n"
    "out vec4 c;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    gl_Position = vec4((1.0 + float(gl_InstanceID) / 20.0) * vPosition.x, vPosition.y + float(gl_InstanceID) / 10.0, vPosition.z, vPosition.w);\n"
    "    c = vPosition;\n"
    "}"
};

const char *fragment_shader_source[] = {
    "#version 320 es\n"
    "precision mediump float;\n"
    "in vec4 c;\n"
    "layout(blend_support_all_equations) out;\n"
    "out vec4 fragmentColor;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    fragmentColor = vec4(1.0f, 0.0f, 0.0f, 0.5f);\n"
    "}"
};

const float triangleVertices[] =
{
    -0.5f, 0.5f, 0.0f,
    -0.5f, -0.5f, 0.0f,
    0.5f, -0.5f, 0.0f,
};

const float triangleColors[] =
{
    1.0, 0.0, 0.0, 1.0,
    0.0, 1.0, 0.0, 1.0,
    0.0, 1.0, 0.0, 1.0,
};

const float triangleTextureCoordinates[] =
{
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f
};

static int width = 1024;
static int height = 600;
static GLuint vs, fs, draw_program;

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

    return 0;
}

static const GLfloat gTriangleVertices[] = {
     0.0f,  0.5f,  0,
    -0.5f, -0.5f,  0,
     0.5f, -0.5f,  0
};

static int mode[] =
{
    GL_MULTIPLY_KHR,
    GL_SCREEN_KHR,
    GL_OVERLAY_KHR,
    GL_DARKEN_KHR,
    GL_LIGHTEN_KHR,
    GL_COLORDODGE_KHR,
    GL_COLORBURN_KHR,
    GL_HARDLIGHT_KHR,
    GL_SOFTLIGHT_KHR,
    GL_DIFFERENCE_KHR,
    GL_EXCLUSION_KHR,
    GL_HSL_HUE_KHR,
    GL_HSL_SATURATION_KHR,
    GL_HSL_COLOR_KHR,
    GL_HSL_LUMINOSITY_KHR
};

static void callback_draw(PADEMO *handle, void *user_data)
{
    GLuint gvPositionHandle = glGetAttribLocation(draw_program, "vPosition");

    glClearColor(0.0f, 0.5f, 0.5f, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glUseProgram(draw_program);

    glVertexAttribPointer(gvPositionHandle, 3, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
    glEnableVertexAttribArray(gvPositionHandle);

    for (unsigned i = 0; i < sizeof(mode) / sizeof(int); i++)
    {
        glBlendEquation(mode[i]);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBlendBarrier();
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
    return init("khr_blend_equation_advanced", callback_draw, setupGraphics, test_cleanup);
}
