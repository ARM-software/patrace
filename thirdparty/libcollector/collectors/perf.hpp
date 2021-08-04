#pragma once

#include "collector_utility.hpp"
#include "interface.hpp"
#include <map>

enum hw_cnt_length
{
    b32 = 0,
    b64,
};

struct snapshot {
    snapshot() : size(0) {}

    uint64_t size;
    long long values[8] = {0};
};

struct event {
    std::string name;
    uint32_t type;
    uint64_t config;
    bool exc_user;    // default is false
    bool exc_kernel;  // default is false
    enum hw_cnt_length len; // default is 32bit pmu counter
};

class event_context
{
public:
    event_context()
    {
        group = -1;
    }

    ~event_context() {}

    bool init(int tid, std::vector<struct event> &events);
    bool start();
    struct snapshot collect(int64_t now);
    bool stop();
    bool deinit();

    inline void update_data(struct snapshot &snap, CollectorValueResults &result)
    {
        for (unsigned int i = 0; i < mCounters.size(); i++)
            result[mCounters[i].name].push_back(snap.values[i]);
    }

private:
    struct counter
    {
        std::string name;
        int fd;
    };

    int group;
    std::vector<struct counter> mCounters;
};

class PerfCollector : public Collector
{
public:
    PerfCollector(const Json::Value& config, const std::string& name);

    virtual bool init() override;
    virtual bool deinit() override;
    virtual bool start() override;
    virtual bool stop() override;
    virtual bool collect(int64_t) override;
    virtual bool available() override;

    virtual void clear() override;
    virtual bool postprocess(const std::vector<int64_t>& timing) override;
    virtual void summarize() override;

private:
    void create_perf_thread();
    void saveResultsFile();

private:
    int mSet = -1;
    bool mAllThread = true;
    std::vector<struct event> mEvents;

    struct perf_thread
    {
        perf_thread(const int tid, const std::string &name): tid(tid), name(name), eventCtx{} {}

        void update_data(struct snapshot& snap)
        {
            eventCtx.update_data(snap, mResultsPerThread);
        }

        void clear()
        {
            for (auto& pair : mResultsPerThread)
                pair.second.clear();
        }

        void postprocess(Json::Value& value)
        {
            Json::Value v;
            Json::Value acc;
            for (const auto& pair : mResultsPerThread)
            {
                bool need_sum = false;
                if (value.isMember(pair.first)) need_sum = true;
                v[pair.first] = Json::arrayValue;

                unsigned int index = 0;
                long long total = 0;
                for (const CollectorValue& cv : pair.second.data())
                {
                    long long s = cv.i64;
                    if (need_sum) s += value[pair.first][index++].asInt64();
                    v[pair.first].append(s);
                    total += s;
                }
                value[pair.first] = v[pair.first];
                acc[pair.first] = total;
            }
            value["SUM"] = acc;
        }

        void summarize()
        {
            for (auto& pair : mResultsPerThread)
            {
                pair.second.summarize();
            }
        }

        const int tid;
        const std::string name;
        event_context eventCtx;
        CollectorValueResults mResultsPerThread;
    };

    std::vector<struct perf_thread> mReplayThreads;
    std::vector<struct perf_thread> mBgThreads;
};
