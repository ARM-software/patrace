#ifdef WIN32
#define WS_APICALL __declspec(dllexport)
#endif


#include "WindowSystem.h"
#include <string>
#include <assert.h>

struct format_pair
{
    GFormats::ColorFormat        c;
    GFormats::DepthStencilFormat ds;
};

static const unsigned int format_count    = 8;
static const format_pair  format_table[8] = { { GFormats::COLOR_FORMAT_RGB8, GFormats::DEPTH_STENCIL_FORMAT_NONE },
                                              { GFormats::COLOR_FORMAT_RGB8, GFormats::DEPTH_STENCIL_FORMAT_D16 },
                                              { GFormats::COLOR_FORMAT_RGB8, GFormats::DEPTH_STENCIL_FORMAT_D24 },
                                              { GFormats::COLOR_FORMAT_RGB8, GFormats::DEPTH_STENCIL_FORMAT_D24_S8 },
                                              { GFormats::COLOR_FORMAT_RGBA8, GFormats::DEPTH_STENCIL_FORMAT_NONE },
                                              { GFormats::COLOR_FORMAT_RGBA8, GFormats::DEPTH_STENCIL_FORMAT_D16 },
                                              { GFormats::COLOR_FORMAT_RGBA8, GFormats::DEPTH_STENCIL_FORMAT_D24 },
                                              { GFormats::COLOR_FORMAT_RGBA8, GFormats::DEPTH_STENCIL_FORMAT_D24_S8 } };


class udriverDisplay : public WindowSystem::Display
{
public:
    virtual unsigned int GetSupportedFormatCount()
    {
        return format_count;
    }
    virtual bool         GetSupportedFormat(unsigned int index, GFormats::ColorFormat* color, GFormats::DepthStencilFormat* depthstencil)
    {
        assert(index < format_count);
        *color        = format_table[index].c;
        *depthstencil = format_table[index].ds;
        return true;
    }
    virtual ~udriverDisplay(){}
private:
};


class udriverWindow : public WindowSystem::Window
{
public:
    udriverWindow(unsigned int width, unsigned int height, const char* title, GFormats::ColorFormat color,
                  GFormats::DepthStencilFormat dsf) : m_width(width), m_height(height), m_title(title), m_color_format(color), m_depthstencil_format(
            dsf)
    {
    }

    virtual const unsigned int               GetWidth()
    {
        return m_width;
    }
    virtual const unsigned int               GetHeight()
    {
        return m_height;
    }
    virtual GFormats::ColorFormat        GetColorFormat()
    {
        return m_color_format;
    }
    virtual GFormats::DepthStencilFormat GetDepthStencilFormat()
    {
        return m_depthstencil_format;
    }
    virtual ~udriverWindow(){}
private:
    unsigned int                 m_width;
    unsigned int                 m_height;
    std::string                  m_title;

    GFormats::ColorFormat        m_color_format;
    GFormats::DepthStencilFormat m_depthstencil_format;
};

static udriverDisplay global_display;

WS_APICALL WindowSystem::Display* WS_APIENTRY WindowSystem::GetDisplay(const char* display_string)
{
    return &global_display;
}

WS_APICALL WindowSystem::Window* WS_APIENTRY WindowSystem::CreateWin(WindowSystem::Display*       display,
                                                                     unsigned int                 width,
                                                                     unsigned int                 height,
                                                                     const char*                  title,
                                                                     GFormats::ColorFormat        color,
                                                                     GFormats::DepthStencilFormat dsformat)
{
    udriverWindow* win = new udriverWindow(width, height, title, color, dsformat);

    return win;
}

WS_APICALL void WS_APIENTRY WindowSystem::DestroyWin(WindowSystem::Display* display, WindowSystem::Window* win)
{
    udriverWindow* w = reinterpret_cast<udriverWindow*>(win);
    delete w;
}

WS_APICALL void WS_APIENTRY WindowSystem::Run(WindowSystem::Window* window, WindowSystem::RunProc proc)
{
    unsigned int frameCounter = 0;

    // Main loop
    while (true)
    {
        if (!proc(frameCounter++))   // break out if false is returned
        {
            break;
        }
    }
}
