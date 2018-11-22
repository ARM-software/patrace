#include <string>
#include <algorithm>

#include "system/path.hpp"
#include "image.hpp"
#include "image_io.hpp"

namespace pat
{

bool ReadImage(Image &image, const char *filepath)
{
    Path path(filepath);
    std::string ext = path.GetExtension();
    // convert to uppercase
    std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);

    if (ext == "PPM")
    {
        return ReadPNM(image, filepath);
    }
    else if (ext == "PNG")
    {
        return ReadPNG(image, filepath);
    }
    else if (ext == "KTX")
    {
        return ReadKTX(image, filepath);
    }
    else if (ext == "ASTC")
    {
        return ReadASTC(image, filepath);
    }
    else if (ext == "TIF" || ext == "TIFF")
    {
        return ReadTIFF(image, filepath);
    }

    PAT_DEBUG_LOG("Read Image (Unexpected extension : %s)\n", ext.c_str());
    return false;
}

bool WriteImage(const Image &image, const char *filepath, bool flip)
{
    Path path(filepath);
    std::string ext = path.GetExtension();
    // convert to uppercase
    std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);

    const UInt32 format = image.Format();
    const UInt32 type = image.Type();

    if (ext == "PPM")
    {
        if (CanWriteAsPNM(format, type))
        {
            return WritePNM(image, filepath, flip);
        }
        else
        {
            PAT_DEBUG_LOG("Unexpected format & type when writing image as PNM : 0x%X 0x%X\n", format, type);
            return false;
        }
    }
    else if (ext == "PNG")
    {
        if (CanWriteAsPNG(format, type))
        {
            return WritePNG(image, filepath, flip);
        }
        else
        {
            PAT_DEBUG_LOG("Unexpected format & type when writing image as PNG : 0x%X 0x%X\n", format, type);
            return false;
        }
    }
    else if (ext == "KTX")
    {
        return WriteKTX(image, filepath, flip);
    }
    else if (ext == "ASTC")
    {
        return WriteASTC(image, filepath, flip);
    }

    PAT_DEBUG_LOG("Write Image (Unexpected extension : %s)\n", ext.c_str());
    return false;
}

} // namespace pat
