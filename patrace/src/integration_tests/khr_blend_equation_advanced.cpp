#include "pa_demo.h"
#include "paframework.h"
#include "paframework_gl.h"

#ifdef PAFRAMEWORK_OPENGL
// Rename GLES extension functions to OpenGL functions
#endif

const char *vertex_shader_source[] = {
    "#version 310 es \n"
    "in vec4 vPosition; \n"
    "out vec4 c; \n"
    " "
    "void main() \n"
    "{ \n"
    "    gl_Position = vec4((1.0 + float(gl_InstanceID) / 20.0) * vPosition.x, vPosition.y + float(gl_InstanceID) / 10.0, vPosition.z, vPosition.w); \n"
    "    c = vPosition; \n"
    "}"
};

const char *fragment_shader_source[] = {
    "#version 310 es \n"
    "#extension GL_KHR_blend_equation_advanced : require \n"
    "precision mediump float; \n"
    "in vec4 c; \n"
    "layout(blend_support_all_equations) out; \n"
    "out vec4 fragmentColor; \n"
    " "
    "void main() \n"
    "{ \n"
    "    fragmentColor = vec4(1.0f, 0.0f, 0.0f, 0.5f); \n"
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

bool check_feature_availability()
{
#ifndef PAFRAMEWORK_OPENGL
    if (!PAFW_GL_Is_GLES_Extension_Supported("GL_KHR_blend_equation_advanced"))
    {
        PALOGE("The extension KHR_blend_equation_advanced not found -- this may not work\n");
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
        PALOGE("The extension KHR_blend_equation_advanced is not available\n");
        return 1;
    }

    // setup color buffer
    //GLenum bufferMode[] = 
    //{
    //    GL_BACK,
    //    GL_NONE,
    //    GL_NONE,
    //    GL_NONE,
    //    GL_NONE,
    //    GL_NONE,
    //    GL_NONE,
    //    GL_NONE
    //};
    //glDrawBuffers(8, bufferMode);

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

static void callback_draw(PAFW_HANDLE pafw_handle, void *user_data)
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

        glBlendBarrierKHR();
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
    return init("khr_blend_equation_advanced", pafw_handle, callback_draw, setupGraphics, test_cleanup);
}
