#include "gl_extension_supported.hpp"

#include <dispatch/eglproc_auto.hpp>
#include <string>

// taken from
// http://www.opengl.org/archives/resources/faq/technical/extensions.htm

GlesFeatures gGlesFeatures;

bool isGlesExtensionSupported(const char *extension)
{
    const GLubyte *extensions = NULL;
    const GLubyte *start;
    GLubyte *where, *terminator;

    if (strcmp(extension, "GL_KHR_debug") == 0)
    {
        const char *vendor = (const char *)_glGetString(GL_VENDOR);
        if (strncasecmp(vendor, "NVIDIA", 6) == 0)
        {
            return true; // NVIDIA support many more extensions than they admit to
        }
    }

    /* Extension names should not have spaces. */
    where = (GLubyte *) strchr(extension, ' ');
    if (where || *extension == '\0') { return 0; }
    extensions = _glGetString(GL_EXTENSIONS);
    /* It takes a bit of care to be fool-proof about parsing the
    OpenGL extensions string. Don't be fooled by sub-strings,
    etc. */
    start = extensions;
    for (;;)
    {
        where = (GLubyte *) strstr((const char *) start, extension);
        if (!where) { break; }
        terminator = where + strlen(extension);
        if (where == start || *(where - 1) == ' ')
            if (*terminator == ' ' || *terminator == '\0') { return 1; }
        start = terminator;
    }
    return false;
}

bool isEglExtensionSupported(EGLDisplay display, const char *extension)
{
    const GLubyte *extensions = NULL;
    const GLubyte *start;
    GLubyte *where, *terminator;

    /* Extension names should not have spaces. */
    where = (GLubyte *) strchr(extension, ' ');
    if (where || *extension == '\0') { return 0; }
    extensions = (GLubyte *)_eglQueryString(display, EGL_EXTENSIONS);
    /* It takes a bit of care to be fool-proof about parsing the
    EGL extensions string. Don't be fooled by sub-strings,
    etc. */
    start = extensions;
    for (;;)
    {
        where = (GLubyte *) strstr((const char *) start, extension);
        if (!where) { break; }
        terminator = where + strlen(extension);
        if (where == start || *(where - 1) == ' ')
            if (*terminator == ' ' || *terminator == '\0') { return 1; }
        start = terminator;
    }
    return false;
}

bool GlesFeatures::Update()
{
    // clean all extensions to be false.
    Support_GL_EXT_discard_framebuffer = false;
    Support_GL_OES_depth24 = false;
    glesVersionCached = -1;

    char delimiter = ' ';

    const char* pExtensions = (const char*)_glGetString(GL_EXTENSIONS);
    if (!pExtensions)
    {
        DBG_LOG("Warning: glGetString(GL_EXTENSIONS) returns NULL. You may need to invoke eglMakeCurrent first.\n");
        return false;
    }

    std::string extensions(pExtensions);
    auto lastPos = extensions.find_first_not_of(delimiter, 0);
    auto pos = extensions.find_first_of(delimiter, lastPos);

    while (pos != std::string::npos || lastPos != std::string::npos)
    {
        mExtensions.push_back(extensions.substr(lastPos, pos - lastPos));

        lastPos = extensions.find_first_not_of(delimiter, pos);
        pos = extensions.find_first_of(delimiter, lastPos);
    }

    for (std::vector<std::string>::size_type i = 0; i < mExtensions.size(); ++i)
    {
        std::string& ext = mExtensions[i];
        if (ext.compare("GL_EXT_discard_framebuffer") == 0) {
            Support_GL_EXT_discard_framebuffer = true;
        } else if (ext.compare("GL_OES_depth24") == 0) {
            Support_GL_OES_depth24 = true;
        }
    }

    return true;
}

int GlesFeatures::glesVersion()
{
    if (glesVersionCached >= 0)
    {
        return glesVersionCached;
    }
    const char *version = (const char *)_glGetString(GL_VERSION);
    int major, minor;
    // OpenGL ES 2.0 Specification - 6.1.5 String Queries - pp. 128
    if (sscanf(version, "OpenGL ES %d.%d", &major, &minor) == 2)
    {
        glesVersionCached = major * 100 + minor * 10; // found GLES 2+ implementation
    }
    else if (sscanf(version, "OpenGL ES-CM %d.%d", &major, &minor) == 2)
    {
        glesVersionCached = major * 100 + minor * 10; // found GLES 1.x implementation
    }
    else
    {
        DBG_LOG("Version string [%s] not parsed correctly -- assuming GLES 2 as fallback.\n", version);
        glesVersionCached = 200;
    }
    return glesVersionCached;
}
