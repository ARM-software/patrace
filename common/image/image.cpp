#include <cstring>
#include <cassert>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "image.hpp"
#include "image_compression.hpp"

namespace
{


}

namespace pat {

bool WithAlphaChannel(UInt32 format)
{
    return format == GL_ALPHA || format == GL_RGBA || format == GL_BGRA_EXT || format == GL_LUMINANCE_ALPHA;
}

UInt32 GetImageChannelCount(UInt32 format)
{
    switch (format)
    {
    case GL_ALPHA:
    case GL_LUMINANCE:
    case GL_DEPTH_COMPONENT:
    case GL_RED_EXT:
        return 1;

    case GL_LUMINANCE_ALPHA:
    case GL_DEPTH_STENCIL_OES:
    case GL_RG_EXT:
        return 2;

    case GL_RGB:
    case GL_BGR_EXT:
        return 3;

    case GL_RGBA:
    case GL_BGRA_EXT:
        return 4;

    default:
        PAT_DEBUG_LOG("Unexpected format : 0x%X\n", format);
        return 0;
    }
}

bool GetImageBitWidth(UInt32 format, UInt32 type, UInt8 *bits)
{
    const UInt32 cn = GetImageChannelCount(format);
    const UInt32 ts = GetImageTypeSize(type);
    memset(bits, 0, Image::MAX_CHANNEL_COUNT);

    switch (type)
    {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
    case GL_HALF_FLOAT:
    case GL_HALF_FLOAT_OES:
    case GL_INT:
    case GL_UNSIGNED_INT:
    case GL_FLOAT:
        for (UInt32 i = 0; i < cn; i++)
            bits[i] = ts * BYTE_BIT_WIDTH;
        return true;

    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT:
        bits[0] = bits[1] = bits[2] = bits[3] = 4;
        return true;

    case GL_UNSIGNED_SHORT_5_6_5:
        bits[0] = bits[2] = 5;
        bits[1] = 6;
        return true;

    case GL_UNSIGNED_SHORT_5_5_5_1:
        bits[0] = bits[1] = bits[2] = 5;
        bits[3] = 1;
        return true;

    case GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT:
        bits[0] = 1;
        bits[1] = bits[2] = bits[3] = 5;
        return true;

    case GL_UNSIGNED_INT_2_10_10_10_REV_EXT:
        bits[0] = 2;
        bits[1] = bits[2] = bits[3] = 10;
        return true;

    case GL_UNSIGNED_INT_24_8_OES:
        bits[0] = 24;
        bits[1] = 8;
        return true;

    case GL_UNSIGNED_INT_5_9_9_9_REV_APPLE:
        bits[0] = 5;
        bits[1] = bits[2] = bits[3] = 9;
        return true;

    case GL_UNSIGNED_INT_10F_11F_11F_REV_APPLE:
        bits[0] = 10;
        bits[1] = bits[2] = 11;
        return true;

    default:
        PAT_DEBUG_LOG("Unexpected data type : 0x%X\n", type);
        return 0;
    }
}

UInt32 GetImageTypeSize(UInt32 type)
{
    switch (type)
    {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
        return 1;

    case GL_SHORT:
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT:
    case GL_UNSIGNED_SHORT_5_5_5_1:
    case GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT:
    case GL_UNSIGNED_SHORT:
    case GL_HALF_FLOAT:
    case GL_HALF_FLOAT_OES:
        return 2;

    case GL_INT:
    case GL_UNSIGNED_INT_2_10_10_10_REV_EXT:
    case GL_UNSIGNED_INT_24_8_OES:
    case GL_UNSIGNED_INT:
    case GL_FLOAT:
    case GL_UNSIGNED_INT_10F_11F_11F_REV_APPLE:
    case GL_UNSIGNED_INT_5_9_9_9_REV_APPLE:
        return 4;

    default:
        PAT_DEBUG_LOG("Unexpected data type : 0x%X\n", type);
        return 0;
    }
}

UInt32 GetImagePixelSize(UInt32 format, UInt32 type)
{
   if (type == GL_UNSIGNED_SHORT_4_4_4_4 ||
       type == GL_UNSIGNED_SHORT_5_5_5_1 ||
       type == GL_UNSIGNED_SHORT_5_6_5 ||
       type == GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT ||
       type == GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT)
           return 2;
   else if (type == GL_UNSIGNED_INT_2_10_10_10_REV_EXT ||
            type == GL_UNSIGNED_INT_24_8_OES)
           return 4;
   else
           return GetImageChannelCount(format) * GetImageTypeSize(type);
}

UInt32 GetImageDataSize(UInt32 width, UInt32 height, UInt32 format, UInt32 type)
{
    return width * height * GetImagePixelSize(format, type);
}

Image::Image()
: _width(0), _height(0), _format(GL_NONE), _type(GL_NONE),
  _dataSize(0), _data(NULL), _ownData(false)
{
}

Image::~Image()
{
    if (_ownData)
    {
        _ownData = false;
        delete [] _data;
    }
    _data = NULL;
}

void Image::SetFormatType(UInt32 format, UInt32 type)
{
    _format = format;
    _type = type;
    if (IsImageCompression(format) == false)
    {
        if (GetImageBitWidth(format, type, _bitWidth))
        {
            for (UInt32 i = 0; i < MAX_CHANNEL_COUNT; ++i)
            {
                _sumBitWidth += _bitWidth[i];
                _maxBitWidth = std::max(_maxBitWidth, _bitWidth[i]);
                if (i == 0)
                    _bitOffset[i] = 0;
                else
                    _bitOffset[i] = _bitOffset[i-1] + _bitWidth[i-1];
            }
        }
        else
        {
            PAT_DEBUG_LOG("Unexpected format & type : 0x%X 0x%X\n", format, type);
        }
    }
}

void Image::SetData(UInt32 dataSize, UInt8 *data, bool copyData, bool transferData)
{
    // Can't set sub data for compression images, no worry about them
    if (IsImageCompression(_format) == false)
    {
        const UInt32 imageSize = GetImageDataSize(_width, _height, _format, _type);
        if (!((dataSize == 0 && data == NULL) || (dataSize == imageSize)))
            return;
    }
    
    if (_ownData)
    {
        delete [] _data;
        _data = NULL;
        _ownData = false;
    }

    if (dataSize)
    {
        if (copyData)
        {
            if (data)
            {
                _data = new UInt8[dataSize];
                memcpy(_data, data, dataSize);
            }
            _dataSize = dataSize;
            _ownData = true;
        }
        else
        {
            _dataSize = dataSize;
            _data = data;
            _ownData = transferData;
        }
    }
}

void Image::SetSubData(UInt32 xoffset, UInt32 yoffset, UInt32 width, UInt32 height, UInt8 *data)
{
    if (!data) return;
    assert(_ownData);

    const UInt32 imageSize = GetImageDataSize(_width, _height, _format, _type);
    PAT_DEBUG_ASSERT((_dataSize == 0 && _data == NULL) || (_dataSize == imageSize), "Incomplete image\n");

    if (_data == NULL)
    {
        _data = new UInt8[imageSize];
        _ownData = true;
    }

    const size_t pixelSize = GetImagePixelSize(_format, _type);
    const UInt8 *src = (const UInt8*)data;
    for (UInt32 j = 0; j < height; ++j)
    {
        for (UInt32 i = 0; i < width; ++i)
        {
            memcpy(_data + (_width * (j + yoffset) + (i + xoffset)) * pixelSize, src + (width * j + i) * pixelSize, pixelSize);
        }
    }
}

void Image::Set(UInt32 w, UInt32 h, UInt32 format, UInt32 type, UInt32 ds, UInt8 *data, bool copyData, bool transferData)
{
    _width = w;
    _height = h;
    SetFormatType(format, type);
    SetData(ds, data, copyData, transferData);
}

bool BGRAToRGBA(Image &input, Image &output)
{
    const UInt32 format = input.Format();
    const UInt32 type = input.Type();
    if (format != GL_BGRA_EXT && type != GL_UNSIGNED_BYTE)
    {
        PAT_DEBUG_LOG("Unexpected format-type pair : 0x%X 0x%X\n", format, type);
        return false;
    }

    UInt8 *newData = new UInt8[input.DataSize()];
    const UInt32 width = input.Width();
    const UInt32 height = input.Height();
    const UInt8 *oldData = input.Data();
    UInt8 *p = newData;
    for (UInt32 j = 0; j < height; j++)
    {
        for (UInt32 i = 0; i < width; i++)
        {
            p[0] = oldData[2];
            p[1] = oldData[1];
            p[2] = oldData[0];
            p[3] = oldData[3];
            oldData += 4; 
            p += 4;
        }
    }

    output.Set(width, height, GL_RGBA, GL_UNSIGNED_BYTE, input.DataSize(), newData, false, true);
    return true;
}

bool GenerateNextMipmapLevel(const Image &input, Image &output)
{
    if (input.Data() == NULL || IsImageCompression(input.Format()))
        return false;

    const UInt32 format = input.Format();
    const UInt32 type = input.Type();
    if ((format != GL_ALPHA && format != GL_LUMINANCE &&
         format != GL_LUMINANCE && format != GL_RGB &&
         format != GL_RGBA && format != GL_BGRA_EXT) ||
        (type != GL_UNSIGNED_BYTE))
    {
        PAT_DEBUG_ASSERT(0, "Unexpected format & type in Mipmap generation : %d %d\n", format, type);
        return false;
    }

    //const UInt32 channelCount = GetImageChannelCount(format);
    //const UInt32 channelTypeSize = GetImageTypeSize(type);
    const UInt32 width = input.Width();
    const UInt32 height = input.Height();

    for (UInt32 i = 0; i * 2 < width; ++i)
    {
        for (UInt32 j = 0; j * 2 < height; ++j)
        {
            //UInt8 c = 0; // how many texels in this 2x2 block
            //if (
            
        }
    }

    return true;
}

bool ConvertToRGB8(const Image &input, Image &output)
{
    const UInt32 width = input.Width();
    const UInt32 height = input.Height();
    const UInt32 format = input.Format();
    const UInt32 type = input.Type();
    const UInt32 outputSize = width * height * 3;

    const UInt8 *ip = input.Data();
    if (!ip) return false;
    UInt8 *outputData = new UInt8[outputSize];
    PAT_DEBUG_ASSERT_NEW(outputData);
    if (!outputData) return false;
    UInt8 *op = outputData;
    
    if (format == GL_LUMINANCE && type == GL_UNSIGNED_BYTE)
    {
        for (UInt32 j = 0; j < height; ++j)
        {
            for (UInt32 i = 0; i < width; ++i)
            {
                *(op++) = *ip;
                *(op++) = *ip;
                *(op++) = *ip;
                ip++;
            }
        }
        output.Set(width, height, GL_RGB, GL_UNSIGNED_BYTE, outputSize, outputData, false, true);
        return true;
    }
    else if (format == GL_RGB && type == GL_UNSIGNED_BYTE)
    {
        memcpy(outputData, input.Data(), outputSize);
        output.Set(width, height, GL_RGB, GL_UNSIGNED_BYTE, outputSize, outputData, false, true);
        return true;
    }
    else
    {
        delete []outputData;
        return false;
    }
}

} /* namespace pat */
