#pragma once

#include "interface.hpp"

class CPUTemperatureCollector : public SysfsCollector
{
public:
    CPUTemperatureCollector(const Json::Value& config, const std::string& name);

protected:
    virtual bool parse(const char* buffer) override;
};
