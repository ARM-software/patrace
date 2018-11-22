#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <cstring>

#include "tool/config.hpp"
#include "eglstate/common.hpp"
#include "image/image_all.hpp"
using namespace pat;

namespace
{

void PrintUsage(const char *argv0)
{
    std::cout <<
        "\n"
        "Usage: " << argv0 << " [OPTION] INPUT_IMAGE [OUTPUT_IMAGE]\n"
        "Version: " PATRACE_VERSION "\n"
        "  The input and output format will be depend on file extensions.\n"
        "  If the output path is not provided, the content is the input image will be displayed.\n"
        "  Accepted formats :\n"
        "      .ktx : Accept all OpenGL formats. (http://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec)\n"
        "      .png : Luminance only, alpha only, RGB or RGBA. (http://en.wikipedia.org/wiki/Portable_Network_Graphics)\n"
        "\n"
        "  -h          help, display this\n"
        "\n"
        ;
}

// Return the lower-case file extension of the input filepath.
// E.g, return ".txt" for "./path_info /foo/bar/baa.txt"
// And return empty string if can't find the extension.
const std::string extension(const std::string &filepath)
{
    const char dot = '.';
    const std::size_t dot_pos = filepath.find_last_of(dot);
    if (dot_pos == std::string::npos)
        return std::string();

    std::string ext = filepath.substr(dot_pos);
    std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
    return ext;
}

const std::string KTX_EXTENSION = ".ktx";
const std::string PNG_EXTENSION = ".png";
const std::string TIFF_EXTENSION = ".tiff";
const std::string JPG_EXTENSION = ".jpg";
const std::string JPEG_EXTENSION = ".jpeg";

} // unnamed namespace

int main(int argc, char **argv)
{
    int i = 1;
    for (; i < argc; ++i)
    {
        const char *arg = argv[i];

        if (arg[0] != '-') {
            break;
        }

        if (!strcmp(arg, "-h")) {
            PrintUsage(argv[0]);
            return 0;
        }
        else {
            std::cout << "Unknown option : " << arg << std::endl;
            PrintUsage(argv[0]);
            return -1;
        }
    }

    if (i + 1 >  argc)
    {
        PrintUsage(argv[0]);
        return -1;
    }

    const std::string input_filename = argv[i++];
    const std::string output_filename = (i >= argc) ? std::string() : argv[i++];
    const std::string input_ext = extension(input_filename);

    typedef mpl::vector<image_type<GL_RGBA, GL_UNSIGNED_BYTE>::type,
                        image_type<GL_RGB, GL_UNSIGNED_SHORT_5_6_5>::type> image_types;
    typedef gil::any_image<image_types> runtime_image_t;
    runtime_image_t input_image;
    uint32_t ogl_format = GL_NONE, ogl_type = GL_NONE;

    if (input_ext == KTX_EXTENSION)
    {
        ktx_read_image(input_filename, input_image, &ogl_format, &ogl_type);
    }
    //else if (input_ext == PNG_EXTENSION)
    //{
        //gil::png_read_image(input_filename, input_image);
    //}
    else
    {
        std::cout << "Unknown input file extension : " << input_ext << std::endl;
        PrintUsage(argv[0]);
        return -1;
    }

    // Print information of the input image
    std::cout << "Input Image :" << std::endl;
    std::cout << "    Image path : " << input_filename << std::endl;
    std::cout << "    Width : " << input_image.width() << std::endl;
    std::cout << "    Height : " << input_image.height() << std::endl;
    std::cout << "    OpenGL Format : " << EnumString(ogl_format) << std::endl;
    std::cout << "    OpenGL Type : " << EnumString(ogl_type) << std::endl;
    //std::cout << "    Data size : " << image.DataSize() << std::endl;
    std::cout << std::endl;

    if (output_filename.empty() == false)
    {
        const std::string output_ext = extension(output_filename);
        if (output_ext == PNG_EXTENSION)
        {
            typedef image_type<GL_RGBA, GL_UNSIGNED_BYTE>::type output_image_t;
            output_image_t output_image(input_image.width(), input_image.height());
            gil::copy_and_convert_pixels(gil::const_view(input_image), gil::view(output_image));

            gil::png_write_view(output_filename, const_view(output_image));

            // Print information of the output image
            std::cout << "Output Image :" << std::endl;
            std::cout << "    Image path : " << output_filename << std::endl;
            std::cout << "    Width : " << output_image.width() << std::endl;
            std::cout << "    Height : " << output_image.height() << std::endl;
        }
        else
        {
            std::cout << "Unknown output file extension : " << output_ext << std::endl;
            PrintUsage(argv[0]);
            return -1;
        }
    }
    //pat::Image image;
    //if (pat::ReadImage(image, input_filename))
    //{
        //if (output_filename && pat::WriteImage(image, output_filename, false))
        //{
            //return 0;
        //}
        //else
        //{
            //std::cout << "Image path : " << input_filename << std::endl;
            //std::cout << "Width : " << image.Width() << std::endl;
            //std::cout << "Height : " << image.Height() << std::endl;
            //std::cout << "Format : " << EnumString(image.Format()) << std::endl;
            //std::cout << "Type : " << EnumString(image.Type()) << std::endl;
            //std::cout << "Data size : " << image.DataSize() << std::endl;
        //}
    //}

    return -1;
}
