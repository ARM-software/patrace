#include <windows.h>

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <string>

#include "os.hpp"
#include "os_string.hpp"


namespace os {

long long timeFrequency = 0LL;

void log(const char *format, ...)
{
    char buf[4096];

    va_list ap;
    va_start(ap, format);
    fflush(stdout);
    vsnprintf(buf, sizeof buf, format, ap);
    va_end(ap);

    OutputDebugStringA(buf);

    /*
     * Also write the message to stderr, when a debugger is not present (to
     * avoid duplicate messages in command line debuggers).
     */
#if _WIN32_WINNT > 0x0400
    if (!IsDebuggerPresent()) {
        fflush(stdout);
        fputs(buf, stderr);
        fflush(stderr);
    }
#endif
}

void abort(void)
{
    ExitProcess(0);
}

std::string getTemporaryFilename(const char* description)
{
    char temp_path[256];
    GetTempPath(256, temp_path);

    char temp_filename[256];
    if (GetTempFileName(temp_path, description, 0, temp_filename) == 0)
    {
        printf("Failed to get a temporary file name with description %s\n", description);
        return "";
    }

    return temp_filename;
}

}