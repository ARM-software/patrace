#pragma once

#include <string>
#include <vector>
#include "interface.hpp"
#include "collector_utility.hpp"
#include "hwcpipe.hpp"
#include "hwc_names.hpp"


struct InfoCapsule{
	InfoCapsule();
	mali_userspace::MaliHWInfo info;
};


class MaliCounterCollector : public Collector
{
public:
    using Collector::Collector;

    MaliCounterCollector();

    MaliCounterCollector(const Json::Value& config, const std::string& name);

    bool init() override;
    bool deinit() override;
    bool start() override;
    bool collect(int64_t) override;
    bool available() override;

private:
	
	InfoCapsule infoc;
	mali_userspace::MaliHWCReader counter_reader;

	std::vector<std::string> header;
	std::vector<std::pair<mali_userspace::MaliCounterBlockName, int>> indices;
	std::vector<int> core_indices;

	std::vector<float> counter_buffer;

	size_t num_counters;

	void build_block_vectors(
		mali_userspace::MaliCounterBlockName block,
		int block_size,
		const std::string& verbose_block_name,
		int core = -1
	);
};
