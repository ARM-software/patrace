#include <vector>
#include <common/file_format.hpp>
#include "dispatch/eglimports.hpp"

typedef enum compressionControlFlag{
    compression_fixed_rate_disabled     = 0,    // disable fixed rate compression
    compression_fixed_rate_default      = 1,    // enable fixed rate compression, the implementation can use default rate on the image.
    compression_fixed_rate_lowest_rate  = 2,    // enable fixed rate compression, use the lowest compression rate supported by the implementation
    compression_fixed_rate_highest_rate = 3,    // enable fixed rate compression, use the highest compression rate supported by the implementation
    compression_fixed_rate_flag_end     = 4     // end flag
} compressionControlFlag;

typedef struct compressionControlInfo {
    GLint *rates;
    GLint rate_size;
    GLint extension;
    GLint rate;
} compressionControlInfo;

void getCompressionInfo(int flag, bool isSurfaceCompress, compressionControlInfo &compressInfo);
std::vector<int> AddtoAttribList(common::Array<int> &attrib_list, int attrib, int value, int attrib_terminator);
