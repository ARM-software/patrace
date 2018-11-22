#include <cstring>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "image.hpp"
#include "image_io.hpp"
#include "image_compression.hpp"
#include "system/environment_variable.hpp"

namespace
{

const int ModifierTable[] = {
    -8,    -2,    2,    8,
    -17,   -5,    5,    17,
    -29,   -9,    9,    29,
    -42,   -13,   13,   42,
    -60,   -18,   18,   60,
    -80,   -24,   24,   80,
    -106,  -33,   33,  106,
    -183,  -47,   47,  183,
};

const unsigned char ModifierIndexTable[] = {
    2, 3, 1, 0,
};

const unsigned char PixelXOffset[] = {
    0, 0, 0, 0,
    1, 1, 1, 1,
    2, 2, 2, 2,
    3, 3, 3, 3,
};

const unsigned char PixelYOffset[] = {
    0, 1, 2, 3,
    0, 1, 2, 3,
    0, 1, 2, 3,
    0, 1, 2, 3,
};

const unsigned char FlipTable1[] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    1, 1, 1, 1,
    1, 1, 1, 1,
};

const unsigned char FlipTable2[] = {
    0, 0, 1, 1,
    0, 0, 1, 1,
    0, 0, 1, 1,
    0, 0, 1, 1,
};

// Convert 3-bit two-complement number to signed byte
char ToSignedGLubyte(unsigned char input)
{
    if (input > 0x03)
        return -(0x08 - input);
    else
        return input;
}

unsigned char Extend5to8Bits(unsigned char input)
{
    return (input << 3) + ((input & 0x1C) >> 2);
}

unsigned char Extend4to8Bits(unsigned char input)
{
    return (input << 4) + input;
}

unsigned char Clamp(unsigned char a, int b)
{
    int c = a + b;
    if (c < 0)
        return 0;
    else if (c > 0xFF)
        return 0xFF;
    else
        return static_cast<unsigned char>(c);
}

void SetPixel(unsigned char *dest, unsigned char r, unsigned char g, unsigned char b, int modifier, int x, int y, int width, int height)
{
    if (x >= width || y >= height)
        return;

    dest += (width * y + x) * 3;
    dest[0] = Clamp(r, modifier);
    dest[1] = Clamp(g, modifier);
    dest[2] = Clamp(b, modifier);
}

void DecodeBlock(const unsigned char *src, unsigned char *dest, int x, int y, int width, int height)
{
    unsigned char buffer[8];
    memcpy(buffer, src, sizeof(unsigned char) * 8);

    const bool diffbit = buffer[3] & 0x02;
    const bool flipbit = buffer[3] & 0x01;

    unsigned char R[2], G[2], B[2];
    if (diffbit)
    {
        const unsigned char _R1 = (buffer[0] & 0xF8) >> 3;
        const char _dR2 = ToSignedGLubyte(buffer[0] & 0x07);
        const unsigned char _G1 = (buffer[1] & 0xF8) >> 3;
        const char _dG2 = ToSignedGLubyte(buffer[1] & 0x07);
        const unsigned char _B1 = (buffer[2] & 0xF8) >> 3;
        const char _dB2 = ToSignedGLubyte(buffer[2] & 0x07);
        const unsigned char _R2 = _R1 + _dR2;
        const unsigned char _G2 = _G1 + _dG2;
        const unsigned char _B2 = _B1 + _dB2;
        R[0] = Extend5to8Bits(_R1);
        R[1] = Extend5to8Bits(_R2);
        G[0] = Extend5to8Bits(_G1);
        G[1] = Extend5to8Bits(_G2);
        B[0] = Extend5to8Bits(_B1);
        B[1] = Extend5to8Bits(_B2);
    }
    else
    {
        const unsigned char _R1 = (buffer[0] & 0xF0) >> 4;
        const unsigned char _R2 = buffer[0] & 0x0F;
        const unsigned char _G1 = (buffer[1] & 0xF0) >> 4;
        const unsigned char _G2 = buffer[1] & 0x0F;
        const unsigned char _B1 = (buffer[2] & 0xF0) >> 4;
        const unsigned char _B2 = buffer[2] & 0x0F; 
        R[0] = Extend4to8Bits(_R1);
        R[1] = Extend4to8Bits(_R2);
        G[0] = Extend4to8Bits(_G1);
        G[1] = Extend4to8Bits(_G2);
        B[0] = Extend4to8Bits(_B1);
        B[1] = Extend4to8Bits(_B2);       
    }

    unsigned char codeWord1 = buffer[3] >> 5;
    unsigned char codeWord2 = (buffer[3] & 0x1C) >> 2;
    const int *modifierTable1 = ModifierTable + 4 * codeWord1;
    const int *modifierTable2 = ModifierTable + 4 * codeWord2;

    x = x << 2;
    y = y << 2;

    for (int i = 0; i < 16; ++i)
    {
        unsigned char index = 0;
        if (i < 8)
        {
            index = ((buffer[5] >> i) & 0x01) << 1;
            index += ((buffer[7] >> i) & 0x01);
        }
        else
        {
            index = ((buffer[4] >> (i - 8)) & 0x01) << 1;
            index += ((buffer[6] >> (i - 8)) & 0x01);
        }

        index = ModifierIndexTable[index];
        const unsigned char partIndex = (!flipbit) ? FlipTable1[i] : FlipTable2[i];
        const int modifier = (partIndex == 0) ? modifierTable1[index] :
            modifierTable2[index];

        SetPixel(dest, R[partIndex], G[partIndex], B[partIndex],
                 modifier, x + PixelXOffset[i],
                 y + PixelYOffset[i], width, height);
    }
}

} // unnamed namespace

namespace pat
{

const char * ETC_COMPRESSION_TOOL = "etcpack";

bool SupportETC1Compression()
{
    return true;
}

bool SupportETC1Uncompression()
{
    return true;
}

bool SupportETC2Compression()
{
    return SupportETC1Compression();
}

bool SupportETC2Uncompression()
{
    return SupportETC1Compression();
}

bool IsETC1Compression(UInt32 format)
{
    return format == GL_ETC1_RGB8_OES;
}

bool IsETC2Compression(UInt32 format)
{
    return format >= GL_COMPRESSED_RGB8_ETC2 && format <= GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC;
}

bool CanCompressAsETC1(UInt32 format, UInt32 type)
{
    return format == GL_RGB && type == GL_UNSIGNED_BYTE;
}

bool CanCompressAsETC2(UInt32 format, UInt32 type)
{
    return (format == GL_RGB || format == GL_RGBA) &&
        type == GL_UNSIGNED_BYTE;
}

bool UncompressFromETC1(const Image &input, Image &output)
{
    const UInt32 format = input.Format();
    PAT_DEBUG_ASSERT(IsETC1Compression(format), "Unexpected format for ETC1 uncompress : %d\n", format);
    if (IsETC1Compression(format) == false)
        return false;

    const unsigned int width = input.Width();
    const unsigned int height = input.Height();
    const int blockCountX = (width + 3) / 4;
    const int blockCountY = (height + 3) / 4;
    const unsigned char *srcData = input.Data();
    unsigned int destSize = 0;
    unsigned char *destData = NULL;
    if (srcData)
    {
        destSize = width * height * 3;
        destData = new unsigned char[destSize];
        for (int j = 0; j < blockCountY; ++j)
        {
            for (int i = 0; i < blockCountX; ++i)
            {
                DecodeBlock(srcData, destData, i, j, width, height); 
                srcData += 8;
            }
        }
    }
    output.Set(width, height, GL_RGB, GL_UNSIGNED_BYTE, destSize, destData, false, true); 
    return true;
}

bool CompressAsETC1(const Image &input, Image &output)
{
    const UInt32 format = input.Format();
    const UInt32 type = input.Type();
    const UInt32 width = input.Width();
    const UInt32 height = input.Height();
    
    if (CanCompressAsETC1(format, type) == false)
    {
        PAT_DEBUG_LOG("Unexpected format for ETC1 compress : %d %d\n", format, type);
        return false;
    }

    if (input.Data() == NULL)
    {
        output.Set(width, height, GL_ETC1_RGB8_OES, GL_NONE, 0, NULL, false, false);
        return true;
    }

    const char *DEFAULT_PPM_FILENAME = "/tmp/texture.ppm";
    const char *DEFAULT_KTX_DIRNAME = "/tmp";
    const char *DEFAULT_KTX_FILENAME = "/tmp/texture.ktx";

    if (WritePNM(input, DEFAULT_PPM_FILENAME, false) == false)
    {
        PAT_DEBUG_LOG("Failed to write to file : %s\n", DEFAULT_PPM_FILENAME);
        return false;
    }

    char buffer[512];
    sprintf(buffer, "%s %s %s -c etc1 -ktx -quiet", ETC_COMPRESSION_TOOL, DEFAULT_PPM_FILENAME, DEFAULT_KTX_DIRNAME);
    if (system(buffer) == -1)
    {
        PAT_DEBUG_LOG("Failed to convert image. Is the ASTC Evaluation Codec (astcenc) under your $PATH? If not, please download it from www.malideveloper.com.\n");
        return false;
    }

    if (ReadKTX(output, DEFAULT_KTX_FILENAME) == false)
    {
        PAT_DEBUG_LOG("Failed to read from file : %s\n", DEFAULT_KTX_FILENAME);
        return false;
    }

    return true;
}

bool CompressAsETC2(const Image &input, Image &output, UInt32 alphaDepth)
{
    const UInt32 PUNCHTHROUGH_ALPHA_DEPTH = 1;
    const UInt32 FULL_ALPHA_DEPTH = 8;
    if (alphaDepth != PUNCHTHROUGH_ALPHA_DEPTH && alphaDepth != FULL_ALPHA_DEPTH)
        return false;

    const UInt32 format = input.Format();
    const UInt32 type = input.Type();
    const UInt32 width = input.Width();
    const UInt32 height = input.Height();

    if (CanCompressAsETC2(format, type) == false)
    {
        PAT_DEBUG_LOG("Unexpected format for ETC2 compress : %d %d\n", format, type);
        return false;
    }

    const char *formatOption = NULL;
    UInt32 output_format = GL_NONE;
    if (format == GL_RGB)
    {
        formatOption = "RGB";
        output_format = GL_COMPRESSED_RGB8_ETC2;
    }
    else
    {
        if (alphaDepth == PUNCHTHROUGH_ALPHA_DEPTH)
        {
            formatOption = "RGBA1";
            output_format = GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
        }
        else
        {
            formatOption = "RGBA8";
            output_format = GL_COMPRESSED_RGBA8_ETC2_EAC;
        }
    }

    if (input.Data() == NULL)
    {
        output.Set(width, height, output_format, GL_NONE, 0, NULL, false, false);
        return true;
    }

    const char *DEFAULT_PNG_FILENAME = "/tmp/texture.png";
    const char *DEFAULT_KTX_DIRNAME = "/tmp";
    const char *DEFAULT_KTX_FILENAME = "/tmp/texture.ktx";

    if (WritePNG(input, DEFAULT_PNG_FILENAME, false) == false)
    {
        PAT_DEBUG_LOG("Failed to write to file : %s\n", DEFAULT_PNG_FILENAME);
        return false;
    }

    char buffer[512];
    sprintf(buffer, "%s %s %s -c etc2 -ktx -quiet -f %s", ETC_COMPRESSION_TOOL, DEFAULT_PNG_FILENAME, DEFAULT_KTX_DIRNAME, formatOption);
    if (system(buffer) == -1)
    {
        PAT_DEBUG_LOG("Failed to convert image. Is the ASTC Evaluation Codec (astcenc) under your $PATH? If not, please download it from www.malideveloper.com.\n");
        return false;
    }

    if (ReadKTX(output, DEFAULT_KTX_FILENAME) == false)
    {
        PAT_DEBUG_LOG("Failed to read from file : %s\n", DEFAULT_KTX_FILENAME);
        return false;
    }

    return true;
}

}
