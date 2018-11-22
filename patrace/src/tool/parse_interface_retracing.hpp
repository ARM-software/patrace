#pragma once

#include "retracer/glws.hpp"
#include "retracer/retracer.hpp"
#include "retracer/retrace_api.hpp"
#include "tool/parse_interface.h"

class ParseInterfaceRetracing : public ParseInterfaceBase
{
public:
    ParseInterfaceRetracing() : ParseInterfaceBase(), mCall(NULL) {}
    ~ParseInterfaceRetracing() { delete mCall; }

    virtual DrawParams getDrawCallCount(common::CallTM *call) override;
    virtual bool open(const std::string& input, const std::string& output = std::string()) override;
    virtual void close() override;
    virtual common::CallTM* next_call() override;

    int64_t getCpuCycles() { return mCpuCycles; }

private:
    int64_t mCpuCycles = 0;
    common::CallTM* mCall;
};
