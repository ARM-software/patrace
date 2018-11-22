#ifndef _INCLUDE_IMAGE_IO_HPP_
#define _INCLUDE_IMAGE_IO_HPP_

#include "base/base.hpp"

namespace pat
{

class Image;

bool WriteImage(const Image &image, const char *filepath, bool flip);
bool ReadImage(Image &image, const char *filepath);

// Read and write PPM images
bool CanWriteAsPNM(UInt32 format, UInt32 type);
bool WritePNM(const Image &image, const char *filepath, bool flip);
bool ReadPNM(Image &image, const char *filepath);

bool CanWriteAsPNG(UInt32 format, UInt32 type);
bool WritePNG(const Image &image, const char *filepath, bool flip);
bool ReadPNG(Image &image, const char *filepath);

bool WriteKTX(const Image &image, const char *filepath, bool flip);
bool ReadKTX(Image &image, const char *filepath);

bool WriteASTC(const Image &image, const char *filepath, bool flip);
bool ReadASTC(Image &image, const char *filepath);

bool ReadTIFF(Image &image, const char *filepath);

bool ASTCBlockDimensionsFromFormat(UInt32 format, UInt8 &bx, UInt8 &by);
bool ASTCFormatFromBlockDimensions(UInt8 bx, UInt8 by, UInt32& format);

} // namespace pat

#endif // _INCLUDE_IMAGE_IO_HPP_
