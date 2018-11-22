#ifndef _GLSTATE_HPP_
#define _GLSTATE_HPP_

#include "dispatch/eglimports.hpp"
#include <string>
#include <vector>

// Forward declarations
namespace image {
    class Image;
}
struct Texture;

namespace glstate {

image::Image* getDrawBufferImage(int attachment=0, int _width=0, int _height=0, GLenum format=GL_RGBA, GLenum type=GL_UNSIGNED_BYTE, int bytes_per_pixel=4);
std::vector<std::string> dumpTexture(Texture& tex, unsigned int callNo, GLfloat* vertices, int face=-1, GLuint* cm_indices=0); // face=-1 if not cube map
GLint getMaxColorAttachments();
GLint getMaxDrawBuffers();
GLint getColorAttachment(GLint drawBuffer);

}

#endif  /* _GLSTATE_HPP_ */