#include "path.hpp"

Path::Path()
 : m_traceFilePath(generateTraceFilePath())
 , m_metaFilePath(generateMetaFilePath())
 , m_stateLogPath(generateStateLogPath())
{
}

const os::String& Path::getTraceFilePath()
{
    return m_traceFilePath;
}

const os::String& Path::getMetaFilePath()
{
    return m_metaFilePath;
}

const os::String& Path::getStateLogPath()
{
    return m_stateLogPath;
}

os::String Path::generateTraceFilePath()
{
    os::String pathPattern = getTraceFilePathPattern();

    // If %u does not exist in pattern, then add it.
    if (!strstr(pathPattern.str(), "%u"))
    {
        pathPattern.append(".%u.pat");
    }

    os::String retPath;

    for (int counter = 1; ; ++counter)
    {
        retPath = os::String::format(pathPattern.str(), counter);

        if (!retPath.exists())
        {
            break;
        }
    }

    return retPath;
}

os::String Path::generateMetaFilePath()
{
    return os::String::format("%s.meta", m_traceFilePath.str());
}

os::String Path::getTraceFilePathPattern()
{
#ifdef ANDROID
   return getTraceFilePathPatternAndroid();
#else
   return getTraceFilePathPatternDesktop();
#endif
}

os::String Path::generateStateLogPath()
{
#ifdef ANDROID
    os::String path = getAndroidPath();
#else
    os::String path = getDesktopPath();
#endif

    const char* envVar = getenv("DEBUG_LOG");
    // If environment variable is missing, then use hardcoded file name
    const char* fileName = envVar ? envVar : "debug.log";

    path.append(fileName);
    return path;
}

os::String Path::getTraceFilePathPatternAndroid()
{
    os::String process = os::getProcessName();
    return os::String::format("/data/apitrace/%s/%s.%%u.pat", process.str(), process.str());
}

os::String Path::getTraceFilePathPatternDesktop()
{
    const char* envVar  = getenv("OUT_TRACE_FILE");
    // If environment variable is missing, then use hardcoded tracefile name
    return envVar ? envVar : "./trace.%u.pat";
}

os::String Path::getAndroidPath()
{
    os::String process = os::getProcessName();
    return os::String::format("/data/apitrace/%s/", process.str());
}

os::String Path::getDesktopPath()
{
    return "./";
}

