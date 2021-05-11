#ifndef _COMMON_FILE_FORMAT_H_
#define _COMMON_FILE_FORMAT_H_

#include <stdint.h> // uintptr_t
#include <common/os.hpp>
#include "trace_limits.hpp"
#include "my_egl_attribs.hpp"

#ifdef WIN32
#   pragma warning(disable: 4200)
#endif

namespace common {

#define VERBOSE_FILE 0

#pragma pack(push, 1)

///////////////////////////////////////////////////////////////////////
// Basic structures
struct String {
    // The maximum length is 2^16=65,536
    unsigned short  byLen;
    char            str[];
};

template <class T>
struct Array {
    unsigned int cnt;
    T *v;

    operator T* () {
        return v;
    }
};

#ifdef PLATFORM_64BIT
// This spesialization owns the pointer it is assigned.
template <>
struct Array<const char*> {
    unsigned int cnt;
    const char** v;

    operator const char** () {
        return v;
    }

    Array():v(NULL) {}
    ~Array() {delete [] v;}
};
#endif


///////////////////////////////////////////////////////////////////////
// Trace file format explanation:
//
// Basically, the binary file contains an array of *blocks*.
// The length(unit: byte) of the block is variable.
// Each block contains an *offset*(aka: toNext) to the next block.
//
// toNext:
//     the offset to the next block in the binary file.
//
//     ...[block n] [ block n+1]....
// pBlockNplus1 = ((char*)pBlockN) + pBlockN->toNext

struct Block {
    unsigned int    toNext;
    char            unknown[];
};

enum HeaderVersion {
    INVALID_VERSION = 0,
    HEADER_VERSION_1 = 3,
    HEADER_VERSION_2,
    HEADER_VERSION_3,
    HEADER_VERSION_4
};

class BHeader {
public:
    unsigned int    toNext;
    unsigned int    magicNo;
    unsigned int    version;
    unsigned int    api;
    unsigned int    winW;
    unsigned int    winH;
    unsigned int    frameCnt;
    unsigned long long callCnt;

    unsigned int    winRedSize, winGreenSize, winBlueSize, winAlphaSize;
    unsigned int    winDepthSize, winStencilSize;
    unsigned int    texCompress;

    BHeader():
        toNext(sizeof(BHeader)),
        magicNo(0), // Correct value is 0x20122012 -- not set here so it doesn't have the correct value if no data is read
        version(0),
        api(0),
        winW(0), winH(0),
        frameCnt(0), callCnt(0),
        winRedSize(0), winGreenSize(0), winBlueSize(0), winAlphaSize(0),
        winDepthSize(0), winStencilSize(0),
        texCompress(0)
    {}
};

class BHeaderV1 : public BHeader {
public:
    BHeaderV1(): BHeader()
    {
        version = HEADER_VERSION_1;
        toNext = sizeof(BHeaderV1);
    }
};

class BHeaderV2 : public BHeader {
public:
    unsigned int defaultThreadid;
    MyEGLAttribs perThreadEGLConfigs[MAX_RETRACE_THREADS];
    unsigned int perThreadClientSideBufferSize[MAX_RETRACE_THREADS];

    BHeaderV2(): BHeader()
    {
        version = HEADER_VERSION_2;
        toNext = sizeof(BHeaderV2);
        defaultThreadid = 0;
        for (int i=0; i<MAX_RETRACE_THREADS; i++) {
            perThreadEGLConfigs[i].timesUsed = 0;
        }
    }
};

class BHeaderV3 {
public:
    unsigned int toNext;
    unsigned int magicNo;
    unsigned int version;

    unsigned int jsonLength;    // actual length of json data
    long long    jsonFileBegin; // file offset begin
    long long    jsonFileEnd;   // file offset end, fixed size

    // The json data will occupy 512 * 1024 - 32 bytes.
    // The size of this this header is 32 bytes.
    // This results in gles calls to start at the 512k (0x8000) position
    // in the trace file.
    static const unsigned int jsonMaxLength = 512 * 1024 - 32;
    BHeaderV3()
    {
        toNext = sizeof(BHeaderV3);
        magicNo = 0x20122012;
        version = HEADER_VERSION_4;
        jsonLength = 0;
        jsonFileBegin = sizeof(BHeaderV3);
        jsonFileEnd = sizeof(BHeaderV3) + jsonMaxLength;
    }
};


enum CALL_ERROR_NO {
    CALL_GL_NO_ERROR = 0,
    CALL_GL_INVALID_ENUM,
    CALL_GL_INVALID_VALUE,
    CALL_GL_INVALID_OPERATION,
    CALL_GL_INVALID_FRAMEBUFFER_OPERATION,
    CALL_GL_OUT_OF_MEMORY,
};

// Please note that BCall is a special *block*.
// Its *toNext* member is optional.
// For fixed length function calls, we eliminate this member to get more compact binary file.
// depend on if this function can be serialized into a fixed length of buffer.
// For example:
// glBindTexture is fixed length, thus it will not use *toNext*
// glTexImage2D is variable length (the last parameter is a blob type), so it will use *toNext*
struct BCall {
    BCall() : funcId(0), tid(0), errNo(0), source(0), reserved(0) {}
    unsigned short funcId;
    unsigned char tid;
    unsigned char errNo : 4;
    unsigned char source : 1; // 0 = native, 1 = injected by tracer
    unsigned char reserved : 3;
};
struct BCall_vlen : BCall { // variable length
    BCall_vlen() : BCall(), toNext(0) {}
    BCall_vlen(const BCall& b) : BCall(b), toNext(0) {}
    unsigned int toNext;
};

#pragma pack(pop)

///////////////////////////////////////////////////////////////////////
// Write functions

#define PTR_OFFSET(x,n)  (((char*)(x) - (char*)0) & ((n) - 1))

#define PTR_PADDING(x,n) (char*)((uintptr_t)(x+(n-1ull)) & ~(n-1ull))

#define PAD_SIZEOF(x)       ( ((sizeof(x)-1)/4+1) * 4 )

inline char* padwrite(char* x) {
    char *y = PTR_PADDING(x, 4);
    // pad with 0, because we'll further compress the buffer with Snappy.
    // 0 will help us to get a better compress ratio.
    for(char *c=x;c<y;++c)
        *c = 0;
    return y;
}

template <class T>
inline char* WriteFixed(char* dest, T val, bool doPadding = true) {
#if VERBOSE_FILE
    if ((PTR_OFFSET(dest, 4) != 0) && doPadding)
        DBG_LOG("Un-aligned mem addr: %p\n", dest);
#endif
    memcpy(dest, &val, sizeof(T));
    return doPadding ? padwrite(dest+sizeof(T)) : (dest+sizeof(T));
}

template <class T>
inline char* Write1DArray(char* dest, unsigned int len, const T* array) {
#if VERBOSE_FILE
    if (PTR_OFFSET(dest, 4) != 0)
        DBG_LOG("Un-aligned mem addr: %p\n", dest);
#endif
    // There are some cases, the array could be an optional parameter
    // so we cannot count on the value of *len*
    if (array == NULL)
        len = 0;
    unsigned int byLen = len*sizeof(T);
    dest = WriteFixed(dest, byLen);
    if (byLen > 0)
        memcpy(dest, array, byLen);
    return padwrite(dest+byLen);
}

// null-terminated
inline char* WriteString(char* dest, const char* src) {
    unsigned int byLen = src ? strlen(src)+1 : 0;
    *(unsigned int*)dest = byLen; dest += sizeof(unsigned int);
    if (byLen)
        memcpy(dest, src, byLen);
    return padwrite(dest+byLen);
}

// non null-terminated
inline char* WriteString(char* dest, const char* src, int len) {
    *(unsigned int*)dest = len; dest += sizeof(unsigned int);
    if (len)
        memcpy(dest, src, len);
    return padwrite(dest+len);
}

inline char* WriteStringArray(char* dest, int cnt, const char* const* strv, const int* lenv = NULL) {
    // assuming the pointer is 32 bit
    dest = WriteFixed<unsigned int>(dest, cnt*sizeof(unsigned int));

    if (cnt == 0)
        return dest;

    unsigned int* offsetArray = (unsigned int*)dest;
    for (int i = 0; i < cnt; ++i)
        offsetArray[i] = 0;
    dest += cnt*sizeof(unsigned int);

    unsigned int *jmp = (unsigned int*)dest;
    dest = WriteFixed(dest, (unsigned int)0); // leave a slot

    if (lenv) {
        for (int i = 0; i < cnt; ++i) {
            if (lenv[i])
                // mem ptr to file ptr
                offsetArray[i] = dest+sizeof(unsigned int)-(char*)(&offsetArray[i]);
            dest = WriteString(dest, strv[i], lenv[i]);
        }
    } else {
        for (int i = 0; i < cnt; ++i) {
            if (strv[i])
                // mem ptr to file ptr
                offsetArray[i] = dest+sizeof(unsigned int)-(char*)(&offsetArray[i]);
            dest = WriteString(dest, strv[i]);
        }
    }

    *jmp = dest-(char*)jmp;
    return dest;
}

///////////////////////////////////////////////////////////////////////
// Read functions
template <class T>
inline char* ToNextBlock(T* pCur) {
    return ((char*)pCur) + pCur->toNext;
}

#if 0
template <>
inline char* ToNextBlock(BCall* pCall) {
    unsigned int byLen = GetSerializationLen(pCall->funcId);
    if (byLen > 0)
        return ((char*)pCall) + byLen;
    else
        return ((char*)pCall) + pCall->toNext;
}
#endif

template <class T>
inline void PeekFixed(char* src, T& val) {
#if VERBOSE_FILE
    if (PTR_OFFSET(src, 4) != 0)
        DBG_LOG("Un-aligned mem addr: %p\n", src);
#endif
    val = *(T*)src;
}

template <class T>
inline char* ReadFixed(char* src, T& val) {
    PeekFixed(src, val);
    return PTR_PADDING(src+sizeof(T), 4);
}

template <class T>
inline char* Read1DArray(char* src, Array<T>& arr) {
#if VERBOSE_FILE
    if (PTR_OFFSET(src, 4) != 0)
        DBG_LOG("Un-aligned mem addr: %p\n", src);
#endif
    unsigned int byLen;
    src = ReadFixed(src, byLen);
    arr.cnt = byLen/sizeof(T);
    arr.v = (byLen > 0) ? (T*)src : NULL;
    return PTR_PADDING(src+byLen, 4);
}

inline char* ReadString(char* src, char*& str) {
    unsigned int strLen;
    PeekFixed(src, strLen);
    str = strLen ? src+sizeof(strLen) : NULL;
    //if (len)
    //    *len = strLen;

    return PTR_PADDING(src+sizeof(strLen)+strLen, 4);
}

#ifndef PLATFORM_64BIT
inline char* ReadStringArray(char* src, Array<const char*>& arr) {
    unsigned int byLen;
    src = ReadFixed(src, byLen);
    arr.cnt = byLen / sizeof(unsigned int);

    arr.v = byLen ? (const char**)src : NULL;
    if (arr.cnt) {
        src += byLen;
        // file ptr to memory ptr
        for (unsigned int i = 0; i < arr.cnt; ++i) {
            if (arr.v[i])
                arr.v[i] += (const char*)(&arr.v[i]) - (const char *)(0);
        }
        // jmp to the next block
        unsigned int jmpLen;
        PeekFixed(src, jmpLen);
        src += jmpLen;
    }
    return src;
}
#else
// For 64 bits system, we allocate an array and later deallocate it
// It's not so efficient, but only happens on desktop, which we don't care so much.
    
// Update 13 June 2014: also happens on 64bit iOS systems, but probably rarely enough
// (e.g. during glShaderSource) that it isn't a large issue.
inline char* ReadStringArray(char* src, Array<const char*>& arr) {
    unsigned int byLen;
    src = ReadFixed(src, byLen);
    arr.cnt = byLen / sizeof(unsigned int);

    // arr.v = byLen ? (const char**)src : NULL;
    if (byLen) {
        arr.v = new const char*[arr.cnt];
        memset(arr.v, 0, sizeof(const char*)*arr.cnt);
    } else
        arr.v = NULL;

    if (arr.cnt) {
        unsigned int *offsetArr = (unsigned int*)src;
        src += byLen;
        // file ptr to memory ptr
        for (unsigned int i = 0; i < arr.cnt; ++i) {
            if (offsetArr[i])
                arr.v[i] = (const char*)(&offsetArr[i]) + offsetArr[i];
        }
        // jmp to the next block
        unsigned int jmpLen;
        PeekFixed(src, jmpLen);
        src += jmpLen;
    }
    return src;
}
#endif

}

#endif
