#pragma once

#include <assert.h>
#include <vector>
#include <string>
#include <cmath>
#include <map>
#include <stdint.h>
#include <stdlib.h>
#include <atomic>
#include <thread>
#include <pthread.h>
#include <chrono>

#include <jsoncpp/json/value.h>

#ifndef DBG_LOG
#ifdef ANDROID
#include <android/log.h>
#define DBG_LOG(...) __android_log_print(ANDROID_LOG_INFO, "LIBCOLLECTOR", __VA_ARGS__)
#else
#define DBG_LOG(...) do { fprintf(stdout, "LIBCOLLECTOR :: FILE: %s, LINE: %d, INFO: ", __FILE__, __LINE__);printf(__VA_ARGS__); } while(0)
#endif
#endif

// Value
union CollectorValue
{
    double fp64;
    uint64_t u64;
    int64_t i64;
};

// Value list
struct CollectorValueList
{
    enum vtype
    {
        TYPE_FP64,
        TYPE_U64,
        TYPE_I64,
        TYPE_UNASSIGNED
    };
    enum vtype type = TYPE_UNASSIGNED;
    std::vector<CollectorValue> list;
    std::vector<CollectorValue> summaries;

    void push_back(double val) { assert(type == TYPE_UNASSIGNED || type == TYPE_FP64); type = TYPE_FP64; CollectorValue fp64; fp64.fp64 = val; list.push_back(fp64); }
    void push_back(float val) { assert(type == TYPE_UNASSIGNED || type == TYPE_FP64); type = TYPE_FP64; CollectorValue fp64; fp64.fp64 = val; list.push_back(fp64); }
    void push_back(int val) { assert(type == TYPE_UNASSIGNED || type == TYPE_I64); type = TYPE_I64; CollectorValue i64; i64.i64 = val; list.push_back(i64); }
    void push_back(long val) { assert(type == TYPE_UNASSIGNED || type == TYPE_I64); type = TYPE_I64; CollectorValue i64; i64.i64 = val; list.push_back(i64); }
    void push_back(long long val) { assert(type == TYPE_UNASSIGNED || type == TYPE_I64); type = TYPE_I64; CollectorValue i64; i64.i64 = val; list.push_back(i64); }
    void push_back(unsigned int val) { assert(type == TYPE_UNASSIGNED || type == TYPE_U64); type = TYPE_U64; CollectorValue u64; u64.u64 = val; list.push_back(u64); }
    void push_back(unsigned long val) { assert(type == TYPE_UNASSIGNED || type == TYPE_U64); type = TYPE_U64; CollectorValue u64; u64.u64 = val; list.push_back(u64); }
    void push_back(CollectorValue val) { assert(type != TYPE_UNASSIGNED); list.push_back(val); }

    void summarize()
    {
        switch (type)
        {
        case TYPE_FP64: { double s = 0.0; for (const auto v : list) s += v.fp64; CollectorValue c; c.fp64 = s / (double)list.size(); summaries.push_back(c); list.clear(); } break;
        case TYPE_I64: { int64_t s = 0; for (const auto v : list) s += v.i64; CollectorValue c; c.i64 = s / (int64_t)list.size(); summaries.push_back(c); list.clear(); } break;
        case TYPE_U64: { uint64_t s = 0; for (const auto v : list) s += v.u64; CollectorValue c; c.u64 = s / (uint64_t)list.size(); summaries.push_back(c); list.clear(); } break;
        case TYPE_UNASSIGNED: assert(false); break;
        }
    }
    void clear() { list.clear(); }
    size_t size() const { return list.size(); }
    CollectorValue at(int index) const { return list.at(index); }
    const std::vector<CollectorValue>& data() const { if (summaries.size() > 0) return summaries; else return list; }
};

typedef std::map<std::string, CollectorValueList> CollectorValueResults;

// General collector class
class Collector
{
public:
    Collector(const Json::Value& config, const std::string& name)
        : finished(false), mSampleRate(100), mCollecting(false), mConfig(config.get(name, Json::Value()))
        , mName(name), mFactor(NAN) {}
    virtual ~Collector() {}

    virtual bool init() { return true; }
    virtual bool deinit() { return true; }

    virtual bool start() { mCollecting = true; return true; }
    virtual bool stop() { mCollecting = false; return true; }
    virtual bool postprocess(const std::vector<int64_t>& timing);
    virtual bool collect( int64_t ) = 0;
    virtual bool collecting() const { return mCollecting; }
    virtual const std::string& name() const { return mName; }
    virtual bool available() = 0;

    virtual const CollectorValueResults& results() const final { return mResults; }
    virtual const Json::Value customResults() const final { return mCustomResult; }
    virtual void doubleTransform(double factor) final { mFactor = factor; }
    virtual void useThreading(int sampleRate) final;
    virtual bool isThreaded() const final { return mIsThreaded; }
    virtual bool isSummarized() const final { return mIsSummarized; }

    virtual void summarize()
    {
        for (auto& pair : mResults)
        {
            pair.second.summarize();
        }
        mIsSummarized = true;
    }

    /// Subclass overrides of this should call this parent function
    virtual void clear()
    {
        for (auto& pair : mResults)
        {
            pair.second.clear();
        }
    }

    virtual void add(const std::string& key, double value) final { mResults[key].push_back(value); }
    virtual void add(const std::string& key, float value) final { mResults[key].push_back(value); }
    virtual void add(const std::string& key, int value) final { mResults[key].push_back(value); }
    virtual void add(const std::string& key, long value) final { mResults[key].push_back(value); }
    virtual void add(const std::string& key, long long value) final { mResults[key].push_back(value); }
    virtual void add(const std::string& key, unsigned value) final { mResults[key].push_back(value); }
    virtual void add(const std::string& key, unsigned long value) final { mResults[key].push_back(value); }

    /// For multi-threaded operation, this loop is called instead of owning class calling collect() directly.
    virtual void loop() final;

    /// If threaded, this holds the thread information
    std::thread thread;
    /// Set this to true in order to stop collecting data
    std::atomic<bool> finished;

    virtual void setDebug(bool debug) final { mDebug = debug; }

protected:
    /// In debug mode?
    bool mDebug = false;
    /// Is this collector multi-threaded?
    bool mIsThreaded = false;
    /// Is this collector being summarized?
    bool mIsSummarized = false;
    /// Ideal sample rate in milliseconds
    int mSampleRate;
    /// Are we collecting?
    bool mCollecting;
    /// Data for each sampling point
    CollectorValueResults mResults;
    /// Configuration JSON from the Collection super-class.
    Json::Value mConfig;
    /// Name of collector
    std::string mName;
    /// Adjustment value for input data (if floating point). Used in inherited classes (if at all).
    double mFactor;
    /// Custom results (replaces sampling points)
    Json::Value mCustomResult;
};

// Specialized collector class for handling /sys filesystem polling
class SysfsCollector : public Collector
{
public:
    SysfsCollector(const Json::Value& config, const std::string& name, const std::vector<std::string>& sysfsfiles, bool accumulative = false);
    ~SysfsCollector();

    virtual bool init();
    virtual bool collect(int64_t);
    virtual bool available() { init(); return mFD >= 0; }

protected:
    virtual bool parse(const char* buffer);
    std::string mSysfsFile;
    std::vector<std::string> mOptions;

private:
    int mFD = -2;
    bool mAccumulative = false;
};

// Manager class
class Collection
{
public:
    Collection(const Json::Value& config);
    ~Collection();

    /// Return a list of functional collectors for this platform.
    std::vector<std::string> available();

    /// Return a list of non-functional collectors for this platform.
    std::vector<std::string> unavailable();

    /// Set verbose debug output flag
    void setDebug(bool value) { mDebug = value; for (auto* c : mCollectors) c->setDebug(true); }

    /// Initialize the given list of collectors. Non-functional collectors will be quietly ignored.
    /// Also, any collectors mentioned in the JSON passed to the constructor will be enabled, if
    /// they are functional. If a collector mentioned in the JSON has a 'required' key set to true,
    /// and that collector is not functional, then no collectors will be initialized and this function
    /// returns false.
    bool initialize(std::vector<std::string> collectors = std::vector<std::string>());

    /// Initialize a single collector. Attempts to initialize a collector more than once is ignored.
    bool initialize_collector(const std::string& name);

    /// Return reference to a named collector.
    Collector* collector(const std::string& name);

    /// Add custom collector
    void addCollector(Collector* collector)
    {
        mCollectors.push_back(collector);
    }

    /// Add generic sysfs collector
    void addSysfsCollector(const std::string& name, std::vector<std::string> sysfsfiles)
    {
        SysfsCollector* collector = new SysfsCollector(mConfig, name, sysfsfiles);
        mCollectors.push_back((Collector*)collector);
    }

    /// Write out the data to file as CSV (data in rows)
    bool writeCSV(const std::string& filename);

    /// Write out the data to file as CSV in the MTV format (data in columns)
    bool writeCSV_MTV(const std::string& filename);

    /// Clear any old results and start collecting data. If the optional customHeaders
    /// vector is passed in, this defines custom data that must be passed in through
    /// later calls to collect().
    void start(const std::vector<std::string>& customHeaders = std::vector<std::string>());

    /// Stop collecting data
    void stop();

    /// Summarize existing data as an average. Useful for looping tests. Once this has been
    /// called once, what you get out with results() later will be these averages.
    void summarize();

    /// Check if any collector is running
    bool is_running() const { return running; }

    /// Collect one data point. If result is specified, this is used to specify a custom
    /// result value.
    void collect(std::vector<int64_t> custom = std::vector<int64_t>());

    /// Get the results as JSON
    Json::Value results();

    const Json::Value& config() { return mConfig; }

private:
    bool running = false;
    Json::Value mConfig;
    std::vector<Collector*> mCollectors;
    std::vector<Collector*> mRunning;
    std::map<std::string, Collector*> mCollectorMap;
    std::vector<int64_t> mTiming;
    std::vector<int64_t> mTimingSummarized;
    std::vector<std::vector<int64_t>> mCustom; // custom results
    std::vector<std::vector<int64_t>> mCustomSummarized; // custom results
    std::vector<std::string> mCustomHeaders;
    int64_t mStartTime = 0;
    int64_t mPreviousTime = 0;
    bool mDebug = false;
};
