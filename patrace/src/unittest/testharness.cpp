#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include <iostream>

int main(int argc, char* argv[])
{
    bool wasSuccessful = true;

    std::cerr  << "Running all unit tests:" << std::endl;
    CPPUNIT_NS::TextUi::TestRunner runner;
    runner.addTest(CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest());
    wasSuccessful = runner.run();

    return wasSuccessful ? 0 : 1;
}
