#include "pa_demo.h"
#include "paframework_gl.h"

#ifdef PAFRAMEWORK_OPENGL
// Rename GLES extension functions to OpenGL functions
#define GL_TEXTURE_CUBE_MAP_ARRAY_EXT GL_TEXTURE_CUBE_MAP_ARRAY
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
#ifdef PAFRAMEWORK_OPENGL
    "#version 400 \n"
#else
    "#version 310 es\n"
#endif
    "in vec4 vPosition; \n"
    "out vec4 texcoord; \n"
    " "
    "void main() \n"
    "{ \n"
    "    gl_Position = vPosition; \n"
    "    if (vPosition.x > -1.0f && vPosition.x < -2.0f / 3.0f) \n"
    "    { \n"
    "        texcoord.xyz = vec3(-1.0f, 0.0f, 0.0f); \n"
    "    } \n"
    "    if (vPosition.x > -2.0f / 3.0f && vPosition.x < -1.0f / 3.0f) \n"
    "    { \n"
    "        texcoord.xyz = vec3(1.0f, 0.0f, 0.0f); \n"
    "    } \n"
    "    if (vPosition.x > -1.0f / 3.0f && vPosition.x < 0.0f) \n"
    "    { \n"
    "        texcoord.xyz = vec3(0.0f, -1.0f, 0.0f); \n"
    "    } \n"
    "    if (vPosition.x > 0.0f && vPosition.x < 1.0f / 3.0f) \n"
    "    { \n"
    "        texcoord.xyz = vec3(0.0f, 1.0f, 0.0f); \n"
    "    } \n"
    "    if (vPosition.x > 1.0f / 3.0f && vPosition.x < 2.0f / 3.0f) \n"
    "    { \n"
    "        texcoord.xyz = vec3(0.0f, 0.0f, -1.0f); \n"
    "    } \n"
    "    if (vPosition.x > 2.0f / 3.0f && vPosition.x < 1.0f) \n"
    "    { \n"
    "        texcoord.xyz = vec3(0.0f, 0.0f, 1.0f); \n"
    "    } \n"
    "    texcoord.w = floor(vPosition.y * 2.0f + 2.0f); \n"
    "}"
};

const char *fragment_shader_source[] = {
#ifdef PAFRAMEWORK_OPENGL
    "#version 400 \n"
#else
    "#version 310 es \n"
#endif
    "#extension GL_EXT_texture_cube_map_array: require \n"
    "precision mediump float; \n"
    "in vec4 texcoord; \n"
    "out vec4 fragmentColor; \n"
    "uniform samplerCubeArray s_texture; \n"
    " \n"
    "void main() \n"
    "{ \n"
    "    vec4 modifiedCoord; \n"
    "    modifiedCoord.xyz = texcoord.xyz; \n"
    "    modifiedCoord.w = mod(texcoord.w, 2.0f); \n"
    "    float lod = floor(texcoord.w / 2.0f); \n"
    "    fragmentColor = textureLod( s_texture, modifiedCoord, lod ); \n"
    "}"
};

static int width = 1024;
static int height = 600;
static GLuint vs, fs, draw_program;
static GLuint g_textureId;

static GLubyte pixels[2 * 6 * 4 * 4 + 2 * 6 * 4 * 1] =
{
        255, 0, 0, 255,
        255, 0, 0, 255,
        255, 0, 0, 255,
        255, 0, 0, 255,

        255, 255, 0, 255,
        255, 255, 0, 255,
        255, 255, 0, 255,
        255, 255, 0, 255,

        255, 0, 255, 255,
        255, 0, 255, 255,
        255, 0, 255, 255,
        255, 0, 255, 255,

        0, 255, 255, 255,
        0, 255, 255, 255,
        0, 255, 255, 255,
        0, 255, 255, 255,

        0, 255, 0, 255,
        0, 255, 0, 255,
        0, 255, 0, 255,
        0, 255, 0, 255,

        0, 0, 255, 255,
        0, 0, 255, 255,
        0, 0, 255, 255,
        0, 0, 255, 255,

        127, 0, 0, 255,
        127, 0, 0, 255,
        127, 0, 0, 255,
        127, 0, 0, 255,

        127, 127, 0, 255,
        127, 127, 0, 255,
        127, 127, 0, 255,
        127, 127, 0, 255,

        127, 0, 127, 255,
        127, 0, 127, 255,
        127, 0, 127, 255,
        127, 0, 127, 255,

        0, 127, 127, 255,
        0, 127, 127, 255,
        0, 127, 127, 255,
        0, 127, 127, 255,

        0, 127, 0, 255,
        0, 127, 0, 255,
        0, 127, 0, 255,
        0, 127, 0, 255,

        0, 0, 127, 255,
        0, 0, 127, 255,
        0, 0, 127, 255,
        0, 0, 127, 255,

        192, 0, 0, 255,

        192, 192, 0, 255,

        192, 0, 192, 255,

        0, 192, 192, 255,

        0, 192, 0, 255,

        0, 0, 192, 255,

        64, 0, 0, 255,

        64, 64, 0, 255,

        64, 0, 64, 255,

        0, 64, 64, 255,

        0, 64, 0, 255,

        0, 0, 64, 255,
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
    if (version >= 300 && PAFW_GL_Is_GLES_Extension_Supported("GL_EXT_texture_cube_map_array"))
    {
        return true;
    }
    else
    {
        PALOGE("The GLES version (currently %d) must be 300 or higher and the extention EXT_texture_cube_map_array must be supported\n", version);
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
        PALOGE("The extension EXT_texture_cube_map_array is not available\n");
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
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY_EXT, g_textureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    //glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY_EXT, 2, GL_RGBA8, 2, 2, 12);
    glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY_EXT, 0, GL_RGBA, 2, 2, 12, 0, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]);
    glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY_EXT, 1, GL_RGBA, 1, 1, 12, 0, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[192]);

    //for (int level = 0; level < 2; level++)
    //{
    //    //for (int layer = 0; layer < 2; layer++)
    //    //{
    //        //for (int face = 0; face < 6; face++)
    //        //{
    //        //    GLenum target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;
    //        glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY_EXT, level, 0, 0, 0, 2, 2, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[level * 192]);
    //        //}
    //    //}
    //}
    //glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_EXT, GL_TEXTURE_MIN_LOD, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_EXT, GL_TEXTURE_MAX_LOD, 1);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_EXT, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_EXT, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_EXT, GL_TEXTURE_WRAP_R, GL_REPEAT);

    //glBindTexture(GL_TEXTURE_2D_ARRAY, g_textureId);
    //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    //glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, 2, 2, 12, 0, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]);
    //glTexImage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA, 1, 1, 12, 0, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[192]);


    //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_LOD, 0);
    //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LOD, 1);
    //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_REPEAT);
    
    glActiveTexture(GL_TEXTURE0);    
    glUniform1i(glGetUniformLocation(draw_program, "s_texture"), 0);

    return 0;
}

static const int triangle_num = 24;
static const GLfloat gTriangleVertices[triangle_num][9] =
{
    { -5.0f / 6.0f, 7.0f / 8.0f, 0.0f, -11.0f / 12.0f, 5.0f / 8.0f, 0.0f, -9.0f / 12.0f, 5.0f / 8.0f, 0.0f },
    { -3.0f / 6.0f, 7.0f / 8.0f, 0.0f, -7.0f / 12.0f, 5.0f / 8.0f, 0.0f, -5.0f / 12.0f, 5.0f / 8.0f, 0.0f },
    { -1.0f / 6.0f, 7.0f / 8.0f, 0.0f, -3.0f / 12.0f, 5.0f / 8.0f, 0.0f, -1.0f / 12.0f, 5.0f / 8.0f, 0.0f },
    { 1.0f / 6.0f, 7.0f / 8.0f, 0.0f, 1.0f / 12.0f, 5.0f / 8.0f, 0.0f, 3.0f / 12.0f, 5.0f / 8.0f, 0.0f },
    { 3.0f / 6.0f, 7.0f / 8.0f, 0.0f, 5.0f / 12.0f, 5.0f / 8.0f, 0.0f, 7.0f / 12.0f, 5.0f / 8.0f, 0.0f },
    { 5.0f / 6.0f, 7.0f / 8.0f, 0.0f, 9.0f / 12.0f, 5.0f / 8.0f, 0.0f, 11.0f / 12.0f, 5.0f / 8.0f, 0.0f },
    { -5.0f / 6.0f, 3.0f / 8.0f, 0.0f, -11.0f / 12.0f, 1.0f / 8.0f, 0.0f, -9.0f / 12.0f, 1.0f / 8.0f, 0.0f },
    { -3.0f / 6.0f, 3.0f / 8.0f, 0.0f, -7.0f / 12.0f, 1.0f / 8.0f, 0.0f, -5.0f / 12.0f, 1.0f / 8.0f, 0.0f },
    { -1.0f / 6.0f, 3.0f / 8.0f, 0.0f, -3.0f / 12.0f, 1.0f / 8.0f, 0.0f, -1.0f / 12.0f, 1.0f / 8.0f, 0.0f },
    { 1.0f / 6.0f, 3.0f / 8.0f, 0.0f, 1.0f / 12.0f, 1.0f / 8.0f, 0.0f, 3.0f / 12.0f, 1.0f / 8.0f, 0.0f },
    { 3.0f / 6.0f, 3.0f / 8.0f, 0.0f, 5.0f / 12.0f, 1.0f / 8.0f, 0.0f, 7.0f / 12.0f, 1.0f / 8.0f, 0.0f },
    { 5.0f / 6.0f, 3.0f / 8.0f, 0.0f, 9.0f / 12.0f, 1.0f / 8.0f, 0.0f, 11.0f / 12.0f, 1.0f / 8.0f, 0.0f },
    { -5.0f / 6.0f, -1.0f / 8.0f, 0.0f, -11.0f / 12.0f, -3.0f / 8.0f, 0.0f, -9.0f / 12.0f, -3.0f / 8.0f, 0.0f },
    { -3.0f / 6.0f, -1.0f / 8.0f, 0.0f, -7.0f / 12.0f, -3.0f / 8.0f, 0.0f, -5.0f / 12.0f, -3.0f / 8.0f, 0.0f },
    { -1.0f / 6.0f, -1.0f / 8.0f, 0.0f, -3.0f / 12.0f, -3.0f / 8.0f, 0.0f, -1.0f / 12.0f, -3.0f / 8.0f, 0.0f },
    { 1.0f / 6.0f, -1.0f / 8.0f, 0.0f, 1.0f / 12.0f, -3.0f / 8.0f, 0.0f, 3.0f / 12.0f, -3.0f / 8.0f, 0.0f },
    { 3.0f / 6.0f, -1.0f / 8.0f, 0.0f, 5.0f / 12.0f, -3.0f / 8.0f, 0.0f, 7.0f / 12.0f, -3.0f / 8.0f, 0.0f },
    { 5.0f / 6.0f, -1.0f / 8.0f, 0.0f, 9.0f / 12.0f, -3.0f / 8.0f, 0.0f, 11.0f / 12.0f, -3.0f / 8.0f, 0.0f },
    { -5.0f / 6.0f, -5.0f / 8.0f, 0.0f, -11.0f / 12.0f, -7.0f / 8.0f, 0.0f, -9.0f / 12.0f, -7.0f / 8.0f, 0.0f },
    { -3.0f / 6.0f, -5.0f / 8.0f, 0.0f, -7.0f / 12.0f, -7.0f / 8.0f, 0.0f, -5.0f / 12.0f, -7.0f / 8.0f, 0.0f },
    { -1.0f / 6.0f, -5.0f / 8.0f, 0.0f, -3.0f / 12.0f, -7.0f / 8.0f, 0.0f, -1.0f / 12.0f, -7.0f / 8.0f, 0.0f },
    { 1.0f / 6.0f, -5.0f / 8.0f, 0.0f, 1.0f / 12.0f, -7.0f / 8.0f, 0.0f, 3.0f / 12.0f, -7.0f / 8.0f, 0.0f },
    { 3.0f / 6.0f, -5.0f / 8.0f, 0.0f, 5.0f / 12.0f, -7.0f / 8.0f, 0.0f, 7.0f / 12.0f, -7.0f / 8.0f, 0.0f },
    { 5.0f / 6.0f, -5.0f / 8.0f, 0.0f, 9.0f / 12.0f, -7.0f / 8.0f, 0.0f, 11.0f / 12.0f, -7.0f / 8.0f, 0.0f },

};


static void callback_draw(PAFW_HANDLE pafw_handle, void *user_data)
{
    PAGL(glClearColor(0.2f, 0.2f, 0.2f, 1.0f));
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
    return init("ext_texture_cube_map_array", pafw_handle, callback_draw, setupGraphics, test_cleanup);
}
