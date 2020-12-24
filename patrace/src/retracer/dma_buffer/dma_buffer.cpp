#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "common/os.hpp"
#include "dma_buffer.hpp"
/*
In case of Linux, a YUV texture is created using DMA buf.
On fpga when dma buf module is inserted it creates a virtual device called dma_buf_te in /dev
This can be used to allocated memory for YUV texture. Depending on the format of YUV (in this case NV12 which used 2 planes),
the number of dma_buffers and their sizes are allocated. That number of bytes from the raw yuv files are copied into those buffers.
eglCreateImageKHR in this case uses EGL_LINUX_DMA_BUF_EXT as target and based on the image attributes
it knows how much data to fetch from which buffers in dma buf.
*/

#define PAGE_SIZE 4096
#define TPI_PAGE_SIZE sysconf(_SC_PAGESIZE)
#define DMA_BUF_TE_IOCTL_BASE 'E'
#define DMA_BUF_TE_ALLOC       _IOR(DMA_BUF_TE_IOCTL_BASE, 0x01, struct dma_buf_te_ioctl_alloc)

/* The DMA buf part of the code is taken from ddk/product/egl/tests/internal/image/mali_egl_image_tests_dmabuf.c */

static void get_strides_heights(const mali_tpi_egl_pixmap_format format,
        const unsigned width, const unsigned height, unsigned * const strides,
        unsigned * const heights)
{
    /* Defaults */
    heights[0] = height;
    heights[1] = 0;
    heights[2] = 0;
    strides[0] = (width + 15) / 16 * 16;
    strides[1] = 0;
    strides[2] = 0;

    switch(format)
    {
    case MALI_TPI_EGL_PIXMAP_FORMAT_NV12_BT601_NARROW:
    case MALI_TPI_EGL_PIXMAP_FORMAT_NV12_BT601_WIDE:
    case MALI_TPI_EGL_PIXMAP_FORMAT_NV12_BT709_NARROW:
    case MALI_TPI_EGL_PIXMAP_FORMAT_NV12_BT709_WIDE:
    case MALI_TPI_EGL_PIXMAP_FORMAT_NV21_BT601_NARROW:
    case MALI_TPI_EGL_PIXMAP_FORMAT_NV21_BT601_WIDE:
    case MALI_TPI_EGL_PIXMAP_FORMAT_NV21_BT709_NARROW:
    case MALI_TPI_EGL_PIXMAP_FORMAT_NV21_BT709_WIDE:
        strides[1] = strides[0];
        heights[1] = height/2;
        break;
    case MALI_TPI_EGL_PIXMAP_FORMAT_P010:
    case MALI_TPI_EGL_PIXMAP_FORMAT_P210:
        strides[0] = width * 2;
        strides[1] = width * 2;
        heights[1] = height/2;
        break;
    case MALI_TPI_EGL_PIXMAP_FORMAT_NV16_BT601_NARROW:
    case MALI_TPI_EGL_PIXMAP_FORMAT_NV16_BT601_WIDE:
    case MALI_TPI_EGL_PIXMAP_FORMAT_NV16_BT709_NARROW:
    case MALI_TPI_EGL_PIXMAP_FORMAT_NV16_BT709_WIDE:
        strides[1] = width;
        heights[1] = height;
        break;
    case MALI_TPI_EGL_PIXMAP_FORMAT_B8G8R8:
        strides[0] = 3 * width;
        break;
    case MALI_TPI_EGL_PIXMAP_FORMAT_YV12_BT601_NARROW:
    case MALI_TPI_EGL_PIXMAP_FORMAT_YV12_BT601_WIDE:
        strides[1] = (strides[0] / 2 + 15) / 16 * 16;
        heights[1] = height / 2;
        strides[2] = strides[1];
        heights[2] = heights[1];
        break;
    default:
        DBG_LOG("Invalid format passed to tpi_to_fourcc_format");
        break;
    }
}

struct dma_buf_te_ioctl_alloc
{
    uint64_t size; /* size of buffer to allocate, in pages */
};

static int alloc_and_map_memory(egl_image_fixture *const fix, const size_t size, bool dmaSharedMemory, const unsigned index)
{
    int fd;
    unsigned pages = (size + TPI_PAGE_SIZE - 1) / TPI_PAGE_SIZE;
    fix->memory_sizes[index] = pages * TPI_PAGE_SIZE;

    if (!dmaSharedMemory)
    {
        dma_buf_te_ioctl_alloc data;
        data.size = pages;
        fd = ioctl(fix->dma_buf_te, DMA_BUF_TE_ALLOC, &data);
    }
    else // On model or x86 where shared memory is created
    {
        char ufname[1024];
        snprintf(ufname, sizeof(ufname), "/dmabuf_egl_%lu", (unsigned long int) getpid());
        fd = shm_open(ufname, O_RDWR | O_CREAT, S_IRWXU);

        if(fd > -1)
        {
            shm_unlink(ufname);
            if (0 != ftruncate(fd, TPI_PAGE_SIZE * pages))
            {
                DBG_LOG("alloc memory buf failed for dma_buf[%d] on model.\n", index);
                close(fd);
                return -1;
            }
        }
    }

    if (fd < 0)
    {
        DBG_LOG("alloc memory buf failed for dma_buf[%d]\n", index);
        return -1;
    }

    void *buf = mmap(NULL, fix->memory_sizes[index], PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (buf == MAP_FAILED)
    {
        DBG_LOG("mmap failed for dma_buf[%d]\n", index);
        close(fd);
        return -1;
    }

    fix->memory_bufs[index] = fd;
    fix->memory_mappings[index] = buf;

    return 0;
}

struct dma_buf_sync
{
	uint64_t flags;
};

#define DMA_BUF_SYNC_READ (1 << 0)
#define DMA_BUF_SYNC_WRITE (2 << 0)
#define DMA_BUF_SYNC_RW (DMA_BUF_SYNC_READ | DMA_BUF_SYNC_WRITE)
#define DMA_BUF_SYNC_START (0 << 2)
#define DMA_BUF_SYNC_END (1 << 2)

#define DMA_BUF_BASE 'b'
#define DMA_BUF_IOCTL_SYNC _IOW(DMA_BUF_BASE, 0, struct dma_buf_sync)

static void dma_buf_raw_sync(int fd, uint64_t flags)
{
	struct dma_buf_sync args = { flags };
	int err;

	err = ioctl(fd, DMA_BUF_IOCTL_SYNC, &args);

    if (err)
    {
        DBG_LOG("sync dma buf failed for fd %d \n", fd);
    }
}

void close_fixture_memory_bufs(egl_image_fixture *const fix)
{
    int close_err;
    int i;

    for (i = 0; i != sizeof(fix->memory_bufs) / sizeof(fix->memory_bufs[0]); ++i)
    {
        if(fix->memory_bufs[i] >= 0)
        {
            close_err = close(fix->memory_bufs[i]);
            fix->memory_bufs[i] = -1;
            if (0 != close_err)
                DBG_LOG("close_fixture_memory_bufs failed! close_err = 0x%x\n", close_err);
        }
    }
}

void unmap_fixture_memory_bufs(egl_image_fixture *const fix)
{
    for (int i=0; i != sizeof(fix->memory_mappings) / sizeof(fix->memory_mappings[0]); ++i)
    {
        if (fix->memory_mappings[i] != NULL)
        {
            int close_err = munmap(fix->memory_mappings[i], fix->memory_sizes[i]);
            fix->memory_mappings[i] = NULL;
            if (0 != close_err) DBG_LOG("unmap fixture memory buf failed for dma_buf[%d]! close_err = 0x%x\n", i, close_err);
        }
    }
}

static int memory_init(egl_image_fixture *const fix, bool dmaSharedMemory)
{
    if (!dmaSharedMemory)
    {
        /* We store the dma_bufs allocated in the fixture so they can be cleaned-up.
         * Ensure there are no dma_bufs already there that would be leaked. */
        fix->dma_buf_te = open("/dev/dma_buf_te", O_RDWR|O_CLOEXEC);
        if (-1 == fix->dma_buf_te)
        {
            DBG_LOG("Error creating /dev/dma_buf_te device on the platform. Perhaps dma-buf-test-exporter.ko has not been loaded\n");
            return -1;
        }
    }
    return 0;
}

unsigned int strides[3];
unsigned int heights[3];

int fill_image_attributes(egl_image_fixture *const fix,
                          const mali_tpi_egl_pixmap_format format,
                          uint32_t fourccformat,
                          const GLint width,
                          const GLint height,
                          bool dmaSharedMemory,
                          int &attrib_size,
                          EGLint *attribs)
{
    if (-1 == memory_init(fix, dmaSharedMemory))
    {
        return -1;
    }

    get_strides_heights(format, width, height, strides, heights);

    if (alloc_and_map_memory(fix, strides[0]*heights[0], dmaSharedMemory, 0) < 0)
    {
        return -1;
    }

    if (strides[1] && heights[1])
    {
        if (alloc_and_map_memory(fix, strides[1]*heights[1], dmaSharedMemory, 1) < 0)
        {
            return -1;
        }
    }

    if (strides[2] && heights[2])
    {
        if (alloc_and_map_memory(fix, strides[2]*heights[2], dmaSharedMemory, 2) < 0)
        {
            return -1;
        }
    }

    attrib_size = 0;
    attribs[attrib_size++] = EGL_WIDTH;
    attribs[attrib_size++] = width;
    attribs[attrib_size++] = EGL_HEIGHT;
    attribs[attrib_size++] = height;

    attribs[attrib_size++] = EGL_LINUX_DRM_FOURCC_EXT;
    attribs[attrib_size++] = (EGLint) fourccformat;

    attribs[attrib_size++] = EGL_DMA_BUF_PLANE0_FD_EXT;
    attribs[attrib_size++] = fix->memory_bufs[0];
    attribs[attrib_size++] = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
    attribs[attrib_size++] = 0;
    attribs[attrib_size++] = EGL_DMA_BUF_PLANE0_PITCH_EXT;
    attribs[attrib_size++] = strides[0];

    attribs[attrib_size++] = EGL_YUV_COLOR_SPACE_HINT_EXT;
    attribs[attrib_size++] = EGL_ITU_REC601_EXT;

    if(strides[1] && heights[1])
    {
        attribs[attrib_size++] = EGL_DMA_BUF_PLANE1_FD_EXT;
        attribs[attrib_size++] = fix->memory_bufs[1];
        attribs[attrib_size++] = EGL_DMA_BUF_PLANE1_OFFSET_EXT;
        attribs[attrib_size++] = 0;
        attribs[attrib_size++] = EGL_DMA_BUF_PLANE1_PITCH_EXT;
        attribs[attrib_size++] = strides[1];
    }

    if(strides[2] && heights[2])
    {
        attribs[attrib_size++] = EGL_DMA_BUF_PLANE2_FD_EXT;
        attribs[attrib_size++] = fix->memory_bufs[2];
        attribs[attrib_size++] = EGL_DMA_BUF_PLANE2_OFFSET_EXT;
        attribs[attrib_size++] = 0;
        attribs[attrib_size++] = EGL_DMA_BUF_PLANE2_PITCH_EXT;
        attribs[attrib_size++] = strides[2];
    }

    switch(format)
    {
    case MALI_TPI_EGL_PIXMAP_FORMAT_NV12_BT601_WIDE:
    case MALI_TPI_EGL_PIXMAP_FORMAT_NV16_BT601_WIDE:
    case MALI_TPI_EGL_PIXMAP_FORMAT_NV21_BT601_WIDE:
    case MALI_TPI_EGL_PIXMAP_FORMAT_YUYV_BT601_WIDE:
    case MALI_TPI_EGL_PIXMAP_FORMAT_YV12_BT601_WIDE:
    case MALI_TPI_EGL_PIXMAP_FORMAT_P010:
    case MALI_TPI_EGL_PIXMAP_FORMAT_P210:
        attribs[attrib_size++] = EGL_SAMPLE_RANGE_HINT_EXT;
        attribs[attrib_size++] = EGL_YUV_FULL_RANGE_EXT;
        break;

    case MALI_TPI_EGL_PIXMAP_FORMAT_NV12_BT601_NARROW:
    case MALI_TPI_EGL_PIXMAP_FORMAT_NV16_BT601_NARROW:
    case MALI_TPI_EGL_PIXMAP_FORMAT_NV21_BT601_NARROW:
    case MALI_TPI_EGL_PIXMAP_FORMAT_YV12_BT601_NARROW:
    case MALI_TPI_EGL_PIXMAP_FORMAT_YUYV_BT601_NARROW:
        attribs[attrib_size++] = EGL_SAMPLE_RANGE_HINT_EXT;
        attribs[attrib_size++] = EGL_YUV_NARROW_RANGE_EXT;
        break;

    default:
        break;
    }

    attribs[attrib_size++] = EGL_NONE;

    return 0;
}

static void sync_memcpy(egl_image_fixture *const fix, size_t size, const unsigned char *data, const unsigned int index, bool dmaSharedMemory)
{
    if (!dmaSharedMemory)
    {
        dma_buf_raw_sync(fix->memory_bufs[index], DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW);
    }

    memcpy((void*)fix->memory_mappings[index], data, size);

    if (!dmaSharedMemory)
    {
        dma_buf_raw_sync(fix->memory_bufs[index], DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW);
    }
}

int refresh_dma_data(egl_image_fixture *const fix, size_t size, const unsigned char *data, bool dmaSharedMemory)
{
    if (fix->format == MALI_TPI_EGL_PIXMAP_FORMAT_B8G8R8)
    {
        sync_memcpy(fix, size, data, 0, dmaSharedMemory);
    }
    else if (fix->format == MALI_TPI_EGL_PIXMAP_FORMAT_YV12_BT601_NARROW ||
             fix->format == MALI_TPI_EGL_PIXMAP_FORMAT_YV12_BT601_WIDE)
    {
        sync_memcpy(fix, strides[0] * heights[0], data, 0, dmaSharedMemory);
        sync_memcpy(fix, strides[1] * heights[1], data+strides[0]*heights[0], 1, dmaSharedMemory);
        sync_memcpy(fix, strides[2] * heights[2], data+strides[0]*heights[0]+strides[1]*heights[1], 2, dmaSharedMemory);
    }
    else
    {
        /* This memcpy is again specific to NV12 and NV21 formats because of the packing.
        The format uses Y plane followed by interleaved UV plane, Y plane which is 2/3rd of the total size
        is in [0] and UV place in [1]*/
        sync_memcpy(fix, strides[0] * heights[0], data, 0, dmaSharedMemory);
        sync_memcpy(fix, strides[1] * heights[1], data+strides[0]*heights[0], 1, dmaSharedMemory);
    }
    return 0;
}
