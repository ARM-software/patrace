#pragma once

#include <vector>
#include <sys/time.h>
#include <sys/resource.h>

#include "collector_utility.hpp"
#include "interface.hpp"

class PerfRecordCollector : public Collector
{
public:
    using Collector::Collector;

    virtual bool init() override;
    virtual bool deinit() override;
    virtual bool start() override;
    virtual bool stop() override;
    virtual bool collect(int64_t) override;
    virtual bool available() override;

private:
    pid_t mPid = 0;
    pid_t mChild = 0;
};
