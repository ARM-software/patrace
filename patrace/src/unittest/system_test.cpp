#include <GLES2/gl2.h>

#include "system_test.hpp"
#include "system/path.hpp"
#include "system/environment_variable.hpp"

using namespace pat;

SystemTest::SystemTest()
{
}

void SystemTest::setUp()
{
}

void SystemTest::tearDown()
{
}

void SystemTest::testLookForInPath()
{
    std::string path_value;
    CPPUNIT_ASSERT(EnvironmentVariable::GetVariableValue("PATH", path_value));

    std::vector<std::string> paths;
    CPPUNIT_ASSERT(EnvironmentVariable::GetVariableValue("PATH", paths));
    CPPUNIT_ASSERT(paths.size() > 0);

    std::string man_abspath;
    CPPUNIT_ASSERT(EnvironmentVariable::SearchUnderSystemPath("man", man_abspath));
    CPPUNIT_ASSERT(man_abspath == "/usr/bin/man");

    //CPPUNIT_ASSERT(manager->FindShaderObject(1, 2, 100) == NULL);
    //CPPUNIT_ASSERT(manager->FindShaderObject(1, 2, 150) != NULL);
    //CPPUNIT_ASSERT(manager->FindShaderObject(1, 2, 150)->source == "test content 1");
    //CPPUNIT_ASSERT(manager->FindShaderObject(1, 1, 150) == NULL);
    //CPPUNIT_ASSERT(manager->FindShaderObject(0, 2, 150) == NULL);

    //System::Instance()->DumpShaderObjects();
}

void SystemTest::testPathBase()
{
#ifdef WIN32
#else
    CPPUNIT_ASSERT(Path::Sep == '/');

    const std::string existsFilepath("/usr/bin/ldd");
    const std::string nonExistsFilepath("/usr/bin/lddd");
    const std::string dirFilepath("/usr/bin/");
#endif
    const Path existsPath(existsFilepath);
    const Path nonExistsPath(nonExistsFilepath);
    CPPUNIT_ASSERT(Path::Exists(existsFilepath) == true);
    CPPUNIT_ASSERT(Path::Exists(nonExistsFilepath) == false);
    CPPUNIT_ASSERT(existsPath.Exists() == true);
    CPPUNIT_ASSERT(nonExistsPath.Exists() == false);

    const Path dirPath(dirFilepath);
    CPPUNIT_ASSERT(Path::IsDirectory(dirFilepath) == true);
    CPPUNIT_ASSERT(Path::IsDirectory(existsFilepath) == false);
    CPPUNIT_ASSERT(dirPath.IsDirectory() == true);
    CPPUNIT_ASSERT(existsPath.IsDirectory() == false);

    const Path path0("/usr"), path1("bin/ldd");
    CPPUNIT_ASSERT(path0 + path1 == Path(std::string("/usr/bin/ldd")));

    CPPUNIT_ASSERT(path0.GetExtension() == "");
    CPPUNIT_ASSERT(Path("ld.so").GetExtension() == "so");
}

void SystemTest::testDirectoryRecursion()
{
#ifdef WIN32
#else
    const std::string base = "/usr/bin";
#endif
    DirectoryIterator di(base);
    di.Reset();
    Path p;
    while (di.Next(p))
    {
    }
}
