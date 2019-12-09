#include "cputemp.hpp"

#include "collector_utility.hpp"

#include <string.h>
#include <stdio.h>
#include <map>

template<typename TK, typename TV>
std::vector<TK> extract_keys(std::map<TK, TV> const& input_map)
{
    std::vector<TK> retval;
    for (auto const& element : input_map)
    {
        retval.push_back(element.first);
    }
    return retval;
}

static const std::map<std::string, double> option_map =
{
    { "/sys/devices/platform/exynos5-tmu/curr_temp", 10.0 },
    { "/sys/devices/platform/exynos5-tmu/temp", 10.0 }, // arndale octa
    { "/sys/class/thermal/thermal_zone1/temp", 10.0 }, // nvidia tk-1
    { "/sys/devices/virtual/thermal/thermal_zone0/temp", 100.0 }, // desktop (for testing)
    { "class/thermal/thermal_zone0/temp", 100.0 }, // teclast tpad-1
};

static const std::vector<std::string> options = extract_keys(option_map);

CPUTemperatureCollector::CPUTemperatureCollector(const Json::Value& config, const std::string& name)
    : SysfsCollector(config, name, options)
{
}

bool CPUTemperatureCollector::parse(const char* buffer)
{
    const char *end = buffer;
    int count = 0;
    do
    {
        char *tmp = NULL;
        const char *prefix = strchr(end, ':');
        if (prefix)
        {
            end = ++prefix; // skip any prefix
        }
        double temp = strtol(end, &tmp, 10);
        if (tmp == buffer)
        {
            return count > 0; // if we read any, we are good; we will try to read all the numbers
        }
        end = tmp;
        if (*end != '\0') end++; // skip delimiter, unless at end of string
        add("cpu_temperature_" + _to_string(count), temp / option_map.at(mSysfsFile));
        count++;
    }
    while (end != NULL && *end != '\0');
    return true;
}
