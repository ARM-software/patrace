#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "image_test.hpp"
#include "eglstate/common.hpp"
#include "system/environment_variable.hpp"
#include "image/image_compression.hpp"
#include "image/image.hpp"
#include "image/image_io.hpp"

using namespace pat;

ImageTest::ImageTest()
{
}

void ImageTest::setUp()
{
}

void ImageTest::tearDown()
{
}

void ImageTest::testBasic()
{
    CPPUNIT_ASSERT(WithAlphaChannel(GL_ALPHA) == true);
    CPPUNIT_ASSERT(WithAlphaChannel(GL_RGBA) == true);
    CPPUNIT_ASSERT(WithAlphaChannel(GL_BGRA_EXT) == true);
    CPPUNIT_ASSERT(WithAlphaChannel(GL_RGB) == false);
    CPPUNIT_ASSERT(WithAlphaChannel(GL_LUMINANCE_ALPHA) == true);
    CPPUNIT_ASSERT(WithAlphaChannel(GL_LUMINANCE) == false);

    CPPUNIT_ASSERT(CanCompressAs(GL_BGRA_EXT, GL_UNSIGNED_BYTE, "ETC2_A1") == false);
    CPPUNIT_ASSERT(CanCompressAs(GL_RGBA, GL_UNSIGNED_BYTE, "ETC2_A1") == true);
    CPPUNIT_ASSERT(CanCompressAs(GL_RGBA, GL_UNSIGNED_BYTE, "ETC2_A6") == false);
}

void ImageTest::testLookForInPath()
{
    std::string path;
    //CPPUNIT_ASSERT(SupportETC1Compression() == EnvironmentVariable::SearchUnderSystemPath(ETC_COMPRESSION_TOOL, path));
    CPPUNIT_ASSERT(SupportETC1Uncompression());

    //CPPUNIT_ASSERT(SupportASTCCompression() == EnvironmentVariable::SearchUnderSystemPath(ASTC_COMPRESSION_TOOL, path));
    //CPPUNIT_ASSERT(SupportASTCUncompression() == EnvironmentVariable::SearchUnderSystemPath(ASTC_COMPRESSION_TOOL, path));

    CPPUNIT_ASSERT(CheckCompressionOptionSupport("ETC1") == SupportETC1Compression());
    CPPUNIT_ASSERT(CheckCompressionOptionSupport("ASTC12x12") == SupportASTCCompression());
}

void ImageTest::testCompressionCommon()
{
    const char **optionList = NULL;
    UInt32 optionNum = 0;
    CPPUNIT_ASSERT(GetCompressionOptionList(optionList, optionNum));
    CPPUNIT_ASSERT(optionNum);

    CPPUNIT_ASSERT(IsValidCompressionOption("ASTC6x6") == true);
    CPPUNIT_ASSERT(IsValidCompressionOption("ASTC6x4") == false);
}

void ImageTest::testBTC()
{
    std::shared_ptr<ImageCompressionFormat> btc(new BTCCompressionFormat);
    CPPUNIT_ASSERT(btc->SupportCompression() == true);
    CPPUNIT_ASSERT(btc->SupportUncompression() == true);
    CPPUNIT_ASSERT(btc->IsCompressed(GL_BTC_LUMINANCE_PAT) == true);
    CPPUNIT_ASSERT(btc->IsCompressed(GL_ETC1_RGB8_OES) == false);
    CPPUNIT_ASSERT(btc->CanCompress(GL_LUMINANCE, GL_UNSIGNED_BYTE) == true);
    CPPUNIT_ASSERT(btc->CanCompress(GL_ALPHA, GL_UNSIGNED_BYTE) == false);

    UInt8 input_data[] = {
        245, 239, 249, 239,
        245, 245, 239, 235,
        245, 245, 245, 245,
        245, 235, 235, 239,
    };
    UInt8 rgb8_data[] = {
        245, 245, 245, 239, 239, 239, 249, 249, 249, 239, 239, 239,
        245, 245, 245, 245, 245, 245, 239, 239, 239, 235, 235, 235,
        245, 245, 245, 245, 245, 245, 245, 245, 245, 245, 245, 245,
        245, 245, 245, 235, 235, 235, 235, 235, 235, 239, 239, 239,
    };
    UInt8 compressed_data[] = {
        0xF1, 0x04, 0xAC, 0xF8,
    };
    UInt8 uncompressed_data[] = {
        244, 236, 244, 236,
        244, 244, 236, 236,
        244, 244, 244, 244,
        244, 236, 236, 236,
    };
    Image input(4, 4, GL_LUMINANCE, GL_UNSIGNED_BYTE, 16, input_data, false, false);
    Image compressed, uncompressed, rgb8;

    CPPUNIT_ASSERT(ConvertToRGB8(input, rgb8));
    CPPUNIT_ASSERT(rgb8.Width() == 4);
    CPPUNIT_ASSERT(rgb8.Height() == 4);
    CPPUNIT_ASSERT(rgb8.Format() == GL_RGB);
    CPPUNIT_ASSERT(rgb8.Type() == GL_UNSIGNED_BYTE);
    CPPUNIT_ASSERT(rgb8.DataSize() == sizeof(rgb8_data));
    CPPUNIT_ASSERT(memcmp(rgb8.Data(), rgb8_data, rgb8.DataSize()) == 0);

    CPPUNIT_ASSERT(btc->Compress(input, compressed));
    CPPUNIT_ASSERT(compressed.Width() == 4);
    CPPUNIT_ASSERT(compressed.Height() == 4);
    CPPUNIT_ASSERT(compressed.Format() == GL_BTC_LUMINANCE_PAT);
    CPPUNIT_ASSERT(compressed.Type() == GL_NONE);
    CPPUNIT_ASSERT(compressed.DataSize() == sizeof(compressed_data));
    CPPUNIT_ASSERT(memcmp(compressed.Data(), compressed_data, compressed.DataSize()) == 0);

    CPPUNIT_ASSERT(btc->Uncompress(compressed, uncompressed));
    CPPUNIT_ASSERT(uncompressed.Width() == 4);
    CPPUNIT_ASSERT(uncompressed.Height() == 4);
    CPPUNIT_ASSERT(uncompressed.Format() == GL_LUMINANCE);
    CPPUNIT_ASSERT(uncompressed.Type() == GL_UNSIGNED_BYTE);
    CPPUNIT_ASSERT(uncompressed.DataSize() == sizeof(uncompressed_data));
    CPPUNIT_ASSERT(memcmp(uncompressed.Data(), uncompressed_data, uncompressed.DataSize()) == 0);

    CPPUNIT_ASSERT(btc->CompressUncompress(input, uncompressed));
    CPPUNIT_ASSERT(uncompressed.Width() == 4);
    CPPUNIT_ASSERT(uncompressed.Height() == 4);
    CPPUNIT_ASSERT(uncompressed.Format() == GL_LUMINANCE);
    CPPUNIT_ASSERT(uncompressed.Type() == GL_UNSIGNED_BYTE);
    CPPUNIT_ASSERT(uncompressed.DataSize() == sizeof(uncompressed_data));
    CPPUNIT_ASSERT(memcmp(uncompressed.Data(), uncompressed_data, uncompressed.DataSize()) == 0);
}

void ImageTest::testETC1()
{
    Image input(2, 2, GL_ETC1_RGB8_OES, GL_NONE, 0, NULL);
    Image output;
    CPPUNIT_ASSERT(Uncompress(input, output));
    CPPUNIT_ASSERT(output.Width() == 2);
    CPPUNIT_ASSERT(output.Height() == 2);
    CPPUNIT_ASSERT(output.Format() == GL_RGB);
    CPPUNIT_ASSERT(output.Type() == GL_UNSIGNED_BYTE);
    CPPUNIT_ASSERT(output.DataSize() == 0);
    CPPUNIT_ASSERT(output.Data() == NULL);

    UInt8 raw_data[] = {
        0x02, 0x04, 0x06, 0x03, 0x05, 0x07,
        0x04, 0x06, 0x08, 0x05, 0x07, 0x09,
        0x04, 0x06, 0x08, 0x05, 0x07, 0x09,
        0x02, 0x04, 0x06, 0x03, 0x05, 0x07
    };
    const UInt8 comp_data[] = {
        0x00, 0x00, 0x00, 0x26,
        0x00, 0x00, 0x00, 0x00
    };
    input.Set(4, 2, GL_RGB, GL_UNSIGNED_BYTE, sizeof(raw_data), raw_data, false, false);
    WritePNM(input, "test.ppm", false);
    CPPUNIT_ASSERT(Compress(input, output, "ETC1"));
    CPPUNIT_ASSERT(output.Width() == 4);
    CPPUNIT_ASSERT(output.Height() == 2);
    CPPUNIT_ASSERT(output.Format() == GL_ETC1_RGB8_OES);
    CPPUNIT_ASSERT(output.Type() == GL_NONE);
    CPPUNIT_ASSERT(output.DataSize() == 8);
    CPPUNIT_ASSERT(memcmp(output.Data(), comp_data, output.DataSize()) == 0);

    CPPUNIT_ASSERT(Uncompress(output, input));
    CPPUNIT_ASSERT(input.Width() == 4);
    CPPUNIT_ASSERT(input.Height() == 2);
    CPPUNIT_ASSERT(input.Format() == GL_RGB);
    CPPUNIT_ASSERT(input.Type() == GL_UNSIGNED_BYTE);
    CPPUNIT_ASSERT(input.DataSize() == 24);
}

void ImageTest::testETC2()
{
    CPPUNIT_ASSERT(IsValidCompressionOption("ETC2_A1"));
    CPPUNIT_ASSERT(IsValidCompressionOption("ETC2_A8"));

    CPPUNIT_ASSERT(IsETC2Compression(GL_ETC1_RGB8_OES) == false);
    CPPUNIT_ASSERT(IsETC2Compression(GL_COMPRESSED_RGBA8_ETC2_EAC));
    CPPUNIT_ASSERT(IsImageCompression(GL_COMPRESSED_RGBA8_ETC2_EAC));
    CPPUNIT_ASSERT(CanCompressAsETC2(GL_RGB, GL_UNSIGNED_BYTE) == true);
    CPPUNIT_ASSERT(CanCompressAsETC2(GL_RGBA, GL_UNSIGNED_BYTE) == true);
    CPPUNIT_ASSERT(CanCompressAsETC2(GL_BGRA_EXT, GL_UNSIGNED_BYTE) == false);

    // unset environment path
    std::string environ_value;
    CPPUNIT_ASSERT(EnvironmentVariable::GetVariableValue(EnvironmentVariable::ENVIRONMENT_VARIABLE_NAME_PATH, environ_value));
    CPPUNIT_ASSERT(EnvironmentVariable::SetVariableValue(EnvironmentVariable::ENVIRONMENT_VARIABLE_NAME_PATH, ""));
    CPPUNIT_ASSERT(SupportETC2Compression() == SupportETC2Uncompression());
    std::string path;
    //CPPUNIT_ASSERT(SupportETC2Compression() == EnvironmentVariable::SearchUnderSystemPath(ETC_COMPRESSION_TOOL, path));
    CPPUNIT_ASSERT(CheckCompressionOptionSupport("ETC2_A1") == CheckCompressionOptionSupport("ETC2_A8"));
    CPPUNIT_ASSERT(CheckCompressionOptionSupport("ETC2_A1") == SupportETC2Compression());
    // reset environment path
    CPPUNIT_ASSERT(EnvironmentVariable::SetVariableValue(EnvironmentVariable::ENVIRONMENT_VARIABLE_NAME_PATH, environ_value));

    UInt8 raw_data[] = {
        0x02, 0x04, 0x06, 0x03, 0x05, 0x07,
        0x04, 0x06, 0x08, 0x05, 0x07, 0x09,
        0x04, 0x06, 0x08, 0x05, 0x07, 0x09,
        0x02, 0x04, 0x06, 0x03, 0x05, 0x07
    };
    //const UInt8 comp_data[] = {
        //0x00, 0x00, 0x00, 0x26,
        //0x00, 0x00, 0x00, 0x00
    //};

    {
    //Image input(6, 2, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, sizeof(raw_data), raw_data, false, false);
    //Image output;
    //CPPUNIT_ASSERT(CompressAsETC2(input, output, 1) == false);
    }

    {
    Image input(4, 2, GL_RGB, GL_UNSIGNED_BYTE, sizeof(raw_data), NULL, false, false);
    Image output;
    CPPUNIT_ASSERT(CompressAsETC2(input, output, 8));
    CPPUNIT_ASSERT(output.Width() == 4);
    CPPUNIT_ASSERT(output.Height() == 2);
    CPPUNIT_ASSERT(output.Format() == GL_COMPRESSED_RGB8_ETC2);
    CPPUNIT_ASSERT(output.Type() == GL_NONE);
    CPPUNIT_ASSERT(output.DataSize() == 16 || output.DataSize() == 0);
    CPPUNIT_ASSERT(output.Data() == NULL);
    }

    {
    Image input(4, 2, GL_RGB, GL_UNSIGNED_BYTE, sizeof(raw_data), raw_data, false, false);
    Image output;
    CPPUNIT_ASSERT(Compress(input, output, "ETC2_A1"));
    CPPUNIT_ASSERT(output.Width() == 4);
    CPPUNIT_ASSERT(output.Height() == 2);
    CPPUNIT_ASSERT(output.Format() == GL_COMPRESSED_RGB8_ETC2);
    CPPUNIT_ASSERT(output.Type() == GL_NONE);
    }

    {
    Image input(3, 2, GL_RGBA, GL_UNSIGNED_BYTE, sizeof(raw_data), raw_data, false, false);
    Image output;
    CPPUNIT_ASSERT(Compress(input, output, "ETC2") == false);
    CPPUNIT_ASSERT(Compress(input, output, "ETC2_A1"));
    CPPUNIT_ASSERT(output.Width() == 3);
    CPPUNIT_ASSERT(output.Height() == 2);
    CPPUNIT_ASSERT(output.Format() == GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2);
    CPPUNIT_ASSERT(output.Type() == GL_NONE);
    }

    {
    Image input(6, 1, GL_RGBA, GL_UNSIGNED_BYTE, sizeof(raw_data), raw_data, false, false);
    Image output;
    CPPUNIT_ASSERT(CompressAsETC2(input, output, 5) == false);
    CPPUNIT_ASSERT(Compress(input, output, "ETC2_A8"));
    CPPUNIT_ASSERT(output.Width() == 6);
    CPPUNIT_ASSERT(output.Height() == 1);
    CPPUNIT_ASSERT(output.Format() == GL_COMPRESSED_RGBA8_ETC2_EAC);
    CPPUNIT_ASSERT(output.Type() == GL_NONE);
    }
    //CPPUNIT_ASSERT(output.DataSize() == 8);
    //CPPUNIT_ASSERT(memcmp(output.Data(), comp_data, output.DataSize()) == 0);

    //CPPUNIT_ASSERT(Uncompress(output, input));
    //CPPUNIT_ASSERT(input.Width() == 4);
    //CPPUNIT_ASSERT(input.Height() == 2);
    //CPPUNIT_ASSERT(input.Format() == GL_RGB);
    //CPPUNIT_ASSERT(input.Type() == GL_UNSIGNED_BYTE);
    //CPPUNIT_ASSERT(input.DataSize() == 24);
}

void ImageTest::testASTC()
{
    std::string path_value;
    CPPUNIT_ASSERT(EnvironmentVariable::GetVariableValue("PATH", path_value));
    CPPUNIT_ASSERT(EnvironmentVariable::SetVariableValue("PATH", ""));

    Image input(2, 2, COMPRESSED_ASTC_RGBA_8x5_OES, GL_NONE, 0, NULL);
    Image output;
    CPPUNIT_ASSERT(Uncompress(input, output));
    CPPUNIT_ASSERT(output.Width() == 2);
    CPPUNIT_ASSERT(output.Height() == 2);
    CPPUNIT_ASSERT(output.DataSize() == 0);
    CPPUNIT_ASSERT(output.Data() == NULL);

    CPPUNIT_ASSERT(EnvironmentVariable::SetVariableValue("PATH", path_value));

    unsigned char compressed_data[] = {
        0xFC, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF};

    input.Set(2, 1, COMPRESSED_ASTC_RGBA_4x4_OES, GL_NONE, sizeof(compressed_data), compressed_data, false, false);
    CPPUNIT_ASSERT(Uncompress(input, output));
    CPPUNIT_ASSERT(output.Width() == 2);
    CPPUNIT_ASSERT(output.Height() == 1);
    CPPUNIT_ASSERT(output.DataSize() != 0);
    CPPUNIT_ASSERT(output.Data() != NULL);

    CPPUNIT_ASSERT(CompressAsASTC(output, input, 6, 4) == false);
    CPPUNIT_ASSERT(Compress(output, input, "ASTC6x5"));
    CPPUNIT_ASSERT(input.Width() == 2);
    CPPUNIT_ASSERT(input.Height() == 1);
    CPPUNIT_ASSERT(input.Format() == COMPRESSED_ASTC_RGBA_6x5_OES);
}

void ImageTest::testMipmap()
{
    Image input, output;
    CPPUNIT_ASSERT(GenerateNextMipmapLevel(input, output) == false);

    input.Set(1, 1, GL_ETC1_RGB8_OES, GL_NONE, 4, NULL, false, false);
    CPPUNIT_ASSERT(GenerateNextMipmapLevel(input, output) == false);

    unsigned char base_data[] = {
        0x02, 0x04, 0x06, 0x03, 0x05, 0x07,
        0x04, 0x06, 0x08, 0x05, 0x07, 0x09,
        0x04, 0x06, 0x08, 0x05, 0x07, 0x09,
        0x02, 0x04, 0x06, 0x03, 0x05, 0x07
    };
    //unsigned char first_level_data[] = {
        //0x04, 0x05, 0x08, 0x04, 0x05, 0x08
    //};
    //unsigned char second_level_data[] = {
        //0x04, 0x05, 0x08
    //};
    const Image base_level(4, 2, GL_RGB, GL_UNSIGNED_BYTE, sizeof(base_data), base_data, false, false);
    Image first_level;
    CPPUNIT_ASSERT(GenerateNextMipmapLevel(base_level, first_level));
    //CPPUNIT_ASSERT(first_level.Width() == 2);
    //CPPUNIT_ASSERT(first_level.Height() == 1);
    //CPPUNIT_ASSERT(first_level.Format() == GL_RGB);
    //CPPUNIT_ASSERT(first_level.Type() == GL_UNSIGNED_BYTE);
    //CPPUNIT_ASSERT(memcmp(first_level.Data(), first_level_data, first_level.DataSize()) == 0);
}
