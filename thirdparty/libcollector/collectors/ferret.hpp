/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2017 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */
#pragma once

#include <map>

#include "collector_utility.hpp"
#include "interface.hpp"

/** Utility class used for postprocessing.
 * Implements most of the functionality also found in the ferret.py script.
 */
class FerretProcess
{
public:
    FerretProcess() {}

    FerretProcess(
        int schedTickHz,
        const std::vector<int>& cpuNr);

    void add(
        const std::string& label,
        double ts,
        int jiffies,
        double freq,
        int cpuNr);

    double duration() const;
    int active_jiffies() const;
    Json::Value summary(const std::string& pid) const;

private:
    std::string label;
    double first;
    double last;
    double jiffyPeriod;
    int jiffies;
    int totJiffiesAllCpus;

    Json::Value maxCpuFreqs;
    Json::Value mcyc;
    Json::Value totJiffies;
    Json::Value freqLogSums;
    Json::Value numFreqLogSums;
};


/** Utility function used for postprocessing.
 *
 * Implements the main function in the ferret.py script.
 *
 * @return Json::Value with per. thread statistics.
 */
Json::Value postprocess_ferret_data(
    const std::string& outputFname,
    const std::vector<std::string>& bannedThreads);


class FerretCollector : public Collector
{
public:
    using Collector::Collector;

    /** Ferret collector constructor.
     *
     * Extracts configuration from @p config and applies defaults.
     *
     * @param[in] config JSON configuration items.
     * @param[in] name   Name of the collector.
     */
    FerretCollector( const Json::Value& config,
                     const std::string& name);


    /** Ferret collector destructor.
     */
    ~FerretCollector();


    /** Ferret collector initializer.
     *
     * Opens file descriptors for several files needed for subsequent collection activities.
     *
     * @return false if collection cannot continue, true otherwise.
     */
    virtual bool init() override;


    /** Ferret collector finalizer.
     *
     * Close open file descriptors.
     *
     * @return false
     */
    virtual bool deinit() override;


    /** Prepare the Ferret collector for data collection.
     *
     * @return false if collection cannot continue, true otherwise.
     */
    virtual bool start() override;


    /** Finalize Ferret data collection.
     *
     * @return false if collection cannot continue, true otherwise.
     */
    virtual bool stop() override;


    /** Collect data on monitored processes.
     *
     * Resolve the PIDs for processes specified by name.  If processes cannot
     * be resolved within a client specified timeout collection is abandoned.
     *
     * Monitor the specified CPUs and processes for as long as processes
     * continue to execute, signalling via a file descriptor (if configured)
     * when all specified processes have exited.
     *
     * @param[in] now  Microseconds since the epoch.
     *
     * @return false if collection cannot continue, otherwise true.
     */
    virtual bool collect(int64_t) override;


    /** Determine if the Ferret collector is available.
     *
     * @return false if collection cannot continue, true otherwise.
     */
    virtual bool available() override;

private:
    /** Poll for a process named @p name, returning its pid.
     *
     * @param[in] name The name of the process to find.
     *
     * @retval -1 if the /proc filesystem cannot be read.
     * @retval 0 if the process is not found.
     * @retval Positive values are pids.
     *
     * @note Asserts that failures to open /proc are not caused by file handle
     * exhaustion.
     */
    pid_t poll_for_named_process( std::string const& name );


    /** Open file descriptors for configured CPU scaling_cur_freq files.
     *
     * @note Error checking is deliberately relaxed since some mobile SoCs
     * are distinctly weird in their CPU numbering; leave some leeway for
     * experimentation.
     *
     * @note Asserts that failures to open /proc are not caused by file handle
     * exhaustion.
     */
    void open_cpufreq_fds( void );


    /** Close file descriptors for configured CPU scaling_cur_freq files.
     */
    void close_cpufreq_fds( void );


    /** Open file descriptors for all tasks running within @p pid.
     *
     * @param[in] pid  The process ID whose tasks shall be opened.
     *
     * @return true if /proc/@p pid/stat is readable.
     */
    bool open_pid_fds( std::string const& );


    /** Close file descriptors for all pids.
     */
    void close_pid_fds( void );


    /** Open the file descriptor corresponding to @p prefix/@p pid/stat.
     *
     * If a file descriptor for the file does not yet exist open the file.
     *
     * If successful the mapping of @pid to file descriptor is stored for
     * subsequent polling.
     *
     * @param[in] prefix
     * @param[in] pid
     *
     * @return true if a mapping from @pid to a valid file descriptor exists.
     *
     * @note Asserts that failures to open /proc are not caused by file handle
     * exhaustion.
     */
    bool open_pid_fd( std::string const& prefix, std::string const& pid );


    /** Enumerate all tasks for process @p pid and open file descriptors.
     *
     * @param[in] pid  The process ID whose tasks shall be opened.
     */
    void enumerate_tasks( std::string const& pid );


    /** Collect CPU frequencies.
     *
     * Collect CPU frequency data and write to the file whose handle is mTraceFd.
     */
    void collect_freqs( void );


    /** Collect CPU utilisation for all monitored processes.
     *
     * When monitored processes exit, close the file descriptor and record an
     * invalid descriptor.
     *
     * @note On one desktop system at least std::map::erase() removes only
     * the value (leaving an fd=0!) and not the key.
     */
    void collect_perproc( void );

    /** The success/failure of the most recent init() call.
     */
    bool mInitSuccess = false;

    /** If this is true, postprocessing is enabled member is valid.
     */
    bool mEnablePostprocess = false;

    /** The OS scheduler ticks per second.
     */
    long mClockTicks;

    /** The CPU numbers to monitor.
     */
    std::vector<int> mCpus;

    /** A list of thread (comm) names to exclude during postprocessing.
     */
    std::vector<std::string> mBannedThreads;

    /** The file output path.
     */
    std::string mOutputDir;

    /** The fd of the output file.
     */
    int mTraceFd;

    /** The writeable fd for status notifications, or -1.
     */
    int mStatusFd = -1;

    /** The PID of the monitoring process.
     */
    std::string mPid;

    /** The names of processes to monitor.
     */
    std::vector<std::string> mProcessNames;

    /** The PIDs of all processes to monitor.
     */
    std::vector<std::string> mPids;

    /** The remaining names of processes whose PIDs are still sought.
     */
    std::vector<std::string> mWatchList;

    /** Microseconds since the Epoch when the first collect() call was invoked.
     */
    std::chrono::microseconds mFirstCollect;

    /** Timeout for reduction of mWatchList to length zero.
     */
    std::chrono::seconds mPollTimeout;

    /** Map PIDs to fds for PID status files, or -1 if the process has exited.
     */
    std::map<std::string, int> mPidFdMap;

    /** Map of CPU numbers to scaling_cur_freq fds.
     */
    std::map<int, int> mCpufreqFdMap;
};
