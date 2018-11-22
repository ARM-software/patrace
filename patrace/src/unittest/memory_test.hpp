#ifndef _INCLUDE_MEMORY_TEST_
#define _INCLUDE_MEMORY_TEST_

#include <cppunit/extensions/HelperMacros.h>

class MemoryTest : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE(MemoryTest);

    CPPUNIT_TEST(testMemoryBase);
    CPPUNIT_TEST(testMD5); 
    CPPUNIT_TEST(testDataInitialization);
    CPPUNIT_TEST(testClientSideBufferObjectSet);

	CPPUNIT_TEST_SUITE_END();

public:
    MemoryTest();

    virtual void setUp();
    virtual void tearDown();

    void testMemoryBase();
    void testMD5();
    void testDataInitialization();
    void testClientSideBufferObjectSet();
};

#endif // _INCLUDE_MEMORY_TEST_
