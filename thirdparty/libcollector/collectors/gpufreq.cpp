#include "gpufreq.hpp"

#include <stdio.h>

static const std::vector<std::string> options =
{
    "/sys/devices/platform/gpusysfs/gpu_clock", // note 4s, Samsung Galaxy S7 - Mali
    "/sys/devices/e8970000.mali/devfreq/gpufreq/cur_freq", // Huawei Mate8
    "/sys/devices/platform/e82c0000.mali/devfreq/gpufreq/cur_freq", // Huawei Mate 10 Pro
    "/sys/devices/platform/mali.0/clock",
    "/sys/devices/platform/mali-t604.0/clock",
    "/sys/devices/ffa30000.gpu/clock", // firefly
    "/sys/class/devfreq/gk20a.0/cur_freq", // nvidia tk-1
    "/sys/devices/soc/b00000.qcom,kgsl-3d0/kgsl/kgsl-3d0/gpuclk", // Samsung Galaxy S7 - Adreno
    "/sys/devices/soc.0/fdb00000.qcom,kgsl-3d0/kgsl/kgsl-3d0/gpuclk", // LG Flex 2
    "/sys/devices/soc.0/1c00000.qcom,kgsl-3d0/devfreq/1c00000.qcom,kgsl-3d0/cur_freq", // Adreno 510
    "/sys/devices/platform/soc/5000000.qcom,kgsl-3d0/kgsl/kgsl-3d0/gpuclk", // Adreno 630
    "/sys/module/mali/parameters/mali_gpu_clk", // utgard
    "/sys/kernel/gpu/gpu_clock", // standardized path in Android
    "/sys/kernel/debug/clk/clk-g3d/clk_rate", // hikey960
    "/sys/devices/platform/soc/3d00000.qcom,kgsl-3d0/kgsl/kgsl-3d0/devfreq/cur_freq", // S21 Adreno
};

GPUFreqCollector::GPUFreqCollector(const Json::Value& config, const std::string& name)
    : SysfsCollector(config, name, options)
{
    if (mConfig.isMember("path"))
    {
        mOptions.insert(mOptions.begin(), config["gpufreq"]["path"].asString());
    }
}

bool GPUFreqCollector::parse(const char* buffer)
{
    const int ONE_MILLION = 1000000;
    // old driver?
    int freq;
    int ret = sscanf(buffer, "Current sclk_g3d[G3D_BLK] = %dMhz", &freq);
    if (ret != 1)
    {
        ret = sscanf(buffer, "Current clk mali = %dMhz", &freq); // firefly
    }
    if (ret != 1)
    {
        // no, new driver, which has only a simple number
        char* end;
        freq = strtol(buffer, &end, 10);
        if (buffer == end)
        {
            return false;
        }
        if (freq < 0)
        {
            freq = 0;
        }
        // on qcom, they store hz, not mhz... so transform to mhz
        if (freq > ONE_MILLION)
        {
            freq /= ONE_MILLION;
        }
    }
    add("gpufreq", freq * 1000);

    return true;
}
