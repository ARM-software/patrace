#ifndef _IMAGE_HPP_
#define _IMAGE_HPP_

#include <iostream>
#include "base/base.hpp"

namespace pat {

class Image
{
public:
    static const UInt32 MAX_CHANNEL_COUNT = 4;

    Image();
    Image(UInt32 width, UInt32 height,
        UInt32 format, UInt32 type,
        UInt32 dataSize, UInt8 *data,
        bool copyData = true, bool transferData = false)
    : _width(width), _height(height),
      _data(NULL), _ownData(false),
      _maxBitWidth(0), _sumBitWidth(0)
    {
        SetFormatType(format, type);
        SetData(dataSize, data, copyData, transferData);
    }
    ~Image();

    bool Empty() const { return _width == 0 || _height == 0 || _dataSize == 0 || _data == NULL; }

    // if the input instance owns the data, the ownership will be transfered
    UInt32 Width() const { return _width; }
    UInt32 Height() const { return _height; }
    UInt32 Format() const { return _format; }
    UInt32 Type() const { return _type; }
    UInt32 DataSize() const { return _dataSize; }
    const UInt8 * Data() const { return _data; }
    void SetOwnData(bool v) { _ownData = v; }
    
    void Set(UInt32 w, UInt32 h, UInt32 format, UInt32 type, UInt32 ds, UInt8 *data, bool copyData, bool transferData);
    void SetSubData(UInt32 xoffset, UInt32 yoffset, UInt32 width, UInt32 height, UInt8 *data);

private:
    Image(const Image &other);
    Image &operator =(const Image &other);
    
    void SetFormatType(UInt32 format, UInt32 type);
    void SetData(UInt32 dataSize, UInt8 *data, bool copyData, bool transferData);

    UInt32 _width, _height;
    UInt32 _format, _type;
    UInt32 _dataSize;
    UInt8 *_data;
    bool _ownData; // If _ownData equal true, destructor will delete data
    
    // Only meaningful for uncompressed image
    UInt8 _bitWidth[MAX_CHANNEL_COUNT];
    UInt8 _bitOffset[MAX_CHANNEL_COUNT];
    UInt8 _maxBitWidth;
    UInt8 _sumBitWidth;
};
typedef std::shared_ptr<Image> ImagePtr;

bool IsImageCompression(UInt32 format);
UInt32 GetImageChannelCount(UInt32 format);
UInt32 GetImageTypeSize(UInt32 type);
// the memory space of bits should be allocated outside, and at least 4 bytes
bool GetImageBitWidth(UInt32 format, UInt32 type, UInt8* bits);
UInt32 GetImagePixelSize(UInt32 format, UInt32 type); // in bytes
UInt32 GetImageDataSize(UInt32 width, UInt32 height, UInt32 format, UInt32 type);

bool WithAlphaChannel(UInt32 format);

bool BGRAToRGBA(Image &input, Image &output);

bool GenerateNextMipmapLevel(const Image &input, Image &output);

// For UI display
bool ConvertToRGB8(const Image &input, Image &output);

} /* namespace pat */

#endif /* _IMAGE_HPP_ */
