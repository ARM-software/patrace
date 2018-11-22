#ifndef _TRACEVIEW_TEXTURE_FRAME_
#define _TRACEVIEW_TEXTURE_FRAME_

#include <wx/wx.h>

#include "image/image.hpp"

// Display a input wxBitmap, which should be a RGB bitmap
class ImagePanel : public wxPanel
{
public:
    ImagePanel(wxWindow *parent, const wxBitmap *bitmap);
    virtual ~ImagePanel();

    void OnPaint(wxPaintEvent &event);

    DECLARE_EVENT_TABLE()

private:
    const wxBitmap *_bitmap;
};

// Window for texture viewing, and the input image will be
// converted to a RGB bitmap if necessary
class TextureFrame : public wxFrame
{
public:
    TextureFrame(wxWindow *parent, const pat::Image &image);

private:
    // Resize the frame/window according to the size of the bitmap to be displayed
    void ResizeForImage(const wxBitmap *bitmap);
};

#endif
