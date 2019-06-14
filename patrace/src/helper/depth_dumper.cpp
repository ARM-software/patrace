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

DepthDumper::~DepthDumper()
{
    _glDeleteShader(depthCopyVS);
    _glDeleteShader(depthCopyCubeVS);
    _glDeleteShader(depthCopyFS);
    _glDeleteShader(depthCopyCubeFS);
    _glDeleteProgram(depthCopyProgram);
    _glDeleteProgram(depthCopyCubeProgram);
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

    //////////////////////////////////////////////////////////////////////////////////////

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
void DepthDumper::get_depth_texture_image(GLuint sourceTexture, int width, int height, GLvoid *pixels, TexType texType, int id)
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
    _glBindTexture(GL_TEXTURE_2D, depthTexture);

    _glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, 0);

    GLint prev_viewport[4];
    _glGetIntegerv(GL_VIEWPORT, prev_viewport);
    _glViewport(0, 0, width, height);

    GLboolean pre_use_scissor_test = GL_FALSE;
    GLboolean pre_color_mask[4] = {GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE};
    _glGetBooleanv(GL_SCISSOR_TEST, &pre_use_scissor_test);
    _glDisable(GL_SCISSOR_TEST);
    _glGetBooleanv(GL_COLOR_WRITEMASK, pre_color_mask);
    _glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    GLboolean pre_blend = GL_FALSE;
    _glGetBooleanv(GL_BLEND, &pre_blend);
    _glDisable(GL_BLEND);

    GLint pre_va = 0;
    _glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &pre_va);
    _glBindVertexArray(0);

    GLint prev_program_id, prev_vertex_buffer_id, prev_index_buffer_id;
    _glGetIntegerv(GL_CURRENT_PROGRAM, &prev_program_id);
    _glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prev_vertex_buffer_id);
    _glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &prev_index_buffer_id);
    _glUseProgram(depthCopyProgram);
    if (texType == TexCubemap) {
        _glUseProgram(depthCopyCubeProgram);
        _glUniform1i(cubemapIdLocation, id);
    }
    _glBindBuffer(GL_ARRAY_BUFFER, depthVertexBuf);
    _glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, depthIndexBuf);

    //////////////////////////////////////////////////////////////////////////////////////

    _glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    _glEnableVertexAttribArray(0);
    _glActiveTexture(GL_TEXTURE0);
    _glBindTexture(target, sourceTexture);

    GLint prev_compare_mode = 0, prev_min_filter = 0, prev_mag_filter = 0;
    GLfloat prev_clear_color[4], prev_clear_depth;
    _glGetTexParameteriv(target, GL_TEXTURE_COMPARE_MODE, &prev_compare_mode);
    _glGetTexParameteriv(target, GL_TEXTURE_MIN_FILTER, &prev_min_filter);
    _glGetTexParameteriv(target, GL_TEXTURE_MAG_FILTER, &prev_mag_filter);
    _glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    _glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    _glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    _glGetFloatv(GL_COLOR_CLEAR_VALUE, prev_clear_color);
    _glGetFloatv(GL_DEPTH_CLEAR_VALUE, &prev_clear_depth);

    GLint compare_now = 0;
    _glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, &compare_now);

    _glClearColor(0.0, 0.0, 0.0, 0.0);
    _glClearDepthf(1.0);
    _glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    _glDrawElements(GL_TRIANGLES, 2 * 3, GL_UNSIGNED_INT, 0);   // draw 2 triangles
    _glReadPixels(0, 0, width, height, GL_RED, GL_FLOAT, pixels);

    _glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, prev_compare_mode);
    _glTexParameteri(target, GL_TEXTURE_MIN_FILTER, prev_min_filter);
    _glTexParameteri(target, GL_TEXTURE_MAG_FILTER, prev_mag_filter);
    _glBindFramebuffer(GL_READ_FRAMEBUFFER, prev_read_fbo);
    _glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prev_draw_fbo);
    _glViewport(prev_viewport[0], prev_viewport[1], prev_viewport[2], prev_viewport[3]);
    _glBindTexture(GL_TEXTURE_2D, prev_texture_2d);
    _glUseProgram(prev_program_id);
    _glBindBuffer(GL_ARRAY_BUFFER, prev_vertex_buffer_id);
    _glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prev_index_buffer_id);
    _glClearColor(prev_clear_color[0], prev_clear_color[1], prev_clear_color[2], prev_clear_color[3]);
    _glClearDepthf(prev_clear_depth);

    pre_use_scissor_test ? _glEnable(GL_SCISSOR_TEST) : _glDisable(GL_SCISSOR_TEST);
    pre_blend ? _glEnable(GL_BLEND) : _glDisable(GL_BLEND);
    _glColorMask(pre_color_mask[0], pre_color_mask[1], pre_color_mask[2], pre_color_mask[3]);

    _glBindVertexArray(pre_va);
}
