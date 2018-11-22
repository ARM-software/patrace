#include "pa_demo.h"
#include "paframework_gl.h"

#ifdef PAFRAMEWORK_OPENGL
// Rename GLES extension functions to OpenGL functions
//#define glTexBufferEXT glTexBuffer
#define glTexBufferRangeEXT glTexBufferRange
//#define GL_TEXTURE_BUFFER_EXT GL_TEXTURE_BUFFER
#define GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT_EXT GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT
#define GL_TEXTURE_BUFFER_SIZE_EXT GL_TEXTURE_BUFFER_SIZE
#define GL_TEXTURE_BUFFER_OFFSET_EXT GL_TEXTURE_BUFFER_OFFSET
#endif

static GLuint gvPositionHandle;

PACKED(struct params
{
    GLuint count;
    GLuint primCount;
    GLuint first;
    GLuint baseInstance;
});

const char *vertex_shader_source[] = {
    "#version 310 es\n"
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
    "#version 310 es\n"
    "#extension GL_EXT_texture_buffer: require\n"
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


bool check_feature_availability()
{
    GLint major_version;
    GLint minor_version;
    int version;

    glGetIntegerv(GL_MAJOR_VERSION, &major_version);
    glGetIntegerv(GL_MINOR_VERSION, &minor_version);
    version = 100 * major_version + 10 * minor_version;

#ifndef PAFRAMEWORK_OPENGL
    if (version >= 300 && PAFW_GL_Is_GLES_Extension_Supported("GL_EXT_texture_buffer"))
    {
        return true;
    }
    else
    {
        PALOGE("The GLES version (currently %d) must be 300 or higher and the extention EXT_texture_buffer must be supported\n", version);
        return true;
    }    
#else
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
        PALOGE("The extension EXT_texture_buffer is not available\n");
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

    glGenBuffers(1, &g_bufferId);
    glBindBuffer(GL_TEXTURE_BUFFER_EXT, g_bufferId);
    glBufferData(GL_TEXTURE_BUFFER_EXT, sizeof(pixels), pixels, GL_STATIC_READ);
    glTexBufferEXT(GL_TEXTURE_BUFFER_EXT, GL_RGBA8, g_bufferId);
    
    glActiveTexture(GL_TEXTURE0);    
    glUniform1i(glGetUniformLocation(draw_program, "s_texture"), 0);

    GLint value;
    glGetIntegerv(GL_TEXTURE_BUFFER_EXT, &value);
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE_EXT, &value);
    glGetIntegerv(GL_TEXTURE_BINDING_BUFFER_EXT, &value);
    glGetIntegerv(GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT_EXT, &value);

    GLsizei length;
    GLint size;
    GLenum type;
    GLchar name[255];
    glGetActiveUniform(draw_program, 0, 1, &length, &size, &type, name);

    glGetTexLevelParameteriv(GL_TEXTURE_BUFFER_EXT, 0, GL_TEXTURE_BUFFER_DATA_STORE_BINDING_EXT, &value);
    glGetTexLevelParameteriv(GL_TEXTURE_BUFFER_EXT, 0, GL_TEXTURE_BUFFER_OFFSET_EXT, &value);
    glGetTexLevelParameteriv(GL_TEXTURE_BUFFER_EXT, 0, GL_TEXTURE_BUFFER_SIZE_EXT, &value);
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
    return init("ext_texture_buffer", pafw_handle, callback_draw, setupGraphics, test_cleanup);
}

