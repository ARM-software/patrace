#pragma once

#include "retracer/glws.hpp"
#include "retracer/retracer.hpp"
#include "retracer/retrace_api.hpp"
#include "tool/parse_interface.h"
#include "json/json.h"

#include <unordered_set>
#include <unordered_map>

typedef std::unordered_map<std::string, std::string> cache_type;

struct RenderpassJson
{
    Json::Value data; // final output
    std::string filename;
    std::string dirname;
    std::unordered_set<int> stored_programs;
    bool started = false;

    cache_type shader_cache;
    cache_type index_cache;
    cache_type vertex_cache;

    RenderpassJson() {}
};

class ParseInterfaceRetracing : public ParseInterfaceBase
{
public:
    ParseInterfaceRetracing() : ParseInterfaceBase(), mCall(NULL) {}
    ~ParseInterfaceRetracing() { delete mCall; }

    virtual DrawParams getDrawCallCount(common::CallTM *call) override;
    virtual bool open(const std::string& input, const std::string& output = std::string()) override;
    virtual void close() override;
    virtual common::CallTM* next_call() override;
    virtual void loop(Callback c, void *data) override;

    virtual int64_t getCpuCycles() { return mCpuCycles; }

    virtual void completed_drawcall(int frame, const DrawParams& params, const StateTracker::RenderPass &rp);
    virtual void completed_renderpass(const StateTracker::RenderPass &rp);

private:
    void thread(const int threadidx, const int our_tid, Callback c, void *data);

    RenderpassJson mRenderpass;
    int64_t mCpuCycles = 0;
    common::CallTM* mCall;
};
