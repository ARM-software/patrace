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
    GLint depthCopyVS, depthCopyCubeVS, depthCopyFS, depthCopyCubeFS, depthCopyCubeProgram, depthCopyProgram;
    GLuint depthFBO, depthTexture;
    GLuint depthVertexBuf, depthIndexBuf;
    GLint cubemapIdLocation;

    void initializeDepthCopyer();
    void get_depth_texture_image(GLuint sourceTexture, int width, int height, GLvoid *pixels, TexType texType, int id);
};
