#ifndef __GFormats_h__
#define __GFormats_h__

//
// Format list supported by EGL and Window system for external memory to the driver.
// The GLES side supports all GLES formats.
//

namespace GFormats
{
enum ColorFormat
{
    COLOR_FORMAT_NONE,
    COLOR_FORMAT_RGBA8,
    COLOR_FORMAT_RGB8
};

enum DepthStencilFormat
{
    DEPTH_STENCIL_FORMAT_NONE,
    DEPTH_STENCIL_FORMAT_D16,
    DEPTH_STENCIL_FORMAT_D24,
    DEPTH_STENCIL_FORMAT_D24_S8
};
}

#endif
