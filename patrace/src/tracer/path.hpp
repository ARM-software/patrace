#if !defined(_PATH_HPP_)
#define _PATH_HPP_

#include "common/os_string.hpp"

class Path
{
public:
    Path();
    const os::String& getTraceFilePath();
    const os::String& getMetaFilePath();
    const os::String& getStateLogPath();

private:
    os::String generateTraceFilePath();
    os::String generateMetaFilePath();
    os::String generateStateLogPath();
    os::String getTraceFilePathPattern();
    os::String getTraceFilePathPatternAndroid();
    os::String getTraceFilePathPatternDesktop();
    os::String getAndroidPath();
    os::String getDesktopPath();
    os::String m_traceFilePath;

    os::String m_metaFilePath;
    os::String m_stateLogPath;
};

#endif // !defined(_PATH_HPP_)
