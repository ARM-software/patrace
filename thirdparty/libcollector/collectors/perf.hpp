#pragma once

#include "interface.hpp"
#include <map>

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

private:
    std::map<std::string, int> mCounters;
    int mSet = 0;
};
