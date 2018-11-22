#ifndef _TRACEVIEW_CALL_DETAIL_H_
#define _TRACEVIEW_CALL_DETAIL_H_

#include <wx/richtext/richtextctrl.h>

#include <common/trace_model.hpp>

using namespace common;


class CallDetail : public wxRichTextCtrl
{
public:
    CallDetail(wxWindow* parent, wxWindowID id = -1, const wxString& value = wxEmptyString,
        const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
        long style = wxRE_MULTILINE, const wxValidator& validator = wxDefaultValidator,
        const wxString& name = wxTextCtrlNameStr);

    void Update(CallTM* call);
};

#endif
