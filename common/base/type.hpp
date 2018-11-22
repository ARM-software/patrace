#ifndef PAT_BASE_TYPE
#define PAT_BASE_TYPE

#define BYTE_BIT_WIDTH      8 

#if (defined __cplusplus && !defined(_MSC_VER)) || (__STDC_VERSION__ >= 199901L)

#include <stdint.h>

typedef uint8_t             UInt8;
typedef int8_t              SInt8;
typedef uint16_t            UInt16;
typedef int16_t             SInt16;
typedef uint32_t            UInt32;
typedef int32_t             SInt32;
typedef uint64_t            UInt64;
typedef int64_t             SInt64;

#else

typedef unsigned char       UInt8;
typedef char                SInt8;
typedef unsigned short      UInt16;
typedef short               SInt16;
typedef unsigned int        UInt32;
typedef int                 SInt32;
typedef unsigned long long  UInt64;
typedef signed long long    SInt64;

#endif

typedef float               Float32;
typedef double              Float64;

#endif
