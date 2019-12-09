#pragma once

#include <vector>
#include <string>
#include <set>

#include "interface.hpp"

class StreamlineCollector : public Collector
{
public:
    using Collector::Collector;

    virtual bool init() override;
    virtual bool deinit() override;
    virtual bool start() override;
    virtual bool stop() override;
    virtual bool postprocess(const std::vector<int64_t>& timing) override;
    virtual bool collect(int64_t) override;
    virtual bool available() override;
};
