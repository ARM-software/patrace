/**************************************************************************
 *
 * Copyright 2011 Jose Fonseca
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/


#include "glstate.hpp"

#include <string.h>

#include <algorithm>
#include <iostream>

#include "dispatch/eglproc_auto.hpp"

#include "retracer/retracer.hpp"

#include "common/image.hpp"
#include "helper/shaderutility.hpp"
#include "helper/depth_dumper.hpp"

namespace glstate {

using namespace retracer;

static std::string cube_faces[6] = { "up", "down", "right", "left", "forward", "back" };

GLint getMaxColorAttachments()
{
    GLint maxAttachments = 1;
    const Context *context = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext();
    if(context && context->_profile >= PROFILE_ES3)
        _glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxAttachments);
    return maxAttachments;
}

GLint getMaxDrawBuffers()
{
    GLint maxDrawBuffers = 1;
    const Context *context = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext();
    if(context && context->_profile >= PROFILE_ES3)
        _glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
    return maxDrawBuffers;
}

GLint getColorAttachment(GLint drawBuffer)
{
    GLint attachment = GL_COLOR_ATTACHMENT0;
    const Context *context = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext();
    if(context && context->_profile >= PROFILE_ES3)
        _glGetIntegerv(GL_DRAW_BUFFER0+drawBuffer, &attachment);
    return attachment;
}

GLint getDepthAttachment()
{
    GLint attachment = GL_FALSE;
    const Context *context = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext();
    if(context && context->_profile >= PROFILE_ES3)
        _glGetIntegerv(GL_DEPTH_TEST, &attachment);
    return attachment;
}

static GLuint createAndCompileShader(const std::string& shaderSrc, GLuint type)
{
    // Create the shader object
    GLuint shader = _glCreateShader(type);
    if(shader == 0)
    {
        gRetracer.reportAndAbort("Error creating shader with type 0x%x.\n", type);
    }

    ShaderInfo si(shader);
    si.setSource(shaderSrc);
    si.compile();

    if(si.compileStatus == GL_FALSE)
    {
        DBG_LOG("Error compiling shader:\n%s\n", si.getInfoLog().c_str());
        _glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static bool isDepth = false;

void getDepth(int width, int height, GLvoid *pixels, DepthDumper &depthDumper, GLenum internalFormat)
{
    // malloc tex
    GLint prev_texture_2d;
    _glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_texture_2d);
    GLuint tex;
    _glGenTextures(1, &tex);
    _glBindTexture(GL_TEXTURE_2D, tex);
    _glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    _glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    _glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    _glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    _glTexStorage2D(GL_TEXTURE_2D, 1, internalFormat, width, height);

    // set draw and read frame buffer
    GLint prev_read_fbo, prev_draw_fbo;
    _glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prev_read_fbo);
    _glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prev_draw_fbo);
    GLuint fb;
    _glGenFramebuffers(1, &fb);
    _glBindFramebuffer(GL_READ_FRAMEBUFFER, prev_draw_fbo);
    _glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb);
    _glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex, 0);

    // blitFramebuffer
    _glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,
                       GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    depthDumper.get_depth_texture_image(tex, width, height, pixels, internalFormat, DepthDumper::Tex2D, 0);

    // recover state machine
    _glBindFramebuffer(GL_READ_FRAMEBUFFER, prev_read_fbo);
    _glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prev_draw_fbo);
    _glBindTexture(GL_TEXTURE_2D, prev_texture_2d);

    // release all resources
    _glDeleteFramebuffers(1, &fb);
    _glDeleteTextures(1, &tex);
}

void getTexRenderBufInfo(GLenum attachment, GLenum &readTexFormat, GLenum &readTexType, int &bytesPerPixel, int &channel, GLint &internalFormat,  GLint attachmentType)
{
    GLint redSize, greenSize, blueSize, alphaSize, depthSize, stencilSize;
    _glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE, &redSize);
    _glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE, &greenSize);
    _glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE, &blueSize);
    _glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE, &alphaSize);
    _glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &depthSize);
    _glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, &stencilSize);

    if (attachmentType == GL_RENDERBUFFER)
    {
        _glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &internalFormat);
    }
    else if (attachmentType == GL_TEXTURE)
    {
        GLint textureName;
        _glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &textureName);
        Context& context = gRetracer.getCurrentContext();
        Texture t;
        t.handle = context.getTextureMap().RValue(textureName);
        t.update(0, 0);
        internalFormat = t.internalFormat;
    }

    if (redSize && greenSize && blueSize)
    {
        isDepth = false;
        if (alphaSize)
            if (internalFormat == GL_RGBA16UI)
                readTexFormat = GL_RGBA_INTEGER;
            else
                readTexFormat = GL_RGBA;
        else
            if (internalFormat == GL_RGB16UI)
                readTexFormat = GL_RGB_INTEGER;
            else
                readTexFormat = GL_RGB;
        if (redSize == 32 && greenSize == 32 && blueSize == 32 && alphaSize == 32) // GL_RGBA32F
        {
            readTexType = GL_FLOAT;
            channel = 4;
        }
        else if (redSize == 16 && greenSize == 16 && blueSize == 16 && alphaSize == 16) // GL_RGBA16F
        {
            if (internalFormat == GL_RGBA16UI)
                readTexType = GL_UNSIGNED_INT;
            else
                readTexType = GL_HALF_FLOAT;
            channel = 4;
        }
        else if (redSize == 11 && greenSize == 11 && blueSize == 10) // GL_R11G11B10
        {
            readTexType = GL_UNSIGNED_INT_10F_11F_11F_REV;
            channel = 4;        // 32bits, need a 4-channel png file to store it
        }
        else if (redSize == 10 && greenSize == 10 && blueSize == 10 && alphaSize == 2) // GL_RGB10_A2
        {
            readTexType = GL_UNSIGNED_INT_2_10_10_10_REV;
            channel = 4;        // 32bits, need a 4-channel png file to store it
        }
        else if (redSize == 8 && greenSize == 8 && blueSize == 8 && alphaSize == 0) // GL_RGB8
        {
            readTexFormat = GL_RGB;
            channel = 3;
        }
        else if (redSize == 4 && greenSize == 4 && blueSize == 4 && alphaSize == 4) // GL_RGBA4
        {
            readTexType = GL_UNSIGNED_SHORT_4_4_4_4;
            channel = 2;        // 16bits, need a 2-channel png file to store it
        }
        else if (redSize == 5 && greenSize == 5 && blueSize == 5 && alphaSize == 1) // GL_RGB5A_1
        {
            readTexType = GL_UNSIGNED_SHORT_5_5_5_1;
            channel = 2;        // 16bits, need a 2-channel png file to store it
        }
        else if (redSize == 5 && greenSize == 6 && blueSize == 5) // GL_RGB565
        {
            readTexType = GL_UNSIGNED_SHORT_5_6_5;
            channel = 2;        // 16bits, need a 2-channel png file to store it
        }
    }
    else
    {
        if (internalFormat == GL_R16F)
        {
            readTexFormat = GL_RED;
            readTexType = GL_HALF_FLOAT;
            isDepth = false;
            channel = 2;        // 16bits, need a 2-channel png file to store it
        }
        else if (internalFormat == GL_R32F)
        {
            readTexFormat = GL_RED;
            readTexType = GL_FLOAT;
            isDepth = false;
            channel = 4;        // 32bits, need a 4-channel png file to store it
        }
        else if (internalFormat == GL_R8)
        {
            readTexFormat = GL_RED;
            readTexType = GL_UNSIGNED_BYTE;
            isDepth = false;
            channel = 1;        // 8bits, need a 1-channel png file to store it
        }
        else if (internalFormat == GL_RG16F)
        {
            readTexFormat = GL_RG_EXT;
            readTexType = GL_HALF_FLOAT;
            isDepth = false;
            channel = 4;        // 32bits, need a 4-channel png file to store it
        }
        else if (internalFormat == GL_DEPTH_COMPONENT24)
        {
            readTexFormat = GL_DEPTH_COMPONENT;
            readTexType = GL_UNSIGNED_INT;
            isDepth = true;
            bytesPerPixel = 4;
            channel = 4;        // 32bits, need a 4-channel png file to store it
        }
        else if (internalFormat == GL_DEPTH_COMPONENT32F)
        {
            readTexFormat = GL_DEPTH_COMPONENT;
            readTexType = GL_FLOAT;
            isDepth = true;
            bytesPerPixel = 4;
            channel = 4;        // 32bits, need a 4-channel png file to store it
        }
        else if (internalFormat == GL_DEPTH24_STENCIL8)
        {
            readTexFormat = GL_DEPTH_COMPONENT;
            readTexType = GL_UNSIGNED_INT_24_8;
            isDepth = true;
            bytesPerPixel = 4;
            channel = 4;        // 32bits, need a 4-channel png file to store it
        }
        else if (internalFormat == GL_DEPTH_COMPONENT16)
        {
            readTexFormat = GL_DEPTH_COMPONENT;
            readTexType = GL_UNSIGNED_INT;
            isDepth = true;
            bytesPerPixel = 4;
            channel = 4;
        }
        else
        {
            isDepth = true;
            bytesPerPixel = 4;
            DBG_LOG("Couldn't figure out format to use for internalformat 0x%x\n", internalFormat);
        }
    }
    if (!isDepth)
        bytesPerPixel = (redSize + greenSize + blueSize + alphaSize) / 8;
    GLint implReadFormat, implReadType;
    _glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &implReadFormat);
    _glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE,   &implReadType);
    DBG_LOG("readTexFormat = 0x%x, readTexType = 0x%x\n", readTexFormat, readTexType);
    DBG_LOG("implTexFormat = 0x%x, implTexType = 0x%x\n", implReadFormat, implReadType);
    DBG_LOG("isDepth = %d, bytesPerPixel = %d\n", isDepth, bytesPerPixel);
}

void getDimensions(int drawFramebuffer, GLenum attachment, int& width, int& height, GLenum &format, GLenum &type, int &bytesPerPixel, int &channel, int &internalFormat)
{
    if (drawFramebuffer == 0)
    {
        if (gRetracer.mOptions.mDoOverrideResolution)
        {
            width = gRetracer.mOptions.mOverrideResW;
            height = gRetracer.mOptions.mOverrideResH;
        }
        else
        {
            retracer::Drawable* curDrawable = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getDrawable();
            width = curDrawable->winWidth;
            height = curDrawable->winHeight;
        }
    }
    else
    {
        GLint attachmentType = 0;
        _glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &attachmentType);
        if (attachmentType == GL_RENDERBUFFER)
        {
            GLint attachmentName = 0;
            _glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &attachmentName);

            GLint oRenderBuffer;
            _glGetIntegerv(GL_RENDERBUFFER_BINDING, &oRenderBuffer);
            _glBindRenderbuffer(GL_RENDERBUFFER, attachmentName);
            _glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);
            _glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);
            getTexRenderBufInfo(attachment, format, type, bytesPerPixel, channel, internalFormat, attachmentType);
            _glBindRenderbuffer(GL_RENDERBUFFER, oRenderBuffer);
        }
        else if (attachmentType == GL_TEXTURE)
        {
            width = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurAppVP.w;
            height = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurAppVP.h;
            getTexRenderBufInfo(attachment, format, type, bytesPerPixel, channel, internalFormat, attachmentType);
        }
        else
        {
            _glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &attachmentType);
            width = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurAppVP.w;
            height = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurAppVP.h;
            getTexRenderBufInfo(attachment, format, type, bytesPerPixel, channel, internalFormat, attachmentType);
            isDepth = true;
            DBG_LOG("Hard-code to snapshot osr...\n");
        }
    }
}

image::Image* getDrawBufferImage(int attachment, int _width, int _height, GLenum format, GLenum type, int bytes_per_pixel, int channel)
{
    GLint draw_framebuffer = 0;
    const Context* context = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext();
    if (context && context->_profile >= PROFILE_ES3 && draw_framebuffer != -1)
    {
        _glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_framebuffer);
    }

    int width = _width;
    int height = _height;
    GLint internalFormat;
    if (!width || !height)
    {
        getDimensions(draw_framebuffer, attachment, width, height, format, type, bytes_per_pixel, channel, internalFormat);
    }

    int width_multiplier = bytes_per_pixel / channel;   // if bytes_per_pixel > 4, we need more than one 4-channel pixels to store it.
    image::Image *image = new image::Image(width * width_multiplier, height, channel, true);
    if (!image)
    {
        DBG_LOG("Warning: image cannot be created!\n");
        return NULL;
    }

    while (glGetError() != GL_NO_ERROR) {}

    GLint oldReadBuffer;
    if (context && context->_profile >= PROFILE_ES3)
    {
        _glGetIntegerv(GL_READ_BUFFER, &oldReadBuffer);
        if (draw_framebuffer != 0 && attachment != GL_DEPTH_ATTACHMENT)
        {
            _glReadBuffer(attachment);
        }
    }

    int oldAlignment = 4;
    _glGetIntegerv(GL_PACK_ALIGNMENT, &oldAlignment);
    _glPixelStorei(GL_PACK_ALIGNMENT, 1);

    if (!isDepth) {
        _glReadPixels(0, 0, width, height, format, type, image->pixels);
    }
    else {      // depth attachment, can't use glReadPixels on arm GPUs
#ifdef ENABLE_X11
        _glReadPixels(0, 0, width, height, format, type, image->pixels);
#else   // ENABLE_X11 not being defined
        DepthDumper depthDumper;
        depthDumper.initializeDepthCopyer();
        getDepth(width, height, image->pixels, depthDumper, internalFormat);
#endif  // ENABLE_X11 end
        isDepth = false;
    }
    _glPixelStorei(GL_PACK_ALIGNMENT, oldAlignment);

    if(context && context->_profile >= PROFILE_ES3)
    {
        _glReadBuffer(oldReadBuffer);
    }

    GLenum error = _glGetError();
    if (error != GL_NO_ERROR) {
        do {
            DBG_LOG("warning: 0x%x while getting snapshot\n", error);
            error = _glGetError();
        } while(error != GL_NO_ERROR);
        delete image;
        return NULL;
    }

    return image;
}

std::vector<std::string> dumpTexture(Texture& texture, unsigned int callNo, GLfloat* vertices, int face, GLuint* cm_indices)
{
    // Using a simple frag shader, dump the attached texture
#define STRINGIZE(x) #x

    static std::string vshader_basic = std::string("#version 300 es\n") + STRINGIZE(
        in vec2 v_position;
        out vec2 t_pos;
        void main()
        {
            vec2 pos = (v_position*2.0)-vec2(1.0);
            gl_Position = vec4(pos, 0.0, 1.0);
            t_pos = v_position;
        });

    static std::string vshader_cubemap = std::string("#version 300 es\n") + STRINGIZE(
        in highp vec2 v_position;
        in highp vec3 v_tex_coord;
        out highp vec3 f_tex_coord;
        void main()
        {
            f_tex_coord = v_tex_coord;
            gl_Position = vec4(v_position, 0.0f, 1.0f);
        });

    static std::string fshader_depth = std::string("#version 300 es\n") + STRINGIZE(
        precision highp float;
        uniform highp sampler2D u_tex;
        in vec2 t_pos;
        out highp uint fragColor;
        void main()
        {
            vec2 coord = t_pos;
            fragColor = floatBitsToUint(texture(u_tex, coord).x);
        });

    static std::string fshader_depth_cubemap = std::string("#version 300 es\n") + STRINGIZE(
        precision highp float;
        uniform highp samplerCube u_tex;
        in highp vec3 f_tex_coord;
        out highp uint fragColor;
        void main()
        {
            fragColor = floatBitsToUint(texture(u_tex, f_tex_coord).x);

        });

    static std::string fshader_depth_array = std::string("#version 300 es\n") + STRINGIZE(
        precision highp float;
        uniform highp sampler2DArray u_tex;
        uniform float u_depth;
        in vec2 t_pos;
        out highp uint fragColor;
        void main()
        {
            vec3 coord = vec3(t_pos, u_depth);
            fragColor = floatBitsToUint(texture(u_tex, coord).x);
        });

    static std::string fshader_rgba = std::string("#version 300 es\n") + STRINGIZE(
        precision highp float;
        uniform highp sampler2D u_tex;
        in vec2 t_pos;
        out vec4 oColor;
        void main()
        {
            vec2 coord = t_pos;
            oColor = texture(u_tex, coord);
        });

    static std::string fshader_rgba_cubemap = std::string("#version 300 es\n") + STRINGIZE(
        precision highp float;
        uniform highp samplerCube u_tex;
        in highp vec3 f_tex_coord;
        out vec4 oColor;
        void main()
        {
            oColor = texture(u_tex, f_tex_coord);
        });

    static std::string fshader_rgb = std::string("#version 300 es\n") + STRINGIZE(
        precision highp float;
        uniform highp sampler2D u_tex;
        in vec2 t_pos;
        out vec4 oColor;
        void main()
        {
            vec2 coord = t_pos;
            oColor = vec4(texture(u_tex, coord).xyz, 1.0);
        });

    static std::string fshader_rgb_cubemap = std::string("#version 300 es\n") + STRINGIZE(
            precision highp float;
            uniform highp samplerCube u_tex;
            in highp vec3 f_tex_coord;
            out vec4 oColor;
            void main()
            {
                oColor = vec4(texture(u_tex, f_tex_coord).xyz, 1.0);
            });

    static std::string fshader_rgba_array = std::string("#version 300 es\n") + STRINGIZE(

        precision highp float;
        uniform highp sampler2DArray u_tex;
        uniform float u_depth;
        in vec2 t_pos;
        out vec4 fragColor;
        void main()
        {
            vec3 coord = vec3(t_pos, u_depth);
            fragColor = texture(u_tex, coord);
        });

    static std::string fshader_rgb_array = std::string("#version 300 es\n") + STRINGIZE(

        precision highp float;
        uniform highp sampler2DArray u_tex;
        uniform float u_depth;
        in vec2 t_pos;
        out vec4 fragColor;
        void main()
        {
            vec3 coord = vec3(t_pos, u_depth);
            fragColor = vec4(texture(u_tex, coord).xyz, 1.0);
        });

#undef STRINGIZE

    std::vector<std::string> results;
    // The state will restored when this object goes out of scope
    State savedState(
            {GL_CULL_FACE, GL_DEPTH_TEST, GL_BLEND, GL_SCISSOR_TEST},
            {GL_ACTIVE_TEXTURE, GL_DRAW_FRAMEBUFFER_BINDING,
             GL_READ_FRAMEBUFFER_BINDING, GL_CURRENT_PROGRAM, GL_VIEWPORT,
             GL_ARRAY_BUFFER_BINDING, GL_VERTEX_ARRAY_BINDING},
            {GL_COLOR_CLEAR_VALUE, GL_COLOR_CLEAR_VALUE},
            true);


    texture.update(0, face);

    GLint oldTex = 0;
    _glGetIntegerv(texture.binding, &oldTex);

    if (!texture.valid)
    {
        DBG_LOG("Texture with id %d does not exist.\n", texture.handle);
        return results;
    }

    std::stringstream ss;
    ss << texture;
    DBG_LOG("Dumping texture: %s\n", ss.str().c_str());

    std::string* vshader = &vshader_basic;
    std::string* fshader = 0;
    GLint out_type = 0;
    GLenum pixel_format = 0;
    GLint int_format = 0;
    bool useSampler = false;
    bool isCubemap = false;
    if (texture.hasSize(0, 0, 0, 0, 24) ||
        texture.hasSize(0, 0, 0, 0, 16))
    {
        int_format = GL_R32UI;
        pixel_format = GL_RED_INTEGER;
        out_type = GL_UNSIGNED_INT;

        switch (texture.binding)
        {
            case GL_TEXTURE_BINDING_2D:
                fshader = &fshader_depth;
                DBG_LOG("Using fshader_depth\n");
                break;
            case GL_TEXTURE_BINDING_2D_ARRAY:
                fshader = &fshader_depth_array;
                DBG_LOG("Using fshader_depth_array\n");
                useSampler = true;
                break;
            case GL_TEXTURE_BINDING_CUBE_MAP:
                fshader = &fshader_depth_cubemap;
                isCubemap = true;
                DBG_LOG("Using fshader_depth_cubemap\n");
                break;
            default:
                DBG_LOG("Warning unhandled texture binding\n");
                return results;
        }
    }
    else if (texture.hasSize(16, 0, 0, 0, 0) && texture.binding == GL_TEXTURE_BINDING_2D && texture.internalFormat == GL_R16F)
    {
        fshader = &fshader_depth;
        pixel_format = GL_RED_INTEGER;
        out_type = GL_UNSIGNED_INT;
        int_format = GL_R32UI;
        DBG_LOG("Using depth fshader with R16F\n");
    }
    else if(texture.hasSize(11, 11, 10, 0, 0) && texture.binding == GL_TEXTURE_BINDING_2D && texture.internalFormat == GL_R11F_G11F_B10F) 
    {
        fshader = &fshader_rgba;
        pixel_format = GL_RGBA;
        out_type = GL_FLOAT;
        int_format = GL_RGBA32F;
        DBG_LOG("Using rgba fshader with R11F_G11F_B10F\n");
    }
    else if (texture.hasSize(10, 10, 10, 2, 0))
    {
        fshader = &fshader_rgba;
        pixel_format = GL_RGBA;
        out_type = GL_UNSIGNED_INT_2_10_10_10_REV;
        int_format = GL_RGB10_A2;
        DBG_LOG("Using standard fshader with rgba101010_a2\n");
    }
    else if (texture.hasSize(8, 0, 0, 0, 0) ||
             texture.hasSize(8, 8, 0, 0, 0) ||
             texture.hasSize(8, 8, 8, 0, 0))
    {
        switch (texture.binding)
        {
            case GL_TEXTURE_BINDING_2D:
                fshader = &fshader_rgb;
                DBG_LOG("Using fshader_rgb\n");
                break;
            case GL_TEXTURE_BINDING_2D_ARRAY:
                fshader = &fshader_rgb_array;
                DBG_LOG("Using fshader_rgb_array\n");
                break;
            case GL_TEXTURE_BINDING_CUBE_MAP:
                fshader = &fshader_rgb_cubemap;
                isCubemap = true;
                DBG_LOG("Using fshader_rgb_cubemap\n");
                break;
            default:
                DBG_LOG("Warning unhandled texture binding\n");
                return results;
        }
        pixel_format = GL_RGBA;
        out_type = GL_UNSIGNED_BYTE;
        int_format = GL_RGBA;
    }
    else if (texture.hasSize(8, 8, 8, 8, 0))
    {
        switch (texture.binding)
        {
            case GL_TEXTURE_BINDING_2D:
                fshader = &fshader_rgba;
                DBG_LOG("Using fshader_rgba\n");
                break;
            case GL_TEXTURE_BINDING_2D_ARRAY:
                fshader = &fshader_rgba_array;
                DBG_LOG("Using fshader_rgba_array\n");
                break;
            case GL_TEXTURE_BINDING_CUBE_MAP:
                fshader = &fshader_rgba_cubemap;
                isCubemap = true;
                DBG_LOG("Using fshader_rgba_cubemap\n");
                break;
            default:
                DBG_LOG("Warning unhandled texture binding\n");
                return results;
        }
        pixel_format = GL_RGBA;
        out_type = GL_UNSIGNED_BYTE;
        int_format = GL_RGBA;
    }
    else
    {
        DBG_LOG("Warning: Unhandled texture\n");
        return results;
    }

    if (isCubemap)
    {
        vshader = &vshader_cubemap;
    }

    _glDisable(GL_CULL_FACE);
    _glDisable(GL_DEPTH_TEST);
    _glDisable(GL_BLEND);
    _glDisable(GL_SCISSOR_TEST);
    _glActiveTexture(GL_TEXTURE0);
    GLuint fbo = 0;
    _glGenFramebuffers(1, &fbo);
    GLuint fbotex = 0;
    _glGenTextures(1, &fbotex);

    _glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    _glBindTexture(GL_TEXTURE_2D, fbotex);
    _glTexImage2D(GL_TEXTURE_2D,
                 0, int_format, texture.width, texture.height,
                 0, pixel_format, out_type, NULL);
    _glBindTexture(GL_TEXTURE_2D, 0);
    _glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbotex, 0);

    GLenum status;
    status = _glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if(status != GL_FRAMEBUFFER_COMPLETE)
    {
        DBG_LOG("Framebuffer incomplete\n");
        return results;
    }

    // Build shader program
    GLuint vs = createAndCompileShader(*vshader, GL_VERTEX_SHADER);
    GLuint fs = createAndCompileShader(*fshader, GL_FRAGMENT_SHADER);
    GLuint prog = _glCreateProgram();
    _glAttachShader(prog, vs);
    _glAttachShader(prog, fs);

    _glBindAttribLocation(prog, 0, "v_position");
    if (isCubemap)
    {
        _glBindAttribLocation(prog, 1, "v_tex_coord");
    }
    _glLinkProgram(prog);
    _glUseProgram(prog);
    _glUniform1i(_glGetUniformLocation(prog, "u_tex"), 0);

    _glViewport(0, 0, texture.width, texture.height);
    _glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    // Set up state to read out the texture

    _glBindTexture(texture.target, texture.handle);
    GLint compareMode = GL_NONE;
    _glGetTexParameteriv(texture.target, GL_TEXTURE_COMPARE_MODE, &compareMode);
    _glTexParameteri(texture.target, GL_TEXTURE_COMPARE_MODE, GL_NONE);

    GLint oldBoundSampler = State::getIntegerv(GL_SAMPLER_BINDING)[0];

    GLuint sampler = 0;
    if (useSampler)
    {
        _glGenSamplers(1, &sampler);
        _glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        _glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        _glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        _glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        _glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        _glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    }

    _glBindSampler(0, sampler);

    GLint oldMinFilter = State::getTexParameteri(texture.target, GL_TEXTURE_MIN_FILTER)[0];
    GLint oldMagFilter = State::getTexParameteri(texture.target, GL_TEXTURE_MAG_FILTER)[0];

    _glTexParameteri(texture.target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    _glTexParameteri(texture.target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    if (isCubemap)
    {
        GLuint vbo, vao, ibo;
        _glGenBuffers(1, &vbo);
        _glBindBuffer(GL_ARRAY_BUFFER, vbo);
        _glGenVertexArrays(1, &vao);
        _glBufferData(GL_ARRAY_BUFFER, 20 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
        _glBindVertexArray(vao);
        _glEnableVertexAttribArray(0);
        _glEnableVertexAttribArray(1);
        _glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 0);
        _glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
        _glGenBuffers(1, &ibo);
        _glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        _glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), cm_indices, GL_STATIC_DRAW);
        _glClear(GL_COLOR_BUFFER_BIT);
        _glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        _glDeleteBuffers(1, &vbo);
        _glDeleteBuffers(1, &ibo);
        _glDeleteVertexArrays(1, &vao);

        std::stringstream fileName;
        fileName << retracer::gRetracer.mOptions.mSnapshotPrefix << std::setw(10) << std::setfill('0') << callNo << "_tex" << texture.handle;
        int bytes_per_pixel = 0;
        image::Image* image = 0;
        if (texture.internalFormat == GL_R11F_G11F_B10F) //Should handle what happens if texture.internalFormat cannot be written to a .png file -> writes raw data to file
        {
            fileName << "cube_" << face << ".raw";
            bytes_per_pixel = 16;
            image = getDrawBufferImage(GL_COLOR_ATTACHMENT0, texture.width, texture.height, pixel_format, out_type, bytes_per_pixel);
            image->writePixelData(fileName.str().c_str());
        }
        else
        {
            fileName << "cube_" << cube_faces[face] << ".png";
            bytes_per_pixel = 4;
            image = getDrawBufferImage(GL_COLOR_ATTACHMENT0, texture.width, texture.height, pixel_format, out_type, bytes_per_pixel);
            image->writePNG(fileName.str().c_str());
        }

        delete image;

        DBG_LOG("Wrote cubemap %i's %s face to %s\n", texture.handle, cube_faces[face].c_str(), fileName.str().c_str());
        results.push_back(fileName.str());
    }
    else
    {

        _glBindBuffer(GL_ARRAY_BUFFER, 0);
        _glBindVertexArray(0);
        _glEnableVertexAttribArray(0);
        _glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
        for(int i = 1; i < getMaxVertexAttribs(); i++)
        {
            _glDisableVertexAttribArray(i);
        }

        for (int i = 0; i < texture.depth; ++i)
        {
            std::stringstream fileName;
            fileName << retracer::gRetracer.mOptions.mSnapshotPrefix << std::setw(10) << std::setfill('0') << callNo << "_tex" << texture.handle;

            if (texture.binding == GL_TEXTURE_BINDING_2D_ARRAY)
            {
                _glUniform1f(_glGetUniformLocation(prog, "u_depth"), i);
                fileName << "_d" << i;
            }

            _glClear(GL_COLOR_BUFFER_BIT);
            _glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            int bytes_per_pixel = 0;
            image::Image* image = 0;
            if (texture.internalFormat == GL_R11F_G11F_B10F) //Should handle what happens if texture.internalFormat cannot be written to a .png file -> writes raw data to file
            {
                fileName << ".raw";
                bytes_per_pixel = 16;
                image = getDrawBufferImage(GL_COLOR_ATTACHMENT0, texture.width, texture.height, pixel_format, out_type, bytes_per_pixel);
                image->writePixelData(fileName.str().c_str());
            }
            else
            {
                fileName << ".png";
                bytes_per_pixel = 4;
                image = getDrawBufferImage(GL_COLOR_ATTACHMENT0, texture.width, texture.height, pixel_format, out_type, bytes_per_pixel);
                image->writePNG(fileName.str().c_str());
            }

            delete image;

            DBG_LOG("Wrote texture %d to %s\n", texture.handle, fileName.str().c_str());
            results.push_back(fileName.str());
        }
    }

    _glDeleteProgram(prog);
    _glDeleteShader(vs);
    _glDeleteShader(fs);
    _glDeleteSamplers(1, &sampler);
    _glDeleteFramebuffers(1, &fbo);
    _glDeleteTextures(1, &fbotex);

    // Reset some state
    _glTexParameteri(texture.target, GL_TEXTURE_COMPARE_MODE, compareMode);
    _glTexParameteri(texture.target, GL_TEXTURE_MAG_FILTER, oldMagFilter);
    _glTexParameteri(texture.target, GL_TEXTURE_MIN_FILTER, oldMinFilter);
    _glBindSampler(0, oldBoundSampler);
    _glBindTexture(texture.target, oldTex);

    return results;
}


} /* namespace glstate */
