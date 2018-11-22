#ifndef _TOOL_TRACEVIEW_APP_H_
#define _TOOL_TRACEVIEW_APP_H_

#include <wx/wx.h>

enum {
    FrameCallTree_id = wxID_HIGHEST+1,
    ToolBar_id,
    CallDetail_id,
};

class MainFrame;

class TraceViewApp : public wxApp
{
public:
    TraceViewApp();
    virtual ~TraceViewApp();

    virtual bool OnInit();
    virtual void OnInitCmdLine(wxCmdLineParser &parser);
    virtual bool OnCmdLineParsed(wxCmdLineParser &parser);

    MainFrame *mpMainFrame;

private:
    wxString _trace_filepath;
};

DECLARE_APP(TraceViewApp)

#endif
