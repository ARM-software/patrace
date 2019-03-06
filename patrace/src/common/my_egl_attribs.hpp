#ifndef _EGL_CONFIG_VALUES_H_
#define _EGL_CONFIG_VALUES_H_

struct MyEGLAttribs
{
    // This class stores attribs retrieved with eglGetConfigAttrib
    unsigned char redSize = 0;
    unsigned char greenSize = 0;
    unsigned char blueSize = 0;
    unsigned char alphaSize = 0;
    unsigned char depthSize = 0;
    unsigned char stencilSize = 0;
    unsigned char msaaSamples = 0;
    unsigned int timesUsed = 0;
    int traceConfigId = 0; // Different for each platform, useful for debug
};

#endif
