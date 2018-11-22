#ifndef _INCLUDE_CONTEXT_TEST
#define _INCLUDE_CONTEXT_TEST_

#include <cppunit/extensions/HelperMacros.h>

class ContextTest : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE(ContextTest);

    //CPPUNIT_TEST(testRenderbufferObject);
    //CPPUNIT_TEST(testVBO);

    CPPUNIT_TEST(testEnumString);
    CPPUNIT_TEST(testInitialState);
    CPPUNIT_TEST(testShader);
    CPPUNIT_TEST(testTexturObject);
    CPPUNIT_TEST(testFBO);

	CPPUNIT_TEST_SUITE_END();

public:
    ContextTest();

    virtual void setUp();
    virtual void tearDown();

    void testEnumString();
    void testInitialState();
    void testShader();
    void testTexturObject();
    void testFBO();

    //void testRenderbufferObject();
    //void testVBO();
};

#endif
