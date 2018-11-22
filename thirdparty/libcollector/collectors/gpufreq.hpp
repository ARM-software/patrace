#pragma once

#include <vector>

#include "interface.hpp"

class GPUFreqCollector : public SysfsCollector
{
public:
    GPUFreqCollector(const Json::Value& config, const std::string& name);

protected:
    virtual bool parse(const char* buffer) override;
};
