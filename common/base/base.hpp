#ifndef _PAT_BASE_BASE_
#define _PAT_BASE_BASE_

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>
#include <bitset>
#include <memory>

#include "type.hpp"

#define PAT_PRINTF printf
#define PAT_ASSERT_QUIT exit(1)

#define PAT_DEBUG_TRACE \
    PAT_PRINTF( "In file: %s  line:%4d\n" , __FILE__, __LINE__)

#define PAT_DEBUG_TRACE_START printf("****************************************\n")
#define PAT_DEBUG_TRACE_END printf("\n")

#define PAT_DEBUG_ASSERT(expr, msg, ...) \
    do {\
        if (!(expr)) \
        { \
            PAT_DEBUG_TRACE_START; \
            PAT_PRINTF("ASSERT EXIT: "); \
            PAT_DEBUG_TRACE; \
            PAT_PRINTF(msg, ##__VA_ARGS__); \
            PAT_DEBUG_TRACE_END; \
            PAT_ASSERT_QUIT; \
        }\
    } while(0)

#define PAT_DEBUG_ASSERT_POINTER(pointer) PAT_DEBUG_ASSERT(pointer, ("Null pointer " #pointer) )
#define PAT_DEBUG_ASSERT_NEW(pointer) PAT_DEBUG_ASSERT(pointer, "Memory allocation by new operator failed.")

#define PAT_DEBUG_LOG(msg, ...) \
    do {\
        {\
            PAT_DEBUG_TRACE_START; \
            PAT_PRINTF("LOG: "); \
            PAT_DEBUG_TRACE; \
            PAT_PRINTF(msg, ##__VA_ARGS__); \
            PAT_DEBUG_TRACE_END; \
        }\
    } while(0)

#define PRINT_MEMORY(mem, len) \
    do {\
        { \
            printf("\nMEMORY PRINT BEGIN : %d >>>>>>>>>>>> {\n", (int)len); \
          \
            const unsigned char *begin = (const unsigned char*)mem; \
            for (size_t i = 0; i < len; ++i) \
            { \
                printf("0x%X ", *(begin++)); \
                if ((i+1) % 16 == 0) \
                    printf("\n"); \
            } \
          \
            printf("\nMEMORY PRINT END : %d <<<<<<<<<<<<< }\n", (int)len); \
        }\
    } while(0)

#ifndef GL_BGR_EXT
#define GL_BGR_EXT 0x80E0
#endif

#ifndef GL_ABGR_EXT
#define GL_ABGR_EXT 0x8000
#endif

#ifndef GL_HALF_FLOAT
#define GL_HALF_FLOAT 0x140B
#endif

#ifndef GL_DOUBLE
#define GL_DOUBLE 0x140A
#endif

#ifndef GL_UNSIGNED_BYTE_3_3_2
#define GL_UNSIGNED_BYTE_3_3_2 0x8032
#endif

#ifndef GL_UNSIGNED_BYTE_2_3_3_REV
#define GL_UNSIGNED_BYTE_2_3_3_REV 0x8362
#endif

#ifndef GL_UNSIGNED_SHORT_5_6_5_REV
#define GL_UNSIGNED_SHORT_5_6_5_REV 0x8364
#endif

#ifndef GL_UNSIGNED_INT_10_10_10_2
#define GL_UNSIGNED_INT_10_10_10_2 0x8036
#endif

#ifndef GL_UNSIGNED_INT_8_8_8_8
#define GL_UNSIGNED_INT_8_8_8_8 0x8035
#endif

#ifndef GL_UNSIGNED_INT_8_8_8_8_REV
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#endif

#define GL_BTC_LUMINANCE_PAT            0x93EF
#define GL_PALETTE8_RGB5_A1_OES         0x8B99

#ifndef GL_COMPRESSED_RGB8_ETC2
#define GL_COMPRESSED_RGB8_ETC2                          0x9274
#define GL_COMPRESSED_SRGB8_ETC2                         0x9275
#define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2      0x9276
#define GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2     0x9277
#define GL_COMPRESSED_RGBA8_ETC2_EAC                     0x9278
#define GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC              0x9279
#endif

#define COMPRESSED_ASTC_RGBA_4x4_OES    0x93B0
#define COMPRESSED_ASTC_RGBA_5x4_OES    0x93B1
#define COMPRESSED_ASTC_RGBA_5x5_OES    0x93B2
#define COMPRESSED_ASTC_RGBA_6x5_OES    0x93B3
#define COMPRESSED_ASTC_RGBA_6x6_OES    0x93B4
#define COMPRESSED_ASTC_RGBA_8x5_OES    0x93B5
#define COMPRESSED_ASTC_RGBA_8x6_OES    0x93B6
#define COMPRESSED_ASTC_RGBA_8x8_OES    0x93B7
#define COMPRESSED_ASTC_RGBA_10x5_OES   0x93B8
#define COMPRESSED_ASTC_RGBA_10x6_OES   0x93B9
#define COMPRESSED_ASTC_RGBA_10x8_OES   0x93BA
#define COMPRESSED_ASTC_RGBA_10x10_OES  0x93BB
#define COMPRESSED_ASTC_RGBA_12x10_OES  0x93BC
#define COMPRESSED_ASTC_RGBA_12x12_OES  0x93BD

#define COMPRESSED_ASTC_SRGB8_ALPHA8_4x4_OES    0x93D0
#define COMPRESSED_ASTC_SRGB8_ALPHA8_5x4_OES    0x93D1
#define COMPRESSED_ASTC_SRGB8_ALPHA8_5x5_OES    0x93D2
#define COMPRESSED_ASTC_SRGB8_ALPHA8_6x5_OES    0x93D3
#define COMPRESSED_ASTC_SRGB8_ALPHA8_6x6_OES    0x93D4
#define COMPRESSED_ASTC_SRGB8_ALPHA8_8x5_OES    0x93D5
#define COMPRESSED_ASTC_SRGB8_ALPHA8_8x6_OES    0x93D6
#define COMPRESSED_ASTC_SRGB8_ALPHA8_8x8_OES    0x93D7
#define COMPRESSED_ASTC_SRGB8_ALPHA8_10x5_OES   0x93D8
#define COMPRESSED_ASTC_SRGB8_ALPHA8_10x6_OES   0x93D9
#define COMPRESSED_ASTC_SRGB8_ALPHA8_10x8_OES   0x93DA
#define COMPRESSED_ASTC_SRGB8_ALPHA8_10x10_OES  0x93DB
#define COMPRESSED_ASTC_SRGB8_ALPHA8_12x10_OES  0x93DC
#define COMPRESSED_ASTC_SRGB8_ALPHA8_12x12_OES  0x93DD

#define COMPRESSED_ASTC_RGBA_3x3x3_OES          0x93C0
#define COMPRESSED_ASTC_RGBA_4x3x3_OES          0x93C1
#define COMPRESSED_ASTC_RGBA_4x4x3_OES          0x93C2
#define COMPRESSED_ASTC_RGBA_4x4x4_OES          0x93C3
#define COMPRESSED_ASTC_RGBA_5x4x4_OES          0x93C4
#define COMPRESSED_ASTC_RGBA_5x5x4_OES          0x93C5
#define COMPRESSED_ASTC_RGBA_5x5x5_OES          0x93C6
#define COMPRESSED_ASTC_RGBA_6x5x5_OES          0x93C7
#define COMPRESSED_ASTC_RGBA_6x6x5_OES          0x93C8
#define COMPRESSED_ASTC_RGBA_6x6x6_OES          0x93C9

#define COMPRESSED_ASTC_SRGB8_ALPHA8_3x3x3_OES  0x93E0
#define COMPRESSED_ASTC_SRGB8_ALPHA8_4x3x3_OES  0x93E1
#define COMPRESSED_ASTC_SRGB8_ALPHA8_4x4x3_OES  0x93E2
#define COMPRESSED_ASTC_SRGB8_ALPHA8_4x4x4_OES  0x93E3
#define COMPRESSED_ASTC_SRGB8_ALPHA8_5x4x4_OES  0x93E4
#define COMPRESSED_ASTC_SRGB8_ALPHA8_5x5x4_OES  0x93E5
#define COMPRESSED_ASTC_SRGB8_ALPHA8_5x5x5_OES  0x93E6
#define COMPRESSED_ASTC_SRGB8_ALPHA8_6x5x5_OES  0x93E7
#define COMPRESSED_ASTC_SRGB8_ALPHA8_6x6x5_OES  0x93E8
#define COMPRESSED_ASTC_SRGB8_ALPHA8_6x6x6_OES  0x93E9

#define COMPRESSED_ASTC_FORMAT_BEGIN COMPRESSED_ASTC_RGBA_4x4_OES
#define COMPRESSED_ASTC_FORMAT_END COMPRESSED_ASTC_SRGB8_ALPHA8_6x6x6_OES

#endif // _PAT_BASE_BASE_
