#include <ui/call_detail.h>

CallDetail::CallDetail(wxWindow* parent, wxWindowID id , const wxString& value,
        const wxPoint& pos, const wxSize& size,
        long style, const wxValidator& validator,
        const wxString& name)
    : wxRichTextCtrl(parent, id, value, pos, size, style, validator, name)
{
}

void CallDetail::Update(CallTM* call)
{
    Clear();

    if (!call)
        return;

    SetDefaultStyle(wxRichTextAttr());
    BeginSuppressUndo();

    BeginAlignment(wxTEXT_ALIGNMENT_LEFT);
    BeginBold();

    BeginFontSize(14);

    wxString wsStr = wxString::FromAscii(call->ToStr(false).c_str());
    WriteText(wsStr);

    EndFontSize();
    EndBold();
    EndAlignment();
    EndSuppressUndo();

}

