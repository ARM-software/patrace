#pragma once

#include <stdint.h>
#include <vector>
#include <sys/time.h>
#include <sys/resource.h>

#include "interface.hpp"

class MemoryCollector : public Collector
{
public:
    using Collector::Collector;

    virtual bool init() override;
    virtual bool deinit() override;
    virtual bool start() override;
    virtual bool collect(int64_t) override;
    virtual bool available() override;

private:
    int64_t initialAvailableRAM = 0;
};
