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
    "#version 320 es \n"
    "precision mediump float; \n"
    "in vec4 texcoord; \n"
    "out vec4 fragmentColor; \n"
    "uniform highp samplerCubeArray s_texture; \n"
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
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, g_textureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_RGBA, 2, 2, 12, 0, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]);
    glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, GL_RGBA, 1, 1, 12, 0, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[192]);

    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_LOD, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAX_LOD, 1);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_REPEAT);

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


static void callback_draw(PADEMO *handle, void *user_data)
{
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
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
    return init("ext_texture_cube_map_array", callback_draw, setupGraphics, test_cleanup);
}
