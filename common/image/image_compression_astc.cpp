#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "image.hpp"
#include "image_io.hpp"
#include "image_compression.hpp"
#include "system/environment_variable.hpp"

namespace
{

const char *DEFAULT_KTX_FILENAME = "/tmp/texture.ktx";
const char *DEFAULT_ASTC_FILENAME = "/tmp/texture.astc";
    
}

namespace pat
{

const char *ASTC_COMPRESSION_TOOL = "astcenc";

bool SupportASTCCompression()
{
    //std::string path;
    //return EnvironmentVariable::SearchUnderSystemPath(ASTC_COMPRESSION_TOOL, path);
    return true;
}

bool SupportASTCUncompression()
{
    return SupportASTCCompression();
}

bool IsASTCCompression(UInt32 format)
{
    return format >= COMPRESSED_ASTC_FORMAT_BEGIN && format <= COMPRESSED_ASTC_FORMAT_END;
}

bool CanCompressAsASTC(UInt32 format, UInt32 type)
{
    return ((format == GL_RGB || format == GL_RGBA ||
             format == GL_BGRA_EXT || format == GL_LUMINANCE ||
             format == GL_LUMINANCE_ALPHA) &&
            (type == GL_UNSIGNED_BYTE || type == GL_UNSIGNED_SHORT ||
             type == GL_HALF_FLOAT_OES || type == GL_FLOAT));
}

bool CompressAsASTC(const Image &input, Image &output, UInt8 bx, UInt8 by)
{
    const UInt32 format = input.Format();
    const UInt32 type = input.Type();

    if (CanCompressAsASTC(format, type) == false)
    {
        PAT_DEBUG_LOG("Unexpected format for ASTC compress : %d %d\n", format, type);
        return false;
    }

    UInt32 astc_format = GL_NONE;
    if (ASTCFormatFromBlockDimensions(bx, by, astc_format) == false)
    {
        //PAT_DEBUG_LOG("Unexpected block dimensions for ASTC : %dx%d\n", bx, by);
        return false;
    }

    if (input.Data() == NULL)
    {
        output.Set(input.Width(), input.Height(), astc_format, GL_NONE, 0, NULL, false, false);
        return true;
    }

    if (WriteKTX(input, DEFAULT_KTX_FILENAME, false) == false)
    {
        PAT_DEBUG_LOG("Failed to write to file : %s\n", DEFAULT_KTX_FILENAME);
        return false;
    }

    char buffer[512];
    sprintf(buffer, "%s -c %s %s %dx%d -thorough -silentmode", ASTC_COMPRESSION_TOOL, DEFAULT_KTX_FILENAME, DEFAULT_ASTC_FILENAME, bx, by);
    if (system(buffer) == -1)
    {
        PAT_DEBUG_LOG("Failed to convert image. Is the ASTC Evaluation Codec (astcenc) under your $PATH? If not, please download it from www.malideveloper.com.\n");
        return false;
    }

    if (ReadASTC(output, DEFAULT_ASTC_FILENAME) == false)
    {
        PAT_DEBUG_LOG("Failed to read from file : %s\n", DEFAULT_ASTC_FILENAME);
        return false;
    }

    return true;
}

bool UncompressFromASTC(const Image &input, Image &output)
{
    const UInt32 format = input.Format();
    if (IsASTCCompression(format) == false)
    {
        PAT_DEBUG_LOG("Unexpected format for ASTC uncompression : %d\n", format);
        return false;
    }

    if (input.Data() == NULL)
    {
        output.Set(input.Width(), input.Height(), GL_RGBA, GL_UNSIGNED_BYTE, 0, NULL, false, false);
        return true;
    }

    if (WriteASTC(input, DEFAULT_ASTC_FILENAME, false) == false)
    {
        PAT_DEBUG_LOG("Failed to write to file : %s\n", DEFAULT_ASTC_FILENAME);
        return false;
    }

    char buffer[512];
    sprintf(buffer, "%s -ds %s %s -thorough -silentmode", ASTC_COMPRESSION_TOOL, DEFAULT_ASTC_FILENAME, DEFAULT_KTX_FILENAME);
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

