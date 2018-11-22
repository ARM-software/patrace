#include "base64.hpp"

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

namespace common {

static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static int modulo_table[] = {0, 2, 1};

char *base64_encode(const char *data,
        size_t input_length,
        size_t *output_length)
{
    assert(output_length);
    *output_length = ((input_length - 1) / 3) * 4 + 4;

    char *output_data = new char[*output_length];

    if (!output_data)
        return NULL;

    for (size_t i = 0, j = 0; i < input_length;) 
    {
        uint32_t first_o =  i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t second_o = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t third_o =  i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (first_o << 0x10) + (second_o << 0x08) + third_o;

        output_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        output_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        output_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        output_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < modulo_table[input_length % 3]; i++)
    {
        output_data[*output_length - 1 - i] = '=';
    }

    return output_data;
}

}
