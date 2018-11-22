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

#include <tracer/glstate.hpp>
#include <tracer/egltrace.hpp>

#include <dispatch/eglproc_auto.hpp>
#include <helper/eglsize.hpp>
#include <common/image.hpp>

#include <string.h>
#include <algorithm>
#include <iostream>


namespace glstate {

image::Image* getDrawBufferImage()
{
    GLint draw_framebuffer = 0;
    if (GetCurTraceContext(GetThreadId())->profile >= 2)
    {
        _glGetIntegerv(GL_FRAMEBUFFER_BINDING, &draw_framebuffer);
    }

    GLint width = 0;
    GLint height = 0;

    if (draw_framebuffer == 0)
    {
        width = gTraceOut->mpBinAndMeta->winSurWidth;
        height = gTraceOut->mpBinAndMeta->winSurHeight;
    }
    else
    {
        GLint type, name;
        _glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
        _glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &name);

        if (type == GL_RENDERBUFFER)
        {
            GLint oRenderBuffer;
            _glGetIntegerv(GL_RENDERBUFFER_BINDING, &oRenderBuffer);
            _glBindRenderbuffer(GL_RENDERBUFFER, name);
            _glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);
            _glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);
            _glBindRenderbuffer(GL_RENDERBUFFER, oRenderBuffer);
        }
        else if (type == GL_TEXTURE)
        {
            int xywh[4];
            _glGetIntegerv(GL_VIEWPORT, xywh);
            width = xywh[2];
            height = xywh[3];
        }
        else
        {
            width = 800;
            height = 480;
            DBG_LOG("Hard-code to snapshot osr...\n");
            // return NULL;
        }
    }

    const GLenum format = GL_RGBA;
    const GLenum type = GL_UNSIGNED_BYTE;
    image::Image *image = new image::Image(width, height, 4, true);
    if (!image) {
        return NULL;
    }

    while (_glGetError() != GL_NO_ERROR) {}

    int oldAlignment = 4;
    _glGetIntegerv(GL_PACK_ALIGNMENT, &oldAlignment);
    _glPixelStorei(GL_PACK_ALIGNMENT, 1);

    _glReadPixels(0, 0, width, height, format, type, image->pixels);
    _glPixelStorei(GL_PACK_ALIGNMENT, oldAlignment);

    GLenum error = _glGetError();
    if (error != GL_NO_ERROR) {
        do {
            DBG_LOG("warning: %x while getting snapshot\n", error);
            error = _glGetError();
        } while(error != GL_NO_ERROR);
        delete image;
        return NULL;
    }

    return image;
}


} /* namespace glstate */
