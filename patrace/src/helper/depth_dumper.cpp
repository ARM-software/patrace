#include "depth_dumper.hpp"
#include "common/image.hpp"
#include "dispatch/eglproc_auto.hpp"
#include "helper/shaderutility.hpp"

const GLchar *DepthDumper::depthCopyVSCode =
"#version 310 es\n\
 in highp vec2 a_Position;\n\
 out highp vec2 v_TexCoordinate;\n\
 void main() {\n\
     v_TexCoordinate = a_Position * 0.5 + vec2(0.5, 0.5);\n\
     gl_Position = vec4(a_Position, 0.0f, 1.0f);\n\
 }";

const GLchar *DepthDumper::depthCopyFSCode =
"#version 310 es\n\
 uniform highp sampler2D u_Texture;\n\
 in highp vec2 v_TexCoordinate;\n\
 out highp float fragColor;\n\
 void main() {\n\
     fragColor = texture(u_Texture, v_TexCoordinate).x;\n\
 }";

const GLchar *DepthDumper::D32FS8_FSCode =
"#version 310 es\n\
 uniform highp sampler2D u_Texture;\n\
 in highp vec2 v_TexCoordinate;\n\
 out highp vec2 fragColor;\n\
 void main() {\n\
     highp float depth = texture(u_Texture, v_TexCoordinate).x;\n\
     highp float stencil = texture(u_Texture, v_TexCoordinate).y/255.0;\n\
     fragColor = vec2(depth, stencil);\n\
 }";

const GLchar *DepthDumper::DS_dFSCode =
"#version 310 es\n\
 uniform highp sampler2D u_Texture;\n\
 in highp vec2 v_TexCoordinate;\n\
 out highp vec4 fragColor;\n\
 void main()\n\
 {\n\
     const highp float max24int = 256.0 * 256.0 * 256.0 - 1.0;\n\
     highp float depth = texture(u_Texture, v_TexCoordinate).r;\n\
     depth = clamp(depth, 0.0, 1.0);\n\
     highp float result = depth * max24int;\n\
     highp int   result_i = int(result);\n\
     highp int r, g, b;\n\
     r = result_i & 0xff;\n\
     g = (result_i >> 8) & 0xff;\n\
     b = (result_i >> 16) & 0xff;\n\
     fragColor = vec4(255.0, float(r), float(g), float(b))/255.0;\n\
 }";

const GLchar *DepthDumper::DS_sFSCode =
"#version 310 es\n\
 uniform highp usampler2D u_Texture;\n\
 in highp vec2 v_TexCoordinate;\n\
 out highp vec4 fragColor;\n\
 void main()\n\
 {\n\
     highp uint stencil = texture(u_Texture, v_TexCoordinate).r;\n\
     fragColor = vec4(float(stencil), 0.0, 0.0, 0.0)/255.0;\n\
}";

const GLchar *DepthDumper::depthCopyCubeVSCode =
"#version 310 es\n\
 in highp vec2 a_Position;\n\
 out highp vec2 v_TexCoordinate;\n\
 void main() {\n\
     v_TexCoordinate = a_Position;\n\
     gl_Position = vec4(a_Position, 0.0f, 1.0f);\n\
 }";

const GLchar *DepthDumper::depthCopyCubeFSCode =
"#version 310 es\n\
 uniform highp samplerCube u_Texture;\n\
 uniform int cubemapId;\n\
 in highp vec2 v_TexCoordinate;\n\
 out highp float fragColor;\n\
 void main() {\n\
     highp vec3 texCoord;\n\
     switch(cubemapId) {\n\
         case 0:\n\
             texCoord = vec3(1.0, -v_TexCoordinate.y, -v_TexCoordinate.x);\n\
             break;\n\
         case 1:\n\
             texCoord = vec3(-1.0, -v_TexCoordinate.y, v_TexCoordinate.x);\n\
             break;\n\
         case 2:\n\
             texCoord = vec3(v_TexCoordinate.x, 1.0, v_TexCoordinate.y);\n\
             break;\n\
         case 3:\n\
             texCoord = vec3(v_TexCoordinate.x, -1.0, -v_TexCoordinate.y);\n\
             break;\n\
         case 4:\n\
             texCoord = vec3(v_TexCoordinate.x, -v_TexCoordinate.y, 1.0);\n\
             break;\n\
         case 5:\n\
             texCoord = vec3(-v_TexCoordinate.x, -v_TexCoordinate.y, -1.0);\n\
             break;\n\
     }\n\
     fragColor = texture(u_Texture, texCoord).x;\n\
 }";

const GLchar *DepthDumper::depthArrayCopyFSCode =
"#version 310 es\n\
 uniform highp sampler2DArray u_Texture;\n\
 uniform int layerIdx;\n\
 in highp vec2 v_TexCoordinate;\n\
 out highp float fragColor;\n\
 void main() {\n\
     highp vec3 texCoord;\n\
     texCoord = vec3(v_TexCoordinate.x, v_TexCoordinate.y, layerIdx);\n\
     fragColor = texture(u_Texture, texCoord).x;\n\
 }";

DepthDumper::~DepthDumper()
{
    _glDeleteShader(depthCopyVS);
    _glDeleteShader(depthCopyCubeVS);
    _glDeleteShader(depthCopyFS);
    _glDeleteShader(depthCopyCubeFS);
    _glDeleteShader(DSCopy_dFS);
    _glDeleteShader(DSCopy_sFS);
    _glDeleteShader(D32FS8Copy_FS);
    _glDeleteProgram(depthCopyProgram);
    _glDeleteProgram(depthCopyCubeProgram);
    _glDeleteProgram(depthDSCopyProgram);
    _glDeleteProgram(stencilDSCopyProgram);
    _glDeleteProgram(D32FS8CopyProgram);
    _glDeleteFramebuffers(1, &depthFBO);
}

void DepthDumper::initializeDepthCopyer()
{
    GLint prev_fbo = 0;
    _glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    _glGenFramebuffers(1, &depthFBO);
    _glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);

    GLint prev_texture_2d;
    _glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_texture_2d);
    _glGenTextures(1, &depthTexture);
    _glBindTexture(GL_TEXTURE_2D, depthTexture);
    _glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    _glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    _glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    _glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    _glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, depthTexture, 0);

    // vertex shader
    depthCopyVS = _glCreateShader(GL_VERTEX_SHADER);
    _glShaderSource(depthCopyVS, 1, &depthCopyVSCode, 0);
    _glCompileShader(depthCopyVS);

    GLint vert_status;
    _glGetShaderiv(depthCopyVS, GL_COMPILE_STATUS, &vert_status);
    if (vert_status == GL_FALSE)
        printShaderInfoLog(depthCopyVS, "depthCopy vertex shader");

    // vertex shader for cubemap
    depthCopyCubeVS = _glCreateShader(GL_VERTEX_SHADER);
    _glShaderSource(depthCopyCubeVS, 1, &depthCopyCubeVSCode, 0);
    _glCompileShader(depthCopyCubeVS);

    _glGetShaderiv(depthCopyCubeVS, GL_COMPILE_STATUS, &vert_status);
    if (vert_status == GL_FALSE)
        printShaderInfoLog(depthCopyCubeVS, "depthCopyCube vertex shader");

    //////////////////////////////////////////////////////////////////////////////////////

    // fragment shader
    depthCopyFS = _glCreateShader(GL_FRAGMENT_SHADER);
    _glShaderSource(depthCopyFS, 1, &depthCopyFSCode, 0);
    _glCompileShader(depthCopyFS);

    GLint frag_status;
    _glGetShaderiv(depthCopyFS, GL_COMPILE_STATUS, &frag_status);
    if (frag_status == GL_FALSE)
        printShaderInfoLog(depthCopyFS, "depthCopy fragment shader");

    // fragment shader for cubemap
    depthCopyCubeFS = _glCreateShader(GL_FRAGMENT_SHADER);
    _glShaderSource(depthCopyCubeFS, 1, &depthCopyCubeFSCode, 0);
    _glCompileShader(depthCopyCubeFS);

    _glGetShaderiv(depthCopyCubeFS, GL_COMPILE_STATUS, &frag_status);
    if (frag_status == GL_FALSE)
        printShaderInfoLog(depthCopyCubeFS, "depthCopyCube fragment shader");

    // fragment shader for D24S8 depth
    DSCopy_dFS = _glCreateShader(GL_FRAGMENT_SHADER);
    _glShaderSource(DSCopy_dFS, 1, &DS_dFSCode, 0);
    _glCompileShader(DSCopy_dFS);

    _glGetShaderiv(DSCopy_dFS, GL_COMPILE_STATUS, &frag_status);
    if (frag_status == GL_FALSE)
        printShaderInfoLog(DSCopy_dFS, "D24S8 depthCopy fragment shader");

    // fragment shader for D24S8 stencil
    DSCopy_sFS = _glCreateShader(GL_FRAGMENT_SHADER);
    _glShaderSource(DSCopy_sFS, 1, &DS_sFSCode, 0);
    _glCompileShader(DSCopy_sFS);

    _glGetShaderiv(DSCopy_sFS, GL_COMPILE_STATUS, &frag_status);
    if (frag_status == GL_FALSE)
        printShaderInfoLog(DSCopy_sFS, "D24S8 stencilCopy fragment shader");

    // fragment shader for D32FS8
    D32FS8Copy_FS = _glCreateShader(GL_FRAGMENT_SHADER);
    _glShaderSource(D32FS8Copy_FS, 1, &D32FS8_FSCode, 0);
    _glCompileShader(D32FS8Copy_FS);

    _glGetShaderiv(D32FS8Copy_FS, GL_COMPILE_STATUS, &frag_status);
    if (frag_status == GL_FALSE)
        printShaderInfoLog(D32FS8Copy_FS, "D32FS8 Copy fragment shader");

    // fragment shader for 2DArray depth
    depthArrayCopyFS = _glCreateShader(GL_FRAGMENT_SHADER);
    _glShaderSource(depthArrayCopyFS, 1, &depthArrayCopyFSCode, 0);
    _glCompileShader(depthArrayCopyFS);

    _glGetShaderiv(depthArrayCopyFS, GL_COMPILE_STATUS, &frag_status);
    if (frag_status == GL_FALSE)
        printShaderInfoLog(depthCopyCubeFS, "depthArrayCopyFS fragment shader");

    //////////////////////////////////////////////////////////////////////////////////////

    // program
    GLint prev_program_id;
    _glGetIntegerv(GL_CURRENT_PROGRAM, &prev_program_id);
    depthCopyProgram = _glCreateProgram();
    _glAttachShader(depthCopyProgram, depthCopyVS);
    _glAttachShader(depthCopyProgram, depthCopyFS);
    _glBindAttribLocation(depthCopyProgram, 0, "a_Position");
    _glLinkProgram(depthCopyProgram);
    GLint link_status;
    _glGetProgramiv(depthCopyProgram, GL_LINK_STATUS, &link_status);
    if (link_status == GL_FALSE)
        printProgramInfoLog(depthCopyProgram, "depthCopy program");
    _glUseProgram(depthCopyProgram);

    GLint u_Texture_location = getUniLoc(depthCopyProgram, "u_Texture");    // get uniform location
    _glUniform1i(u_Texture_location, 0);             // set uniform variables

    // program for DS depth
    depthDSCopyProgram = _glCreateProgram();
    _glAttachShader(depthDSCopyProgram, depthCopyVS);
    _glAttachShader(depthDSCopyProgram, DSCopy_dFS);
    _glBindAttribLocation(depthDSCopyProgram, 0, "a_Position");
    _glLinkProgram(depthDSCopyProgram);
    _glGetProgramiv(depthDSCopyProgram, GL_LINK_STATUS, &link_status);
    if (link_status == GL_FALSE)
        printProgramInfoLog(depthDSCopyProgram, "depthDSCopy program");
    _glUseProgram(depthDSCopyProgram);

    u_Texture_location = getUniLoc(depthDSCopyProgram, "u_Texture");    // get uniform location
    _glUniform1i(u_Texture_location, 0);             // set uniform variables

    // program for DS stencil
    stencilDSCopyProgram = _glCreateProgram();
    _glAttachShader(stencilDSCopyProgram, depthCopyVS);
    _glAttachShader(stencilDSCopyProgram, DSCopy_sFS);
    _glBindAttribLocation(stencilDSCopyProgram, 0, "a_Position");
    _glLinkProgram(stencilDSCopyProgram);
    _glGetProgramiv(stencilDSCopyProgram, GL_LINK_STATUS, &link_status);
    if (link_status == GL_FALSE)
        printProgramInfoLog(stencilDSCopyProgram, "stencilDSCopy program");
    _glUseProgram(stencilDSCopyProgram);

    u_Texture_location = getUniLoc(stencilDSCopyProgram, "u_Texture");    // get uniform location
    _glUniform1i(u_Texture_location, 0);             // set uniform variables

    // program for D32FS8
    D32FS8CopyProgram = _glCreateProgram();
    _glAttachShader(D32FS8CopyProgram, depthCopyVS);
    _glAttachShader(D32FS8CopyProgram, D32FS8Copy_FS);
    _glBindAttribLocation(D32FS8CopyProgram, 0, "a_Position");
    _glLinkProgram(D32FS8CopyProgram);
    _glGetProgramiv(D32FS8CopyProgram, GL_LINK_STATUS, &link_status);
    if (link_status == GL_FALSE)
        printProgramInfoLog(D32FS8CopyProgram, "depthD32FS8Copy program");
    _glUseProgram(D32FS8CopyProgram);

    u_Texture_location = getUniLoc(D32FS8CopyProgram, "u_Texture");    // get uniform location
    _glUniform1i(u_Texture_location, 0);             // set uniform variables

    // program for cubemap
    depthCopyCubeProgram = _glCreateProgram();
    _glAttachShader(depthCopyCubeProgram, depthCopyCubeVS);
    _glAttachShader(depthCopyCubeProgram, depthCopyCubeFS);
    _glBindAttribLocation(depthCopyCubeProgram, 0, "a_Position");
    _glLinkProgram(depthCopyCubeProgram);
    _glGetProgramiv(depthCopyCubeProgram, GL_LINK_STATUS, &link_status);
    if (link_status == GL_FALSE)
        printProgramInfoLog(depthCopyCubeProgram, "depthCopyCube program");
    _glUseProgram(depthCopyCubeProgram);

    u_Texture_location = getUniLoc(depthCopyCubeProgram, "u_Texture");    // get uniform location
    _glUniform1i(u_Texture_location, 0);             // set uniform variables
    cubemapIdLocation = getUniLoc(depthCopyCubeProgram, "cubemapId");    // get uniform location

    // program for 2dArray
    depthArrayCopyProgram = _glCreateProgram();
    _glAttachShader(depthArrayCopyProgram, depthCopyVS);
    _glAttachShader(depthArrayCopyProgram, depthArrayCopyFS);
    _glBindAttribLocation(depthArrayCopyProgram, 0, "a_Position");
    _glLinkProgram(depthArrayCopyProgram);
    _glGetProgramiv(depthArrayCopyProgram, GL_LINK_STATUS, &link_status);
    if (link_status == GL_FALSE)
        printProgramInfoLog(depthArrayCopyProgram, "depthArrayCopy program");
    _glUseProgram(depthArrayCopyProgram);

    u_Texture_location = getUniLoc(depthArrayCopyProgram, "u_Texture");    // get uniform location
    _glUniform1i(u_Texture_location, 0);             // set uniform variables
    layerIdxLocation = getUniLoc(depthArrayCopyProgram, "layerIdx");    // get uniform location

    //////////////////////////////////////////////////////////////////////////////////////

    // vertex buffer
    GLint prev_vertex_buffer_id;
    _glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prev_vertex_buffer_id);
    _glGenBuffers(1, &depthVertexBuf);
    _glBindBuffer(GL_ARRAY_BUFFER, depthVertexBuf);
    GLfloat v[] = { 1.0, 1.0,           -1.0, 1.0,          -1.0, -1.0,         1.0, -1.0 };
    _glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);

    //////////////////////////////////////////////////////////////////////////////////////

    // index buffer
    GLint prev_index_buffer_id;
    _glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &prev_index_buffer_id);
    _glGenBuffers(1, &depthIndexBuf);
    _glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, depthIndexBuf);
    GLuint index[] = { 0, 1, 2,             0, 2, 3 };
    _glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index), index, GL_STATIC_DRAW);

    //////////////////////////////////////////////////////////////////////////////////////

    _glUseProgram(prev_program_id);
    _glBindBuffer(GL_ARRAY_BUFFER, prev_vertex_buffer_id);
    _glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prev_index_buffer_id);
    _glBindTexture(GL_TEXTURE_2D, prev_texture_2d);
    _glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);
}

// Draw the GL_DEPTH_COMPONENT of a texture to the GL_COLOR_ATTACHMENT0 of another texture
void DepthDumper::get_depth_texture_image(GLuint sourceTexture, int width, int height, GLvoid *pixels, GLint internalFormat, TexType texType, int id)
{
    GLenum target = GL_TEXTURE_2D;
    if (texType == TexCubemap) {
        target = GL_TEXTURE_CUBE_MAP;
    }

    GLint prev_read_fbo = 0, prev_draw_fbo = 0;
    _glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prev_read_fbo);
    _glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prev_draw_fbo);
    _glBindFramebuffer(GL_READ_FRAMEBUFFER, depthFBO);
    _glBindFramebuffer(GL_DRAW_FRAMEBUFFER, depthFBO);

    GLint act;
    _glGetIntegerv(GL_ACTIVE_TEXTURE, &act);
    GLint prev_texture_2d;
    _glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_texture_2d);

    GLint texFormat = GL_R32F;
    GLint format = GL_RED;
    GLint type = GL_FLOAT;
    if (internalFormat == GL_DEPTH24_STENCIL8)
    {
        texFormat = GL_RGBA8;
        format = GL_RGBA;
        type = GL_UNSIGNED_BYTE;
    }
    else if (internalFormat == GL_DEPTH32F_STENCIL8)
    {
        texFormat = GL_RG32F;
        format = GL_RG;
        type = GL_FLOAT;
    }
    _glBindTexture(GL_TEXTURE_2D, depthTexture);
    _glTexImage2D(GL_TEXTURE_2D, 0, texFormat, width, height, 0, format, type, 0);

    GLint pre_sampler;
    _glGetIntegerv(GL_SAMPLER_BINDING, &pre_sampler);
    _glBindSampler(0, 0);

    GLint prev_viewport[4];
    _glGetIntegerv(GL_VIEWPORT, prev_viewport);
    _glViewport(0, 0, width, height);

    GLboolean pre_color_mask[4] = {GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE};
    _glGetBooleanv(GL_COLOR_WRITEMASK, pre_color_mask);
    _glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    GLboolean pre_depth_mask = GL_FALSE;
    _glGetBooleanv(GL_DEPTH_WRITEMASK, &pre_depth_mask);
    _glDepthMask(GL_TRUE);

    GLboolean pre_blend = GL_FALSE;
    _glGetBooleanv(GL_BLEND, &pre_blend);
    _glDisable(GL_BLEND);

    GLboolean pre_use_scissor_test = GL_FALSE;
    _glGetBooleanv(GL_SCISSOR_TEST, &pre_use_scissor_test);
    _glDisable(GL_SCISSOR_TEST);

    GLboolean pre_use_depth_test = GL_FALSE;
    _glGetBooleanv(GL_DEPTH_TEST, &pre_use_depth_test);
    _glDisable(GL_DEPTH_TEST);

    GLboolean pre_cull_face = GL_FALSE;
    _glGetBooleanv(GL_CULL_FACE, &pre_cull_face);
    _glDisable(GL_CULL_FACE);

    GLboolean pre_use_stencil_test = GL_FALSE;
    _glGetBooleanv(GL_STENCIL_TEST, &pre_use_stencil_test);
    _glDisable(GL_STENCIL_TEST);

    GLboolean pre_use_polygon_offset = GL_FALSE;
    _glGetBooleanv(GL_POLYGON_OFFSET_FILL, &pre_use_polygon_offset);
    _glDisable(GL_POLYGON_OFFSET_FILL);

    GLint pre_va = 0;
    _glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &pre_va);
    _glBindVertexArray(0);

    GLint prev_program_id, prev_vertex_buffer_id, prev_index_buffer_id;
    _glGetIntegerv(GL_CURRENT_PROGRAM, &prev_program_id);
    _glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prev_vertex_buffer_id);
    _glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &prev_index_buffer_id);

    if (internalFormat == GL_DEPTH24_STENCIL8)
    {
        _glUseProgram(depthDSCopyProgram);
    }
    else if (internalFormat == GL_DEPTH32F_STENCIL8)
    {
        _glUseProgram(D32FS8CopyProgram);
    }
    else {
        _glUseProgram(depthCopyProgram);
        if (texType == TexCubemap) {
            _glUseProgram(depthCopyCubeProgram);
            _glUniform1i(cubemapIdLocation, id);
        } else if (texType == Tex2DArray) {
            _glUseProgram(depthArrayCopyProgram);
            _glUniform1i(layerIdxLocation, id);
        }
    }
    _glBindBuffer(GL_ARRAY_BUFFER, depthVertexBuf);
    _glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, depthIndexBuf);

    //////////////////////////////////////////////////////////////////////////////////////

    _glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    _glEnableVertexAttribArray(0);
    _glActiveTexture(GL_TEXTURE0);
    _glBindTexture(target, sourceTexture);

    GLint prev_compare_mode = 0, prev_min_filter = 0, prev_mag_filter = 0;
    GLint prev_depth_stencil_mode;
    _glGetTexParameteriv(target, GL_TEXTURE_COMPARE_MODE, &prev_compare_mode);
    _glGetTexParameteriv(target, GL_TEXTURE_MIN_FILTER, &prev_min_filter);
    _glGetTexParameteriv(target, GL_TEXTURE_MAG_FILTER, &prev_mag_filter);
    _glGetTexParameteriv(target, GL_DEPTH_STENCIL_TEXTURE_MODE, &prev_depth_stencil_mode);
    _glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    _glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    _glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    _glTexParameteri(target, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);

    GLint compare_now = 0;
    _glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, &compare_now);

    GLfloat prev_clear_color[4], prev_clear_depth;
    _glGetFloatv(GL_COLOR_CLEAR_VALUE, prev_clear_color);
    _glGetFloatv(GL_DEPTH_CLEAR_VALUE, &prev_clear_depth);
    GLint prev_clear_stencil;
    _glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &prev_clear_stencil);

    _glClearColor(0.0, 0.0, 0.0, 0.0);
    _glClearDepthf(1.0);
    _glClearStencil(0);
    _glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _glDrawElements(GL_TRIANGLES, 2 * 3, GL_UNSIGNED_INT, 0);   // draw 2 triangles to sample depth

    // sample stencil
    if (internalFormat == GL_DEPTH24_STENCIL8)
    {
        _glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);
        _glUseProgram(stencilDSCopyProgram);
        _glTexParameteri(target, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);
        _glDrawElements(GL_TRIANGLES, 2 * 3, GL_UNSIGNED_INT, 0);
        _glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

    _glReadPixels(0, 0, width, height, format, type, pixels);

    // restore state
    _glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, prev_compare_mode);
    _glTexParameteri(target, GL_TEXTURE_MIN_FILTER, prev_min_filter);
    _glTexParameteri(target, GL_TEXTURE_MAG_FILTER, prev_mag_filter);
    _glTexParameteri(target, GL_DEPTH_STENCIL_TEXTURE_MODE, prev_depth_stencil_mode);
    _glBindFramebuffer(GL_READ_FRAMEBUFFER, prev_read_fbo);
    _glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prev_draw_fbo);
    _glViewport(prev_viewport[0], prev_viewport[1], prev_viewport[2], prev_viewport[3]);
    _glActiveTexture(act);
    _glBindTexture(GL_TEXTURE_2D, prev_texture_2d);
    _glBindSampler(0, pre_sampler);
    _glUseProgram(prev_program_id);
    _glBindBuffer(GL_ARRAY_BUFFER, prev_vertex_buffer_id);
    _glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prev_index_buffer_id);
    _glClearColor(prev_clear_color[0], prev_clear_color[1], prev_clear_color[2], prev_clear_color[3]);
    _glClearDepthf(prev_clear_depth);
    _glClearStencil(prev_clear_stencil);

    pre_use_stencil_test ? _glEnable(GL_STENCIL_TEST) : _glDisable(GL_STENCIL_TEST);
    pre_use_scissor_test ? _glEnable(GL_SCISSOR_TEST) : _glDisable(GL_SCISSOR_TEST);
    pre_use_depth_test ? _glEnable(GL_DEPTH_TEST) : _glDisable(GL_DEPTH_TEST);
    pre_blend ? _glEnable(GL_BLEND) : _glDisable(GL_BLEND);
    pre_cull_face ? _glEnable(GL_CULL_FACE) : _glDisable(GL_CULL_FACE);
    pre_use_polygon_offset ? _glEnable(GL_POLYGON_OFFSET_FILL) : _glDisable(GL_POLYGON_OFFSET_FILL);
    _glColorMask(pre_color_mask[0], pre_color_mask[1], pre_color_mask[2], pre_color_mask[3]);
    _glDepthMask(pre_depth_mask);

    _glBindVertexArray(pre_va);
}
