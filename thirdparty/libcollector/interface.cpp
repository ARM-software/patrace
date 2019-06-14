#include "interface.hpp"

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <chrono>

#ifndef __APPLE__
#include "collectors/perf.hpp"
#if !defined(ANDROID)
#include "collectors/perf_record.hpp"
#endif
#include "collectors/cpufreq.hpp"
#include "collectors/gpufreq.hpp"
#include "collectors/procfs_stat.hpp"
#include "collectors/cputemp.hpp"
#include "collectors/memory.hpp"
#include "collectors/debug.hpp"
#include "collectors/power.hpp"
#include "collectors/ferret.hpp"
#if defined(ANDROID) || defined(__ANDROID__)
#include "collectors/streamline.hpp"
#endif
#include "collectors/mali_counters.hpp"
#endif // __APPLE__
#include "collectors/rusage.hpp"

// ---------- Utilities ----------

static int64_t getTime()
{
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
}

// ---------- COLLECTOR ----------

void Collector::useThreading(int sampleRate)
{
    assert(!mCollecting);
    mIsThreaded = true;
    mSampleRate = sampleRate;
}

void Collector::loop()
{
    while (!finished)
    {
        int64_t t1 = getTime();
        collect( t1 );
        int64_t t2 = getTime();

        auto duration = std::chrono::microseconds( t2 - t1 );

        if (!finished && duration < std::chrono::milliseconds(mSampleRate))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(mSampleRate) - duration);
        }
    }
}

// This post-processing assumes that the sampling done in a threaded collector is
// fairly uniform. If it is not, we will need to collect timestamps from it as well,
// in order to do the matching, which has its own costs.
bool Collector::postprocess(const std::vector<int64_t>& timing)
{
    if (mIsThreaded) // remix values based on frame times
    {
        double duration = 0;
        for (const int64_t v : timing)
        {
            duration += v;
        }
        CollectorValueResults tmp;
        for (const auto& kv : mResults)
        {
            if (kv.second.size() == 0)
            {
                return false;
            }
            const double samples = kv.second.size();
            int64_t cur_time = 0;
            for (unsigned i = 0; i < timing.size(); i++)
            {
                // find closest match
                const int index = trunc((double)cur_time / duration * samples);
                cur_time += timing.at(i);
                tmp[kv.first].push_back(kv.second.at(index));
            }
        }
        mResults = tmp;
    }
    return true;
}

// ---------- SYSFS COLLECTOR ----------

SysfsCollector::SysfsCollector(const Json::Value& config, const std::string& name, const std::vector<std::string>& sysfsfiles, bool accumulative)
    : Collector(config, name), mOptions(sysfsfiles), mAccumulative(accumulative)
{
    (void)mAccumulative; // TBD - use me
}

bool SysfsCollector::parse(const char* buffer)
{
    char *end;
    long temp = strtol(buffer, &end, 10);
    if (!*end)
    {
        return false;
    }
    if (std::isnan(mFactor))
    {
        add(mName, temp);
    }
    else
    {
        add(mName, temp * mFactor);
    }
    return true;
}

bool SysfsCollector::collect(int64_t now)
{
    char buf[1024];

    assert(mFD != -1);
    memset(buf, 0x00, sizeof(buf));

    if (read(mFD, buf, sizeof(buf) - 1) == -1)
    {
        perror("read");
        return false;
    }

    if (lseek(mFD, 0, SEEK_SET) == -1)
    {
        perror("lseek");
        return false;
    }

    if (!parse(buf))
    {
        DBG_LOG("%s: Read garbage from %s: \"%s\"\n", mName.c_str(), mSysfsFile.c_str(), buf);
        return false;
    }

    return true;
}

bool SysfsCollector::init()
{
    if (mFD == -2)
    {
        for (const std::string& s : mOptions)
        {
            mFD = open(s.c_str(), O_RDONLY);
            if (mFD < 0)
            {
                mFD = -1;
                continue;
            }
            mSysfsFile = s;
            DBG_LOG("%s: Successfully opened %s\n", mName.c_str(), s.c_str());
            break;
        }
    }

    return mFD != -1;
}

SysfsCollector::~SysfsCollector()
{
    deinit();
    if (mFD != -1)
    {
        close(mFD);
    }
    mFD = -1;
    mCollecting = false;
}

// ---------- COLLECTION ----------

Collection::Collection(const Json::Value& config) : mConfig(config)
{
#ifndef __APPLE__
    mCollectors.push_back(new PerfCollector(config, "perf"));
#if !defined(ANDROID)
    mCollectors.push_back(new PerfRecordCollector(config, "perf_record"));
#endif
    mCollectors.push_back(new SysfsCollector(config, "battery_temperature",
        { "/sys/class/power_supply/battery/temp",
          "/sys/devices/platform/android-battery/power_supply/android-battery/temp", // Nexus 10
          "/sys/class/power_supply/battery/batt_temp" })); // teclast tpad-1
    mCollectors.push_back(new CPUFreqCollector(config, "cpufreq"));
    mCollectors.push_back(new SysfsCollector(config, "memfreq",
        { "/sys/class/devfreq/exynos5-busfreq-mif/cur_freq", // note 3
          "/sys/class/devfreq/exynos5-devfreq-mif/cur_freq", // note 4
          "/sys/devices/17000010.devfreq_mif/devfreq/17000010.devfreq_mif/cur_freq" })); // Mali S7
    mCollectors.push_back(new SysfsCollector(config, "memfreqdisplay",
        { "/sys/devices/17000030.devfreq_disp/devfreq/17000030.devfreq_disp/cur_freq" })); // Mali S7
    mCollectors.push_back(new SysfsCollector(config, "memfreqint",
        { "/sys/class/devfreq/exynos5-busfreq-int/cur_freq",    // note 3
          "/sys/class/devfreq/exynos5-devfreq-int/cur_freq", // note 4
          "/sys/devices/17000020.devfreq_int/devfreq/17000020.devfreq_int/cur_freq" })); // Mali S7
    mCollectors.push_back(new SysfsCollector(config, "gpu_active_time",
        { "/sys/devices/platform/mali.0/power/runtime_active_time",    // mali
          "/sys/devices/platform/pvrsrvkm.0/power/runtime_active_time", // power-vr
          "/sys/devices/virtual/graphics/fb0/power/runtime_active_time" }, // adreno
        true)); // accumulative value
    mCollectors.push_back(new SysfsCollector(config, "gpu_suspended_time",
        { "/sys/devices/platform/mali.0/power/runtime_suspended_time",    // mali
          "/sys/devices/platform/pvrsrvkm.0/power/runtime_suspended_time", // power-vr
          "/sys/devices/virtual/graphics/fb0/power/runtime_suspended_time" }, // adreno (but only for framebuffer zero!)
        true)); // accumulative value
    mCollectors.push_back(new SysfsCollector(config, "cpufreqtrans",
        { "/sys/devices/system/cpu/cpu0/cpufreq/stats/total_trans" },
        true)); // accumulative value
    mCollectors.push_back(new DebugCollector(config, "debug"));
#if defined(ANDROID) || defined(__ANDROID__)
    mCollectors.push_back(new StreamlineCollector(config, "streamline"));
#endif
    mCollectors.push_back(new MemoryCollector(config, "memory"));
    mCollectors.push_back(new CPUTemperatureCollector(config, "cputemp"));
    mCollectors.push_back(new GPUFreqCollector(config, "gpufreq"));
    mCollectors.push_back(new PowerDataCollector(config, "power"));
    mCollectors.push_back(new FerretCollector(config, "ferret"));
    mCollectors.push_back(new ProcFSStatCollector(config, "procfs"));
    mCollectors.push_back(new MaliCounterCollector(config, "malicounters"));
#endif
    mCollectors.push_back(new RusageCollector(config, "rusage"));

    for (Collector* c : mCollectors)
    {
        if (mDebug)
        {
            DBG_LOG("Enabled collector %s\n", c->name().c_str());
        }
        mCollectorMap[c->name()] = c;
    }

    // Various specializations
    mCollectorMap["battery_temperature"]->doubleTransform(0.1); // divide by 10 and store as float
}

Collection::~Collection()
{
    for (Collector* c : mCollectors)
    {
        c->deinit();
        delete c;
    }
}

std::vector<std::string> Collection::available()
{
    std::vector<std::string> list;
    for (Collector* c : mCollectors)
    {
        if (c->available())
        {
            list.push_back(c->name());
        }
    }
    return list;
}

std::vector<std::string> Collection::unavailable()
{
    std::vector<std::string> list;
    for (Collector* c : mCollectors)
    {
        if (!c->available())
        {
            list.push_back(c->name());
        }
    }
    return list;
}

void Collection::initialize_collector(const std::string& name)
{
    for (const Collector* m : mRunning) // check if we already enabled it
    {
        if (m->name() == name)
        {
            return; // if so, ignore it
        }
    }
    mCollectorMap[name]->init();
    mRunning.push_back(mCollectorMap[name]);
    if (mConfig[name].get("threaded", false).asBool())
    {
        int sample_rate = mConfig[name].get("sample_rate", 100).asInt();
        mCollectorMap[name]->useThreading(sample_rate);
    }
}

bool Collection::initialize(std::vector<std::string> list)
{
    for (const std::string& s : list)
    {
        if (mCollectorMap.count(s) > 0 && mCollectorMap[s]->available())
        {
            initialize_collector(s);
        }
    }
    for (const std::string& s : mConfig.getMemberNames())
    {
        if (mCollectorMap.count(s) > 0 && mCollectorMap[s]->available())
        {
            initialize_collector(s);
        }
        else if (mConfig.isMember(s) && mConfig[s].isObject() && mConfig[s].get("required", false).asBool()) // is it required?
        {
            mRunning.clear();
            return false;
        }
    }
    return true;
}

Collector* Collection::collector(const std::string& name)
{
    return mCollectorMap[name];
}

void Collection::start(const std::vector<std::string>& headers)
{
    mTiming.clear();
    mCustom.clear();
    mCustomHeaders.clear();
    mStartTime = getTime();
    mPreviousTime = mStartTime;
    mCustomHeaders = headers;
    mCustom.resize(headers.size());
    for (Collector* c : mRunning)
    {
        c->clear();
        c->start();
        if (c->isThreaded())
        {
            c->finished = false;
            c->thread = std::thread(&Collector::loop, c);
            int failure = pthread_setname_np(
                c->thread.native_handle(),
                c->name().c_str());

            if (failure) {
                DBG_LOG("%s: Failed to set collector thread name, will inherit from parent process.\n", c->name().c_str());
            }
        }
    }

    running = true;
}

void Collection::stop()
{
    // First signal all threaded collectors to stop to end threaded measurements
    for (Collector* c : mRunning)
    {
        if (c->isThreaded())
        {
            c->finished = true;
        }
    }
    // Then stop all collectors (this can take some time)
    std::vector<Collector*> tmp;
    for (Collector* c : mRunning)
    {
        if (c->isThreaded())
        {
            c->thread.join();
        }
        c->stop();
        if (c->postprocess(mTiming))
        {
            tmp.push_back(c); // is valid result
        }
    }
    mRunning = tmp;

    running = false;
}

void Collection::collect(std::vector<int64_t> custom)
{
    const int64_t now = getTime();
    mTiming.push_back(now - mPreviousTime);
    mPreviousTime = now;
    for (Collector* c : mRunning)
    {
        if (!c->isThreaded())
        {
            c->collect( now );
        }
    }
    assert(custom.size() == mCustomHeaders.size());
    for (unsigned i = 0; i < mCustomHeaders.size(); i++)
    {
        mCustom[i].push_back(custom[i]);
    }
}

Json::Value Collection::results()
{
    Json::Value results;
    for (Collector* c : mRunning)
    {
        Json::Value v = c->customResults();
        if (!v.empty()) // overrides sampling data, if exists
        {
            results[c->name()] = v;
            continue;
        }
        for (const auto& pair : c->results())
        {
            v[pair.first] = Json::arrayValue;
            for (const CollectorValue& cv : pair.second)
            {
                switch (cv.type)
                {
                case CollectorValue::TYPE_FP64: v[pair.first].append(cv.fp64); break;
                case CollectorValue::TYPE_U64: v[pair.first].append(static_cast<Json::UInt64>(cv.u64)); break;
                case CollectorValue::TYPE_I64: v[pair.first].append(static_cast<Json::Int64>(cv.i64)); break;
                }
            }
        }
        results[c->name()] = v;
    }
    Json::Value v;
    v["time"] = Json::arrayValue;
    results["timing"] = v;
    for (int64_t t : mTiming)
    {
        results["timing"]["time"].append(static_cast<Json::Value::Int64>(t));
    }
    for (unsigned i = 0; i < mCustomHeaders.size(); i++)
    {
        results["custom"][mCustomHeaders[i]] = Json::arrayValue;
        for (int64_t t : mCustom[i])
        {
            results["custom"][mCustomHeaders[i]].append(static_cast<Json::Value::Int64>(t));
        }
    }
    if (mConfig.isMember("provenance")) // pass provenance through from config to results
    {
        results["provenance"] = mConfig["provenance"];
    }
    return results;
}

bool Collection::writeCSV_MTV(const std::string& filename)
{
    FILE *fp = fopen(filename.c_str(), "w");
    if (!fp)
    {
        fprintf(stderr, "FAILED to open %s: %s\n", filename.c_str(), strerror(errno));
        return false;
    }
    for (Collector* c : mRunning)
    {
        for (const auto& pair : c->results())
        {
            fprintf(fp, "%s, %s", c->name().c_str(), pair.first.c_str());
            for (const CollectorValue& value : pair.second)
            {
                switch (value.type)
                {
                case CollectorValue::TYPE_FP64: fprintf(fp, ", %f", value.fp64); break;
                case CollectorValue::TYPE_I64: fprintf(fp, ", %lld", (long long)value.i64); break;
                case CollectorValue::TYPE_U64: fprintf(fp, ", %llu", (unsigned long long)value.u64); break;
                }
            }
            fprintf(fp, "\n");
        }
    }
    fprintf(fp, "frametime, time");
    for (int64_t t : mTiming)
    {
        fprintf(fp, ", %lld", (long long)t);
    }
    for (unsigned i = 0; i < mCustomHeaders.size(); i++)
    {
        fprintf(fp, "\ncustom, %s", mCustomHeaders[i].c_str());
        for (int64_t t : mCustom[i])
        {
            fprintf(fp, ", %lld", (long long)t);
        }
    }
    return (fclose(fp) == 0);
}

bool Collection::writeCSV(const std::string& filename)
{
    FILE *fp = fopen(filename.c_str(), "w");
    if (!fp)
    {
        fprintf(stderr, "FAILED to open %s: %s\n", filename.c_str(), strerror(errno));
        return false;
    }
    int rows = 0;
    fprintf(fp, "frametime");
    for (unsigned field = 0; field < mCustomHeaders.size(); field++)
    {
        fprintf(fp, ", %s", mCustomHeaders[field].c_str());
    }
    for (Collector* c : mRunning)
    {
        for (const auto& pair : c->results())
        {
            fprintf(fp, ", %s:%s", c->name().c_str(), pair.first.c_str());
            if (pair.second.size() > 0)
            {
                rows = pair.second.size();
            }
        }
    }
    fprintf(fp, "\n");
    for (int i = 0; i < rows; i++)
    {
        fprintf(fp, "%lld", (long long)mTiming[i]);
        for (unsigned field = 0; field < mCustomHeaders.size(); field++)
        {
            fprintf(fp, ", %lld", (long long)mCustom[field][i]);
        }
        for (Collector* c : mRunning)
        {
            for (const auto& pair : c->results())
            {
                const CollectorValue& value = pair.second[i];
                switch (value.type)
                {
                case CollectorValue::TYPE_FP64: fprintf(fp, ", %f", value.fp64); break;
                case CollectorValue::TYPE_I64: fprintf(fp, ", %lld", (long long)value.i64); break;
                case CollectorValue::TYPE_U64: fprintf(fp, ", %llu", (unsigned long long)value.u64); break;
                }
            }
        }
        fprintf(fp, "\n");
    }
    return (fclose(fp) == 0);
}
