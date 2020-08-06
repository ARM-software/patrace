/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2017 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#include "ferret.hpp"

#include <set>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <cassert>
#include <algorithm>

#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

#include <jsoncpp/json/value.h>


/** Fields to capture from /proc/<pid>/task/<tpid>/stat.
 *
 * See "man 5 proc".
 *
 * Note fields must be in ascending order.
 */
const struct stat_fields
{
    const char *label;
    int field_nr;
}
pid_stat_spec[] =
{
    {"pid", 0},
    {"comm", 1},
    {"ppid", 3},
    {"utime", 13},
    {"stime", 14},
    {"cutime", 15},
    {"cstime", 16},
    {"num_threads", 19},
    {"starttime", 21},
    {"processor", 38}
};


void get_cpu_cores( std::vector<int>& cores )
{
    const std::string prefix = "/sys/devices/system/cpu/cpu";
    const std::string suffix = "/cpufreq/scaling_cur_freq";

    for ( size_t i = 0; i < 256; ++i )
    {
        const std::string freqFile = prefix + _to_string( i ) + suffix;
        if ( file_exists( freqFile ) )
        {
            cores.push_back(i);
        }
    }
}

/* Collector JSON args:
 * cpus: List of CPUS to monitor, defaults to all (up to a maximum of 256 cores) if not specified.
 * enable_postprocessing: If this is set to true, ferret will postprocess the results for convenience (giving derived CPU load stats).
 * banned_threads: Any thread names in this list (thread name = comm name) will not be included in the postprocessing, so they will not contribute to the derived CPU stats.
 * output_dir: Directory to store result files in. Must already exist.
 * threaded: Run in threaded mode, default is true.
 * process_names: Names of processes to monitor. Primarily used by burrow.
 */

FerretCollector::FerretCollector( const Json::Value& config,
                                  const std::string& name ) : Collector( config, name )
{
    mClockTicks = sysconf( _SC_CLK_TCK );
    mPid = _to_string(getpid());
    mEnablePostprocess = false;

    /*
     * Absorb configuration.
     *
     * Assumes either constructor arguments OR JSON configuration but not both.
     */

    if( mConfig.isMember( "cpus" ) )
    {
        Json::Value const& cpuArr = mConfig[ "cpus" ];

        if( cpuArr.isArray() )
        {
            Json::Value const& cpuArr = mConfig[ "cpus" ];

            for( unsigned i = 0; i < cpuArr.size() ; ++i )
            {
                mCpus.push_back( cpuArr[i].asInt() );
            }
        }
        else
        {
            DBG_LOG( "%s: JSON member cpus is not an array.\n", mName.c_str() );
        }
    } else {
        if (mDebug) DBG_LOG( "%s: No cpu filter specified, monitoring all cores by default.\n", mName.c_str() );
        get_cpu_cores( mCpus );
    }

    if( mConfig.isMember( "enable_postprocessing" ) )
    {
        mEnablePostprocess = true;
    }

    if( mConfig.isMember( "banned_threads" ) )
    {
        Json::Value const& bannedThreads = mConfig[ "banned_threads" ];

        if( bannedThreads.isArray() )
        {
            Json::Value const& bannedThreads = mConfig[ "banned_threads" ];

            for( unsigned i = 0; i < bannedThreads.size() ; ++i )
            {
                mBannedThreads.push_back( bannedThreads[i].asString() );
            }
        }
        else
        {
            DBG_LOG( "%s: JSON member banned_threads is not an array.\n", mName.c_str() );
        }
    } else if ( mEnablePostprocess ) {
        DBG_LOG( "%s: Postprocessing enabled, but no process filter specified. Filtering only ferret threads by default (to mimnimize measurement pollution by instrumentation).\n", mName.c_str() );
        mBannedThreads.push_back( "ferret" );
    }

    if( mConfig.isMember( "process_names" ) )
    {
        Json::Value const& nameArr = mConfig[ "process_names" ];

        if( nameArr.isArray() )
        {
            for( unsigned i = 0; i < nameArr.size(); ++i )
            {
                mProcessNames.push_back( nameArr[i].asString() );
            }

            int timeout = mConfig.get( "poll_timeout", 30 ).asInt();

            mPollTimeout = std::chrono::seconds( timeout );
            mStatusFd = mConfig.get( "status_fd", -1 ).asInt();
        }
        else
        {
            DBG_LOG( "%s: JSON member process_names is not an array.\n", mName.c_str() );
        }
    }

    if( mConfig.isMember( "output_dir" ) )
    {
        Json::Value const& outString = mConfig["output_dir"];

        if( outString.isString() )
        {
            mOutputDir = outString.asString();

            while( mOutputDir[ mOutputDir.size() - 1 ] == '/' )
            {
                mOutputDir.erase( mOutputDir.size() - 1, 1 );
            }
        }
        else
        {
            DBG_LOG( "%s: JSON member output_dir is not a string.\n", mName.c_str() );
        }
    }

    /*
     * Error checking/correction, apply defaults.
     */

    if( !mConfig.isMember("threaded") )
    {
        /* If threading isn't specified, default to threaded at double the scheduler tick rate.
         */
        mIsThreaded = true;
        mSampleRate = 1000 / (2 * mClockTicks);
    }

    /*
     * Confirm setup
     */

    std::string cpuWatch = "";
    for( auto cpu : mCpus )
    {
        cpuWatch += " " + _to_string( cpu );
    }
    if (mDebug) DBG_LOG( "%s: monitoring CPUs [%s]\n", mName.c_str(), cpuWatch.c_str() + 1 );
}


FerretCollector::~FerretCollector()
{
    /* Several examples of instance destruction without calling deinit(), so just be sure.
     */
    deinit();
}


bool FerretCollector::init( void )
{
    if( mInitSuccess )
    {
        return mInitSuccess;
    }

    /* Be optimistic until proven otherwise.
     */
    mInitSuccess = true;

    open_cpufreq_fds();

    if( mCpus.size() == 0 || ( mCpufreqFdMap.size() != mCpus.size() ) )
    {
        mInitSuccess = false;

        if( mCpus.size() == 0 )
        {
            DBG_LOG( "%s: No CPUs specified.\n", mName.c_str() );
        }
        else
        {
            DBG_LOG( "%s: one or more CPU frequencies are unreadable.\n", mName.c_str() );
        }
    }

    if( mInitSuccess )
    {
        for( auto pid : mPids )
        {
            if( !open_pid_fds( pid ) )
            {
                DBG_LOG( "%s: unable to read process %s status.\n", mName.c_str(), pid.c_str() );
            }
        }
    }

    if( mOutputDir.size() == 0 )
    {
#ifdef ANDROID
        mInitSuccess = false;
        DBG_LOG( "%s: output_dir is unspecified.\n", mName.c_str() );
#else
        mOutputDir = ".";
        if (mDebug) DBG_LOG( "%s: output_dir is absent, using %s\n", mName.c_str(), mOutputDir.c_str() );
#endif
    }

    if( mProcessNames.size() )
    {
        if( mStatusFd < 0 )
        {
            mInitSuccess = false;
            DBG_LOG( "%s: Bad/missing status_fd.\n", mName.c_str() );
        }
    }

    if( !mInitSuccess )
    {
        close_cpufreq_fds();
        close_pid_fds();
    }

    return mInitSuccess;
}


bool FerretCollector::deinit( void )
{
    close_cpufreq_fds();
    close_pid_fds();

    mInitSuccess = false;
    return mInitSuccess;
}


bool FerretCollector::available( void )
{
    /* available() is called before init() but we cannot know if we're available until
     * try to init().
     */
    if( !mInitSuccess )
    {
        mInitSuccess = init();
    }
    return mInitSuccess;
}


bool FerretCollector::start( void )
{
    bool success = true;

    if( !mInitSuccess )
    {
        return mInitSuccess;
    }

    if( mCollecting )
    {
        return mCollecting;
    }

    if( !mIsThreaded )
    {
        DBG_LOG( "%s: The Ferret experience is best threaded.\n", mName.c_str() );
    }

    mFirstCollect = std::chrono::seconds( 0 );
    mWatchList = mProcessNames;

    static int increment = 0;
    const std::string prefix = mOutputDir + "/ferret_run_";
    const std::string extension = ".data";
    const std::string output = prefix + _to_string( increment++ ) + extension;

    mCustomResult["output"] = output;

    unlink( output.c_str() );

    mTraceFd = open( output.c_str(), O_CREAT | O_RDWR, 0666 );

    if( mTraceFd < 0 )
    {
        DBG_LOG( "%s: failed to open(%s).\n", mName.c_str(), output.c_str() );
        perror( mName.c_str() );
        success = false;
    }

    if( success )
    {
        std::string header = "I _SC_CLK_TCK " + _to_string( mClockTicks ) + "\n";

        header += "I CPUList";
        for( auto cpu : mCpus )
        {
            header += " " + _to_string( cpu );
        }
        header += "\n";

        header += "I WatchList";
        for( auto watch : mWatchList )
        {
            header += " " + watch;
        }
        header += "\n";

        header += "I Status";
        for( unsigned i = 0; i < ( sizeof( pid_stat_spec ) / sizeof( pid_stat_spec[0] ) ); ++i )
        {
            header += " " + std::string( pid_stat_spec[i].label );
        }
        header += "\n";

        ssize_t wrote = write( mTraceFd, header.c_str(), header.size() );

        if( wrote < (ssize_t) header.size() )
        {
            DBG_LOG( "%s: failed to write(%s).\n", mName.c_str(), output.c_str() );
            success = false;
        }
    }

    if( success )
    {
        mCollecting = true;
    }

    return mCollecting;
}


bool FerretCollector::stop()
{
    if( !mCollecting )
    {
        return mCollecting;
    }
    mCollecting = false;

    close( mTraceFd );

    if( mEnablePostprocess )
    {
        // This is a c++ port of the ferret.py script (intended to make it possible to get CPU performance data without any postprocessing)
        mCustomResult["postprocessed_results"] = postprocess_ferret_data( mCustomResult["output"].asString(), mBannedThreads );
    }

    return !mCollecting;
}


void FerretCollector::collect_freqs( void )
{
    std::string row_err;
    std::string row_freq;

    for( auto it : mCpufreqFdMap )
    {
        const int cpuNr = it.first;
        const int fd = it.second;

        const size_t bufsz = 100;
        char buf[ bufsz + 1 ];

        ssize_t ret = read( fd, buf, bufsz );

        if( ret > 0 )
        {
            lseek( fd, 0, SEEK_SET );

            /* Frequencies are always followed by a single unwanted newline.
             */
            buf[ ret - 1 ] = '\0';

            row_freq += " " + _to_string( cpuNr ) + " " + buf;
        }
        else
        {
            row_err += " (CPU " + _to_string( cpuNr ) + " freq read() failed)";
        }
    }

    if( row_err.size() )
    {
        row_err.insert( 0, 1, 'E' );
        row_err += "\n";
        ssize_t wrote = write( mTraceFd, row_err.c_str(), row_err.size() );

        ( void ) wrote;
    }

    if( row_freq.size() )
    {
        row_freq.insert( 0, 1, 'F' );
        row_freq += "\n";
        ssize_t wrote = write( mTraceFd, row_freq.c_str(), row_freq.size() );

        (void) wrote;
    }
}


/** Extract fields from "stat" files in the proc filesystem.
 *
 * Fields specified by pid_stat_spec are extracted from @p buf read from file
 * /proc/<pid>/task/<tpid>/stat and written to @p row.
 *
 * @param[in]  buf  A buffer whose content is a stat file.
 * @param[out] row  The output buffer.
 */
static void parse_stat( char *buf, std::string& row )
{
    const ssize_t field_max = sizeof( pid_stat_spec ) / sizeof( pid_stat_spec[0] );

    const int FIELD_COMM = 1;

    std::vector<char*> fields;

    int field_count = 0;
    int match_count = 0;

    char *s = buf;

    bool in_field = false;
    int curr_field = 0;

    while( ( match_count <= field_max ) && ( *s != '\0' ) && ( *s != '\n' ) )
    {
        if( *s == ' ' )
        {
            if( curr_field == FIELD_COMM )
            {
                /* The 'comm' field is the process name, and may contain spaces.
                 * Substitute an underscore for easier parsing of results.
                 */
                *s = '_';
            }
            else
            {
                /* Split other fields with NUL in preparation for output.
                 */
                *s = '\0';
                in_field = false;
            }
        }
        else
        {
            if( in_field && curr_field == FIELD_COMM )
            {
                if( *s == ')' )
                {
                    /* The 'comm' field is the process name, and is enclosed by
                     * parens.  A closing paren is thus the end of the field.
                     *
                     * No attempt is made to deal with programs whose names include
                     * parens.
                     */
                    curr_field = field_count;
                }
            }

            if( !in_field )
            {
                in_field = true;

                if( field_count == 1 )
                {
                    curr_field = field_count;
                }

                if( field_count == pid_stat_spec[match_count].field_nr )
                {
                    /* Record the position of the first character of each field.
                     *
                     * Note that we cannot extract the field now since we don't
                     * yet know where the field ends.
                     */
                    fields.push_back( s );
                    match_count++;
                }
                field_count++;
            }
        }
        s++;
    }

    /* Now write the terminated fields.
     */
    for( char* it : fields )
    {
        const std::string field = it;
        row += " " + field;
    }
}


void FerretCollector::collect_perproc( void )
{
    for( auto it = mPidFdMap.begin(); it != mPidFdMap.end(); ++it )
    {
        int fd = it->second;
        std::string const& pid = it->first;

        if( fd < 0 )
        {
            /* PID has already exited.
             */
            continue;
        }

        const size_t bufsz = 4096;
        char buf[ bufsz + 1 ];

        ssize_t ret = read( fd, buf, bufsz );

        if( ret > 0 )
        {
            lseek( fd, 0, SEEK_SET );

            /* NUL terminate the buffer.
             */
            buf[ ret ] = '\0';

            std::string row;

            parse_stat( buf, row );

            row.insert( 0, 1, 'S' );
            row += "\n";

            ssize_t wrote = write( mTraceFd, row.c_str(), row.size() );

            (void) wrote;
        }
        else
        {
            const std::string row = "E " + pid + " has gone\n";
            ssize_t wrote = write( mTraceFd, row.c_str(), row.size() );

            (void) wrote;
            close( fd );

            /* Mark the PID as "gone".
             */
            it->second = -1;
        }
    }
}


/** Notify a watcher via @p fd of a change in @p status.
 *
 * Updated @p status is written to @p fd only if it differs from
 * the last invocation.
 *
 * @param[in] fd     A writeable file descriptor for notification.
 * @param[in] status The latest status.
 */
static void notify( int fd, int status )
{
    static int lastStatus = -99;

    if( fd < 0 ) return;

    if( lastStatus != status )
    {
        ssize_t s = write( fd, &status, sizeof(status) );
        (void)s; // elaborate way to silence the compiler
        lastStatus = status;
    }
}


bool FerretCollector::collect( int64_t now )
{
    if ( !mInitSuccess )
    {
        DBG_LOG( "%s: Attempted to collect but initialization was not successful, aborting.\n", mName.c_str() );
        assert( mInitSuccess );
    }

    if ( !mCollecting )
    {
        DBG_LOG( "%s: Attempted to collect but collector has not been started, aborting.\n", mName.c_str() );
        assert( mCollecting );
    }

    /* Detect time of first call.
     */
    if( mFirstCollect == std::chrono::seconds( 0 ) )
    {
        mFirstCollect = std::chrono::microseconds( now );
    }

    /*
     * Poll for process(es) by name.
     */

    for( auto w = mWatchList.begin(); w != mWatchList.end(); /* do nothing */ )
    {
        pid_t pid = poll_for_named_process( *w );

        if( pid > 0 )
        {
            mPids.push_back( _to_string(pid) );
            w = mWatchList.erase( w );
        }
        else if( pid == 0 )
        {
            /* Not found.  Ignore and hope we find it before the timeout.
             */
            ++w;
        }
        else
        {
            /* Cannot open /proc.
             *
             * This is a serious fault.
             */
            DBG_LOG( "%s: unable to open /proc.\n", mName.c_str() );
            assert( false );
        }
    }

    /*
     * Check for timeout.
     */

    if( mWatchList.size() )
    {
        auto wait_duration = std::chrono::microseconds(now) - mFirstCollect;

        if( wait_duration > mPollTimeout )
        {
            notify( mStatusFd, -1 );
        }

        /* Don't start collection until all (if any) named processes are found.
         */
        return mCollecting;
    }

    /* We've converted process names (if any) into PIDs and are now actively collecting.
     */
    notify( mStatusFd, 1 );

    /*
     * Locate threads for all monitored PIDs, leaving fd's open in mPidFdMap.
     */

    enumerate_tasks( mPid );

    for( auto pid : mPids )
    {
        enumerate_tasks( pid );
    }

    const std::string row = "T " + _to_string( now ) + "\n";

    ssize_t wrote = write( mTraceFd, row.c_str(), row.size() );

    (void) wrote;

    collect_freqs();
    collect_perproc();

    /*
     * Determine if the monitored processes have exited.
     */

    unsigned active = mPids.size();

    for( auto pid : mPids )
    {
        if( mPidFdMap.find( pid ) != mPidFdMap.end() )
        {
            assert( active > 0 );

            active -= ( mPidFdMap[ pid ] < 0 );
        }
    }

    if( active == 0 )
    {
        /* No processes left to monitor.
         */
        notify( mStatusFd, 0 );
    }

    return mCollecting;
}

void FerretCollector::open_cpufreq_fds( void )
{
    for( auto cpu : mCpus )
    {
        const std::string prefix = "/sys/devices/system/cpu/cpu";
        const std::string suffix = "/cpufreq/scaling_cur_freq";
        const std::string freqFile = prefix + _to_string( cpu ) + suffix;

        if( mCpufreqFdMap.find( cpu ) != mCpufreqFdMap.end() )
        {
            /* Already open.
             */
            continue;
        }

        int fd = open( freqFile.c_str(), O_RDONLY );

        if( fd < 0 )
        {
            DBG_LOG( "%s: failed to open(%s)\n", mName.c_str(), freqFile.c_str() );
            assert( errno != ENFILE );
        }
        else
        {
            if( lseek( fd, 0, SEEK_SET ) != 0 )
            {
                perror( mName.c_str() );
                close( fd );
            }
            else
            {
                mCpufreqFdMap[ cpu ] = fd;
            }
        }
    }
}


void FerretCollector::close_cpufreq_fds( void )
{
    for( auto iter:mCpufreqFdMap )
    {
        close( iter.second );
    }
    mCpufreqFdMap.clear();
}


bool FerretCollector::open_pid_fd( std::string const& prefix, std::string const& pid )
{
    const std::string suffix = "/stat";
    const std::string statFile = prefix + "/" + pid + suffix;

    if( mPidFdMap.find( pid ) == mPidFdMap.end() )
    {
        int fd = open( statFile.c_str(), O_RDONLY );

        if( fd < 0 )
        {
            DBG_LOG( "%s: failed to open(%s)\n", mName.c_str(), statFile.c_str() );
            assert( errno != ENFILE );
        }
        else
        {
            if( lseek( fd, 0, SEEK_SET ) != 0 )
            {
                perror( mName.c_str() );
                close( fd );
            }
            else
            {
                mPidFdMap[ pid ] = fd;
            }
        }
    }

    bool success = mPidFdMap.find( pid ) != mPidFdMap.end();

    return success;
}


bool FerretCollector::open_pid_fds( std::string const& pid )
{
    const std::string prefix = "/proc";

    bool success = open_pid_fd( prefix, pid );

    enumerate_tasks( pid );

    return success;
}


void FerretCollector::close_pid_fds( void )
{
    for( auto iter:mPidFdMap )
    {
        const int fd = iter.second;

        if( fd < 0 )
        {
            continue;
        }
        close( fd );
    }
    mPidFdMap.clear();
}


void FerretCollector::enumerate_tasks( std::string const& pid )
{
    const std::string prefix = "/proc";
    const std::string suffix = "/task";
    const std::string taskDir = prefix + "/" + pid + suffix;

    DIR *dirp = NULL;

    dirp = opendir( taskDir.c_str() );
    if( !dirp )
    {
        return;
    }

    struct dirent *dp = NULL;

    do
    {
        if( ( dp = readdir( dirp ) ) != NULL )
        {
            if( isdigit( dp->d_name[ 0 ] ) )
            {
                (void) open_pid_fd( taskDir, dp->d_name );
            }
        }

    }
    while( dp != NULL );

    closedir( dirp );
}


pid_t FerretCollector::poll_for_named_process( std::string const& name )
{
    const std::string procDir = "/proc";

    pid_t found = 0;

    DIR *dirp = NULL;
    struct dirent *dp = NULL;

    dirp = opendir( procDir.c_str() );

    if( !dirp )
    {
        return (pid_t) -1;
    }

    do
    {
        if( ( dp = readdir( dirp ) ) != NULL )
        {
            if( isdigit( dp->d_name[ 0 ] ) )
            {
                const std::string cmdlinePath = procDir + "/" + dp->d_name + "/cmdline";

                int fd = open( cmdlinePath.c_str(), O_RDONLY );

                if( fd > 0 )
                {
                    const size_t bufsz = 100;
                    char buf[ bufsz + 1 ];

                    ssize_t ret = read( fd, buf, bufsz );
                    close( fd );

                    if( ret > 0 )
                    {
                        buf[ ret ] = '\0';

                        if( std::strcmp( name.c_str(), buf ) == 0 )
                        {
                            /* We found it.
                             */
                            found = (pid_t) atoi( dp->d_name );
                            DBG_LOG( "ferret: found '%s' (pid=%d)\n", name.c_str(), found )
                            break;
                        }
                    }
                }
                else
                {
                    /* It's reasonable to be unable to read this provided we had sufficient
                     * file handles.  Ignore.
                     */
                    assert( errno != ENFILE );
                }
            }
        }
    }
    while( dp != NULL );

    closedir( dirp );

    return found;
}


// The rest is postprocessing utility
FerretProcess::FerretProcess(
    int schedTickHz,
    const std::vector<int>& cpuNrs):
        label(""),
        first(0.0),
        last(0.0),
        jiffyPeriod(1.0 / double(schedTickHz)),
        jiffies(0),
        totJiffiesAllCpus(0)
{
    for ( int i : cpuNrs )
    {
        std::string cpu_index = _to_string(i);

        mcyc[cpu_index] = 0;
        totJiffies[cpu_index] = 0;
        freqLogSums[cpu_index] = 0.0;
        numFreqLogSums[cpu_index] = 0;
    }
}


void FerretProcess::add(
    const std::string& nlabel,
    double ts,
    int njiffies,
    double freq,
    int cpuNr
) {
    std::string cpu_index = _to_string(cpuNr);

    label = nlabel;
    if ( first == 0.0 )
    {
        first = ts;
        jiffies = njiffies;
    }

    last = ts;

    int diffJiffy = njiffies - jiffies;
    jiffies = njiffies;

    totJiffiesAllCpus += diffJiffy;
    totJiffies[cpu_index] = totJiffies[cpu_index].asInt() + diffJiffy;

    if ( freq )
    {
        mcyc[cpu_index] = mcyc[cpu_index].asDouble() + diffJiffy * jiffyPeriod * freq;
        freqLogSums[cpu_index] = freqLogSums[cpu_index].asDouble() + std::log(freq);
        numFreqLogSums[cpu_index] = numFreqLogSums[cpu_index].asInt() + 1;
    }
}


double FerretProcess::duration() const
{
    return last - first;
}


int FerretProcess::active_jiffies() const
{
    return totJiffiesAllCpus;
}


Json::Value FerretProcess::summary(const std::string& pid) const
{
    Json::Value result;

    double active = jiffyPeriod * active_jiffies();

    if ( !active )
    {
        return result;
    }

    result["pid"] = pid;
    result["name"] = label;
    result["duration"] = duration();

    Json::Value actives;
    Json::Value::Members cpu_keys = totJiffies.getMemberNames();
    double total_active = 0.0;
    for ( auto& cpuNr : cpu_keys )
    {
        actives[cpuNr] = totJiffies[cpuNr].asInt() * jiffyPeriod;
        total_active += totJiffies[cpuNr].asInt() * jiffyPeriod;
    }

    result["active_sum"] = total_active;
    result["active"] = actives;

    double total_cycles = 0.0;
    for ( auto& cpuNr : cpu_keys )
    {
        total_cycles += mcyc[cpuNr].asDouble();
    }

    result["MCyc_sum"] = total_cycles;
    result["MCyc"] = mcyc;

    result["MHz"] = Json::Value();
    for ( auto& cpuNr : cpu_keys )
    {
        size_t numFreqs = numFreqLogSums[cpuNr].asInt();

        if ( numFreqs == 0 )
        {
            result["MHz"][cpuNr] = 0;
            continue;
        }

        double freqSum = freqLogSums[cpuNr].asDouble();
        result["MHz"][cpuNr] = std::exp( freqSum / numFreqs );
    }

    return result;
}


static void ferret_process_info(
    Json::Value& traceProperties,
    const std::vector<std::string>& rowData,
    Json::Value& state
) {
    static std::map<std::string, int> infoTypeMap = {
        {"_SC_CLK_TCK", 0},
        {"CPUList", 1},
        {"WatchList", 2},
        {"Status", 3}
    };

    std::string infoType = rowData[1];

    int typeID = infoTypeMap[infoType];

    switch ( typeID )
    {
        case 0: {
            traceProperties["tick"] = _stoi(rowData[2]);
            break;
        }
        case 1: {
            std::vector<int> cpu_idices;
            std::set<int> duplication_checker;
            for ( size_t i = 2; i < rowData.size(); ++i )
            {
                int cpu_number = _stoi(rowData[i]);
                if ( duplication_checker.find(cpu_number) != duplication_checker.end() )
                {
                    DBG_LOG( "Duplicate CPU index %d specified in libcollector module ferret parameters.\n", cpu_number );
                    assert( false );
                }

                duplication_checker.insert(cpu_number);
                cpu_idices.push_back(cpu_number);
            }

            std::sort(cpu_idices.begin(), cpu_idices.end());

            traceProperties["cpus"] = Json::Value(Json::arrayValue);

            for ( int cpu_index : cpu_idices )
            {
                traceProperties["cpus"].append(cpu_index);
            }

            break;
        }
        case 2: {
            traceProperties["watch"] = Json::Value(Json::arrayValue);

            for ( size_t i = 2; i < rowData.size(); ++i )
            {
                traceProperties["watch"].append(rowData[i]);
            }

            break;
        }
        case 3: {
            traceProperties["stat_fields"] = Json::Value(Json::arrayValue);

            for ( size_t i = 2; i < rowData.size(); ++i )
            {
                traceProperties["stat_fields"].append(rowData[i]);
            }
            break;
        }
    }
}


static void ferret_process_time(
    Json::Value& traceProperties,
    const std::vector<std::string>& rowData,
    Json::Value& state
) {
    double now = static_cast<double>( _stol( rowData[1] ) ) * 1.0e-6;
    state.clear();
    state["time"] = now;
}


static void ferret_process_status(
    Json::Value& traceProperties,
    const std::vector<std::string>& rowData,
    Json::Value& state
) {
    Json::Value dataDict;
    std::string pid_str;

    for ( int i = 1; i < static_cast<int>(rowData.size()); ++i )
    {
        std::string stat_field = traceProperties["stat_fields"][i - 1].asString();

        if ( stat_field ==  "comm")
        {
            std::string stripped_string = std::string(
                rowData[i].begin() + 1,
                rowData[i].end() - 1);

            dataDict[stat_field] = stripped_string;
        } else if ( stat_field == "pid" ) {
            pid_str = rowData[i];
            dataDict[stat_field] = _stoi(rowData[i]);
        } else {
            dataDict[stat_field] = _stoi(rowData[i]);
        }
    }

    if ( !state.isMember("pid") )
    {
        state["pid"] = Json::Value();
    }

    state["pid"][pid_str] = dataDict;
}


static double freqScale(int freq)
{
    double ffreq = static_cast<double>(freq);
    return ffreq / 1000.0;
}


static void ferret_process_cpu(
    Json::Value& traceProperties,
    const std::vector<std::string>& rowData,
    Json::Value& state
) {
    std::vector<std::string> cpuNrs;
    std::vector<double> cpuFreqs;
    for ( size_t i = 1; i < rowData.size(); ++i )
    {
        if (i % 2)
        {
            cpuNrs.push_back(rowData[i]);
        } else {
            cpuFreqs.push_back( freqScale( _stoi( rowData[i] ) ) );
        }
    }

    Json::Value cpu_data;

    for ( size_t i = 0; i < cpuNrs.size(); ++i )
    {
        cpu_data[cpuNrs[i]] = cpuFreqs[i];
    }

    state["cpu"] = cpu_data;
}


static void ferret_process_skip(
    Json::Value& traceProperties,
    const std::vector<std::string>& rowData,
    Json::Value& state
) {}


Json::Value postprocess_ferret_data(const std::string& outputFname, const std::vector<std::string>& banned_threads) {
    DBG_LOG( "Running ferret postprocessing for output data in file: %s\n", outputFname.c_str() );
    std::map<char, void(*)(Json::Value&, const std::vector<std::string>&, Json::Value&)> record_map = {
        {'I', ferret_process_info},
        {'T', ferret_process_time},
        {'S', ferret_process_status},
        {'F', ferret_process_cpu},
        {'#', ferret_process_skip},
        {'E', ferret_process_skip}
    };

    std::ifstream infile( outputFname );
    if ( !infile.is_open() )
    {
        DBG_LOG( "Failed to open ferret data file: %s\n", outputFname.c_str() );
        assert( false );
    }

    std::string row;
    std::vector<std::string> tokens;

    Json::Value state;
    Json::Value traceProperties;

    std::map<std::string, FerretProcess> pidHistory;

    bool has_data = false;

    bool initData = false;
    bool started = false;
    double traceStart = 0.0;
    double traceEnd = 0.0;
    std::vector<int> cpuNrs;
    int tick = 100;

    while ( std::getline( infile, row ) )
    {
        has_data = true;
        tokens.clear();
        splitString( row.c_str(), ' ', tokens );

        char record_type = tokens[0][0];
        if ( record_type == 'T' ) {
            Json::Value sample = state;

            if ( !sample.empty() )
            {
                if ( !initData )
                {
                    tick = traceProperties["tick"].asInt();

                    if ( !traceProperties.isMember("cpus") )
                    {
                        DBG_LOG( "cpus not in trace properties, possibly broken ferret data!\n" );
                        assert( false );
                    }

                    for ( Json::Value::ArrayIndex i = 0; i != traceProperties["cpus"].size(); ++i )
                    {
                        cpuNrs.push_back(traceProperties["cpus"][i].asInt());
                    }

                    initData = true;
                }


                if ( !started )
                {
                    traceStart = sample["time"].asDouble();
                    started = true;
                }

                traceEnd = sample["time"].asDouble();

                Json::Value::Members pids = sample["pid"].getMemberNames();
                for ( auto pid : pids )
                {
                    Json::Value& pidInfo = sample["pid"][pid];
                    const std::string& comm = pidInfo["comm"].asString();

                    bool pid_banned = false;
                    for (const std::string& banned_comm : banned_threads) {
                        if (comm == banned_comm) {
                            pid_banned = true;
                            break;
                        }
                    }

                    if (pid_banned) {
                        continue;
                    }

                    int processor = pidInfo["processor"].asInt();

                    if ( std::find(cpuNrs.begin(), cpuNrs.end(), processor) == cpuNrs.end() )
                    {
                        DBG_LOG( "Invalid CPU index in ferret sample: %d\n", processor);
                        assert( false );
                    }

                    if ( pidHistory.find(pid) == pidHistory.end() )
                    {
                        pidHistory[pid] = FerretProcess(tick, cpuNrs);
                    }

                    
                    int ustime = pidInfo["utime"].asInt() + pidInfo["stime"].asInt();

                    std::string processor_string = _to_string(processor);
                    double freq = sample["cpu"][processor_string].asDouble();

                    pidHistory[pid].add(comm, sample["time"].asDouble(), ustime, freq, processor);
                }
            }
        }

        if ( record_map.find(record_type) == record_map.end() ) {
            DBG_LOG( "Invalid record type: %c in ferret data file, possibly corrupt output!\n", record_type );
            assert( false );
        }

        record_map[record_type]( traceProperties, tokens, state );
    }

    if ( !has_data )
    {
        DBG_LOG( "Ferret data file: %s was empty! Something went wrong when collecting data.\n", outputFname.c_str() );
        assert( false );
    }

    int skipped = 0;

    std::string result_name = "process_results";

    Json::Value results;
    results[result_name] = Json::arrayValue;

    double max_active = 0.0;
    double max_duration = 0.0;
    double mcycle_sum = 0.0;
    int sample_index = 0;
    int max_index = 0;

    for ( auto& pid_pair : pidHistory )
    {
        const std::string& pid = pid_pair.first;
        FerretProcess& process = pid_pair.second;
        Json::Value psummary = process.summary(pid);
        if ( ( process.active_jiffies() > 0 ) && ( process.duration() > 0.01 * ( traceEnd - traceStart ) ) )
        {
            results[result_name].append( psummary );

            mcycle_sum += psummary["MCyc_sum"].asDouble();

            if ( psummary["MCyc_sum"].asDouble() > max_active )
            {
                max_active = psummary["MCyc_sum"].asDouble();
                max_index = results[result_name].size() - 1;
            }

            if ( process.duration() > max_duration )
            {
                max_duration = process.duration();
            }
        } else {
            std::string thread_name = psummary["name"].asString();
            DBG_LOG(
                "Skipping thread with PID %s and name %s. Active jiffies: %d, Duration: %f.\n",
                pid.c_str(),
                thread_name.c_str(),
                process.active_jiffies(),
                process.duration());

            skipped += 1;
        }

        sample_index += 1;
    }

    results["main_thread_index"] = max_index;
    results["main_thread_megacycles"] = max_active;
    results["megacycles_sum"] = mcycle_sum;

    DBG_LOG( "%d insignificant thread(s) skipped.\n", skipped );

    return results;
}
