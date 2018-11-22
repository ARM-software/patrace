#ifndef _DRAW_QUAD_H_
#define _DRAW_QUAD_H_

class Quad
{
public:
    Quad(int glesVer);
    ~Quad();

    inline void DrawTex(unsigned int textureID)
    {
        mGlesVer == 1 ? DrawTex1(textureID) : DrawTex2(textureID);
    }

    void ReleaseOwnershipOfGLObjects();

private:
    void DrawTex1(unsigned int textureID);
    void DrawTex2(unsigned int textureID);

    void CreateProgram();
    void DeleteProgram();

    void CreateVboEbo();
    void DeleteVboEbo();

    bool         mOwnsGLObjects;
    int          mGlesVer;
    unsigned int mProg;
    unsigned int mTexUnifLoc;
    unsigned int mVBO;
    unsigned int mEBO;
    unsigned int mVAO;
};

#endif
