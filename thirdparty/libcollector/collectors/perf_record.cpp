#include "perf_record.hpp"

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <sysexits.h>

bool PerfRecordCollector::init()
{
    return true;
}

bool PerfRecordCollector::deinit()
{
    return true;
}

bool PerfRecordCollector::available()
{
    return true;
}

bool PerfRecordCollector::start()
{
    static int increment = 0;
    const std::string prefix = "perf_record_";
    const std::string extension = ".data";
    const std::string output = prefix + _to_string(increment++) + extension;

    if (mCollecting)
    {
        return true;
    }

    if (mCustomResult.empty())
    {
        mCustomResult = Json::arrayValue;
    }
    mCustomResult.append(output);

    if (mIsThreaded)
    {
        DBG_LOG("perf_record: It makes no sense to run me threaded! Please stop doing that.\n");
    }

    unlink(output.c_str());
    mCollecting = true;
    mPid = getpid();
    mChild = fork();

    if (mChild == 0)
    {
        // We are the child
        //printf("Starting perf record collection (target pid is %d, saving to %s)\n", (int)mPid, output.c_str());
        const std::string mypid = _to_string(mPid);
        //int fd = open("/dev/null", O_RDWR);
        //dup2(fd, 1);
        //dup2(fd, 2);
        const char* const args[] = { "perf", "record", "--freq", "10000", "--pid", mypid.c_str(), "-o", output.c_str(), NULL };
        execvp("perf", (char * const *)args);
        DBG_LOG("perf_record: execvp failed: %s\n", strerror(errno));
        exit(EX_UNAVAILABLE);
        return false;
    }
    else if (mChild > 0)
    {
        usleep(1000000); // let perf start
        return true;
    }
    else
    {
        DBG_LOG("perf_record fork failed: %s\n", strerror(errno));
        return false;
    }
}

bool PerfRecordCollector::stop()
{
    if (!mCollecting)
    {
        return true;
    }
    mCollecting = false;
    if (kill(mChild, SIGINT))
    {
        DBG_LOG("perf_record: Failed to softly kill subprocess - result in doubt\n");
    }
    else
    {
        int status;

        waitpid( mChild, & status, 0 );
        if( WEXITSTATUS( status ) == EX_UNAVAILABLE )
        {
            DBG_LOG("perf_record: exec perf_record failed\n");
        }
    }
    mChild = 0;
    mPid = 0;
    return true;
}

bool PerfRecordCollector::collect(int64_t /* now */)
{
    // actually does nothing
    return true;
}
