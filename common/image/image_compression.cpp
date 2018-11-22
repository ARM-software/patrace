#include "image_compression.hpp"
#include "image.hpp"

namespace
{

const char * COMPRESSION_OPTION_LIST[] = {
    "UNCOMPRESSED",
    "ETC1",
    "ETC2_A1",
    "ETC2_A8",
    "ASTC4x4",
    "ASTC5x4",
    "ASTC5x5",
    "ASTC6x5",
    "ASTC6x6",
    "ASTC8x5",
    "ASTC8x6",
    "ASTC8x8",
    "ASTC10x5",
    "ASTC10x6",
    "ASTC10x8",
    "ASTC10x10",
    "ASTC12x10",
    "ASTC12x12",
};
const UInt32 COMPRESSION_OPTION_COUNT =
    sizeof(COMPRESSION_OPTION_LIST) / sizeof(const char*);

const std::string PREFIX_ETC = "ETC";
const std::string PREFIX_ETC1 = "ETC1";
const std::string PREFIX_ETC2 = "ETC2";
const std::string PREFIX_ASTC = "ASTC";

}

namespace pat
{

bool GetCompressionOptionList(const char **&optionList, UInt32 &optionCount)
{
    optionList = COMPRESSION_OPTION_LIST;
    optionCount = COMPRESSION_OPTION_COUNT;
    return true;
}

bool IsValidCompressionOption(const std::string &option)
{
    for (UInt32 i = 0; i < COMPRESSION_OPTION_COUNT; ++i)
    {
        if (COMPRESSION_OPTION_LIST[i] == option)
            return true;
    }
    return false;
}

bool CheckCompressionOptionSupport(const std::string &option)
{
    if (option.compare(0, PREFIX_ETC.size(), PREFIX_ETC) == 0)
    {
        if (SupportETC1Compression())
        {
            return true;
        }
        else
        {
            PAT_DEBUG_LOG("Can't find the Mali GPU Texture Compression Tool (etcpack) under your $PATH. Please download it from www.malideveloper.com. Exit.\n");
            return false;
        }
    }
    else if (option.compare(0, PREFIX_ASTC.size(), PREFIX_ASTC) == 0)
    {
        if (SupportASTCCompression())
        {
            return true;
        }
        else
        {
            PAT_DEBUG_LOG("Can't find the ASTC Evaluation Codec (astcenc) under your $PATH. Please download it from www.malideveloper.com. Exit.\n");
            return false;
        }
    }
    return false;
}

bool CanCompressAs(UInt32 format, UInt32 type, const std::string &option)
{
    if (IsValidCompressionOption(option) == false)
        return false;

    if (option == "ETC1")
    {
        return CanCompressAsETC1(format, type);
    }
    else if (option.compare(0, PREFIX_ETC2.size(), PREFIX_ETC2) == 0)
    {
        return CanCompressAsETC2(format, type);
    }
    else if (option.compare(0, PREFIX_ASTC.size(), PREFIX_ASTC) == 0)
    {
        return CanCompressAsASTC(format, type);
    }
    else
    {
        return false;
    }
}

bool IsImageCompression(UInt32 format)
{
    return IsETC1Compression(format) ||
           IsETC2Compression(format) ||
           IsASTCCompression(format) ||
           format == GL_BTC_LUMINANCE_PAT ||
           format == GL_PALETTE8_RGB5_A1_OES;
}

bool Uncompress(const Image &input, Image &output)
{
    const UInt32 input_format = input.Format();

    if (IsETC1Compression(input_format))
    {
        return UncompressFromETC1(input, output);
    }
    else if (IsASTCCompression(input_format))
    {
        return UncompressFromASTC(input, output);
    }
    else
    {
        PAT_DEBUG_LOG("Unknown format to uncompress : %d\n", input_format);
        return false;
    }
}

bool Compress(const Image &input, Image &output, const std::string &option)
{
    if (IsValidCompressionOption(option) == false)
        return false;

    const UInt32 format = input.Format();
    const UInt32 type = input.Type();
    if (option == "ETC1")
    {
        if (CanCompressAsETC1(format, type))
        {
            return CompressAsETC1(input, output);
        }
    }
    else if (option.compare(0, PREFIX_ETC2.size(), PREFIX_ETC2) == 0)
    {
        if (CanCompressAsETC2(format, type))
        {
            UInt32 alphaDepth = 0;
            sscanf(option.c_str(), "ETC2_A%d", &alphaDepth);
            return CompressAsETC2(input, output, alphaDepth);
        }
    }
    else if (option == "UNCOMPRESSED")
    {
        return true;
    }
    else if (option.compare(0, PREFIX_ASTC.size(), PREFIX_ASTC) == 0)
    {
        if (CanCompressAsASTC(format, type))
        {
            UInt32 blockDimX = 0, blockDimY = 0;
            sscanf(option.c_str(), "ASTC%dx%d", &blockDimX, &blockDimY);
            return CompressAsASTC(input, output, blockDimX, blockDimY);
        }
    }

    return false;
}

bool ImageCompressionFormat::CompressUncompress(const Image &input, Image &output) const
{
    Image temp;
    return Compress(input, temp) && Uncompress(temp, output);
}

}
