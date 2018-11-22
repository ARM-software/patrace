#pragma once
#include <vector>
#include <stdint.h>
#include "hwc.hpp"

namespace mali_userspace
{
struct MaliHWInfo
{
	bool valid;
	unsigned mp_count;
	unsigned gpu_id;
	unsigned r_value;
	unsigned p_value;
	unsigned core_mask;
};

// Grabs Mali HW info by looking in /sys/devices.
bool get_mali_hw_info(const char *path, MaliHWInfo &hw_info);

class MaliHWCReader
{
public:
	MaliHWCReader(const MaliHWInfo &info, const char *device, uint32_t interval_ns);
	~MaliHWCReader();

	// Checks if the HWC reader is valid.
	bool is_alive() const;

	// Get the HWC fd in case you want to include the HWC fd in epoll or something.
	int get_hwc_fd() const;

	// Gets the buffer size of one HWC payload.
	uint32_t get_buffer_size() const;

	// Gets the number of buffers.
	uint32_t get_buffer_count() const;

	// Get HW counter interface version (usually 4 or 5).
	uint32_t get_hardware_counter_version() const;

	// Gets the number of cores, as specified by MaliHWInfo.
	uint32_t get_num_cores() const;

	// Gets all counter names
	const char * const *get_counter_names() const;

	// Gets all counter names for the given block
	const char * const *get_counter_names(MaliCounterBlockName block) const;

	// Gets the counter name for a counter block and index.
	const char *get_counter_name(MaliCounterBlockName block, unsigned index) const;

	// Finds the first counter which contains the name in "name".
	int find_counter_index_by_name(MaliCounterBlockName block, const char *name);

	// Gets a flat array of the HWC payload for the latest event polled by wait_next_event().
	const uint32_t *get_counters() const;

	// Gets (64) counters associated with a specific block and core.
	const uint32_t *get_counters(MaliCounterBlockName block, unsigned core = 0) const;

	// Gets the counter index which can be used to index into get_counters() directly.
	uint32_t get_counter_index(MaliCounterBlockName block, unsigned index, unsigned core = 0) const;

	// Waits until next HWC data has been received and reads the data.
	bool wait_next_event();

	// Remap a virtual core ID [0, 1, ..., N] to physical core ID (might have holes on some platforms).
	unsigned remap_core_index(unsigned index) const;

	// Gets the timestamp for last event fetched by wait_next_event()
	uint64_t get_timestamp_ns() const;

	// Non-copyable, non-movable.
	void operator=(MaliHWCReader&&) = delete;
	MaliHWCReader(MaliHWCReader&&) = delete;

private:
	MaliHWInfo hw;
	int fd = -1;
	int hwc_fd = -1;
	uint32_t buffer_size = 0;
	uint32_t buffer_count = 0;
	uint32_t hw_ver = 0;
	uint8_t *sample_data = nullptr;

	uint64_t timestamp = 0;
	std::vector<uint32_t> counter_buffer;
	const char * const *names_lut = nullptr;
	bool alive = false;

	bool init(const char *device, unsigned buffer_count, uint32_t interval_ns);
	void term();
	std::vector<unsigned> core_index_remap;
};

}
