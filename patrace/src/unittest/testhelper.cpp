#include <cppunit/config/SourcePrefix.h>

#include "memory_test.hpp"
#include "context_test.hpp"
#include "system_test.hpp"
#include "image_test.hpp"

#define TEST(name) \
/* Registers the fixture into the "all tests" registry */ \
/* Cannot use CPPUNIT_TEST_SUITE_REGISTRATION due to name collision with */ \
/* Variable defined by CPPUNIT_TEST_SUITE_REGISTRATION */ \
static CPPUNIT_NS::AutoRegisterSuite<name> autoregister##name;\
/* Registers the fixture into a registry containing this test suite only */ \
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(name, #name);

TEST(MemoryTest)
TEST(ContextTest)
TEST(SystemTest)
TEST(ImageTest)
