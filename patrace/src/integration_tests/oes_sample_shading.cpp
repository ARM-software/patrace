#include "pa_demo.h"
#include "paframework.h"
#include "paframework_gl.h"

#ifdef PAFRAMEWORK_OPENGL
// Rename GLES extension functions to OpenGL functions
#define glMinSampleShadingOES glMinSampleShading
#define GL_SAMPLE_SHADING_OES     GL_SAMPLE_SHADING
#define GL_MIN_SAMPLE_SHADING_VALUE_OES GL_MIN_SAMPLE_SHADING_VALUE
#endif

static GLuint gvPositionHandle;

const char *vertex_shader_source[] = {
    "#version 310 es \n"
    "in vec4 vPosition; \n"
    "out vec4 c; \n"
    " "
    "void main() \n"
    "{ \n"
    "    gl_Position = vPosition; \n"
    "    c = vPosition; \n"
    "}"
};

const char *fragment_shader_source[] = {
    "#version 310 es \n"
    "#extension GL_OES_sample_variables : require\n"
    "precision mediump float; \n"
    "in vec4 c; \n"
    "out vec4 fragmentColor; \n"
    "uniform int mask; \n"
    " \n"
    "void main() \n"
    "{ \n"
    "    fragmentColor = vec4(gl_SamplePosition, float(gl_SampleID) / float(gl_NumSamples), 1.0f); \n"
    "    gl_SampleMask[0] = mask; \n"
    "}"
};

static int width = 1024;
static int height = 600;
static GLuint vs, fs, draw_program;

bool check_feature_availability()
{
#ifndef PAFRAMEWORK_OPENGL
    if (!PAFW_GL_Is_GLES_Extension_Supported("GL_OES_sample_shading"))
    {
        PALOGE("Extention OES_sample_shading not found -- this may not work\n");
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
        PALOGE("The extension OES_sample_shading is not available\n");
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

//DISCUSSION:
//    GL(4.4) says :
//        "Multisample rasterization is enabled or disabled by calling Enable or
//        Disable with the symbolic constant MULTISAMPLE."
//
//        GL ES(3.0.2) says :
//        "Multisample rasterization cannot be enabled or disabled after a GL
//        context is created."
//
//        RESOLVED.Multisample rasterization should be based on the target
//        surface properties.Will not pick up the explicit multisample
//        enable, but the language for ES3.0.2 doesn't sound right either.
//        Bug 10690 tracks this and it should be fixed in later versions
//        of the ES3.0 specification.
#ifdef PAFRAMEWORK_OPENGL
    glEnable(GL_MULTISAMPLE);
#endif
    glEnable(GL_SAMPLE_SHADING_OES);
    glMinSampleShadingOES(1.0);

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

    GLboolean ret = GL_FALSE;
#ifdef PAFRAMEWORK_OPENGL
    glIsEnabled(GL_MULTISAMPLE);
#endif
    ret = glIsEnabled(GL_SAMPLE_SHADING_OES);
    assert(ret == GL_TRUE);

    GLfloat min_value;
    glGetFloatv(GL_MIN_SAMPLE_SHADING_VALUE_OES, &min_value);

    for (int i = 0; i < triangle_num; i++)
    {
        PAGL(glUseProgram(draw_program));
        glUniform1i(glGetUniformLocation(draw_program, "mask"), multisample_mask[i]);

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
    return init("oes_sample_shading", pafw_handle, callback_draw, setupGraphics, test_cleanup);
}
