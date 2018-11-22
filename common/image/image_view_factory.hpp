#ifndef _COMMON_IMAGE_IMAGE_VIEW_FACTORY_
#define _COMMON_IMAGE_IMAGE_VIEW_FACTORY_

#include "format_type_traits.hpp"

namespace pat
{

// Construct the given image view with the given dimensions and pixel data.
// Triggers a compile assert if the view color space and channel depth
// are not compatible with the given OpenGL format & type pair.
template <typename image_view_t>
void image_view_from_raw_pixels(image_view_t &view,
    std::size_t width, std::size_t height,
    const typename image_view_t::value_type *pixels)
{
    typedef typename image_view_t::value_type pixel_t;
    view = gil::interleaved_view(width, height,
        (pixel_t*)pixels, sizeof(pixel_t) * width);
}

}

#endif
