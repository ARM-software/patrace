#include "ui/texture_frame.hpp"

BEGIN_EVENT_TABLE(ImagePanel, wxPanel)
    EVT_PAINT(ImagePanel::OnPaint)
END_EVENT_TABLE()

ImagePanel::ImagePanel(wxWindow *parent, const wxBitmap *bitmap)
: wxPanel(parent), _bitmap(bitmap)
{
}

ImagePanel::~ImagePanel()
{
    delete _bitmap;
}

void ImagePanel::OnPaint(wxPaintEvent &event)
{
    if (_bitmap && _bitmap->IsOk())
    {
        wxPaintDC dc(this);
        dc.DrawBitmap(*_bitmap, 0, 0, false);
    }
}

TextureFrame::TextureFrame(wxWindow *parent, const pat::Image &image)
: wxFrame(parent, wxID_ANY, wxT("Texture Viewer"))
{
    // Add a layout for the frame/window
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    SetSizer(sizer);

    // TODO: Convert the input image into a RGB bitmap

    // Draw a dummy bitmap
    wxBitmap *bitmap = new wxBitmap;
    sizer->Add(new ImagePanel(this, bitmap), 1, wxEXPAND);

    ResizeForImage(bitmap);
}

void TextureFrame::ResizeForImage(const wxBitmap *bitmap)
{
    if (bitmap && bitmap->IsOk())
    {
        const int width = bitmap->GetWidth();
        const int height = bitmap->GetHeight();

        SetSize(width, height);
    }
}
