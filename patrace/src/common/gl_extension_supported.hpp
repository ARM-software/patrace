#ifndef _EXTENSION_SUPPORTED_HPP_
#define _EXTENSION_SUPPORTED_HPP_

#include <string>
#include <vector>
#include <dispatch/eglproc_auto.hpp>

bool isGlesExtensionSupported(const char *extension);
bool isEglExtensionSupported(EGLDisplay display, const char *extension);

class GlesFeatures
{
public:
    GlesFeatures()
      : Support_GL_EXT_discard_framebuffer(false)
      , Support_GL_OES_depth24(false)
      , glesVersionCached(-1)
    {}
    ~GlesFeatures(){}

    // IMPORTANT!
    //
    // Please pay attention to the GLES 2.0 spec of "glGetString".
    //      -- return a string describing *the current GL connection*.
    // which means if you invoke "Update()" while not connected,
    // you will get *undefined* behaviour.
    // For some implementation, the *undefined* behaviour is simply returning NULL.
    //
    // So we'll return *false* if "glGetString" returns NULL.
    bool Update();

    // functionality queries
    bool isProgramInterfaceSupported() { return glesVersion() >= 310; }

    bool Support_GL_EXT_discard_framebuffer;
    bool Support_GL_OES_depth24;

    int glesVersion();

private:

    std::vector<std::string> mExtensions;

    int glesVersionCached;
};

extern GlesFeatures gGlesFeatures;

#endif
