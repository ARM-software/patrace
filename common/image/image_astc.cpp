#include <cstdio>
#include <cstring>
#include <fstream>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "image.hpp"
#include "image_io.hpp"
#include "image_compression.hpp"

namespace
{

const unsigned char ASTC_FORMAT_NUM = COMPRESSED_ASTC_RGBA_12x12_OES - COMPRESSED_ASTC_RGBA_4x4_OES + 1;
const unsigned char ASTC_DIM_X[ASTC_FORMAT_NUM] =
    {4, 5, 5, 6, 6, 8, 8, 8, 10, 10, 10, 10, 12, 12};
const unsigned char ASTC_DIM_Y[ASTC_FORMAT_NUM] =
    {4, 4, 5, 5, 6, 5, 6, 8,  5,  6,  8, 10, 10, 12};

const GLuint ASTC_HEAD_MAGIC = 0x5CA1AB13;

}

namespace pat
{

bool ASTCBlockDimensionsFromFormat(UInt32 format, UInt8 &bx, UInt8 &by)
{
    if (format >= COMPRESSED_ASTC_RGBA_4x4_OES && format <= COMPRESSED_ASTC_RGBA_12x12_OES)
    {
        bx = ASTC_DIM_X[format - COMPRESSED_ASTC_RGBA_4x4_OES];
        by = ASTC_DIM_Y[format - COMPRESSED_ASTC_RGBA_4x4_OES];
        return true;
    }
    else
    {
        return false;
    }
}

bool ASTCFormatFromBlockDimensions(UInt8 bx, UInt8 by, UInt32& format)
{
    for (UInt32 i = COMPRESSED_ASTC_RGBA_4x4_OES; i <= COMPRESSED_ASTC_RGBA_12x12_OES; ++i)
    {
        UInt8 x, y;
        ASTCBlockDimensionsFromFormat(i, x, y);
        if (bx == x && by == y)
        {
            format = i;
            return true;
        }
    }
    return false;
}

bool WriteASTC(const Image &image, const char *filepath, bool flip)
{
    const unsigned int format = image.Format();
    const unsigned int width = image.Width();
    const unsigned int height = image.Height();
    const unsigned int size = image.DataSize();

    if (flip)
    {
        PAT_DEBUG_LOG("Compressed data can't be flipped, continue\n");
        flip = false;
    }

    if (IsASTCCompression(format) == false)
    {
        PAT_DEBUG_LOG("Unexpected format : %d\n", format);
        return false;
    }

    GLubyte blockDimX, blockDimY;
    if (!ASTCBlockDimensionsFromFormat(format, blockDimX, blockDimY))
    {
        PAT_DEBUG_LOG("Unexpected format : %d\n", format);
        return false;
    }

    std::ofstream os(filepath);
    if (os.is_open() == false)
    {
        PAT_DEBUG_LOG("Failed to open filepath : %s\n", filepath);
        return false;
    }

    os.write((const char *)&ASTC_HEAD_MAGIC, sizeof(GLuint));
    os.write((const char *)&blockDimX, sizeof(GLubyte));
    os.write((const char *)&blockDimY, sizeof(GLubyte));
    const GLubyte blockDimZ = 1;
    os.write((const char *)&blockDimZ, sizeof(GLubyte));

    const GLubyte xsize0 = width & 0xFF;
    const GLubyte xsize1 = (width >> 8) & 0xFF;
    const GLubyte xsize2 = (width >> 16) & 0xFF;
    const GLubyte ysize0 = height & 0xFF;
    const GLubyte ysize1 = (height >> 8) & 0xFF;
    const GLubyte ysize2 = (height >> 16) & 0xFF;
    const GLubyte zsize0 = 1;
    const GLubyte zsize1 = 0;
    const GLubyte zsize2 = 0;
    os.write((const char *)&xsize0, sizeof(GLubyte));
    os.write((const char *)&xsize1, sizeof(GLubyte));
    os.write((const char *)&xsize2, sizeof(GLubyte));
    os.write((const char *)&ysize0, sizeof(GLubyte));
    os.write((const char *)&ysize1, sizeof(GLubyte));
    os.write((const char *)&ysize2, sizeof(GLubyte));
    os.write((const char *)&zsize0, sizeof(GLubyte));
    os.write((const char *)&zsize1, sizeof(GLubyte));
    os.write((const char *)&zsize2, sizeof(GLubyte));

    const unsigned char *p = image.Data();
    os.write((const char*)p, size);

    return true;
}

bool ReadASTC(Image &image, const char *filepath)
{
    std::ifstream is(filepath);
    if (is.is_open() == false)
    {
        PAT_DEBUG_LOG("Failed to open filepath : %s\n", filepath);
        return false;
    }
    
    is.ignore(sizeof(GLuint)); // header magic header
    GLubyte blockDimX, blockDimY;
    is.read((char *)&blockDimX, sizeof(GLubyte));
    is.read((char *)&blockDimY, sizeof(GLubyte));
    is.ignore(sizeof(GLubyte)); // block dimension z
    unsigned int format = GL_NONE;
    if (ASTCFormatFromBlockDimensions(blockDimX, blockDimY, format) == false)
        return false;

    GLubyte xsize0, xsize1, xsize2, ysize0, ysize1, ysize2;
    is.read((char *)&xsize0, sizeof(GLubyte));
    is.read((char *)&xsize1, sizeof(GLubyte));
    is.read((char *)&xsize2, sizeof(GLubyte));
    is.read((char *)&ysize0, sizeof(GLubyte));
    is.read((char *)&ysize1, sizeof(GLubyte));
    is.read((char *)&ysize2, sizeof(GLubyte));
    unsigned int width = (unsigned int)xsize0 + xsize1 * 0x100 + xsize2 * 0x10000;
    unsigned int height = (unsigned int)ysize0 + ysize1 * 0x100 + ysize2 * 0x10000;
    is.ignore(sizeof(GLubyte)); // size z
    is.ignore(sizeof(GLubyte));
    is.ignore(sizeof(GLubyte));

    GLubyte dx, dy;
    ASTCBlockDimensionsFromFormat(format, dx, dy);
    const GLuint blockX = (width + dx - 1) / dx;
    const GLuint blockY = (height + dy - 1) / dy;
    unsigned int size = blockX * blockY * 16;
    if (size == 0)
    {
        PAT_DEBUG_LOG("Unexpected zero-size image data\n");
    }
    unsigned char * buffer = new unsigned char[size];
    is.read((char *)buffer, size);

    image.Set(width, height, format, GL_NONE, size, buffer, false, true);

    return true;
}

}
