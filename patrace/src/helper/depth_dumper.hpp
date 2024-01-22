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
        Tex3D,
        TexEnd
    };
    ~DepthDumper();
    static const GLchar *depthCopyVSCode;
    static const GLchar *depthCopyFSCode;
    static const GLchar *depthArrayCopyFSCode;
    static const GLchar *depthCopyCubeVSCode;
    static const GLchar *depthCopyCubeFSCode;
    static const GLchar *D32FS8_FSCode;
    static const GLchar *DS_dFSCode;
    static const GLchar *DS_sFSCode;
    GLint depthCopyVS, depthCopyCubeVS, depthCopyFS, depthCopyCubeFS, depthCopyCubeProgram, depthCopyProgram;
    GLint DSCopy_dFS, DSCopy_sFS, depthDSCopyProgram, stencilDSCopyProgram;
    GLint D32FS8Copy_FS, D32FS8CopyProgram;
    GLint depthArrayCopyFS, depthArrayCopyProgram;
    GLuint depthFBO, depthTexture;
    GLuint depthVertexBuf, depthIndexBuf;
    GLint cubemapIdLocation;
    GLint layerIdxLocation;

    void initializeDepthCopyer();
    void get_depth_texture_image(GLuint sourceTexture, int width, int height, GLvoid *pixels, GLint internalFormat, TexType texType, int id);
};
