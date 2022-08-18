#ifndef _DMA_BUFFER_HPP_
#define _DMA_BUFFER_HPP_

#include <cstdint>
#include <unistd.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

#define fourcc_code(a,b,c,d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | \
                  ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))
#define DRM_FORMAT_BIG_ENDIAN (1<<31) /* format is big endian instead of little endian */

/* color index */
#define DRM_FORMAT_C8		fourcc_code('C', '8', ' ', ' ') /* [7:0] C */

/* 8 bpp RGB */
#define DRM_FORMAT_RGB332	fourcc_code('R', 'G', 'B', '8') /* [7:0] R:G:B 3:3:2 */
#define DRM_FORMAT_BGR233	fourcc_code('B', 'G', 'R', '8') /* [7:0] B:G:R 2:3:3 */

/* 16 bpp RGB */
#define DRM_FORMAT_XRGB4444	fourcc_code('X', 'R', '1', '2') /* [15:0] x:R:G:B 4:4:4:4 little endian */
#define DRM_FORMAT_XBGR4444	fourcc_code('X', 'B', '1', '2') /* [15:0] x:B:G:R 4:4:4:4 little endian */
#define DRM_FORMAT_RGBX4444	fourcc_code('R', 'X', '1', '2') /* [15:0] R:G:B:x 4:4:4:4 little endian */
#define DRM_FORMAT_BGRX4444	fourcc_code('B', 'X', '1', '2') /* [15:0] B:G:R:x 4:4:4:4 little endian */

#define DRM_FORMAT_ARGB4444	fourcc_code('A', 'R', '1', '2') /* [15:0] A:R:G:B 4:4:4:4 little endian */
#define DRM_FORMAT_ABGR4444	fourcc_code('A', 'B', '1', '2') /* [15:0] A:B:G:R 4:4:4:4 little endian */
#define DRM_FORMAT_RGBA4444	fourcc_code('R', 'A', '1', '2') /* [15:0] R:G:B:A 4:4:4:4 little endian */
#define DRM_FORMAT_BGRA4444	fourcc_code('B', 'A', '1', '2') /* [15:0] B:G:R:A 4:4:4:4 little endian */

#define DRM_FORMAT_XRGB1555	fourcc_code('X', 'R', '1', '5') /* [15:0] x:R:G:B 1:5:5:5 little endian */
#define DRM_FORMAT_XBGR1555	fourcc_code('X', 'B', '1', '5') /* [15:0] x:B:G:R 1:5:5:5 little endian */
#define DRM_FORMAT_RGBX5551	fourcc_code('R', 'X', '1', '5') /* [15:0] R:G:B:x 5:5:5:1 little endian */
#define DRM_FORMAT_BGRX5551	fourcc_code('B', 'X', '1', '5') /* [15:0] B:G:R:x 5:5:5:1 little endian */

#define DRM_FORMAT_ARGB1555	fourcc_code('A', 'R', '1', '5') /* [15:0] A:R:G:B 1:5:5:5 little endian */
#define DRM_FORMAT_ABGR1555	fourcc_code('A', 'B', '1', '5') /* [15:0] A:B:G:R 1:5:5:5 little endian */
#define DRM_FORMAT_RGBA5551	fourcc_code('R', 'A', '1', '5') /* [15:0] R:G:B:A 5:5:5:1 little endian */
#define DRM_FORMAT_BGRA5551	fourcc_code('B', 'A', '1', '5') /* [15:0] B:G:R:A 5:5:5:1 little endian */

#define DRM_FORMAT_RGB565	fourcc_code('R', 'G', '1', '6') /* [15:0] R:G:B 5:6:5 little endian */
#define DRM_FORMAT_BGR565	fourcc_code('B', 'G', '1', '6') /* [15:0] B:G:R 5:6:5 little endian */

/* 24 bpp RGB */
#define DRM_FORMAT_RGB888	fourcc_code('R', 'G', '2', '4') /* [23:0] R:G:B little endian */
#define DRM_FORMAT_BGR888	fourcc_code('B', 'G', '2', '4') /* [23:0] B:G:R little endian */

/* 32 bpp RGB */
#define DRM_FORMAT_XRGB8888	fourcc_code('X', 'R', '2', '4') /* [31:0] x:R:G:B 8:8:8:8 little endian */
#define DRM_FORMAT_XBGR8888	fourcc_code('X', 'B', '2', '4') /* [31:0] x:B:G:R 8:8:8:8 little endian */
#define DRM_FORMAT_RGBX8888	fourcc_code('R', 'X', '2', '4') /* [31:0] R:G:B:x 8:8:8:8 little endian */
#define DRM_FORMAT_BGRX8888	fourcc_code('B', 'X', '2', '4') /* [31:0] B:G:R:x 8:8:8:8 little endian */

#define DRM_FORMAT_ARGB8888	fourcc_code('A', 'R', '2', '4') /* [31:0] A:R:G:B 8:8:8:8 little endian */
#define DRM_FORMAT_ABGR8888	fourcc_code('A', 'B', '2', '4') /* [31:0] A:B:G:R 8:8:8:8 little endian */
#define DRM_FORMAT_RGBA8888	fourcc_code('R', 'A', '2', '4') /* [31:0] R:G:B:A 8:8:8:8 little endian */
#define DRM_FORMAT_BGRA8888	fourcc_code('B', 'A', '2', '4') /* [31:0] B:G:R:A 8:8:8:8 little endian */

#define DRM_FORMAT_XRGB2101010	fourcc_code('X', 'R', '3', '0') /* [31:0] x:R:G:B 2:10:10:10 little endian */
#define DRM_FORMAT_XBGR2101010	fourcc_code('X', 'B', '3', '0') /* [31:0] x:B:G:R 2:10:10:10 little endian */
#define DRM_FORMAT_RGBX1010102	fourcc_code('R', 'X', '3', '0') /* [31:0] R:G:B:x 10:10:10:2 little endian */
#define DRM_FORMAT_BGRX1010102	fourcc_code('B', 'X', '3', '0') /* [31:0] B:G:R:x 10:10:10:2 little endian */

#define DRM_FORMAT_ARGB2101010	fourcc_code('A', 'R', '3', '0') /* [31:0] A:R:G:B 2:10:10:10 little endian */
#define DRM_FORMAT_ABGR2101010	fourcc_code('A', 'B', '3', '0') /* [31:0] A:B:G:R 2:10:10:10 little endian */
#define DRM_FORMAT_RGBA1010102	fourcc_code('R', 'A', '3', '0') /* [31:0] R:G:B:A 10:10:10:2 little endian */
#define DRM_FORMAT_BGRA1010102	fourcc_code('B', 'A', '3', '0') /* [31:0] B:G:R:A 10:10:10:2 little endian */

/* packed YCbCr */
#define DRM_FORMAT_YUYV		fourcc_code('Y', 'U', 'Y', 'V') /* [31:0] Cr0:Y1:Cb0:Y0 8:8:8:8 little endian */
#define DRM_FORMAT_YVYU		fourcc_code('Y', 'V', 'Y', 'U') /* [31:0] Cb0:Y1:Cr0:Y0 8:8:8:8 little endian */
#define DRM_FORMAT_UYVY		fourcc_code('U', 'Y', 'V', 'Y') /* [31:0] Y1:Cr0:Y0:Cb0 8:8:8:8 little endian */
#define DRM_FORMAT_VYUY		fourcc_code('V', 'Y', 'U', 'Y') /* [31:0] Y1:Cb0:Y0:Cr0 8:8:8:8 little endian */

#define DRM_FORMAT_AYUV		fourcc_code('A', 'Y', 'U', 'V') /* [31:0] A:Y:Cb:Cr 8:8:8:8 little endian */

/*
 * 2 plane YCbCr
 * index 0 = Y plane, [7:0] Y
 * index 1 = Cr:Cb plane, [15:0] Cr:Cb little endian
 * or
 * index 1 = Cb:Cr plane, [15:0] Cb:Cr little endian
 */
#define DRM_FORMAT_NV12		fourcc_code('N', 'V', '1', '2') /* 2x2 subsampled Cr:Cb plane */
#define DRM_FORMAT_NV21		fourcc_code('N', 'V', '2', '1') /* 2x2 subsampled Cb:Cr plane */
#define DRM_FORMAT_NV16		fourcc_code('N', 'V', '1', '6') /* 2x1 subsampled Cr:Cb plane */
#define DRM_FORMAT_NV61		fourcc_code('N', 'V', '6', '1') /* 2x1 subsampled Cb:Cr plane */
#define DRM_FORMAT_P010     fourcc_code('P', '0', '1', '0') /* 2x2 subsampled Cr:Cb plane 10 bits per channel */

/*
 * 3 plane YCbCr
 * index 0: Y plane, [7:0] Y
 * index 1: Cb plane, [7:0] Cb
 * index 2: Cr plane, [7:0] Cr
 * or
 * index 1: Cr plane, [7:0] Cr
 * index 2: Cb plane, [7:0] Cb
 */
#define DRM_FORMAT_YUV410	fourcc_code('Y', 'U', 'V', '9') /* 4x4 subsampled Cb (1) and Cr (2) planes */
#define DRM_FORMAT_YVU410	fourcc_code('Y', 'V', 'U', '9') /* 4x4 subsampled Cr (1) and Cb (2) planes */
#define DRM_FORMAT_YUV411	fourcc_code('Y', 'U', '1', '1') /* 4x1 subsampled Cb (1) and Cr (2) planes */
#define DRM_FORMAT_YVU411	fourcc_code('Y', 'V', '1', '1') /* 4x1 subsampled Cr (1) and Cb (2) planes */
#define DRM_FORMAT_YUV420	fourcc_code('Y', 'U', '1', '2') /* 2x2 subsampled Cb (1) and Cr (2) planes */
#define DRM_FORMAT_YVU420	fourcc_code('Y', 'V', '1', '2') /* 2x2 subsampled Cr (1) and Cb (2) planes */
#define DRM_FORMAT_YUV422	fourcc_code('Y', 'U', '1', '6') /* 2x1 subsampled Cb (1) and Cr (2) planes */
#define DRM_FORMAT_YVU422	fourcc_code('Y', 'V', '1', '6') /* 2x1 subsampled Cr (1) and Cb (2) planes */
#define DRM_FORMAT_YUV444	fourcc_code('Y', 'U', '2', '4') /* non-subsampled Cb (1) and Cr (2) planes */
#define DRM_FORMAT_YVU444	fourcc_code('Y', 'V', '2', '4') /* non-subsampled Cr (1) and Cb (2) planes */
#define DRM_FORMAT_Q410     fourcc_code('Q', '4', '1', '0') /* non-subsampled (444) YCbCr */
#define DRM_FORMAT_P210     fourcc_code('P', '2', '1', '0')
#define DRM_FORMAT_Q401     fourcc_code('Q', '4', '0', '1')

/** @brief A format for a pixmap.
 *
 * Not all the possible formats for pixmaps correspond to values of EGLConfig. This enumeration allows you to specify
 * those formats when creating a pixmap with #mali_tpi_egl_create_external_pixmap().
 *
 * YUV formats should be listed first, before MALI_TPI_EGL_PIXMAP_FORMAT_YUV_COUNT.
 * AFBC formats should be listed second, before MALI_TPI_EGL_PIXMAP_FORMAT_AFBC_AND_YUV_COUNT.
 *
 * The size and order of this array must match that of mali_tpip_tpi_format_to_gbm in mali_tpi_linux_egl_gbm.c
 */
typedef enum mali_tpi_egl_pixmap_format
{
	MALI_TPI_EGL_PIXMAP_FORMAT_INVALID = -1,
	MALI_TPI_EGL_PIXMAP_FORMAT_FIRST,
	MALI_TPI_EGL_PIXMAP_FORMAT_FIRST_YUV_FORMAT = MALI_TPI_EGL_PIXMAP_FORMAT_FIRST,

	MALI_TPI_EGL_PIXMAP_FORMAT_YV12_BT601_NARROW = MALI_TPI_EGL_PIXMAP_FORMAT_FIRST,
	MALI_TPI_EGL_PIXMAP_FORMAT_YV12_BT601_WIDE,
	MALI_TPI_EGL_PIXMAP_FORMAT_YV12_BT709_NARROW,
	MALI_TPI_EGL_PIXMAP_FORMAT_YV12_BT709_WIDE,
	MALI_TPI_EGL_PIXMAP_FORMAT_YV12_BT2020_NARROW,
	MALI_TPI_EGL_PIXMAP_FORMAT_YV12_BT2020_WIDE,
	MALI_TPI_EGL_PIXMAP_FORMAT_NV12_BT601_NARROW,
	MALI_TPI_EGL_PIXMAP_FORMAT_NV12_BT601_WIDE,
	MALI_TPI_EGL_PIXMAP_FORMAT_NV12_BT709_NARROW,
	MALI_TPI_EGL_PIXMAP_FORMAT_NV12_BT709_WIDE,
	MALI_TPI_EGL_PIXMAP_FORMAT_NV12_BT2020_NARROW,
	MALI_TPI_EGL_PIXMAP_FORMAT_NV12_BT2020_WIDE,
	MALI_TPI_EGL_PIXMAP_FORMAT_NV16_BT601_NARROW,
	MALI_TPI_EGL_PIXMAP_FORMAT_NV16_BT601_WIDE,
	MALI_TPI_EGL_PIXMAP_FORMAT_NV16_BT709_NARROW,
	MALI_TPI_EGL_PIXMAP_FORMAT_NV16_BT709_WIDE,
	MALI_TPI_EGL_PIXMAP_FORMAT_NV16_BT2020_NARROW,
	MALI_TPI_EGL_PIXMAP_FORMAT_NV16_BT2020_WIDE,
	MALI_TPI_EGL_PIXMAP_FORMAT_NV21_BT601_NARROW,
	MALI_TPI_EGL_PIXMAP_FORMAT_NV21_BT601_WIDE,
	MALI_TPI_EGL_PIXMAP_FORMAT_NV21_BT709_NARROW,
	MALI_TPI_EGL_PIXMAP_FORMAT_NV21_BT709_WIDE,
	MALI_TPI_EGL_PIXMAP_FORMAT_NV21_BT2020_NARROW,
	MALI_TPI_EGL_PIXMAP_FORMAT_NV21_BT2020_WIDE,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUYV_BT601_NARROW,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUYV_BT601_WIDE,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUYV_BT709_NARROW,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUYV_BT709_WIDE,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUYV_BT2020_NARROW,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUYV_BT2020_WIDE,
	MALI_TPI_EGL_PIXMAP_FORMAT_Y0L2,
	MALI_TPI_EGL_PIXMAP_FORMAT_P010,
	MALI_TPI_EGL_PIXMAP_FORMAT_P210,
	MALI_TPI_EGL_PIXMAP_FORMAT_Y210,
	MALI_TPI_EGL_PIXMAP_FORMAT_Y410,
	MALI_TPI_EGL_PIXMAP_FORMAT_A2Y10U10V10,
	MALI_TPI_EGL_PIXMAP_FORMAT_Y10U10Y10V10,
	MALI_TPI_EGL_PIXMAP_FORMAT_V10Y10U10Y10,
	MALI_TPI_EGL_PIXMAP_FORMAT_Y10_U10V10_422,
	MALI_TPI_EGL_PIXMAP_FORMAT_Y10_U10V10_420,
	MALI_TPI_EGL_PIXMAP_FORMAT_Y16_U16V16_420,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV_COUNT,

	/* ALL YUV formats before this */
	MALI_TPI_EGL_PIXMAP_FORMAT_A8B8G8R8_AFBC = MALI_TPI_EGL_PIXMAP_FORMAT_YUV_COUNT,
	MALI_TPI_EGL_PIXMAP_FORMAT_X8B8G8R8_AFBC,
	MALI_TPI_EGL_PIXMAP_FORMAT_B8G8R8_AFBC,
	MALI_TPI_EGL_PIXMAP_FORMAT_B5G6R5_AFBC,

	MALI_TPI_EGL_PIXMAP_FORMAT_A4B4G4R4_AFBC,
	MALI_TPI_EGL_PIXMAP_FORMAT_A1B5G5R5_AFBC,
	MALI_TPI_EGL_PIXMAP_FORMAT_A2B10G10R10_AFBC,
	MALI_TPI_EGL_PIXMAP_FORMAT_A8B8G8R8_AFBC_SPLITBLK,
	MALI_TPI_EGL_PIXMAP_FORMAT_X8B8G8R8_AFBC_SPLITBLK,
	MALI_TPI_EGL_PIXMAP_FORMAT_B8G8R8_AFBC_SPLITBLK,
	MALI_TPI_EGL_PIXMAP_FORMAT_B5G6R5_AFBC_SPLITBLK,
	MALI_TPI_EGL_PIXMAP_FORMAT_A2B10G10R10_AFBC_SPLITBLK,

	MALI_TPI_EGL_PIXMAP_FORMAT_A8B8G8R8_AFBC_SPLITBLK_WIDEBLK,
	MALI_TPI_EGL_PIXMAP_FORMAT_X8B8G8R8_AFBC_SPLITBLK_WIDEBLK,
	MALI_TPI_EGL_PIXMAP_FORMAT_B8G8R8_AFBC_SPLITBLK_WIDEBLK,
	MALI_TPI_EGL_PIXMAP_FORMAT_B5G6R5_AFBC_WIDEBLK,
	MALI_TPI_EGL_PIXMAP_FORMAT_A2B10G10R10_AFBC_SPLITBLK_WIDEBLK,

	MALI_TPI_EGL_PIXMAP_FORMAT_RGB_AFBC_TILED_START,
	MALI_TPI_EGL_PIXMAP_FORMAT_A8B8G8R8_AFBC_TILED = MALI_TPI_EGL_PIXMAP_FORMAT_RGB_AFBC_TILED_START,
	MALI_TPI_EGL_PIXMAP_FORMAT_X8B8G8R8_AFBC_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_B8G8R8_AFBC_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_B5G6R5_AFBC_TILED,

	MALI_TPI_EGL_PIXMAP_FORMAT_A4B4G4R4_AFBC_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_A1B5G5R5_AFBC_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_A2B10G10R10_AFBC_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_A8B8G8R8_AFBC_SPLITBLK_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_X8B8G8R8_AFBC_SPLITBLK_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_B8G8R8_AFBC_SPLITBLK_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_B5G6R5_AFBC_SPLITBLK_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_A2B10G10R10_AFBC_SPLITBLK_TILED,

	MALI_TPI_EGL_PIXMAP_FORMAT_A8B8G8R8_AFBC_SPLITBLK_WIDEBLK_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_X8B8G8R8_AFBC_SPLITBLK_WIDEBLK_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_B8G8R8_AFBC_SPLITBLK_WIDEBLK_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_A2B10G10R10_AFBC_SPLITBLK_WIDEBLK_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_B5G6R5_AFBC_WIDEBLK_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_RGB_AFBC_TILED_END = MALI_TPI_EGL_PIXMAP_FORMAT_B5G6R5_AFBC_WIDEBLK_TILED,

	MALI_TPI_EGL_PIXMAP_FORMAT_FIRST_AFBC_YUV_FORMAT,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_8BIT_BT601_NARROW_AFBC = MALI_TPI_EGL_PIXMAP_FORMAT_FIRST_AFBC_YUV_FORMAT,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_8BIT_BT601_WIDE_AFBC,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_8BIT_BT709_NARROW_AFBC,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_8BIT_BT709_WIDE_AFBC,

	MALI_TPI_EGL_PIXMAP_FORMAT_YUV422_8BIT_BT601_NARROW_AFBC,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV422_8BIT_BT601_WIDE_AFBC,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV422_8BIT_BT709_NARROW_AFBC,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV422_8BIT_BT709_WIDE_AFBC,

	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_10BIT_AFBC,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV422_10BIT_AFBC,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_8BIT_BT601_NARROW_AFBC_SPLITBLK,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_8BIT_BT601_WIDE_AFBC_SPLITBLK,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_8BIT_BT709_NARROW_AFBC_SPLITBLK,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_8BIT_BT709_WIDE_AFBC_SPLITBLK,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_8BIT_BT601_NARROW_AFBC_WIDEBLK,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_8BIT_BT601_WIDE_AFBC_WIDEBLK,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_8BIT_BT709_NARROW_AFBC_WIDEBLK,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_8BIT_BT709_WIDE_AFBC_WIDEBLK,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV422_8BIT_BT601_NARROW_AFBC_WIDEBLK,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV422_8BIT_BT601_WIDE_AFBC_WIDEBLK,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV422_8BIT_BT709_NARROW_AFBC_WIDEBLK,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV422_8BIT_BT709_WIDE_AFBC_WIDEBLK,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_10BIT_AFBC_WIDEBLK,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV422_10BIT_AFBC_WIDEBLK,

	MALI_TPI_EGL_PIXMAP_FORMAT_YUV_AFBC_TILED_START,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_8BIT_BT601_NARROW_AFBC_TILED = MALI_TPI_EGL_PIXMAP_FORMAT_YUV_AFBC_TILED_START,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_8BIT_BT601_WIDE_AFBC_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_8BIT_BT709_NARROW_AFBC_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_8BIT_BT709_WIDE_AFBC_TILED,

	MALI_TPI_EGL_PIXMAP_FORMAT_YUV422_8BIT_BT601_NARROW_AFBC_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV422_8BIT_BT601_WIDE_AFBC_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV422_8BIT_BT709_NARROW_AFBC_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV422_8BIT_BT709_WIDE_AFBC_TILED,

	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_10BIT_AFBC_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV422_10BIT_AFBC_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_8BIT_BT601_NARROW_AFBC_SPLITBLK_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_8BIT_BT601_WIDE_AFBC_SPLITBLK_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_8BIT_BT709_NARROW_AFBC_SPLITBLK_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_8BIT_BT709_WIDE_AFBC_SPLITBLK_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_8BIT_BT601_NARROW_AFBC_WIDEBLK_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_8BIT_BT601_WIDE_AFBC_WIDEBLK_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_8BIT_BT709_NARROW_AFBC_WIDEBLK_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_8BIT_BT709_WIDE_AFBC_WIDEBLK_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV422_8BIT_BT601_NARROW_AFBC_WIDEBLK_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV422_8BIT_BT601_WIDE_AFBC_WIDEBLK_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV422_8BIT_BT709_NARROW_AFBC_WIDEBLK_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV422_8BIT_BT709_WIDE_AFBC_WIDEBLK_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV420_10BIT_AFBC_WIDEBLK_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV422_10BIT_AFBC_WIDEBLK_TILED,
	MALI_TPI_EGL_PIXMAP_FORMAT_YUV_AFBC_TILED_END = MALI_TPI_EGL_PIXMAP_FORMAT_YUV422_10BIT_AFBC_WIDEBLK_TILED,

	/* ALL AFBC formats before this */
	MALI_TPI_EGL_PIXMAP_FORMAT_AFBC_AND_YUV_COUNT,
	/* First uncompressed RGB format must have the same value as the AFBC_AND_YUV count to avoid
	 * invalid value < MALI_TPI_EGL_PIXMAP_FORMAT_AFBC_AND_YUV_COUNT */
	MALI_TPI_EGL_PIXMAP_FORMAT_R8G8B8A8 = MALI_TPI_EGL_PIXMAP_FORMAT_AFBC_AND_YUV_COUNT,
	MALI_TPI_EGL_PIXMAP_FORMAT_B8G8R8A8,
	MALI_TPI_EGL_PIXMAP_FORMAT_A8B8G8R8,
	MALI_TPI_EGL_PIXMAP_FORMAT_A8R8G8B8,
	MALI_TPI_EGL_PIXMAP_FORMAT_R8G8B8X8,
	MALI_TPI_EGL_PIXMAP_FORMAT_B8G8R8X8,
	MALI_TPI_EGL_PIXMAP_FORMAT_X8B8G8R8,
	MALI_TPI_EGL_PIXMAP_FORMAT_X8R8G8B8,
	MALI_TPI_EGL_PIXMAP_FORMAT_R8G8B8,
	MALI_TPI_EGL_PIXMAP_FORMAT_B8G8R8,
	MALI_TPI_EGL_PIXMAP_FORMAT_R5G6B5,
	MALI_TPI_EGL_PIXMAP_FORMAT_B5G6R5,
	MALI_TPI_EGL_PIXMAP_FORMAT_A4B4G4R4,
	MALI_TPI_EGL_PIXMAP_FORMAT_A4R4G4B4,
	MALI_TPI_EGL_PIXMAP_FORMAT_B4G4R4A4,
	MALI_TPI_EGL_PIXMAP_FORMAT_R4G4B4A4,
	MALI_TPI_EGL_PIXMAP_FORMAT_A1B5G5R5,
	MALI_TPI_EGL_PIXMAP_FORMAT_A1R5G5B5,
	MALI_TPI_EGL_PIXMAP_FORMAT_B5G5R5A1,
	MALI_TPI_EGL_PIXMAP_FORMAT_R5G5B5A1,
	MALI_TPI_EGL_PIXMAP_FORMAT_A2B10G10R10,
	MALI_TPI_EGL_PIXMAP_FORMAT_A16B16G16R16_FLOAT,

	MALI_TPI_EGL_PIXMAP_FORMAT_L8,
	/* must be last entry! */
	MALI_TPI_EGL_PIXMAP_FORMAT_COUNT
} mali_tpi_egl_pixmap_format;

struct egl_image_fixture
{
    //Assume DMA BUF is always supported
    int dma_buf_te;
    /* dma_bufs created by tests that need to be freed during cleanup */
    int memory_bufs[3];
    void *memory_mappings[3];
    size_t memory_sizes[3];

    int attrib_size;
    EGLint attribs[50];
    mali_tpi_egl_pixmap_format format;

    egl_image_fixture(mali_tpi_egl_pixmap_format _format)
    {
        dma_buf_te = 0;
        memory_bufs[0] = 0;
        memory_bufs[1] = 0;
        memory_bufs[2] = 0;
        memory_mappings[0] = 0;
        memory_mappings[1] = 0;
        memory_mappings[2] = 0;
        memory_sizes[0] = 0;
        memory_sizes[1] = 0;
        memory_sizes[2] = 0;
        attrib_size = 0;
        format = _format;
    }
};

extern unsigned int strides[3];
extern unsigned int heights[3];

int fill_image_attributes(egl_image_fixture *const fix,
                          const mali_tpi_egl_pixmap_format format,
                          uint32_t fourccformat,
                          const GLint width,
                          const GLint height,
                          bool dmaSharedMemory,
                          int &attrib_size,
                          EGLint *attribs);

int refresh_dma_data(egl_image_fixture *const fix,
                     size_t size,
                     const unsigned char *data, bool dmaSharedMemory);

void unmap_fixture_memory_bufs(egl_image_fixture *const fix);
void close_fixture_memory_bufs(egl_image_fixture *const fix);

#endif //_DMA_BUFFER_HPP_
