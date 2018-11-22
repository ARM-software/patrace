#ifndef _COMMON_BASE64_HPP_
#define _COMMON_BASE64_HPP_

#include <stddef.h>

namespace common 
{
    /* caller must delete[] the return value after use. */
    char* base64_encode(const char *data, 
                        size_t input_length, 
                        size_t *output_length);
}

#endif
