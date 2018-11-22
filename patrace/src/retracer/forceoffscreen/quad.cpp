#include "quad.h"

#include "helper/states.h"
#include <dispatch/eglproc_auto.hpp>


Quad::Quad(int glesVer)
    : mOwnsGLObjects(true)
    , mGlesVer(glesVer)
    , mProg(0)
    , mTexUnifLoc(0)
    , mVBO(0)
    , mEBO(0)
{
    if (mGlesVer > 1)
    {
        CreateProgram();
    }

    CreateVboEbo();
}

Quad::~Quad()
{
    if (mOwnsGLObjects)
    {
        DeleteVboEbo();
        if (mGlesVer > 1)
        {
            DeleteProgram();
        }
    }
}

void Quad::ReleaseOwnershipOfGLObjects()
{
    mOwnsGLObjects = false;
}

void Quad::DrawTex1(unsigned int textureID)
{
    GLboolean       oEnableDepthTest;
    GLboolean       oEnableTex2D;
    GLboolean       oEnableCullFace;
    GLboolean       oEnableBlend;
    GLfloat         oMVMtx[16];
    GLfloat         oPrjMtx[16];
    GLfloat         oTexMtx[16];

    unsigned int    oActiveTex;
    unsigned int    oBindTex;
    int             oFrontFace;
    int             oMatrixMode;
    unsigned int    oVAO;
    // 1. Save current render state
    _glGetBooleanv(GL_DEPTH_TEST, &oEnableDepthTest);
    _glGetError(); // for a driver bug on Galaxy tablet 7.7
    _glGetBooleanv(GL_TEXTURE_2D, &oEnableTex2D);
    _glGetBooleanv(GL_CULL_FACE, &oEnableCullFace);
    _glGetBooleanv(GL_BLEND, &oEnableBlend);

    _glGetIntegerv(GL_MATRIX_MODE, &oMatrixMode);
    _glGetFloatv(GL_MODELVIEW_MATRIX, oMVMtx);
    _glGetFloatv(GL_PROJECTION_MATRIX, oPrjMtx);
    _glGetFloatv(GL_TEXTURE_MATRIX, oTexMtx);
    _glGetIntegerv(GL_MATRIX_MODE, (GLint*) &oMatrixMode);

    _glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&oActiveTex);
    _glActiveTexture(GL_TEXTURE0);
    _glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&oBindTex);
    _glGetIntegerv(GL_FRONT_FACE, &oFrontFace);
    _glGetIntegerv(GL_VERTEX_ARRAY_BINDING_OES, (GLint*)&oVAO);
    GLboolean oVertexArray = _glIsEnabled(GL_VERTEX_ARRAY);
    GLboolean oTextureCoordArray = _glIsEnabled(GL_TEXTURE_COORD_ARRAY);
    GLboolean oColorArray = _glIsEnabled(GL_COLOR_ARRAY);
    GLboolean oNormalArray = _glIsEnabled(GL_NORMAL_ARRAY);

    // 2. Draw


    _glDisable(GL_DEPTH_TEST);
    _glEnable(GL_TEXTURE_2D);
    _glDisable(GL_CULL_FACE);
    _glDisable(GL_BLEND);
    _glMatrixMode(GL_MODELVIEW);
    _glLoadIdentity();
    _glMatrixMode(GL_PROJECTION);
    _glLoadIdentity();
    _glMatrixMode(GL_TEXTURE);
    _glLoadIdentity();

    _glBindTexture(GL_TEXTURE_2D, textureID);
    _glBindVertexArrayOES(mVAO);
    _glFrontFace(GL_CCW);
    _glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);
    _glBindVertexArrayOES(oVAO);


    // 3. Restore render state
    if (!oVertexArray)
    {
        _glDisableClientState(GL_VERTEX_ARRAY);
    }
    if (!oTextureCoordArray)
    {
        _glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    if (oColorArray)
    {
        _glEnableClientState(GL_COLOR_ARRAY);
    }
    if (oNormalArray)
    {
        _glEnableClientState(GL_NORMAL_ARRAY);
    }



    _glBindTexture(GL_TEXTURE_2D, oBindTex);
    _glActiveTexture(oActiveTex);

    _glMatrixMode(GL_MODELVIEW);
    _glMultMatrixf(oMVMtx);
    _glMatrixMode(GL_PROJECTION);
    _glMultMatrixf(oPrjMtx);
    _glMatrixMode(GL_TEXTURE);
    _glMultMatrixf(oTexMtx);
    _glMatrixMode(oMatrixMode);

    if (!oEnableTex2D) _glDisable(GL_TEXTURE_2D);
    if (oEnableCullFace) _glEnable(GL_CULL_FACE);
    if (oEnableBlend) _glEnable(GL_BLEND);
    oEnableDepthTest ? _glEnable(GL_DEPTH_TEST) : _glDisable(GL_DEPTH_TEST);
    _glFrontFace(oFrontFace);
}

void Quad::DrawTex2(unsigned int textureID)
{
    // 1. Save current render state
    // Store state, restore when exiting this function
    State state({GL_DEPTH_TEST},
                {GL_ACTIVE_TEXTURE, GL_FRONT_FACE},
                 {}, true);
    unsigned int oVAO;
    if(mGlesVer >= 3)
    {
         oVAO= State::getIntegerv(GL_VERTEX_ARRAY_BINDING)[0];
    }
    else
    {
         oVAO= State::getIntegerv(GL_VERTEX_ARRAY_BINDING_OES)[0];
    }
    //_glGetUniformiv(oProg, mTexUnifLoc, &oUniform);
    // 2. Draw
    _glDisable(GL_DEPTH_TEST);

    _glUseProgram(mProg);
    _glActiveTexture(GL_TEXTURE0);
    GLint oTexId = State::getIntegerv(GL_TEXTURE_BINDING_2D)[0];
    _glBindTexture(GL_TEXTURE_2D, textureID);
    _glUniform1i(mTexUnifLoc, 0);
    if(mGlesVer >= 3)
    {
        _glBindVertexArray(mVAO);
    }
    else
    {
        _glBindVertexArrayOES(mVAO);
    }
    _glFrontFace(GL_CCW);
    _glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

    // 3. Restore render state
    State::setInteger(GL_TEXTURE_BINDING_2D, oTexId);
    if(mGlesVer >= 3)
    {
        State::setInteger(GL_VERTEX_ARRAY_BINDING, oVAO);
    }
    else
    {
        State::setInteger(GL_VERTEX_ARRAY_BINDING_OES, oVAO);
    }
    // The state is restored when 'state' goes out of scope

}

void Quad::CreateProgram()
{
    const char *srcVert =
        "#ifdef GL_ES\n"
        "precision highp float;\n"
        "#endif\n"
        "attribute vec2 myVertex;\n"
        "attribute vec2 myTexCoord;\n"
        "varying  vec2 vTexCoord;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(myVertex.x, myVertex.y, 0.0, 1.0);\n"
        "    vTexCoord = myTexCoord;\n"
        "}\n"
        "";

    const char *srcFrag =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "uniform sampler2D tex;\n"
        "varying vec2 vTexCoord;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = texture2D(tex, vTexCoord);\n"
        "}\n"
        "";

    unsigned int VS = _glCreateShader(GL_VERTEX_SHADER);
    _glShaderSource(VS, 1, (const char**)&srcVert, NULL);
    _glCompileShader(VS);

    unsigned int FS = _glCreateShader(GL_FRAGMENT_SHADER);
    _glShaderSource(FS, 1, (const char**)&srcFrag, NULL);
    _glCompileShader(FS);

    mProg = _glCreateProgram();
    _glAttachShader(mProg, VS);
    _glAttachShader(mProg, FS);
    _glBindAttribLocation(mProg, 0, "myVertex");
    _glBindAttribLocation(mProg, 1, "myTexCoord");

    _glLinkProgram(mProg);
    mTexUnifLoc = _glGetUniformLocation(mProg, "tex");

    _glDeleteShader(VS);
    _glDeleteShader(FS);
}

void Quad::DeleteProgram()
{
    _glDeleteProgram(mProg);
    mProg = 0;
    mTexUnifLoc = 0;
}

void Quad::CreateVboEbo()
{
    // Store state, restore when exiting this function
    State state({}, {GL_ARRAY_BUFFER_BINDING, GL_ELEMENT_ARRAY_BUFFER_BINDING});

    // 0.VAO
    if(mGlesVer >= 3)
    {
        _glGenVertexArrays(1, &mVAO);
        _glBindVertexArray(mVAO);
    }
    else
    {
        _glGenVertexArraysOES(1, &mVAO);
        _glBindVertexArrayOES(mVAO);
    }
    if(mGlesVer == 1)
    {
        _glEnableClientState(GL_VERTEX_ARRAY);
        _glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        _glDisableClientState(GL_COLOR_ARRAY);
        _glDisableClientState(GL_NORMAL_ARRAY);

    }
    // 1. VBO
    float vertices_texcoords[8+8]; // 8 pos, 8 coords
    // vertices positions
    vertices_texcoords[0] = -1.0f; //0
    vertices_texcoords[1] = -1.0f; //0

    vertices_texcoords[2] =  1.0f; //1
    vertices_texcoords[3] = -1.0f; //1

    vertices_texcoords[4] =  1.0f; //2
    vertices_texcoords[5] =  1.0f; //2

    vertices_texcoords[6] = -1.0f; //3
    vertices_texcoords[7] =  1.0f; //3

    // texture coordinates
    vertices_texcoords[8    ] =  0.0f; //0
    vertices_texcoords[8 + 1] =  0.0f; //0

    vertices_texcoords[8 + 2] =  1.0f; //1
    vertices_texcoords[8 + 3] =  0.0f; //1

    vertices_texcoords[8 + 4] =  1.0f; //2
    vertices_texcoords[8 + 5] =  1.0f; //2

    vertices_texcoords[8 + 6] =  0.0f; //3
    vertices_texcoords[8 + 7] =  1.0f; //3

    _glGenBuffers(1, &mVBO);
    _glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    _glBufferData(GL_ARRAY_BUFFER, sizeof(float) * (8+8), (const void*)vertices_texcoords, GL_STATIC_DRAW);
    if(mGlesVer == 1)
    {
        _glVertexPointer(2, GL_FLOAT, 0, 0);
        _glTexCoordPointer(2, GL_FLOAT, 0, (const void*)(sizeof(float)*8));
    }
    else
    {
        _glEnableVertexAttribArray(0);
        _glVertexAttribPointer(0, 2, GL_FLOAT, 0, 0, 0);
        _glEnableVertexAttribArray(1);
        _glVertexAttribPointer(1, 2, GL_FLOAT, 0, 0, (const void*)(sizeof(float)*8));


    }
    // 2. EBO
    unsigned char indicesTmp[] =
    {
        0, 1, 2,
        0, 2, 3
    };
    _glGenBuffers(1, &mEBO);
    _glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
    _glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned char) * 6, (const void*)indicesTmp, GL_STATIC_DRAW);
    if(mGlesVer >= 3)
    {
        _glBindVertexArray(0);
    }
    else
    {
        _glBindVertexArrayOES(0);
    }
}

void Quad::DeleteVboEbo()
{
    _glDeleteBuffers(1, &mVBO);
    _glDeleteBuffers(1, &mEBO);
    if(mGlesVer >= 3)
    {
        _glDeleteVertexArrays(1, &mVAO);
    }
    else
    {
        _glDeleteVertexArraysOES(1, &mVAO);
    }
}
