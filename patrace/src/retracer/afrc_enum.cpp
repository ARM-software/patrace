#include "afrc_enum.hpp"
#include <retracer/retracer.hpp>
#include <retracer/retrace_api.hpp>

using namespace retracer;

void getCompressionInfo(int flag, bool isSurfaceCompression, compressionControlInfo &compressInfo)
{
    if (isSurfaceCompression) {
        compressInfo.extension = EGL_SURFACE_COMPRESSION_EXT;
        switch (flag) {
            case compression_fixed_rate_disabled:
                compressInfo.rate = EGL_SURFACE_COMPRESSION_FIXED_RATE_NONE_EXT;
                break;
            case compression_fixed_rate_default:
                compressInfo.rate = EGL_SURFACE_COMPRESSION_FIXED_RATE_DEFAULT_EXT;
                break;
            case compression_fixed_rate_lowest_rate: // lowest compression rate -> highest BPC
                compressInfo.rate = compressInfo.rates[compressInfo.rate_size-1];
                break;
            case compression_fixed_rate_highest_rate: // highest compression rate -> lowest BPC
                compressInfo.rate =  compressInfo.rates[0];
                break;
            default:
                break;
        }
    }
    else {
        compressInfo.extension = GL_SURFACE_COMPRESSION_EXT;
        switch (flag) {
            case compression_fixed_rate_disabled:
                compressInfo.rate = GL_SURFACE_COMPRESSION_FIXED_RATE_NONE_EXT;
                break;
            case compression_fixed_rate_default:
                compressInfo.rate = GL_SURFACE_COMPRESSION_FIXED_RATE_DEFAULT_EXT;
                break;
            case compression_fixed_rate_lowest_rate:
                compressInfo.rate = compressInfo.rates[compressInfo.rate_size-1];
                break;
            case compression_fixed_rate_highest_rate:
                compressInfo.rate = compressInfo.rates[0];
                break;
            default:
                break;
        }
    }
}

std::vector<int> AddtoAttribList(common::Array<int> &attrib_list, int attrib, int value, int attrib_terminator)
{
    std::vector<int> new_list;
    bool found = false;
    if (attrib_list.cnt != 0) {
        for (int i = 0; attrib_list[i] != attrib_terminator; i+=2) {
            new_list.push_back(attrib_list[i]);
            if (attrib_list[i] == attrib) {
                new_list.push_back(value);
                found = true;
            }
            else {
                new_list.push_back(attrib_list[i+1]);
            }
        }
    }
    if (!found && attrib != attrib_terminator) {
        new_list.push_back(attrib);
        new_list.push_back(value);
    }
    new_list.push_back(attrib_terminator);

    return new_list;
}
