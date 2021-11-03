#include "eglsize.hpp"
#include "shaderutility.hpp"
#include <string>

static int bisect_val(int min, int max, bool is_valid_val(int val))
{
    bool valid;

    while (1) {
        int try_val = min + (max - min + 1) / 2;

        valid = is_valid_val(try_val);
        if (min == max)
            break;

        if (valid)
            min = try_val;
        else
            max = try_val - 1;
    }

    return valid ? min : -1;
}

static bool is_valid_width(int val)
{
    _glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, val, 1);
    return _glGetError() == GL_NO_ERROR;
}

static bool is_valid_height(int val)
{
    _glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 1, val);
    return _glGetError() == GL_NO_ERROR;
}

static int detect_size(int *width_ret, int *height_ret)
{
    GLint max_tex_size;
    int width;
    int height;

    max_tex_size = 0;
    _glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tex_size);

    width = bisect_val(1, max_tex_size, is_valid_width);
    if (width < 0)
        return -1;

    height = bisect_val(1, max_tex_size, is_valid_height);
    if (height < 0)
        return -1;

    *width_ret = width;
    *height_ret = height;

    return 0;
}

static void _eglCreateImageKHR_get_image_size(EGLImageKHR image, image_info *info)
{
    GLuint fbo = 0;
    GLuint orig_fbo = 0;
    GLuint texture = 0;
    GLuint orig_texture;
    GLenum status;

    _glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint *)&orig_fbo);
    _glGenFramebuffers(1, &fbo);
    _glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    _glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint *)&orig_texture);
    _glGenTextures(1, &texture);
    _glBindTexture(GL_TEXTURE_2D, texture);

    _glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);

    info->width = 0;
    info->height = 0;

    _glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_TEXTURE_2D, texture, 0);
    status = _glCheckFramebufferStatus(GL_FRAMEBUFFER);
    _glDisable(GL_DEBUG_OUTPUT_KHR);
    if (status == GL_FRAMEBUFFER_COMPLETE) {
        if (detect_size(&info->width, &info->height) != 0)
            DBG_LOG("%s: can't detect image size\n", __func__);
    } else {
        DBG_LOG("%s: error: %x\n", __func__, status);
    }
    _glEnable(GL_DEBUG_OUTPUT_KHR);

    /* Don't leak errors to the traced application. */
    (void)_glGetError();

    _glBindTexture(GL_TEXTURE_2D, orig_texture);
    _glDeleteTextures(1, &texture);

    _glBindFramebuffer(GL_FRAMEBUFFER, orig_fbo);
    _glDeleteFramebuffers(1, &fbo);
}

static void get_texture_external_image(image_info *info)
{
    GLuint fbo = 0;
    GLint texture;

    _glGetIntegerv(GL_TEXTURE_BINDING_EXTERNAL_OES, &texture);
    if (!texture)
        return;

    GLint prev_read_fbo = 0, prev_draw_fbo = 0;
    _glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prev_read_fbo);
    _glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prev_draw_fbo);
    _glGenFramebuffers(1, &fbo);
    _glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // pixel
    GLint pre_pixel_pack = 0;
    GLint pre_pixel_unpack = 0;
    _glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &pre_pixel_pack);
    _glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &pre_pixel_unpack);

    GLint pack_row_len = 0;
    GLint pack_skip_pixels = 0;
    GLint pack_skip_rows = 0;
    GLint pack_align = 0;

    GLint unpack_row_len = 0;
    GLint unpack_image_height = 0;
    GLint unpack_skip_pixels = 0;
    GLint unpack_skip_rows = 0;
    GLint unpack_skip_images = 0;
    GLint unpack_align = 0;
    _glGetIntegerv(GL_PACK_ROW_LENGTH, &pack_row_len);
    _glGetIntegerv(GL_PACK_SKIP_PIXELS,&pack_skip_pixels);
    _glGetIntegerv(GL_PACK_SKIP_ROWS, &pack_skip_rows);
    _glGetIntegerv(GL_PACK_ALIGNMENT, &pack_align);
    _glGetIntegerv(GL_UNPACK_ROW_LENGTH, &unpack_row_len);
    _glGetIntegerv(GL_UNPACK_IMAGE_HEIGHT, &unpack_image_height);
    _glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &unpack_skip_pixels);
    _glGetIntegerv(GL_UNPACK_SKIP_ROWS, &unpack_skip_rows);
    _glGetIntegerv(GL_UNPACK_SKIP_IMAGES, &unpack_skip_images);
    _glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpack_align);
    _glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    _glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    _glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    _glPixelStorei(GL_PACK_SKIP_PIXELS,0);
    _glPixelStorei(GL_PACK_SKIP_ROWS, 0);
    _glPixelStorei(GL_PACK_ALIGNMENT, 1);
    _glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    _glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
    _glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    _glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    _glPixelStorei(GL_UNPACK_SKIP_IMAGES, 0);
    _glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // tex 2d
    GLint pre_active_tex;
    _glGetIntegerv(GL_ACTIVE_TEXTURE, &pre_active_tex);
    GLint prev_texture_2d;
    _glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_texture_2d);
    GLuint texture_2d;
    _glGenTextures(1, &texture_2d);
    _glBindTexture(GL_TEXTURE_2D, texture_2d);
    _glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    _glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    _glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    _glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    _glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, info->width, info->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    _glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_2d, 0);

    GLint prev_viewport[4];
    _glGetIntegerv(GL_VIEWPORT, prev_viewport);
    _glViewport(0, 0, info->width, info->height);
    GLboolean pre_color_mask[4] = {false, false, false, false};
    GLboolean pre_depth_mask = false;
    GLfloat pre_color[4];
    GLfloat pre_depth;
    GLint pre_stencil;
    GLint pre_sampler;
    _glGetBooleanv(GL_COLOR_WRITEMASK, pre_color_mask);
    _glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    _glGetBooleanv(GL_DEPTH_WRITEMASK, &pre_depth_mask);
    _glDepthMask(GL_TRUE);
    _glGetFloatv(GL_COLOR_CLEAR_VALUE, pre_color);
    _glGetFloatv(GL_DEPTH_CLEAR_VALUE, &pre_depth);
    _glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &pre_stencil);
    _glGetIntegerv(GL_SAMPLER_BINDING, &pre_sampler);

    GLboolean pre_use_scissor_test = false;
    _glGetBooleanv(GL_SCISSOR_TEST, &pre_use_scissor_test);
    _glDisable(GL_SCISSOR_TEST);
    GLboolean pre_blend = false;
    _glGetBooleanv(GL_BLEND, &pre_blend);
    _glDisable(GL_BLEND);
    GLboolean pre_depth_test = false;
    _glGetBooleanv(GL_DEPTH_TEST, &pre_depth_test);
    _glDisable(GL_DEPTH_TEST);
    GLint pre_frontface;
    _glGetIntegerv(GL_FRONT_FACE, &pre_frontface);
    _glFrontFace(GL_CCW);
    GLboolean pre_cull_test = false;
    _glGetBooleanv(GL_CULL_FACE, &pre_cull_test);
    _glDisable(GL_CULL_FACE);
    GLboolean pre_stencil_test = false;
    _glGetBooleanv(GL_STENCIL_TEST, &pre_stencil_test);
    _glDisable(GL_STENCIL_TEST);

    // vao
    GLint pre_va = 0;
    _glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &pre_va);
    _glBindVertexArray(0);

    // vertex shader
    GLuint vertex_shader_id = _glCreateShader(GL_VERTEX_SHADER);
    const GLchar *vertex_shader =
   "#version 300 es\n\
    in vec2 a_Position;\n\
    out highp vec2 v_TexCoordinate;\n\
    void main() {\n\
        v_TexCoordinate = a_Position * 0.5 + vec2(0.5, 0.5);\n\
        gl_Position = vec4(a_Position, 0.0f, 1.0f);\n\
    }";
    _glShaderSource(vertex_shader_id, 1, &vertex_shader, 0);
    _glCompileShader(vertex_shader_id);

    GLint vert_status, frag_status;
    _glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &vert_status);
    if (vert_status == GL_FALSE)
        printShaderInfoLog(vertex_shader_id, "texture_external_image vertex shader");

    //////////////////////////////////////////////////////////////////////////////////////

    // fragment shader
    GLuint fragment_shader_id = _glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar *fragment_shader =
   "#version 300 es\n\
    #extension GL_OES_EGL_image_external : require\n\
    uniform samplerExternalOES u_Texture;\n\
    in highp vec2 v_TexCoordinate;\n\
    layout(location = 0) out highp vec4 fragColor;\n\
    void main() {\n\
        fragColor = texture(u_Texture, v_TexCoordinate);\n\
    }";
    _glShaderSource(fragment_shader_id, 1, &fragment_shader, 0);
    _glCompileShader(fragment_shader_id);

    _glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &frag_status);
    if (frag_status == GL_FALSE)
        printShaderInfoLog(fragment_shader_id, "texture_external_image fragment shader");

    //////////////////////////////////////////////////////////////////////////////////////

    // program
    GLint prev_program_id;
    _glGetIntegerv(GL_CURRENT_PROGRAM, &prev_program_id);
    GLuint program_id = _glCreateProgram();
    _glAttachShader(program_id, vertex_shader_id);
    _glAttachShader(program_id, fragment_shader_id);
    _glBindAttribLocation(program_id, 0, "a_Position");
    _glLinkProgram(program_id);
    GLint link_status;
    _glGetProgramiv(program_id, GL_LINK_STATUS, &link_status);
    if (link_status == GL_FALSE)
        printProgramInfoLog(program_id, "texture_external_image shader");
    _glUseProgram(program_id);

    //////////////////////////////////////////////////////////////////////////////////////

    GLint u_Texture_location = getUniLoc(program_id, "u_Texture");    // get uniform location
    _glUniform1i(u_Texture_location, 0);             // set uniform variables

    //////////////////////////////////////////////////////////////////////////////////////

    // vertex buffer
    GLint prev_vertex_buffer_id;
    _glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prev_vertex_buffer_id);
    GLuint vertex_buffer_id;
    _glGenBuffers(1, &vertex_buffer_id);
    _glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_id);
    GLfloat v[] = { 1.0, 1.0,           -1.0, 1.0,          -1.0, -1.0,         1.0, -1.0 };
    _glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);

    //////////////////////////////////////////////////////////////////////////////////////

    // index buffer
    GLint prev_index_buffer_id;
    _glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &prev_index_buffer_id);
    GLuint index_buffer_id;
    _glGenBuffers(1, &index_buffer_id);
    _glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_id);
    GLuint index[] = { 0, 1, 2,             0, 2, 3 };
    _glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index), index, GL_STATIC_DRAW);

    //////////////////////////////////////////////////////////////////////////////////////

    // draw
    _glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    _glEnableVertexAttribArray(0);
    _glActiveTexture(GL_TEXTURE0);
    _glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
    _glBindSampler(0, 0);
    _glClearColor(0.0, 0.0, 0.0, 0.0);
    _glClearDepthf(1.0f);
    _glClearStencil(0);
    _glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    _glDrawElements(GL_TRIANGLES, 2 * 3, GL_UNSIGNED_INT, 0);   // draw 2 triangles
    _glReadPixels(0, 0, info->width, info->height, info->format, info->type, info->pixels);

    /* Don't leak errors to the traced application. */
    (void)_glGetError();

    // restore
    _glBindFramebuffer(GL_READ_FRAMEBUFFER, prev_read_fbo);
    _glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prev_draw_fbo);
    _glDeleteFramebuffers(1, &fbo);

    _glViewport(prev_viewport[0], prev_viewport[1], prev_viewport[2], prev_viewport[3]);

    _glActiveTexture(pre_active_tex);
    _glBindTexture(GL_TEXTURE_2D, prev_texture_2d);
    _glDeleteTextures(1, &texture_2d);
    _glBindSampler(0, pre_sampler);

    _glUseProgram(prev_program_id);
    _glDeleteShader(vertex_shader_id);
    _glDeleteShader(fragment_shader_id);
    _glDeleteProgram(program_id);

    _glBindBuffer(GL_ARRAY_BUFFER, prev_vertex_buffer_id);
    _glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prev_index_buffer_id);
    _glDeleteBuffers(1, &vertex_buffer_id);
    _glDeleteBuffers(1, &index_buffer_id);

    _glBindBuffer(GL_PIXEL_PACK_BUFFER, pre_pixel_pack);
    _glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pre_pixel_unpack);
    _glPixelStorei(GL_PACK_ROW_LENGTH, pack_row_len);
    _glPixelStorei(GL_PACK_SKIP_PIXELS,pack_skip_pixels);
    _glPixelStorei(GL_PACK_SKIP_ROWS, pack_skip_rows);
    _glPixelStorei(GL_PACK_ALIGNMENT, pack_align);
    _glPixelStorei(GL_UNPACK_ROW_LENGTH, unpack_row_len);
    _glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, unpack_image_height);
    _glPixelStorei(GL_UNPACK_SKIP_PIXELS, unpack_skip_pixels);
    _glPixelStorei(GL_UNPACK_SKIP_ROWS, unpack_skip_rows);
    _glPixelStorei(GL_UNPACK_SKIP_IMAGES, unpack_skip_images);
    _glPixelStorei(GL_UNPACK_ALIGNMENT, unpack_align);

    pre_use_scissor_test ? _glEnable(GL_SCISSOR_TEST) : _glDisable(GL_SCISSOR_TEST);
    pre_blend ? _glEnable(GL_BLEND) : _glDisable(GL_BLEND);
    pre_depth_test ? _glEnable(GL_DEPTH_TEST) : _glDisable(GL_DEPTH_TEST);
    pre_cull_test ? _glEnable(GL_CULL_FACE) : _glDisable(GL_CULL_FACE);
    pre_stencil_test ? _glEnable(GL_STENCIL_TEST) : _glDisable(GL_STENCIL_TEST);
    _glFrontFace(pre_frontface);
    _glColorMask(pre_color_mask[0], pre_color_mask[1], pre_color_mask[2], pre_color_mask[3]);
    _glDepthMask(pre_depth_mask);
    _glClearColor(pre_color[0], pre_color[1], pre_color[2], pre_color[3]);
    _glClearDepthf(pre_depth);
    _glClearStencil(pre_stencil);

    _glBindVertexArray(pre_va);
}

static void get_texture_2d_image(image_info *info)
{
    GLuint fbo = 0;
    GLint prev_fbo = 0;
    GLint texture;
    GLenum status;

    _glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
    if (!texture)
        return;

    _glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    _glGenFramebuffers(1, &fbo);
    _glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    _glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                            texture, 0);
    status = _glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        DBG_LOG("error: %d\n", status);
    _glReadPixels(0, 0, info->width, info->height, info->format, info->type, info->pixels);

    /* Don't leak errors to the traced application. */
    (void)_glGetError();

    _glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);
    _glDeleteFramebuffers(1, &fbo);
}

struct image_info* _EGLImageKHR_get_image_info(EGLImageKHR image, GLenum target, int width, int height)
{
    GLuint tex;
    GLint bound_tex = 0;
    struct image_info *info;

    info = new image_info;
    info->internalformat = GL_RGBA;
    info->format = GL_RGBA;
    info->type = GL_UNSIGNED_BYTE;

    _glGenTextures(1, &tex);
    bool is_external_tex = false;
    if (target == EGL_NATIVE_BUFFER_ANDROID || target == EGL_LINUX_DMA_BUF_EXT)
        is_external_tex = true;
    else
        is_external_tex = false;

    if (is_external_tex) {
        info->width = width;
        info->height = height;
        _glGetIntegerv(GL_TEXTURE_BINDING_EXTERNAL_OES, &bound_tex);
        _glBindTexture(GL_TEXTURE_EXTERNAL_OES, tex);
        _glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, image);
    }
    else {
        _glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound_tex);
        _glBindTexture(GL_TEXTURE_2D, tex);
        _glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);
        _eglCreateImageKHR_get_image_size(image, info);
    }

    info->size = _glTexImage2D_size(info->format, info->type, info->width, info->height);
    info->pixels = new char[info->size];

    if (is_external_tex) {
        get_texture_external_image(info);
        _glBindTexture(GL_TEXTURE_EXTERNAL_OES, bound_tex);
    }
    else {
        get_texture_2d_image(info);
        _glBindTexture(GL_TEXTURE_2D, bound_tex);
    }
    _glDeleteTextures(1, &tex);

    return info;
}

void _EGLImageKHR_free_image_info(struct image_info *info)
{
    delete []static_cast<char *>(info->pixels);
    delete info;
}

