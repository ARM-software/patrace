#include "streamline.hpp"

#include <stdio.h>

#include "collector_utility.hpp"
#include "streamline_annotate.h"

bool StreamlineCollector::init()
{
    ANNOTATE_SETUP;
    return true;
}

bool StreamlineCollector::available()
{
    // TBD: Find a way to tell if gator is actually running
    return true;
}

bool StreamlineCollector::deinit()
{
    return true;
}

bool StreamlineCollector::start()
{
    if (mCollecting)
    {
        return true;
    }
    mCollecting = true;
    ANNOTATE_CHANNEL(0, "Measured frames");
    return true;
}

bool StreamlineCollector::stop()
{
    if (mCollecting)
    {
        ANNOTATE_CHANNEL_END(0);
    }
    mCollecting = false;
    return true;
}

bool StreamlineCollector::postprocess(const std::vector<int64_t>& timing)
{
    // We need to override the parent postprocess, since we have no data.
    return true;
}

bool StreamlineCollector::collect(int64_t /* now */)
{
    return true;
}
