/**************************************************************************
 *
 * Copyright 2011 Jose Fonseca
 * Copyright 2008-2010 VMware, Inc.
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


#include <zlib.h>
#include <png.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fstream>

#include "image.hpp"
#include "os.hpp"

namespace image {


static const int png_compression_level = Z_BEST_SPEED;


bool Image::writePNG(const char *filename) const
{
    FILE *fp;
    png_structp png_ptr;
    png_infop info_ptr;

    fp = fopen(filename, "wb");
    if (!fp)
    {
        DBG_LOG("Failed to open %s: %s\n", filename, strerror(errno));
        goto no_fp;
    }

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
        goto no_png;

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr,  NULL);
        goto no_png;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        goto no_png;
    }

    png_init_io(png_ptr, fp);

    int color_type;
    switch (channels) {
    case 4:
        color_type = PNG_COLOR_TYPE_RGB_ALPHA;
        break;
    case 3:
        color_type = PNG_COLOR_TYPE_RGB;
        break;
    case 2:
        color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
        break;
    case 1:
        color_type = PNG_COLOR_TYPE_GRAY;
        break;
    default:
        assert(0);
        return false;
    }

    png_set_IHDR(png_ptr, info_ptr, width, height, 8, color_type,
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_set_compression_level(png_ptr, png_compression_level);

    png_write_info(png_ptr, info_ptr);

    if (!flipped) {
        for (unsigned y = 0; y < height; ++y) {
            png_bytep row = (png_bytep)(pixels + y*width*channels);
            png_write_rows(png_ptr, &row, 1);
        }
    } else {
        unsigned y = height;
        while (y--) {
            png_bytep row = (png_bytep)(pixels + y*width*channels);
            png_write_rows(png_ptr, &row, 1);
        }
    }

    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    fclose(fp);
    return true;

no_png:
    fclose(fp);
    unlink(filename);

no_fp:
    return false;
}


Image *
readPNG(const char *filename)
{
    FILE *fp;
    png_structp png_ptr;
    png_infop info_ptr;
    png_infop end_info;
    Image *image;

    fp = fopen(filename, "rb");
    if (!fp)
        goto no_fp;

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
        goto no_png;

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        goto no_png;
    }

    end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        goto no_png;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        goto no_png;
    }

    png_init_io(png_ptr, fp);

    png_read_info(png_ptr, info_ptr);

    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type, compression_type, filter_method;

    png_get_IHDR(png_ptr, info_ptr,
                 &width, &height,
                 &bit_depth, &color_type, &interlace_type,
                 &compression_type, &filter_method);

    image = new Image(width, height);
    if (!image)
        goto no_image;

    /* Convert to RGBA8 */
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png_ptr);
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);
    if (bit_depth == 16)
        png_set_strip_16(png_ptr);

    for (unsigned y = 0; y < height; ++y) {
        png_bytep row = (png_bytep)(image->pixels + y*width*4);
        png_read_row(png_ptr, row, NULL);
    }

    png_read_end(png_ptr, info_ptr);
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    fclose(fp);
    return image;

no_image:
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
no_png:
    fclose(fp);
no_fp:
    return NULL;
}


struct png_tmp_buffer
{
    char *buffer;
    size_t size;
};

static void
pngWriteCallback(png_structp png_ptr, png_bytep data, png_size_t length)
{
    struct png_tmp_buffer *buf = (struct png_tmp_buffer*) png_get_io_ptr(png_ptr);
    size_t nsize = buf->size + length;

    /* allocate or grow buffer */
    if (buf->buffer)
        buf->buffer = (char*)realloc(buf->buffer, nsize);
    else
        buf->buffer = (char*)malloc(nsize);

    if (!buf->buffer)
        png_error(png_ptr, "Buffer allocation error");

    memcpy(buf->buffer + buf->size, data, length);
    buf->size += length;
}

bool writePixelsToBuffer(unsigned char *pixels,
                         unsigned width, unsigned height, unsigned numChannels,
                         bool flipped,
                         char **buffer,
                         int *size)
{
    struct png_tmp_buffer png_mem;
    png_structp png_ptr;
    png_infop info_ptr;
    int type;

    png_mem.buffer = NULL;
    png_mem.size = 0;

    switch (numChannels) {
    case 4:
        type = PNG_COLOR_TYPE_RGB_ALPHA;
        break;
    case 3:
        type = PNG_COLOR_TYPE_RGB;
        break;
    case 2:
        type = PNG_COLOR_TYPE_GRAY_ALPHA;
        break;
    case 1:
        type = PNG_COLOR_TYPE_GRAY;
        break;
    default:
        goto no_png;
    }

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
        goto no_png;

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr,  NULL);
        goto no_png;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        goto no_png;
    }

    png_set_write_fn(png_ptr, &png_mem, pngWriteCallback, NULL);

    png_set_IHDR(png_ptr, info_ptr, width, height, 8,
                 type, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_set_compression_level(png_ptr, png_compression_level);

    png_write_info(png_ptr, info_ptr);

    if (!flipped) {
        for (unsigned y = 0; y < height; ++y) {
            png_bytep row = (png_bytep)(pixels + y*width*numChannels);
            png_write_rows(png_ptr, &row, 1);
        }
    } else {
        unsigned y = height;
        while (y--) {
            png_bytep row = (png_bytep)(pixels + y*width*numChannels);
            png_write_rows(png_ptr, &row, 1);
        }
    }

    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    *buffer = png_mem.buffer;
    *size = png_mem.size;

    return true;

no_png:
    *buffer = NULL;
    *size = 0;

    if (png_mem.buffer)
        free(png_mem.buffer);
    return false;
}

} /* namespace image */
