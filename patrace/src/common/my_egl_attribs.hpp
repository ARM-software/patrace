#ifndef _EGL_CONFIG_VALUES_H_
#define _EGL_CONFIG_VALUES_H_

struct MyEGLAttribs
{
    // This class stores attribs retrieved with eglGetConfigAttrib
    unsigned char redSize, greenSize, blueSize, alphaSize;
    unsigned char depthSize, stencilSize;
    unsigned char msaaSamples;
    unsigned int timesUsed;
    int traceConfigId; // Different for each platform, useful for debug
};

#endif
