/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2017 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#include "interface.hpp"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <jsoncpp/json/writer.h>
#include <jsoncpp/json/value.h>


#ifndef DEBUG
// otherwise we will get complaints about assert()ed variables being unused
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif


#ifdef ANDROID

#include <android/log.h>

#ifndef LOGI
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "burrow", __VA_ARGS__);
#endif

#ifndef LOGE
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "burrow", __VA_ARGS__);
#endif

#ifndef LOGD
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "burrow", __VA_ARGS__);
#endif

#else

#ifndef LOGI
#define LOGI(...) do { fprintf( stdout, __VA_ARGS__); } while(0)
#endif

#ifndef LOGE
#define LOGE(...) do { fprintf( stderr, __VA_ARGS__ ); } while(0)
#endif

#ifndef LOGD
#define LOGD(...) do { \
                         fprintf( stderr, "burrow %s:%d: ", __FILE__, __LINE__ ); \
                         fprintf( stderr, __VA_ARGS__ ); \
                     } while(0)
#endif
#endif


static bool monitor( const int maxCpu, std::string const& process,
                     std::string const& outDir, int timeout )
{
    bool success = false;

    Json::Value ferret;
    Json::Value cpus;

    for( int cpu = 0; cpu <= maxCpu; ++cpu )
    {
        cpus.append( cpu );
    }
    ferret["cpus"] = cpus;

    Json::Value process_names;
    process_names.append( process );
    ferret["process_names"] = process_names;

    /* We use a pipe to obtain "live" status information from Ferret.
     */
    int fds[2];

    if( pipe( fds ) < 0 )
    {
        perror( "pipe" );
        exit( 1 );
    }

    /* We cannot function without the Ferret collector.  The collector writes to the pipe
     * and we read the status (see later).
     */
    ferret["required"] = true;
    ferret["status_fd"] = fds[1];

    ferret["output_dir"] = outDir;
    ferret["poll_timeout"] = timeout;

    Json::Value cfg;
    cfg["ferret"] = ferret;

    Collection c( cfg );

    success = c.initialize();

    if( success )
    {
        bool finished = false;
        int status, last_status = -99;

        c.start();

        while( !finished )
        {
            /* Read integer status values from the collector.
             *
             * -1 indicates a timeout or other error, yielding no data.
             *  1 indicates that data capture is progressing.
             *  0 indicates that the named process has exited.
             */
            int recvd = read( fds[0], &status, sizeof(status) );

            assert( recvd == sizeof(status) );

            if( status != last_status )
            {
                last_status = status;

                if( status == -1 )
                {
                    LOGE( "ERROR: timed out waiting for '%s'\n", process.c_str() );
                    finished = true;
                    success = false;
                }
                else if( status == 0 )
                {
                    LOGI( "INFO: process '%s' has finished.\n", process.c_str() );
                    finished = true;
                }
                else
                {
                    LOGI( "INFO: Process '%s' found, logging in progress.\n", process.c_str() );
                }
            }
        }
        c.stop();
    }

    close( fds[0] );
    close( fds[1] );

    if( success )
    {
        /* Dump the JSON formatted results for standardised result parsing.
         */
        Json::Value results = c.results();
        Json::StyledWriter writer;
        std::string data = writer.write( results );

        LOGI( "Results:\n%s", data.c_str() );
    }
    return success;
}


void usage( int status )
{
    static const char message[] =
        "usage: burrow [-h] -c <max CPU number> PROGRAM\n\n"
        "Burrow (+Ferret) measures the True CPU Load of PROGRAM.\n"
        "\n"
        "The burrow tool runs on a Linux/Android target, extracting "
        "data for subsequent analysis by Ferret.\n"
        "\n"
        "This is achieved by polling the /proc and /sys filesystems to "
        "determine the CPU utilisation, the\n"
        "CPU occupancy of each thread in PROGRAM, and the CPU frequency of all monitored CPUs.\n"
        "Specifically, the "
        "/sys/devices/system/cpu/cpu[cpu_nr]/cpufreq/scaling_cur_freq and the\n"
        "/proc/[pid]/task/*/stat files are polled at a rate of twice _SC_CLK_TCK and "
        "recorded in file \nferret_run_0.data.\n"
        "\n"
        "    -h  Display this message\n"
        "    -c  Maximum CPU number to monitor.\n"
        "    -o  Output file path (Linux default '.', required for Android).\n"
        "    -t  Maximum time (integer seconds, default 30) to wait for PROGRAM to appear.\n"
        "\n";

    LOGE( message );
    exit( status );
}


int main( int argc, char **argv )
{
    extern char *optarg;
    extern int optind;
    int c;

    int maxCpu = -1;
    int timeout = 30;
    std::string outputPath;

    while( ( c = getopt(argc, argv, "hc:o:t:") ) != -1 )
    {
        switch( c )
        {
            case 'h':
                usage( 0 );
                break;
            case 'c':
                maxCpu = (int) atoi( optarg );
                break;
            case 'o':
                outputPath = optarg;
                break;
            case 't':
                timeout = (int) atoi( optarg );
                break;
            default:
                usage( 2 );
                break;
        }
    }

#ifdef ANDROID
    if( outputPath.size() == 0 )
    {
        LOGE( "ERROR: an output path is required.\n" );
        usage( 2 );
    }
#endif

    if( timeout < 1 )
    {
        LOGE( "ERROR: positive, non-zero timeouts are required.\n" );
        usage( 2 );
    }

    if( maxCpu < 0 )
    {
        LOGE( "ERROR: maximum CPU number is required.\n" );
        usage( 2 );
    }

    if( optind >= argc )
    {
        LOGE( "ERROR: missing executable\n" );
        usage( 2 );
    }

    if( argc - optind > 1 )
    {
        LOGE( "ERROR: too many executables\n" );
        usage( 2 );
    }

    std::string processName = argv[optind];

    bool success = monitor( maxCpu, processName, outputPath, timeout );

    return success ? 0 : 1;
}
