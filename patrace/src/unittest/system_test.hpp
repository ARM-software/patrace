#ifndef _INCLUDE_SYSTEM_TEST_
#define _INCLUDE_SYSTEM_TEST_

#include <cppunit/extensions/HelperMacros.h>

class SystemTest : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE(SystemTest);

    CPPUNIT_TEST(testPathBase);
    CPPUNIT_TEST(testDirectoryRecursion);
    CPPUNIT_TEST(testLookForInPath);

	CPPUNIT_TEST_SUITE_END();

public:
    SystemTest();

    virtual void setUp();
    virtual void tearDown();

    void testPathBase();
    void testDirectoryRecursion();
    void testLookForInPath();
};

#endif
