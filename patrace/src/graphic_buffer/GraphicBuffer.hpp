#pragma once

#include "DynamicLibrary.hpp"
#include <cstdint>
#include <cerrno>

struct ANativeWindowBuffer;

namespace android
{
    class GraphicBuffer;

    // include/system/window.h
    struct android_native_base_t
    {
        int magic;
        int version;
        void* reserved[4];
        void (*incRef)(struct android_native_base_t* base);
        void (*decRef)(struct android_native_base_t* base);
    };

    // include/ui/android_native_buffer.h
    struct android_native_buffer_t
    {
        struct android_native_base_t common;
        int width;
        int height;
        int stride;
        int format;
        int usage;
        // ...
    };
}

// utils/Errors.h
enum status_t
{
    OK					= 0,
    UNKNOWN_ERROR		= (-2147483647-1),
    NO_MEMORY			= -ENOMEM,
    INVALID_OPERATION	= -ENOSYS,
    BAD_VALUE			= -EINVAL,
    BAD_TYPE			= (UNKNOWN_ERROR + 1),
    NAME_NOT_FOUND		= -ENOENT,
    PERMISSION_DENIED	= -EPERM,
    NO_INIT				= -ENODEV,
    ALREADY_EXISTS		= -EEXIST,
    DEAD_OBJECT			= -EPIPE,
    FAILED_TRANSACTION	= (UNKNOWN_ERROR + 2),
    BAD_INDEX			= -E2BIG,
    NOT_ENOUGH_DATA		= (UNKNOWN_ERROR + 3),
    WOULD_BLOCK			= (UNKNOWN_ERROR + 4),
    TIMED_OUT			= (UNKNOWN_ERROR + 5),
    UNKNOWN_TRANSACTION = (UNKNOWN_ERROR + 6),
    FDS_NOT_ALLOWED		= (UNKNOWN_ERROR + 7),
};

#define HAL_PIXEL_FORMAT_YCbCr_420_888 HAL_PIXEL_FORMAT_YCBCR_420_888
#define HAL_PIXEL_FORMAT_YCbCr_422_888 HAL_PIXEL_FORMAT_YCBCR_422_888
#define HAL_PIXEL_FORMAT_YCbCr_444_888 HAL_PIXEL_FORMAT_YCBCR_444_888

/* Internal base formats */

/* Base formats that do not have an identical HAL match
 * are defined starting at the Android private range
 */
#define MALI_GRALLOC_FORMAT_INTERNAL_RANGE_BASE 0x100

enum PixelFormat{
    // ui/PixelFormat.h in Android source code
    PIXEL_FORMAT_UNKNOWN                = 0,
    PIXEL_FORMAT_NONE                   = 0,
    PIXEL_FORMAT_CUSTOM                 = -4,
    PIXEL_FORMAT_TRANSLUCENT            = -3,
    PIXEL_FORMAT_TRANSPARENT            = -2,
    PIXEL_FORMAT_OPAQUE                 = -1,

    // system/graphics.h in Android source code
    HAL_PIXEL_FORMAT_RGBA_8888          = 1,    // 4x8-bit RGBA
    HAL_PIXEL_FORMAT_RGBX_8888          = 2,    // 4x8-bit RGB0
    HAL_PIXEL_FORMAT_RGB_888            = 3,    // 3x8-bit RGB
    HAL_PIXEL_FORMAT_RGB_565            = 4,    // 16-bit RGB
    HAL_PIXEL_FORMAT_BGRA_8888          = 5,    // 4x8-bit BGRA
    HAL_PIXEL_FORMAT_RGBA_5551          = 6,    // 16-bit ARGB
    HAL_PIXEL_FORMAT_RGBA_4444          = 7,    // 16-bit ARGB
    HAL_PIXEL_FORMAT_YCBCR_422_SP       = 16,
    HAL_PIXEL_FORMAT_YCRCB_420_SP       = 17,
    HAL_PIXEL_FORMAT_YCBCR_422_I        = 20,
    HAL_PIXEL_FORMAT_RGBA_FP16          = 22,   // 64-bit RGBA
    HAL_PIXEL_FORMAT_RAW16              = 32,
    HAL_PIXEL_FORMAT_BLOB               = 33,
    HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED = 34,
    HAL_PIXEL_FORMAT_YCBCR_420_888      = 35,
    HAL_PIXEL_FORMAT_RAW_OPAQUE         = 36,
    HAL_PIXEL_FORMAT_RAW10              = 37,
    HAL_PIXEL_FORMAT_RAW12              = 38,
    HAL_PIXEL_FORMAT_YCBCR_422_888      = 39,   // 0x27
    HAL_PIXEL_FORMAT_YCBCR_444_888      = 40,   // 0x28
    HAL_PIXEL_FORMAT_FLEX_RGB_888       = 41,   // 0x29
    HAL_PIXEL_FORMAT_FLEX_RGBA_8888     = 42,   // 0x2A
    HAL_PIXEL_FORMAT_RGBA_1010102       = 43,   // 32-bit RGBA
    HAL_PIXEL_FORMAT_DEPTH_16           = 48,
    HAL_PIXEL_FORMAT_DEPTH_24           = 49,
    HAL_PIXEL_FORMAT_DEPTH_24_STENCIL_8 = 50,
    HAL_PIXEL_FORMAT_DEPTH_32F          = 51,
    HAL_PIXEL_FORMAT_DEPTH_32F_STENCIL_8 = 52,
    HAL_PIXEL_FORMAT_STENCIL_8          = 53,
    HAL_PIXEL_FORMAT_YCBCR_P010         = 54,
    HAL_PIXEL_FORMAT_Y8 = 538982489,    // 0x20203859, it is equivalent to just the Y plane from YV12
    HAL_PIXEL_FORMAT_Y16 = 540422489,
    HAL_PIXEL_FORMAT_YV12 = 842094169,  // 0x32315659, YCrCb 4:2:0 Planar


    // product/android/gralloc/src/mali_gralloc_formats.h
	/* Internal definitions for HAL formats. */
	MALI_GRALLOC_FORMAT_INTERNAL_RGBA_8888 = HAL_PIXEL_FORMAT_RGBA_8888,
	MALI_GRALLOC_FORMAT_INTERNAL_RGBX_8888 = HAL_PIXEL_FORMAT_RGBX_8888,
	MALI_GRALLOC_FORMAT_INTERNAL_RGB_888 = HAL_PIXEL_FORMAT_RGB_888,
	MALI_GRALLOC_FORMAT_INTERNAL_RGB_565 = HAL_PIXEL_FORMAT_RGB_565,
	MALI_GRALLOC_FORMAT_INTERNAL_BGRA_8888 = HAL_PIXEL_FORMAT_BGRA_8888,
#if PLATFORM_SDK_VERSION >= 26
	MALI_GRALLOC_FORMAT_INTERNAL_RGBA_1010102 = HAL_PIXEL_FORMAT_RGBA_1010102,
	MALI_GRALLOC_FORMAT_INTERNAL_RGBA_16161616 = HAL_PIXEL_FORMAT_RGBA_FP16,
#endif /* PLATFORM_SDK_VERSION >= 26 */
	MALI_GRALLOC_FORMAT_INTERNAL_YV12 = HAL_PIXEL_FORMAT_YV12,
	MALI_GRALLOC_FORMAT_INTERNAL_Y8 = HAL_PIXEL_FORMAT_Y8,
	MALI_GRALLOC_FORMAT_INTERNAL_Y16 = HAL_PIXEL_FORMAT_Y16,
	MALI_GRALLOC_FORMAT_INTERNAL_YUV420_888 = HAL_PIXEL_FORMAT_YCbCr_420_888,

	/* Camera specific HAL formats */
	MALI_GRALLOC_FORMAT_INTERNAL_RAW16 = HAL_PIXEL_FORMAT_RAW16,
	MALI_GRALLOC_FORMAT_INTERNAL_RAW12 = HAL_PIXEL_FORMAT_RAW12,
	MALI_GRALLOC_FORMAT_INTERNAL_RAW10 = HAL_PIXEL_FORMAT_RAW10,
	MALI_GRALLOC_FORMAT_INTERNAL_BLOB = HAL_PIXEL_FORMAT_BLOB,

	/* Flexible YUV formats would be parsed but not have any representation as
     * internal format itself but one of the ones below
     */

	/* The internal private formats that have no HAL equivivalent are defined
     * afterwards starting at a specific base range */
	MALI_GRALLOC_FORMAT_INTERNAL_NV12 = MALI_GRALLOC_FORMAT_INTERNAL_RANGE_BASE,
	MALI_GRALLOC_FORMAT_INTERNAL_NV21,
	MALI_GRALLOC_FORMAT_INTERNAL_YUV422_8BIT,

	/* Extended YUV formats
     *
     * NOTE: P010, P210, and Y410 are only supported uncompressed.
     */
	MALI_GRALLOC_FORMAT_INTERNAL_Y0L2,
	MALI_GRALLOC_FORMAT_INTERNAL_P010,
	MALI_GRALLOC_FORMAT_INTERNAL_P210,
	MALI_GRALLOC_FORMAT_INTERNAL_Y210,
	MALI_GRALLOC_FORMAT_INTERNAL_Y410,

	/* Add more internal formats here. Make sure decode_internal_format() is updated. */

	/* These are legacy 0.3 gralloc formats used only by the wrap/unwrap macros. */
	MALI_GRALLOC_FORMAT_INTERNAL_YV12_WRAP,
	MALI_GRALLOC_FORMAT_INTERNAL_Y8_WRAP,
	MALI_GRALLOC_FORMAT_INTERNAL_Y16_WRAP,

	MALI_GRALLOC_FORMAT_INTERNAL_RANGE_LAST,
};

struct android_ycbcr {
    void *y;
    void *cb;
    void *cr;
    size_t ystride;
    size_t cstride;
    size_t chroma_step;

    /** reserved for future use, set to 0 by gralloc's (*lock_ycbcr)() */
    uint32_t reserved[8];
};

// ui/GraphicBuffer.h in Android source code
struct GraphicBufferFunctions
{
    typedef void					(*genericFunc)			();
    typedef status_t				(*initCheckFunc)		(android::GraphicBuffer* buffer);
    typedef status_t				(*lockFunc)				(android::GraphicBuffer* buffer, uint32_t usage, void** vaddr);
    typedef status_t				(*unlockFunc)			(android::GraphicBuffer* buffer);
    typedef ANativeWindowBuffer*	(*getNativeBufferFunc)	(const android::GraphicBuffer* buffer);
    typedef status_t                (*lockYCbCrFunc)        (android::GraphicBuffer* buffer, uint32_t inUsage, android_ycbcr* ycbcr);

    bool                            useConstructor4;
    genericFunc						constructor;
    genericFunc						destructor;
    lockFunc						lock;
    unlockFunc						unlock;
    getNativeBufferFunc				getNativeBuffer;
    initCheckFunc					initCheck;
    lockYCbCrFunc                   lockYCbCr;
    DynamicLibrary                  library;
    GraphicBufferFunctions(const char *fileName);
};

extern GraphicBufferFunctions graphicBufferFunctions;

class GraphicBuffer
{
public:
    // ui/GraphicBuffer.h, hardware/gralloc.h in Android source code
    enum {
        USAGE_SW_READ_NEVER		= 0x00000000,
        USAGE_SW_READ_RARELY	= 0x00000002,
        USAGE_SW_READ_OFTEN		= 0x00000003,
        USAGE_SW_READ_MASK		= 0x0000000f,

        USAGE_SW_WRITE_NEVER	= 0x00000000,
        USAGE_SW_WRITE_RARELY	= 0x00000020,
        USAGE_SW_WRITE_OFTEN	= 0x00000030,
        USAGE_SW_WRITE_MASK		= 0x000000f0,

        USAGE_SOFTWARE_MASK		= USAGE_SW_READ_MASK | USAGE_SW_WRITE_MASK,

        USAGE_PROTECTED			= 0x00004000,

        USAGE_HW_TEXTURE		= 0x00000100,
        USAGE_HW_RENDER			= 0x00000200,
        USAGE_HW_2D				= 0x00000400,
        USAGE_HW_COMPOSER		= 0x00000800,
        USAGE_HW_VIDEO_ENCODER	= 0x00010000,
        USAGE_HW_MASK			= 0x00071F00,
    };

    GraphicBuffer(uint32_t width, uint32_t height, PixelFormat format, uint32_t usage);
    GraphicBuffer(void *ptr);
    ~GraphicBuffer();

    status_t lock(uint32_t usage, void** vaddr);
    status_t lockYCbCr(uint32_t usage, android_ycbcr* ycbcr);
    status_t unlock();
    ANativeWindowBuffer *getNativeBuffer() const;
    int getWidth() const;
    int getHeight() const;
    int getStride() const;
    int getFormat() const;
    int getUsage() const;
    void setWidth(int width);
    void setHeight(int height);
    void setStride(int stride);
    void setFormat(int format);
    void setUsage(int usage);
    void *getImpl() const;

private:
    GraphicBufferFunctions *functions;
    android::GraphicBuffer *impl = nullptr;
};
