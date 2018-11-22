#ifndef _INCLUDE_IMAGE_TEST_
#define _INCLUDE_IMAGE_TEST_

#include <cppunit/extensions/HelperMacros.h>

class ImageTest : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE(ImageTest);

    CPPUNIT_TEST(testBasic);
    CPPUNIT_TEST(testLookForInPath);
    CPPUNIT_TEST(testCompressionCommon);
    CPPUNIT_TEST(testBTC);
    CPPUNIT_TEST(testETC1);
    CPPUNIT_TEST(testETC2);
    CPPUNIT_TEST(testASTC);
    CPPUNIT_TEST(testMipmap);

	CPPUNIT_TEST_SUITE_END();

public:
    ImageTest();

    virtual void setUp();
    virtual void tearDown();

    void testBasic();
    void testLookForInPath();
    void testCompressionCommon();
    void testBTC();
    void testETC1();
    void testETC2();
    void testASTC();
    void testMipmap();

    void testFormatTypeTraits();
    void testGenerateImageViewFromRawPixels();
    void testUncompressedImageConversion();
    void testKTXIO();
    void testKTXDynamicIO();
};

#endif
