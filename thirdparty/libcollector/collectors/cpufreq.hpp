#pragma once

#include <cassert>
#include <list>

#include "interface.hpp"

struct Core
{
    FILE *time_in_state = nullptr;
    FILE *freq_file = nullptr;
    int core = -1; // core number
    std::string corename;
    std::vector<int> frequencies; // states
    std::vector<int64_t> times; // times in state

    Core(FILE* tis, FILE* cf, int c, const std::string& n)
        : time_in_state(tis), freq_file(cf), core(c), corename(n) {}

    ~Core()
    {
        assert(time_in_state == nullptr);
        assert(freq_file == nullptr);
    }
};

class CPUFreqCollector : public Collector
{
    using Collector::Collector;

public:
    virtual bool init() override;
    virtual bool deinit() override;
    virtual bool start() override;
    virtual bool collect(int64_t) override;
    virtual bool available() override;

private:
    std::list<Core> mCores;
};
