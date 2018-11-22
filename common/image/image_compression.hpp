#ifndef _INCLUDE_TEXTURE_COMPRESSION_HPP_
#define _INCLUDE_TEXTURE_COMPRESSION_HPP_

#include "base/base.hpp"

namespace pat
{

class Image;

class ImageCompressionFormat
{
public:
    virtual ~ImageCompressionFormat() {}

    virtual bool SupportCompression() const { return true; }
    virtual bool SupportUncompression() const { return true; }

    virtual bool IsCompressed(UInt32) const { return false; }
    virtual bool CanCompress(UInt32, UInt32) const { return false; }

    virtual bool Compress(const Image &input, Image &output) const = 0;
    virtual bool Uncompress(const Image &input, Image &output) const = 0;
    // Uncompress immediately after compressing, mainly for testing and quality comparision
    bool CompressUncompress(const Image &input, Image &output) const;
};

// Block Truncation Coding
class BTCCompressionFormat : public ImageCompressionFormat
{
    virtual bool IsCompressed(UInt32 format) const;
    virtual bool CanCompress(UInt32 format, UInt32 type) const;

    virtual bool Compress(const Image &input, Image &output) const;
    virtual bool Uncompress(const Image &input, Image &output) const;
};

// To fetch the option list of image compression, not one-to-one with image compression format
bool GetCompressionOptionList(const char **&optionList, UInt32 &optionCount);
bool IsValidCompressionOption(const std::string &option);
bool CheckCompressionOptionSupport(const std::string &option);

bool CanCompressAs(UInt32 format, UInt32 type, const std::string &option);
bool Uncompress(const Image &input, Image &output);
bool Compress(const Image &input, Image &output, const std::string &option);

///////////////////////////////////////////////////////////////
// ETC compression
///////////////////////////////////////////////////////////////

// ETC1 uncompression is always supported; ETC1 compression is supported when MALI texture tool can be found in $PATH
// ETC2 compression and uncompression are supported when MALI texture tool can be found is $PATH
extern const char * ETC_COMPRESSION_TOOL;
bool SupportETC1Compression();
bool SupportETC1Uncompression();
bool SupportETC2Compression();
bool SupportETC2Uncompression();

bool IsETC1Compression(UInt32 format);
bool IsETC2Compression(UInt32 format);
// whether this format & type combination is supported
bool CanCompressAsETC1(UInt32 format, UInt32 type);
bool CanCompressAsETC2(UInt32 format, UInt32 type);

bool CompressAsETC1(const Image &input, Image &output);
// Uncompress to GL_RGB & GL_UNSIGNED_BYTE
bool UncompressFromETC1(const Image &input, Image &output);

// only support alpha depth to be 1 or 8
bool CompressAsETC2(const Image &input, Image &output, UInt32 alphaDepth);

///////////////////////////////////////////////////////////////
// ASTC compression
///////////////////////////////////////////////////////////////

// ASTC compression and uncompression are supported when ASTC encoder can be found in $PATH
extern const char * ASTC_COMPRESSION_TOOL;
bool SupportASTCCompression();
bool SupportASTCUncompression();

bool IsASTCCompression(UInt32 format);
// whether this format & type combination is supported
bool CanCompressAsASTC(UInt32 format, UInt32 type);

bool CompressAsASTC(const Image &input, Image &output, UInt8 bx, UInt8 by);
bool UncompressFromASTC(const Image &input, Image &output);

}

#endif // _INCLUDE_TEXTURE_COMPRESSION_HPP_
