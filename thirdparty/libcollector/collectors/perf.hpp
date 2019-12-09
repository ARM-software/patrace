#pragma once

#include "interface.hpp"

class PerfCollector : public Collector
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
    int mPerfFD = -1;
};
