#include <wx/cmdline.h>

#include "app.h"
#include "ui/main_frame.h"
#include <common/api_info.hpp>
#include <common/parse_api.hpp>

using namespace common;

IMPLEMENT_APP(TraceViewApp)

namespace
{
    const wxCmdLineEntryDesc COMMAND_LINE_DESC[] =
    {
        { wxCMD_LINE_SWITCH, wxT_2("h"), wxT_2("help"), wxT_2("Displays help on the command line parameters"),
            wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
        { wxCMD_LINE_PARAM, NULL, NULL, wxT_2("trace_filepath"),
            wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_NONE }
    };
}

TraceViewApp::TraceViewApp():mpMainFrame(NULL)
{
}


TraceViewApp::~TraceViewApp()
{

}

// Initialize the application
bool TraceViewApp::OnInit()
{
    // OnInitCmdLine and OnCmdLineParsed will be called by
    // wxApp::OnInit
    if (!wxApp::OnInit())
        return false;

    gApiInfo.RegisterEntries(parse_callbacks);

    // Create the main application window
    mpMainFrame = new MainFrame(wxT("Tracefile viewer"));
    mpMainFrame->CreateStatusBar();
    // Show it
    mpMainFrame->Show(true);

    if (_trace_filepath.IsEmpty() == false)
    {
        if (mpMainFrame->OpenFile(_trace_filepath))
        {
            mpMainFrame->SetStatusText(wxString::Format(wxT("Successfully open %s"), _trace_filepath.c_str()));
        }
        else
        {
            mpMainFrame->SetStatusText(wxString::Format(wxT("Failed to open %s"), _trace_filepath.c_str()));
        }
    }

    return true;
}

void TraceViewApp::OnInitCmdLine(wxCmdLineParser &parser)
{
    parser.SetDesc(COMMAND_LINE_DESC);
    parser.SetSwitchChars(_T("-"));
}

bool TraceViewApp::OnCmdLineParsed(wxCmdLineParser &parser)
{
    if (!wxApp::OnCmdLineParsed(parser))
        return false;

    _trace_filepath = parser.GetParam(0);
    return true;
}
