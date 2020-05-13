#include "../../thirdparty/opengl-registry/api/GLES3/gl31.h"

namespace image {
    class Image;
}

class DepthDumper
{
public:
    enum TexType
    {
        Tex2D = 0,
        Tex2DArray,
        TexCubemap,
        TexCubemapArray,
        TexEnd
    };
    ~DepthDumper();
    static const GLchar *depthCopyVSCode;
    static const GLchar *depthCopyFSCode;
    static const GLchar *depthCopyCubeVSCode;
    static const GLchar *depthCopyCubeFSCode;
    static const GLchar *DS_dFSCode;
    static const GLchar *DS_sFSCode;
    GLint depthCopyVS, depthCopyCubeVS, depthCopyFS, depthCopyCubeFS, depthCopyCubeProgram, depthCopyProgram;
    GLint DSCopy_dFS, DSCopy_sFS, depthDSCopyProgram, stencilDSCopyProgram;
    GLuint depthFBO, depthTexture;
    GLuint depthVertexBuf, depthIndexBuf;
    GLint cubemapIdLocation;

    void initializeDepthCopyer();
    void get_depth_texture_image(GLuint sourceTexture, int width, int height, GLvoid *pixels, GLint internalFormat, TexType texType, int id);
};
