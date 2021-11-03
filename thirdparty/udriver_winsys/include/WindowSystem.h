#ifndef __WindowSystem_h__
#define __WindowSystem_h__

#include "gformats.h"   // XXX need to fix

#ifdef WIN32
#ifndef WS_APICALL
#define WS_APICALL __declspec(dllimport)
#endif

#ifndef WS_APIENTRY
#define WS_APIENTRY __stdcall
#endif

#else
#define WS_APICALL
#define WS_APIENTRY
#endif

namespace WindowSystem
{
typedef bool (* RunProc)(unsigned int frameNumber);

struct Window
{
    virtual const unsigned int           GetWidth()              = 0;
    virtual const unsigned int           GetHeight()             = 0;
    virtual GFormats::ColorFormat        GetColorFormat()        = 0;
    virtual GFormats::DepthStencilFormat GetDepthStencilFormat() = 0;
};

struct Display
{
    virtual unsigned int GetSupportedFormatCount()                                                                                        = 0;
    virtual bool         GetSupportedFormat(unsigned int index, GFormats::ColorFormat* color, GFormats::DepthStencilFormat* depthstencil) = 0;
};


WS_APICALL Display* WS_APIENTRY GetDisplay(const char* name);
WS_APICALL Window*  WS_APIENTRY CreateWin(WindowSystem::Display*       display,
                                          unsigned int                 width,
                                          unsigned int                 height,
                                          const char*                  title,
                                          GFormats::ColorFormat        color,
                                          GFormats::DepthStencilFormat format);
WS_APICALL void WS_APIENTRY DestroyWin(WindowSystem::Display* display, WindowSystem::Window* win);

WS_APICALL void WS_APIENTRY Run(WindowSystem::Window* window, RunProc proc);
}



#endif // __WindowSystem_h__
