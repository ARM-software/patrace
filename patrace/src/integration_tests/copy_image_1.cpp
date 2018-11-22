#include "pa_demo.h"
#include "paframework_gl.h"

#ifdef PAFRAMEWORK_OPENGL
// Rename GLES extension functions to OpenGL functions
#define glCopyImageSubDataEXT glCopyImageSubData
#endif

PACKED(struct params
{
    GLuint count;
    GLuint primCount;
    GLuint first;
    GLuint baseInstance;
});

const char *vertex_shader_source[] = GLSL_VS(
    in vec4 a_v4Position;
    in vec2 a_v4TexCoord;
    out vec2 v_v4TexCoord;
    void main()
    {
        v_v4TexCoord = a_v4TexCoord;
        gl_Position = a_v4Position;
    }
);

const char *fragment_shader_source[] = GLSL_FS(
    precision mediump float;
    in vec2 v_v4TexCoord;
    uniform sampler2D s_texture;
    out vec4 fragColor;
    void main()
    {
        fragColor = texture(s_texture, v_v4TexCoord);
    }
);

const float triangleVertices[] =
{
    -0.5f,  0.5f, 0.0f,
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
static GLuint indirect_buffer_obj, vpos_obj, vcol_obj, vs, fs, draw_program, vao;
static GLuint g_textureId, g_textureId2, vtex_coord_obj;

bool check_feature_availability()
{
    GLint major_version;
    GLint minor_version;
    int version;

    glGetIntegerv(GL_MAJOR_VERSION, &major_version);
    glGetIntegerv(GL_MINOR_VERSION, &minor_version);
    version = 100 * major_version + 10 * minor_version;

#ifndef PAFRAMEWORK_OPENGL
    if (version >= 300 && PAFW_GL_Is_GLES_Extension_Supported("GL_EXT_copy_image"))
    {
        return true;
    }
    PALOGE("The GLES version (currently %d) must be 300 or higher and the extention EXT_copy_image must be supported\n", version);
    return false;
#else
    if (version >= 430)
    {
        return true;
    }
    PALOGE("The OpenGLversion (currently %d) must be 430 or higher\n", version);
    return false;
#endif
}

static GLubyte pixels[4 * 3] =
{
    255,   0,   0,
      0, 255,   0,
      0,   0, 255,
    255, 255,   0
};

static GLubyte pixels2[4 * 3] =
{
    0,   255,     255,
    255,   255,     0,
    255,     0,   255,
    255,   255,   255
};

static int setupGraphics(PAFW_HANDLE pafw_handle, int w, int h, void *user_data)
{
    struct params obj;

    setup();

    width = w;
    height = h;

    if (!check_feature_availability())
    {
        PALOGE("The extension EXT_copy_image is not available\n");
        return 1;
    }

    // setup space
    glViewport(0, 0, width, height);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.5f, 1.0f);
    glClearDepthf(1.0f);
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
    obj.count = 3;
    obj.primCount = 1;
    obj.first = 0;
    obj.baseInstance = 0;
    glGenBuffers(1, &indirect_buffer_obj);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect_buffer_obj);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(obj), &obj, GL_STATIC_READ);
    GLuint iLocPosition = glGetAttribLocation(draw_program, "a_v4Position");
    GLuint iLocTexCoord = glGetAttribLocation(draw_program, "a_v4TexCoord");
    glGenBuffers(1, &vcol_obj);
    glBindBuffer(GL_ARRAY_BUFFER, vcol_obj);
    glBufferData(GL_ARRAY_BUFFER, 3 * 4 * sizeof(GLfloat), triangleColors, GL_STATIC_DRAW);
    glGenBuffers(1, &vpos_obj);
    glBindBuffer(GL_ARRAY_BUFFER, vpos_obj);
    glBufferData(GL_ARRAY_BUFFER, 3 * 3 * sizeof(GLfloat), triangleVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(iLocPosition);
    glVertexAttribPointer(iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glGenBuffers(1, &vtex_coord_obj);
    glBindBuffer(GL_ARRAY_BUFFER, vtex_coord_obj);
    glBufferData(GL_ARRAY_BUFFER, 2 * 3 * sizeof(GLfloat), triangleTextureCoordinates, GL_STATIC_DRAW);
    glEnableVertexAttribArray(iLocTexCoord);
    glVertexAttribPointer(iLocTexCoord, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    glGenTextures(1, &g_textureId);
    glBindTexture(GL_TEXTURE_2D, g_textureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, 2, 2);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 2, 2, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenTextures(1, &g_textureId2);
    glBindTexture(GL_TEXTURE_2D, g_textureId2);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, 2, 2);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 2, 2, GL_RGB, GL_UNSIGNED_BYTE, pixels2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    glCopyImageSubDataEXT(g_textureId2, GL_TEXTURE_2D, 0, 0, 0, 0, g_textureId, GL_TEXTURE_2D, 0, 0, 0, 0, 2, 2, 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_textureId);
    glUniform1i(glGetUniformLocation(draw_program, "s_texture" ), 0);

    return 0;
}

static void callback_draw(PAFW_HANDLE pafw_handle, void *user_data)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(draw_program);
    glDrawArraysIndirect(GL_TRIANGLES, NULL);
    glStateDump_ARM();
    assert_fb(width, height);
}

static void test_cleanup(PAFW_HANDLE pafw_handle, void *user_data)
{
    glDeleteVertexArrays(1, &vao);
    glDeleteShader(vs);
    glDeleteShader(fs);
    glDeleteProgram(draw_program);
    glDeleteBuffers(1, &indirect_buffer_obj);
    glDeleteBuffers(1, &vcol_obj);
    glDeleteBuffers(1, &vpos_obj);
}

#include "paframework_android_glue.h"

int PAFW_Entry_Point(PAFW_HANDLE pafw_handle)
{
    return init("copy_image_1", pafw_handle, callback_draw, setupGraphics, test_cleanup);
}
