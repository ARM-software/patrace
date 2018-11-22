#include <cstdio>
#include <cstring>
#include <fstream>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "base/base.hpp"
#include "image.hpp"
#include "image_io.hpp"
#include "image_compression.hpp"

namespace
{

const char * KTX_IDENTIFIER = "\xabKTX 11\xbb\r\n\x1a\n";
unsigned int KTX_IDENTIFIER_LENGTH = strlen(KTX_IDENTIFIER);
unsigned int KTX_ENDIANNESS = 0x04030201;
unsigned int KTX_PIXEL_DEPTH = 1;
unsigned int KTX_NUMBER_OF_ARRAY_ELEMENTS = 0;
unsigned int KTX_NUMBER_OF_FACES = 1;
unsigned int KTX_NUMBER_OF_MIPMAP_LEVEL = 1;
unsigned int KTX_BYTES_OF_KEY_VALUE_DATA = 0;

} // unamed namespace

namespace pat
{

bool WriteKTX(const Image &image, const char *filepath, bool flip)
{
    const unsigned int format = image.Format();
    const unsigned int type = image.Type();
    const unsigned int width = image.Width();
    const unsigned int height = image.Height();
    const unsigned char *p = image.Data();
    // Image size is meaningless if the pixel pointer is NULL
    const unsigned int size = p ? image.DataSize() : 0;

    if (IsImageCompression(format) && flip)
    {
        PAT_DEBUG_LOG("Compressed data can't be flipped, continue\n");
        return false;
    }

    std::ofstream os(filepath);
    if (os.is_open() == false)
    {
        PAT_DEBUG_LOG("Failed to open filepath : %s\n", filepath);
        return false;
    }

    os << KTX_IDENTIFIER;
    os.write((const char *)&KTX_ENDIANNESS, sizeof(unsigned int));

    os.write((const char *)&type, sizeof(unsigned int));
    const unsigned int type_size = IsImageCompression(format) ? 1 : GetImageTypeSize(type);
    os.write((const char *)&type_size, sizeof(unsigned int));
    os.write((const char *)&format, sizeof(unsigned int));
    os.write((const char *)&format, sizeof(unsigned int));
    const unsigned int base_internal_format = IsImageCompression(format) ? GL_RGB : format;
    os.write((const char *)&base_internal_format, sizeof(unsigned int));
    os.write((const char *)&width, sizeof(unsigned int));
    os.write((const char *)&height, sizeof(unsigned int));
    os.write((const char *)&KTX_PIXEL_DEPTH, sizeof(unsigned int));
    os.write((const char *)&KTX_NUMBER_OF_ARRAY_ELEMENTS, sizeof(unsigned int));
    os.write((const char *)&KTX_NUMBER_OF_FACES, sizeof(unsigned int));
    os.write((const char *)&KTX_NUMBER_OF_MIPMAP_LEVEL, sizeof(unsigned int));
    os.write((const char *)&KTX_BYTES_OF_KEY_VALUE_DATA, sizeof(unsigned int));

    os.write((const char *)&size, sizeof(unsigned int));

    if (p)
    {
        if (flip)
        {
            const unsigned int widthSize = GetImagePixelSize(format, type) * width;
            for (int j = height - 1; j >= 0; --j)
            {
                const unsigned char *l = p + widthSize * j;
                os.write((const char *)l, widthSize);
            }
        }
        else
        {
            os.write((const char*)p, size);
        }
    }

    return true;
}

bool ReadKTX(Image &image, const char *filepath)
{
    std::ifstream is(filepath);
    if (is.is_open() == false)
    {
        PAT_DEBUG_LOG("Failed to open filepath : %s\n", filepath);
        return false;
    }

    char buffer[256];
    is.read(buffer, KTX_IDENTIFIER_LENGTH);
    if (strncmp(buffer, KTX_IDENTIFIER, KTX_IDENTIFIER_LENGTH) != 0)
    {
        printf("Invalid KTX identifier : %s\n", buffer);
        return false;
    }

    GLuint read_endianness = 0;
    is.read((char *)&read_endianness, sizeof(GLuint));
    if (read_endianness != KTX_ENDIANNESS)
    {
        printf("Invalid KTX endianness : %d\n", read_endianness);
        return false;
    }

    unsigned int format = GL_NONE;
    unsigned int type = GL_NONE;
    unsigned int width = 0;
    unsigned int height = 0;
    
    is.read((char *)&type, sizeof(GLuint));
    is.ignore(sizeof(GLuint));
    is.ignore(sizeof(GLuint));
    is.read((char *)&format, sizeof(GLuint));
    is.ignore(sizeof(GLuint));
    is.read((char *)&width, sizeof(GLuint));
    is.read((char *)&height, sizeof(GLuint));
    is.ignore(sizeof(GLuint));
    is.ignore(sizeof(GLuint));
    is.ignore(sizeof(GLuint));
    is.ignore(sizeof(GLuint));
    GLuint bytes_of_key_value_data = 0;
    is.read((char *)&bytes_of_key_value_data, sizeof(GLuint));
    is.ignore(bytes_of_key_value_data);
    
    GLsizei size = 0;
    is.read((char *)&size, sizeof(GLuint));
    unsigned char *data = new unsigned char[size];
    is.read((char *)data, size);

    image.Set(width, height, format, type, size, data, false, true);

    return true;
}

} // namespace pat
