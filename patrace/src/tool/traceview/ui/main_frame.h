#ifndef _TRACEVIEW_MAIN_FRAME_H_
#define _TRACEVIEW_MAIN_FRAME_H_

#include "app.h"

#include <wx/wx.h>
#include <wx/srchctrl.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include "wx/splitter.h"
#include "wx/gauge.h" // display how far loading has come


#include <common/trace_model.hpp>

class FrameCallTree;
class CallDetail;

using namespace common;

// Declare our main frame class
class MainFrame : public wxFrame
{
    enum {
        DisplayQueryCallsCheck_id = wxID_HIGHEST+1,
        FilterText_id,
        SearchNextCallBtn_id,
        SearchPreCallBtn_id,
        GotoCallText_id,
        GotoCallBtn_id,
        MemFrameBeg_id,
        MemFrameEnd_id,
        MemConsumption_id,
    };
public:
    // Constructor
    MainFrame(const wxString& title);
    virtual ~MainFrame();

    void InitToolBar();

    bool OpenFile(const wxString &filepath);
    void UpdateCallDetail(CallTM* call);

    //////////////////////////////////////////////////////
    // Event handlers
    void OnOpen(wxCommandEvent& event);
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnSize(wxSizeEvent& event);
    // Display Filters
    void OnDisplayQueryCallsChanged(wxCommandEvent& event);
    void OnDisplayFilter(wxCommandEvent& event);
    // search
    void OnSearchNextCall(wxCommandEvent& event);
    void OnSearchPreCall(wxCommandEvent& event);
    // goto
    void OnGotoOneCall(wxCommandEvent& event);
    // mem consumption
    void OnSetFrameBegin(wxCommandEvent& event);
    void OnSetFrameEnd(wxCommandEvent& event);

	void loadProgressStart(int numCallsToLoad) {
		mpFrameLoadProgress->SetRange(numCallsToLoad);
		mpFrameLoadProgress->SetValue(0);
		wxWindow::Update();

	}
	void loadProgressUpdate(int callNum) {
		mpFrameLoadProgress->SetValue(callNum);
		wxWindow::Update(); // updates display of this and all children
	}
	
    TraceFileTM mTraceFile;

private:
    void UpdateMemConsumption();

    wxSplitterWindow *mpSplitter;

    wxToolBar*      mpToolBar;
    FrameCallTree*  mpFrameCallTree;
    CallDetail*     mpCallDetail;
	wxGauge*		mpFrameLoadProgress;

    wxSearchCtrl*   mpSearchCallCtrl;
    wxTextCtrl*     mpGotoCallTextCtrl;
    wxTextCtrl*     mpMemFrameBegCtrl;
    wxTextCtrl*     mpMemFrameEndCtrl;
    wxStaticText*   mpMemConsumption;

    int    mFrameBeg;
    int    mFrameEnd;

    // This class handles events
    DECLARE_EVENT_TABLE()
};

#endif
