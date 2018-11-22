#include <cstdio>
#include <cstring>
#include <fstream>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "image.hpp"
#include "image_io.hpp"

namespace pat
{

bool CanWriteAsPNM(UInt32 format, UInt32 type)
{
    if ((format == GL_LUMINANCE && type == GL_UNSIGNED_BYTE) ||
        (format == GL_RGB && type == GL_UNSIGNED_BYTE))
        return true;
    else
        return false;
}

bool WritePNM(const Image &image, const char *filepath, bool flip)
{
    std::ofstream os(filepath);
    if (os.is_open() == false)
    {
        PAT_DEBUG_LOG("Failed to open filepath : %s\n", filepath);
        return false;
    }

    const unsigned int format = image.Format();
    const unsigned int type = image.Type();
    const unsigned int width = image.Width();
    const unsigned int height = image.Height();

    if ((format == GL_LUMINANCE && type == GL_UNSIGNED_BYTE) ||
        (format == GL_RGB && type == GL_UNSIGNED_BYTE))
    {
        // accepted format & type pairs
    }
    else
    {
        PAT_DEBUG_LOG("Unexpected format & type pair : 0x%X 0x%X\n", format, type);
        return false;
    }

    unsigned char cn = GetImageChannelCount(format);
    os << (cn == 1 ? "P5" : "P6") << "\n";
    os << width << " " << height << "\n";
    os << "255" << "\n";

    const unsigned char *p = image.Data();
    const unsigned int pixelSize = GetImagePixelSize(format, type);
    const unsigned int widthSize = pixelSize * width;

    if (flip)
    {
        for (int j = height - 1; j >= 0; --j)
        {
            const unsigned char *l = p + widthSize * j;
            os.write((const char *)l, widthSize);
        }
    }
    else
    {
        os.write((const char*)p, widthSize * height);
    }

    return true;
}

bool ReadPNM(Image &image, const char *filepath)
{
    std::ifstream is(filepath);
    if (is.is_open() == false)
    {
        PAT_DEBUG_LOG("Failed to open filepath : %s\n", filepath);
        return false;
    }

    unsigned int format = GL_NONE;
    unsigned int type = GL_NONE;
    unsigned int width = 0;
    unsigned int height = 0;

    char read_channels[2];
    is.read(read_channels, 2);
    if (strncmp(read_channels, "P5", 2) == 0)
    {
        format = GL_LUMINANCE;
        type = GL_UNSIGNED_BYTE;
    }
    else if (strncmp(read_channels, "P6", 2) == 0)
    {
        format = GL_RGB;
        type = GL_UNSIGNED_BYTE;
    }
    else
    {
        PAT_DEBUG_LOG("Invalid format in pnm file : %s\n", read_channels);
        return false;
    }
    
    is >> width;
    is >> height;

    unsigned int max_value = 0;
    is >> max_value;

    const unsigned int dataSize = GetImageDataSize(width, height, format, type);

    unsigned char *data = new unsigned char[dataSize];
    is.read((char *)data, dataSize);
    image.Set(width, height, format, type, dataSize, data, false, true);

    return true;
}

} // namespace pat
