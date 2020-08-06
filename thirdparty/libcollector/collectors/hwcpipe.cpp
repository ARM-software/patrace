#include "hwc.hpp"
#include "hwcpipe.hpp"
#include <stdio.h>
#include <dirent.h>
#include <string>
#include <vector>

namespace mali_userspace
{
bool get_mali_hw_info(const char *path, MaliHWInfo &hw_info)
{
	hw_info.valid = false;
	int fd = open(path, O_RDWR);
	if (fd < 0)
		return false;
	{
		uku_version_check_args version_check_args;
		version_check_args.header.id = UKP_FUNC_ID_CHECK_VERSION;
		version_check_args.major = 10;
		version_check_args.minor = 2;

		if (mali_ioctl(fd, version_check_args) != 0)
		{
			DBG_LOG("Failed to check version.\n");
			close(fd);
			return false;
		}

		DBG_LOG("Got version %u.%u.\n", version_check_args.major, version_check_args.minor);
	}

	{
		kbase_uk_hwcnt_reader_set_flags flags;
		memset(&flags, 0, sizeof(flags));
		flags.header.id = KBASE_FUNC_SET_FLAGS;
		flags.create_flags = BASE_CONTEXT_CREATE_KERNEL_FLAGS;
		if (mali_ioctl(fd, flags) != 0)
		{
			DBG_LOG("Failed settings flags ioctl.\n");
			close(fd);
			return false;
		}
	}

#if 1
	// Old ioctl API for compat.
	{
		kbase_uk_gpuprops props = {};
		props.header.id = KBASE_FUNC_GPU_PROPS_REG_DUMP;
		if (mali_ioctl(fd, props) != 0)
		{
			DBG_LOG("Failed settings flags ioctl.\n");
			close(fd);
			return false;
		}

		hw_info.gpu_id = props.props.core_props.product_id;
		//LOG("Found product ID: %u\n", hw_info.gpu_id);
		hw_info.r_value = props.props.core_props.major_revision;
		hw_info.p_value = props.props.core_props.minor_revision;
		for (uint32_t i = 0; i < props.props.coherency_info.num_core_groups; i++)
			hw_info.core_mask |= props.props.coherency_info.group[i].core_mask;
		hw_info.mp_count = __builtin_popcountll(hw_info.core_mask);
		//LOG("Found core mask: 0x%x\n", hw_info.core_mask);

		close(fd);
		hw_info.valid = true;
		return true;
	}
#else
	{
		kbase_ioctl_get_gpuprops get_props = {};
		int ret;
		if ((ret = ioctl(fd, KBASE_IOCTL_GET_GPUPROPS, &get_props)) < 0)
		{
			LOG("Failed getting GPU properties.\n");
			close(fd);
			return false;
		}

		get_props.size = ret;
		std::vector<uint8_t> buffer(ret);
		get_props.buffer.value = buffer.data();
		ret = ioctl(fd, KBASE_IOCTL_GET_GPUPROPS, &get_props);
		if (ret < 0)
		{
			LOG("Failed getting GPU properties.\n");
			close(fd);
			return false;
		}

		LOG("Property buffer is %d bytes.\n", ret);

#define READ_U8(p)  ((p)[0])
#define READ_U16(p) (READ_U8((p))  | (uint16_t(READ_U8((p) + 1)) << 8))
#define READ_U32(p) (READ_U16((p)) | (uint32_t(READ_U16((p) + 2)) << 16))
#define READ_U64(p) (READ_U32((p)) | (uint64_t(READ_U32((p) + 4)) << 32))

		gpu_props props = {};

		const auto *ptr = buffer.data();
		int size = ret;
		while (size > 0)
		{
			uint32_t type = READ_U32(ptr);
			uint32_t value_type = type & 3;
			uint64_t value;

			ptr += 4;
			size -= 4;

			switch (value_type)
			{
			case KBASE_GPUPROP_VALUE_SIZE_U8:
				value = READ_U8(ptr);
				ptr++;
				size--;
				break;
			case KBASE_GPUPROP_VALUE_SIZE_U16:
				value = READ_U16(ptr);
				ptr += 2;
				size -= 2;
				break;
			case KBASE_GPUPROP_VALUE_SIZE_U32:
				value = READ_U32(ptr);
				ptr += 4;
				size -= 4;
				break;
			case KBASE_GPUPROP_VALUE_SIZE_U64:
				value = READ_U64(ptr);
				ptr += 8;
				size -= 8;
				break;
			}

			for (unsigned i = 0; gpu_property_mapping[i].type; i++)
			{
				if (gpu_property_mapping[i].type == (type >> 2))
				{
					auto offset = gpu_property_mapping[i].offset;
					void *p = reinterpret_cast<uint8_t *>(&props) + offset;
					switch (gpu_property_mapping[i].size)
					{
					case 1:
						*reinterpret_cast<uint8_t *>(p) = value;
						break;
					case 2:
						*reinterpret_cast<uint16_t *>(p) = value;
						break;
					case 4:
						*reinterpret_cast<uint32_t *>(p) = value;
						break;
					case 8:
						*reinterpret_cast<uint64_t *>(p) = value;
						break;
					default:
						LOG("Invalid property size.");
						close(fd);
						return false;
					}
					break;
				}
			}
		}

		hw_info.gpu_id = props.product_id;
		LOG("Found product ID: %u\n", hw_info.gpu_id);
		hw_info.r_value = props.major_revision;
		hw_info.p_value = props.minor_revision;
		for (uint32_t i = 0; i < props.num_core_groups; i++)
			hw_info.core_mask |= props.core_mask[i];
		hw_info.mp_count = __builtin_popcountll(hw_info.core_mask);

		close(fd);
		return true;
	}
#endif
}

bool MaliHWCReader::is_alive() const
{
	return alive;
}

int MaliHWCReader::get_hwc_fd() const
{
	return hwc_fd;
}

uint32_t MaliHWCReader::get_buffer_size() const
{
	return buffer_size;
}

uint32_t MaliHWCReader::get_buffer_count() const
{
	return buffer_count;
}

uint32_t MaliHWCReader::get_hardware_counter_version() const
{
	return hw_ver;
}

uint32_t MaliHWCReader::get_num_cores() const
{
	return hw.mp_count;
}

const char * const *MaliHWCReader::get_counter_names() const
{
	return names_lut;
}

const char * const *MaliHWCReader::get_counter_names(MaliCounterBlockName block) const
{
	return names_lut + (MALI_NAME_BLOCK_SIZE * block);
}

const char *MaliHWCReader::get_counter_name(MaliCounterBlockName block, unsigned index) const
{
	return names_lut[MALI_NAME_BLOCK_SIZE * block + index];
}

int MaliHWCReader::find_counter_index_by_name(MaliCounterBlockName block, const char *name)
{
	const char * const *names = &names_lut[MALI_NAME_BLOCK_SIZE * block];
	for (unsigned i = 0; i < MALI_NAME_BLOCK_SIZE; i++)
		if (strstr(names[i], name))
			return int(i);

	return -1;
}

const uint32_t *MaliHWCReader::get_counters() const
{
	return counter_buffer.data();
}

const uint32_t *MaliHWCReader::get_counters(MaliCounterBlockName block, unsigned core) const
{
	if (hw_ver < 5) // Old style
	{
		// Not sure how this is supposed to work. It might be always 4 cores,
		// or tightly packed. Should probably look at buffer_size to figure out the padding.
		switch (block)
		{
			case MALI_NAME_BLOCK_JM:
				return counter_buffer.data() + MALI_NAME_BLOCK_SIZE * 6;
			case MALI_NAME_BLOCK_MMU:
				return counter_buffer.data() + MALI_NAME_BLOCK_SIZE * 5;
			case MALI_NAME_BLOCK_TILER:
				return counter_buffer.data() + MALI_NAME_BLOCK_SIZE * 4;
			default:
				return counter_buffer.data() + MALI_NAME_BLOCK_SIZE * remap_core_index(core);
		}
	}
	else
	{
		switch (block)
		{
			case MALI_NAME_BLOCK_JM:
				return counter_buffer.data() + MALI_NAME_BLOCK_SIZE * 0;
			case MALI_NAME_BLOCK_MMU:
				return counter_buffer.data() + MALI_NAME_BLOCK_SIZE * 2;
			case MALI_NAME_BLOCK_TILER:
				return counter_buffer.data() + MALI_NAME_BLOCK_SIZE * 1;
			default:
				return counter_buffer.data() + MALI_NAME_BLOCK_SIZE * (3 + remap_core_index(core));
		}
	}
}

uint32_t MaliHWCReader::get_counter_index(MaliCounterBlockName block, unsigned index, unsigned core) const
{
	uint32_t base_index = get_counters(block, core) - counter_buffer.data();
	return base_index + index;
}

bool MaliHWCReader::init(const char *device, unsigned buffer_count, uint32_t interval_ns)
{
	term();
	this->buffer_count = buffer_count;

	fd = open(device, O_RDWR | O_CLOEXEC | O_NONBLOCK);
	if (fd < 0)
	{
		DBG_LOG("Failed to open /dev/mali0.\n");
		return false;
	}

	{
		kbase_uk_hwcnt_reader_version_check_args check;
		memset(&check, 0, sizeof(check));
		if (mali_ioctl(fd, check) != 0)
		{
			DBG_LOG("Failed to get ABI version.\n");
			return false;
		}
		else if (check.major < 10)
		{
			DBG_LOG("Unsupported ABI version 10.\n");
			return false;
		}
	}

	{
		kbase_uk_hwcnt_reader_set_flags flags;
		memset(&flags, 0, sizeof(flags));
		flags.header.id = KBASE_FUNC_SET_FLAGS;
		flags.create_flags = BASE_CONTEXT_CREATE_KERNEL_FLAGS;
		if (mali_ioctl(fd, flags) != 0)
		{
			DBG_LOG("Failed settings flags ioctl.\n");
			return false;
		}
	}

	{
		kbase_uk_hwcnt_reader_setup setup;
		memset(&setup, 0, sizeof(setup));
		setup.header.id = KBASE_FUNC_HWCNT_READER_SETUP;
		setup.buffer_count = buffer_count;
		setup.jm_bm = -1;
		setup.shader_bm = -1;
		setup.tiler_bm = -1;
		setup.mmu_l2_bm = -1;
		setup.fd = -1;

		if (mali_ioctl(fd, setup) != 0)
		{
			if (setup.header.ret != 0)
			{
				DBG_LOG("Failed setting hwcnt reader ioctl. Setup header not 0\n");
				return false;
			}
			else
			{
				DBG_LOG("Failed setting hwcnt reader ioctl. Setup header is 0\n");
				return false;
			}
		}
		hwc_fd = setup.fd;
	}

	{
		uint32_t api_version = ~HWCNT_READER_API;
		if (ioctl(hwc_fd, KBASE_HWCNT_READER_GET_API_VERSION, &api_version) != 0)
		{
			DBG_LOG("Could not determine hwcnt reader API.\n");
			return false;
		}
		else if (api_version != HWCNT_READER_API)
		{
			DBG_LOG("Invalid API version.\n");
			return false;
		}
	}

	if (ioctl(hwc_fd, KBASE_HWCNT_READER_GET_BUFFER_SIZE, &buffer_size) != 0)
	{
		DBG_LOG("Failed to get buffer size.\n");
		return false;
	}

	if (ioctl(hwc_fd, KBASE_HWCNT_READER_GET_HWVER, &hw_ver) != 0)
	{
		DBG_LOG("Could not determine HW version.\n");
		return false;
	}

	sample_data = static_cast<uint8_t *>(mmap(nullptr, buffer_count * buffer_size, PROT_READ, MAP_PRIVATE, hwc_fd, 0));
	if (sample_data == MAP_FAILED || !sample_data)
	{
		DBG_LOG("Failed to map sample data.\n");
		return false;
	}

	if (ioctl(hwc_fd, KBASE_HWCNT_READER_SET_INTERVAL, interval_ns) != 0)
	{
		DBG_LOG("Could not set hwcnt reader interval.\n");
		return false;
	}

	bool found = false;
	for (auto &product : products)
	{
		if ((product.product_mask & hw.gpu_id) == product.product_id)
		{
			found = true;
			names_lut = product.names_lut;
		}
	}

	if (!found)
	{
		DBG_LOG("Could not identify GPU.\n");
		return false;
	}

	alive = true;
	counter_buffer.resize(buffer_size / sizeof(uint32_t));

	// Build core remap table.
	core_index_remap.clear();
	core_index_remap.reserve(hw.mp_count);
	unsigned mask = hw.core_mask;
	while (mask)
	{
		unsigned bit = __builtin_ctz(mask);
		core_index_remap.push_back(bit);
		mask &= ~(1u << bit);
	}

	DBG_LOG("Successfully initialized HW counter reader!");

	return true;
}

unsigned MaliHWCReader::remap_core_index(unsigned index) const
{
	return core_index_remap[index];
}

MaliHWCReader::MaliHWCReader(const MaliHWInfo &hw_info, const char *device, uint32_t interval_ns)
	: hw(hw_info)
{
	alive = false;
	if (hw_info.valid){
		// Apparently, we need to re-try this multiple times.
		for (unsigned i = 1; i < 1024 && !alive; i++){
			init(device, i, interval_ns);
		}
	}
}

void MaliHWCReader::term()
{
	if (sample_data)
		munmap(sample_data, buffer_count * buffer_size);
	if (hwc_fd >= 0)
		close(hwc_fd);
	if (fd >= 0)
		close(fd);

	sample_data = nullptr;
	hwc_fd = -1;
	fd = -1;
}

MaliHWCReader::~MaliHWCReader()
{
	term();
}

uint64_t MaliHWCReader::get_timestamp_ns() const
{
	return timestamp;
}

bool MaliHWCReader::wait_next_event()
{
	pollfd fds[1] = {};
	fds[0].fd = hwc_fd;
	fds[0].events = POLLIN;

	int count = poll(fds, 1, -1);
	if (count < 0)
	{
		DBG_LOG("poll() failed.\n");
		return false;
	}

	if (fds[0].revents & POLLIN)
	{
		kbase_hwcnt_reader_metadata meta;
		if (ioctl(hwc_fd, KBASE_HWCNT_READER_GET_BUFFER, &meta) != 0)
		{
			DBG_LOG("Failed READER_GET_BUFFER.\n");
			return false;
		}

		memcpy(counter_buffer.data(), sample_data + buffer_size * meta.buffer_idx, buffer_size);
		timestamp = meta.timestamp;

		if (ioctl(hwc_fd, KBASE_HWCNT_READER_PUT_BUFFER, &meta) != 0)
		{
			DBG_LOG("Failed READER_PUT_BUFFER.\n");
			return false;
		}

		return true;
	}
	else if (fds[0].revents & POLLHUP)
	{
		DBG_LOG("HWC hung up.\n");
		return false;
	}
	else
		return false;
}
}
