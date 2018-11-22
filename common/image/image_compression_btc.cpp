#include <GLES2/gl2.h>

#include "image.hpp"
#include "image_compression.hpp"

namespace
{

const UInt32 BLOCK_WIDTH = 4;
const UInt32 BLOCK_HEIGHT = 4;
const UInt32 COMPRESSED_BLOCK_SIZE = 4;

}

namespace pat
{

bool BTCCompressionFormat::IsCompressed(UInt32 format) const
{
    return GL_BTC_LUMINANCE_PAT == format;
}

bool BTCCompressionFormat::CanCompress(UInt32 format, UInt32 type) const
{
    return format == GL_LUMINANCE && type == GL_UNSIGNED_BYTE;
}

bool BTCCompressionFormat::Compress(const Image &input, Image &output) const
{
    const UInt32 width = input.Width();
    const UInt32 height = input.Height();
    if (width % BLOCK_WIDTH || height % BLOCK_HEIGHT)
        return false;

    const UInt32 format = input.Format();
    const UInt32 type = input.Type();
    if (!CanCompress(format, type))
        return false;

    const UInt8 wblock = width / BLOCK_WIDTH;
    const UInt8 hblock = height / BLOCK_HEIGHT;
    const UInt8 *input_data = input.Data();
    if (!input_data) return false;
    const UInt32 output_data_size = wblock * hblock * COMPRESSED_BLOCK_SIZE;
    UInt8 *output_data = new UInt8[output_data_size];
    PAT_DEBUG_ASSERT_NEW(output_data);
    if (!output_data) return false;

    const UInt32 pixelsPerBlock = BLOCK_WIDTH * BLOCK_HEIGHT;
    for (UInt32 by = 0; by < hblock; ++by)
    {
        for (UInt32 bx = 0; bx < wblock; ++bx)
        {
            Float32 sum = 0;
            Float32 squareSum = 0;
            for (UInt32 y = 0; y < BLOCK_HEIGHT; ++y)
            {
                for (UInt32 x = 0; x < BLOCK_WIDTH; ++x)
                {
                    const UInt8 g = input_data[(by * BLOCK_HEIGHT + y) * width + (bx * BLOCK_WIDTH + x)];
                    sum += g;
                    squareSum += g * g;
                }
            }
            const Float32 mean = sum / pixelsPerBlock;
            const Float32 standardDeviation = std::sqrt(squareSum / pixelsPerBlock - mean * mean);

            UInt8 *p = output_data + COMPRESSED_BLOCK_SIZE * (wblock * by + bx);
            p[0] = static_cast<UInt8>(mean);
            p[1] = static_cast<UInt8>(standardDeviation);
            p[2] = p[3] = 0;

            for (UInt32 y = 0; y < BLOCK_HEIGHT/2; ++y)
            {
                for (UInt32 x = 0; x < BLOCK_WIDTH; ++x)
                {
                    const UInt32 i = y * BLOCK_WIDTH + x;
                    const UInt8 g = input_data[(by * BLOCK_HEIGHT + y) * width + (bx * BLOCK_WIDTH + x)];
                    if (g >= mean)
                        p[2] |= 1 << (7 - i);
                }
            }
            for (UInt32 y = BLOCK_HEIGHT/2; y < BLOCK_HEIGHT; ++y)
            {
                for (UInt32 x = 0; x < BLOCK_WIDTH; ++x)
                {
                    const UInt32 i = (y - BLOCK_HEIGHT/2) * BLOCK_WIDTH + x;
                    const UInt8 g = input_data[(by * BLOCK_HEIGHT + y) * width + (bx * BLOCK_WIDTH + x)];
                    if (g >= mean)
                        p[3] |= 1 << (7 - i);
                }
            }
        }
    }
    output.Set(width, height, GL_BTC_LUMINANCE_PAT, GL_NONE, output_data_size, output_data, false, true);

    return true;
}

bool BTCCompressionFormat::Uncompress(const Image &input, Image &output) const
{
    const UInt32 width = input.Width();
    const UInt32 height = input.Height();
    if (width % BLOCK_WIDTH || height % BLOCK_HEIGHT)
        return false;

    const UInt32 format = input.Format();
    if (!IsCompressed(format))
        return false;

    const UInt8 wblock = width / BLOCK_WIDTH;
    const UInt8 hblock = height / BLOCK_HEIGHT;
    const UInt8 *input_data = input.Data();
    if (!input_data)
        return false;
    const UInt32 output_data_size = width * height;
    UInt8 *output_data = new UInt8[output_data_size];
    PAT_DEBUG_ASSERT_NEW(output_data);
    if (!output_data)
        return false;

    const UInt32 pixelsPerBlock = BLOCK_WIDTH * BLOCK_HEIGHT;
    for (UInt32 by = 0; by < hblock; ++by)
    {
        for (UInt32 bx = 0; bx < wblock; ++bx)
        {
            const UInt8 *ip = input_data + COMPRESSED_BLOCK_SIZE * (by * wblock + bx);
            UInt8 *op = output_data + pixelsPerBlock * (by * wblock + bx);
            const Float32 mean = ip[0];
            const Float32 standardDeviation = ip[1];
            Float32 bigThanMean = 0;
            for (SInt32 i = 7; i >= 0; --i)
            {
                if (ip[2] & (1 << i))
                    bigThanMean += 1;
            }
            for (SInt32 i = 7; i >= 0; --i)
            {
                if (ip[3] & (1 << i))
                    bigThanMean += 1;
            }

            const UInt8 a = static_cast<UInt8>(mean - standardDeviation * std::sqrt(bigThanMean / (pixelsPerBlock - bigThanMean)));
            const UInt8 b = static_cast<UInt8>(mean + standardDeviation * std::sqrt((pixelsPerBlock - bigThanMean) / bigThanMean));

            for (SInt32 i = 7; i >= 0; --i)
            {
                if (ip[2] & (1 << i))
                    *(op++) = b;
                else
                    *(op++) = a;
            }
            for (SInt32 i = 7; i >= 0; --i)
            {
                if (ip[3] & (1 << i))
                    *(op++) = b;
                else
                    *(op++) = a;
            }
        }
    }
    output.Set(width, height, GL_LUMINANCE, GL_UNSIGNED_BYTE, output_data_size, output_data, false, true);

    return true;
}

}
