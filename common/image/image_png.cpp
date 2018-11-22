#include <zlib/zlib.h>
#include <cassert>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "libpng/png.h"
#include "image.hpp"
#include "image_io.hpp"
#include "image_compression.hpp"

namespace pat
{

bool CanWriteAsPNG(UInt32 format, UInt32 type)
{
    if ((format == GL_LUMINANCE || format == GL_ALPHA ||
         format == GL_RGB || format == GL_RGBA)
         && type == GL_UNSIGNED_BYTE)
         return true;
    else
        return false;
}

bool WritePNG(const Image &image, const char *filename, bool flip)
{
    const unsigned char *data = image.Data();
    if (!data)
        return false;

    const unsigned int width = image.Width();
    const unsigned int height = image.Height();
    unsigned int format = image.Format();
    unsigned int type = image.Type();

    unsigned char *output_data = NULL;
    if ((format == GL_LUMINANCE || format == GL_ALPHA ||
         format == GL_RGB || format == GL_RGBA)
         && type == GL_UNSIGNED_BYTE)
    {
        // no procession needed
        output_data = const_cast<unsigned char *>(data);
    }
    else if (format == GL_RGB && type == GL_UNSIGNED_SHORT_5_6_5)
    {
        // convert to RGB888
        output_data = new unsigned char[3 * width * height];
        unsigned char *op = output_data;
        const unsigned char *input_data = static_cast<const unsigned char *>(data);
        for (unsigned int j = 0; j < height; j++)
        {
            for (unsigned int i = 0; i < width; i++)
            {
                unsigned short buffer = ((*input_data) << 8) | (*(input_data + 1));
                *(op) = (buffer & 0xF800) >> 11;
                *(op + 1) = (buffer & 0x07E0) >> 5;
                *(op + 2) = (buffer & 0x001F);
                op += 3;
                input_data += 2;
            }
        }
        type = GL_UNSIGNED_BYTE;
    }
    else if (format == GL_BGRA_EXT && type == GL_UNSIGNED_BYTE)
    {
        // convert to BGRA888
        output_data = new unsigned char[4 * width * height];
        unsigned char *op = output_data;
        const unsigned char *input_data = data;
        for (unsigned int j = 0; j < height; j++)
        {
            for (unsigned int i = 0; i < width; i++)
            {
                *(op) = *(input_data + 2);
                *(op + 1) = *(input_data + 1);
                *(op + 2) = *(input_data);
                *(op + 3) = *(input_data + 3);
                input_data += 4;
                op += 4;
            }
        }
        format = GL_RGBA;
    }
    else
    {
        PAT_DEBUG_LOG("PNG WRITE (Incompatible format : %d)\n", format);
        return false;
    }
    
    FILE *fp = fopen(filename, "wb");
    if (!fp)
    {
        PAT_DEBUG_LOG("PNG WRITE (Failed to open file %s to write)\n", filename);
        return false;
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
    {
        PAT_DEBUG_LOG("PNG WRITE (Failed to create write struct)\n");
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr,  NULL);
        PAT_DEBUG_LOG("PNG WRITE (Failed to create info struct)\n");
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        PAT_DEBUG_LOG("PNG WRITE (Failed to set jump buffer)\n");
        return false;
    }

    png_init_io(png_ptr, fp);

    int color_type;
    const unsigned char channels = GetImageChannelCount(format);
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

    png_set_IHDR(png_ptr, info_ptr, width, height, 8, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    const int png_compression_level = Z_BEST_SPEED;
    png_set_compression_level(png_ptr, png_compression_level);
    png_write_info(png_ptr, info_ptr);

    const unsigned char * p = (const unsigned char *)(output_data);
    if (!flip) {
        for (unsigned y = 0; y < height; ++y) {
            png_bytep row = (png_bytep)(p + y*width*channels);
            png_write_rows(png_ptr, &row, 1);
        }
    } else {
        unsigned y = height;
        while (y--) {
            png_bytep row = (png_bytep)(p + y*width*channels);
            png_write_rows(png_ptr, &row, 1);
        }
    }

    if (output_data != data)
        delete [] output_data;

    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    return true;
}

bool ReadPNG(Image &image, const char *filepath)
{
    FILE *fp = fopen(filepath, "rb");
    if (!fp)
    {
        PAT_DEBUG_LOG("PNG READ (Failed to open file %s to read)\n", filepath);
        fclose(fp);
        return false;
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
    {
        PAT_DEBUG_LOG("PNG READ (Failed to create read struct)\n");
        fclose(fp);
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        PAT_DEBUG_LOG("PNG READ (Failed to create info struct)\n");
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(fp);
        return false;
    }

    png_infop end_info = png_create_info_struct(png_ptr);
    if (!end_info)
    {
        PAT_DEBUG_LOG("PNG READ (Failed to create info struct)\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        PAT_DEBUG_LOG("PNG READ (Failed to set jmp)\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        return false;
    }

    png_init_io(png_ptr, fp);
    png_read_info(png_ptr, info_ptr);

    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type, compression_type, filter_method;
    png_get_IHDR(png_ptr, info_ptr,
                 &width, &height,
                 &bit_depth, &color_type, &interlace_type,
                 &compression_type, &filter_method);

    // read as RGBA format
    const unsigned int data_size = width * height * 4;
    unsigned char *data = new unsigned char[data_size];
    if (!data)
    {
        PAT_DEBUG_LOG("PNG READ (Failed to allocate memory)\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        return false;
    }
    
    /* Convert to RGBA8 */
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png_ptr);
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);
    if (bit_depth == 16)
        png_set_strip_16(png_ptr);

    for (unsigned y = 0; y < height; ++y)
    {
        png_bytep row = (png_bytep)(data + y*width*4);
        png_read_row(png_ptr, row, NULL);
    }

    png_read_end(png_ptr, info_ptr);
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    fclose(fp);

    image.Set(width, height, GL_RGBA, GL_UNSIGNED_BYTE, data_size, data, false, true);

    return true;
}

} // namespace pat
