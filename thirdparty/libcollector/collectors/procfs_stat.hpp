#pragma once

#include "interface.hpp"

/* not really sysfs, but same principle */
class ProcFSStatCollector : public SysfsCollector
{
public:
    ProcFSStatCollector(const Json::Value& config, const std::string& name);

protected:
    virtual bool parse(const char* buffer) override;

private:
    long mTicks;
    unsigned long mLastSampleTime;
};
