#ifndef _IMAGE_KTX_IO_
#define _IMAGE_KTX_IO_

#include "ktx_io_private.hpp"

// If the input KTX file don't have any pixel data,
// the pixels in the final image is unintialized.

namespace pat
{

// Returns the width and height of the KTX file at the specified location.
// Throws std::ios_base::failure if the location does not correspond to a valid KTX file
inline gil::point2<std::ptrdiff_t> ktx_read_dimensions(const char *filename) {
    detail::ktx_reader m(filename);
    return m.get_dimensions();
}

// Returns the width and height of the KTX file at the specified location.
// Throws std::ios_base::failure if the location does not correspond to a valid KTX file
inline gil::point2<std::ptrdiff_t> ktx_read_dimensions(const std::string& filename) {
    return ktx_read_dimensions(filename.c_str());
}

// Allocates a new image whose dimensions are determined by the given ktx image file, and loads the pixels into it.
// Triggers a compile assert if the image color space or channel depth are not supported by the KTX format.
// Throws std::ios_base::failure if the file is not a valid KTX file.
// Throws opengl_format_type_incompatible_failure if its color space or channel depth are not
// compatible with the ones specified by Image
template <typename image_t>
inline void ktx_read_image(const char *filename, image_t &image, uint32_t *ogl_format = NULL, uint32_t *ogl_type = NULL)
{
    detail::ktx_reader m(filename);
    m.read_image(image, ogl_format, ogl_type);
}

// Allocates a new image whose dimensions are determined by the given ktx image file, and loads the pixels into it.
template <typename image_t>
inline void ktx_read_image(const std::string &filename, image_t &image, uint32_t *ogl_format = NULL, uint32_t *ogl_type = NULL)
{
    ktx_read_image(filename.c_str(), image, ogl_format, ogl_type);
}

// Opens the given KTX file name, selects the first type in Images whose color space and channel are compatible to those of the image file
// and creates a new image of that type with the dimensions specified by the image file.
// Throws opengl_format_type_incompatible_failure if none of the types in Images are compatible with the type on disk.
template <typename Images>
inline void ktx_read_image(const char* filename, gil::any_image<Images>& im, uint32_t *ogl_format = NULL, uint32_t *ogl_type = NULL) {
    detail::ktx_reader_dynamic m(filename);
    m.read_image(im, ogl_format, ogl_type);
}

// Reads a KTX image into a run-time instantiated image
template <typename Images>
inline void ktx_read_image(const std::string& filename, gil::any_image<Images>& im, uint32_t *ogl_format = NULL, uint32_t *ogl_type = NULL) {
    ktx_read_image(filename.c_str(), im, ogl_format, ogl_type);
}

// Saves the view to a KTX file specified by the given KTX image file name.
// Throws std::ios_base::failure if it fails to create the file.
template <typename View>
inline void ktx_write_view(const char* filename, const View &view, uint32_t ogl_format, uint32_t ogl_type)
{
    detail::ktx_writer m(filename, ogl_format, ogl_type);
    m.apply(view);
}

// Saves the view to a ktx file specified by the given ktx image file name.
template <typename View>
inline void ktx_write_view(const std::string& filename, const View& view, uint32_t ogl_format, uint32_t ogl_type)
{
    ktx_write_view(filename.c_str(), view, ogl_format, ogl_type);
}

// Saves the currently instantiated view to a ktx file specified by the given ktx image file name.
// Throws opengl_format_type_incompatible_failure if the currently instantiated view type is not supported for writing by the I/O extension
// Throws std::ios_base::failure if it fails to create the file.
template <typename Views>
inline void ktx_write_view(const char* filename, const gil::any_image_view<Views>& views, uint32_t ogl_format, uint32_t ogl_type) {
    detail::ktx_writer_dynamic m(filename, ogl_format, ogl_type);
    m.write_view(views);
}

// Saves the currently instantiated view to a ktx file specified by the given ktx image file name.
template <typename Views>
inline void ktx_write_view(const std::string& filename,const gil::any_image_view<Views>& views, uint32_t ogl_format, uint32_t ogl_type) {
    ktx_write_view(filename.c_str(), views, ogl_format, ogl_type);
}

}

#endif
