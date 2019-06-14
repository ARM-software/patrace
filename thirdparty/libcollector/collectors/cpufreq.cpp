#include "cpufreq.hpp"
#include "collector_utility.hpp"

#include <errno.h>
#include <stdio.h>
#include <string.h>

bool CPUFreqCollector::init()
{
    // Just in case, clean up...
    deinit();
    mCores.clear();

    // find all CPU cores
    std::string filename_cf = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq";
    int core = 0;
    FILE *cf = fopen(filename_cf.c_str(), "r");
    while (cf)
    {
        long freq = 0;
        long times = 0;
        std::string filename_tis = "/sys/devices/system/cpu/cpu" + _to_string(core) + "/cpufreq/stats/time_in_state";
        FILE *tis = fopen(filename_tis.c_str(), "r");
        mCores.emplace_back(tis, cf, core, "cpu_" + _to_string(core));
        while (tis && fscanf(tis, "%ld %ld\n", &freq, &times) == 2)
        {
            mCores.back().frequencies.push_back(freq);
        }

        core++;
        std::string filename_cf = "/sys/devices/system/cpu/cpu" + _to_string(core) + "/cpufreq/scaling_cur_freq";
        cf = fopen(filename_cf.c_str(), "r");
    }
    return core > 0;
}

bool CPUFreqCollector::deinit()
{
    for (Core& c : mCores)
    {
        if (c.time_in_state)
        {
            fclose(c.time_in_state);
            c.time_in_state = nullptr;
        }
        fclose(c.freq_file);
        c.freq_file = nullptr;
    }
    return true;
}

bool CPUFreqCollector::start()
{
    for (Core& c : mCores)
    {
        long freq = 0;
        long times = 0;
        if (c.time_in_state)
        {
            rewind(c.time_in_state);
            while (fscanf(c.time_in_state, "%ld %ld\n", &freq, &times) == 2)
            {
                c.times.push_back(times);
            }
        }
    }
    return true;
}

bool CPUFreqCollector::collect(int64_t /* now */)
{
    int64_t highest_avg = 0;
    for (Core& c : mCores)
    {
        long freq = 0;
        int64_t sum = 0;
        int64_t values = 0;
        if (c.time_in_state)
        {
            int idx = 0;
            long times = 0;
            rewind(c.time_in_state);
            while (fscanf(c.time_in_state, "%ld %ld\n", &freq, &times) == 2)
            {
                const int64_t relative = times - c.times[idx]; // want value relative to last sampling point
                sum += relative * c.frequencies[idx];
                values += relative;
                c.times[idx] = times;
                idx++;
            }
        }
        else
        {
            rewind(c.freq_file);
            while (fscanf(c.freq_file, "%ld\n", &freq) == 1)
            {
                sum = freq;
                values = 1;
            }
        }
        if (sum == 0) // this can happen - time_in_state updates relatively slowly - so reuse previous result
        {
            if (mResults[c.corename].size() == 0) // no previous result?
            {
                // Just read the current frequency
                rewind(c.freq_file);
                int r = fscanf(c.freq_file, "%ld", &freq);
                (void)r; // elaborate way to silence compiler
                sum = freq;
            }
            else
            {
                sum = mResults[c.corename].back().i64; // reuse
            }
            values = 1;
        }
        const int64_t avg = sum / values;
        add(c.corename, avg);
        highest_avg = avg > highest_avg ? avg : highest_avg;
    }
    add("highest_avg", highest_avg);
    return true;
}

bool CPUFreqCollector::available()
{
    return true; // TBD: check if we are on MacOSX or something equally obscure
}
