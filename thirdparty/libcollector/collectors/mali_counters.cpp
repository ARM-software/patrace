#include "mali_counters.hpp"


InfoCapsule::InfoCapsule(){
    if (!mali_userspace::get_mali_hw_info("/dev/mali0", info)){
        DBG_LOG("Failed to get mali HW info. You can safely ignore this if you are not running on a Mali platform or you dont care about HW counters\n");
    }
}

MaliCounterCollector::MaliCounterCollector(const Json::Value& config, const std::string& name):
    Collector(config, name),
    infoc(),
    counter_reader(infoc.info, "/dev/mali0", 1000000){

};

bool MaliCounterCollector::init(){
    if (!infoc.info.valid){
        DBG_LOG("Collector invalid, not initializing\n");
        return false;
    }

    if (!counter_reader.is_alive())
    {
        DBG_LOG("Failed to create HWC reader.\n");
        return false;
    }

    build_block_vectors(
        mali_userspace::MALI_NAME_BLOCK_JM,
        mali_userspace::MALI_NAME_BLOCK_SIZE,
        "JM"
    );

    build_block_vectors(
        mali_userspace::MALI_NAME_BLOCK_TILER,
        mali_userspace::MALI_NAME_BLOCK_SIZE,
        "TILER"
    );

    build_block_vectors(
        mali_userspace::MALI_NAME_BLOCK_MMU,
        mali_userspace::MALI_NAME_BLOCK_SIZE,
        "MMU"
    );


    int num_shader_cores = counter_reader.get_num_cores();
    for (int corenum = 0; corenum < num_shader_cores; ++ corenum){
        build_block_vectors(
            mali_userspace::MALI_NAME_BLOCK_SHADER,
            mali_userspace::MALI_NAME_BLOCK_SIZE,
            "SC:" + _to_string(corenum),
            corenum
        );
    }

    num_counters = header.size();
    counter_buffer.resize(num_counters);

    return true;
}


void MaliCounterCollector::build_block_vectors(mali_userspace::MaliCounterBlockName block, int block_size, const std::string& verbose_block_name, int core){
    const char * const * names = counter_reader.get_counter_names(block);
    for (int index = 0; index < block_size; ++index){
        if (names[index][0] != '\0'){
            header.push_back(verbose_block_name + "_" + std::string(names[mali_userspace::MALI_NAME_BLOCK_JM * block_size + index]));
            indices.emplace_back(
                block,
                counter_reader.find_counter_index_by_name(
                    block,
                    names[index]
                )
            );
            core_indices.push_back(core);
        }
    }
}

bool MaliCounterCollector::available(){
    // Return true if the platform has a mali device (should probably improve this, all mali dev. might not support counters?)
    int fd = open("/dev/mali0", O_RDWR);
    if (fd < 0){
        return false;
    }

    close(fd);

    return true;
}

bool MaliCounterCollector::deinit(){
    return true;
}

bool MaliCounterCollector::start(){
    if (!infoc.info.valid){
        DBG_LOG("Collector invalid, not starting\n");
        return false;
    }

    //Probably dont need to store these
    DBG_LOG("Starting Mali counter collector...\n");
    for (size_t index = 0; index < num_counters; ++index){
        int core_index = core_indices[index];
        if (core_index == -1){
            counter_buffer[index] = counter_reader.get_counters(
                indices[index].first
            )[indices[index].second];
        }
        else{
            counter_buffer[index] = counter_reader.get_counters(
                indices[index].first,
                core_index
            )[indices[index].second];
        }
    }

    DBG_LOG("Counters cleared! Waiting for next event...\n");

    if (!counter_reader.wait_next_event()){
        DBG_LOG("Could not fetch next event..\n");
        return false;
    }

    DBG_LOG("Counter collector initialized!\n");

    return true;
}

bool MaliCounterCollector::collect(int64_t /* now */){
    if (!infoc.info.valid){
        return false;
    }

    float total_sum = 0;

    //TODO: We can optimize here if we need to
    for (size_t index = 0; index < num_counters; ++index){
        int core_index = core_indices[index];
        if (core_index == -1){
            add(
                header[index],
                counter_reader.get_counters(
                    indices[index].first
                )[indices[index].second]
            );

            total_sum += counter_reader.get_counters(
                    indices[index].first
                )[indices[index].second];
        }
        else{
            add(
                header[index],
                counter_reader.get_counters(
                    indices[index].first,
                    core_index
                )[indices[index].second]
            );

            total_sum += counter_reader.get_counters(
                    indices[index].first,
                    core_index
                )[indices[index].second];
        }
    }

    if (total_sum == 0){
        DBG_LOG("Nothing sampled, exiting\n");
        exit(0);
    }

    return true;
}
