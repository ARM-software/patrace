#include <map>

#include <ui/main_frame.h>

#include <ui/frame_call_tree.h>
#include <ui/call_detail.h>

#pragma GCC diagnostic ignored "-Wwrite-strings"

#include <res/arrow_down_16_ns.xpm>
#include <res/arrow_up_16_ns.xpm>

#include <wx/filedlg.h>
#include <wx/bmpbuttn.h>


BEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_SIZE(MainFrame::OnSize)

    EVT_MENU(wxID_OPEN, MainFrame::OnOpen)
    EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)
    EVT_MENU(wxID_EXIT, MainFrame::OnQuit)

    EVT_CHECKBOX(DisplayQueryCallsCheck_id, MainFrame::OnDisplayQueryCallsChanged)
    EVT_TEXT_ENTER(FilterText_id, MainFrame::OnDisplayFilter)

    EVT_BUTTON(SearchPreCallBtn_id, MainFrame::OnSearchPreCall)
    EVT_BUTTON(SearchNextCallBtn_id, MainFrame::OnSearchNextCall)
    EVT_BUTTON(GotoCallBtn_id, MainFrame::OnGotoOneCall)

    EVT_TEXT_ENTER(MemFrameBeg_id, MainFrame::OnSetFrameBegin)
    EVT_TEXT_ENTER(MemFrameEnd_id, MainFrame::OnSetFrameEnd)

END_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title)
: wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(1024, 768)),
mpSplitter(NULL),
mpFrameCallTree(NULL),
mpCallDetail(NULL),
mpSearchCallCtrl(NULL),
mpGotoCallTextCtrl(NULL),
mpMemFrameBegCtrl(NULL),
mpMemFrameEndCtrl(NULL),
mpMemConsumption(NULL),
mFrameBeg(0),
mFrameEnd(0)
{
    // Create a menu bar
    wxMenu *fileMenu = new wxMenu;
    // The About item should be in the help menu
    wxMenu *helpMenu = new wxMenu;
    helpMenu->Append(wxID_ABOUT, wxT("&About...\tF1"),
        wxT("Show about dialog"));

    fileMenu->Append(wxID_OPEN, wxT("&Open\tAlt-O"),
        wxT("Open a trace file"));
    fileMenu->Append(wxID_EXIT, wxT("E&xit\tAlt-X"),
        wxT("Quit this program"));
    // Now append the freshly created menu to the menu bar...
    wxMenuBar *menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, wxT("&File"));
    menuBar->Append(helpMenu, wxT("&Help"));
    // ... and attach this menu bar to the frame
    SetMenuBar(menuBar);
    
    // Split the main window horizontally
    // 1. the upper area for tree view
    // 2. the below area for detailed information of the current selected call
    mpSplitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition);//, wxSize(1000,600) );

    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(mpSplitter, 
        1, // allow expanding vertically
        wxEXPAND | wxALL, // and horizontally
        5 // border
        );

    // Create a progress bar/gauge for when you expand frames in FrameCallTree
    mpFrameLoadProgress = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxSize(100,10) ); //wxPoint(0,0), wxSize(100,50) );
    mpFrameLoadProgress->SetValue(0);    
    sizer->Add(mpFrameLoadProgress, wxSizerFlags().Expand());
    SetSizer(sizer);    
    

    InitToolBar();  
}

MainFrame::~MainFrame()
{

}

void MainFrame::InitToolBar()
{
    // 1. search
    wxBitmap searchUpBmp(arrow_up_16_ns_xpm);
    wxBitmap searchDownBmp(arrow_down_16_ns_xpm);

    const int style = wxTB_FLAT | wxTB_TEXT | wxTB_HORIZONTAL | wxTB_TOP;
    wxToolBar *toolBar = CreateToolBar(style, ToolBar_id);

    // Enable/Disable the display of query calls
    const wxString displayQueryCallToolTip = wxT("Queries calls, like glGet or eglGet calls, are not loaded unless it's checked.\nFor exact criterion, please refer to IsQueryCall method in trace_model.cpp.");
    wxCheckBox *displayQueryCallsCheckBox = new wxCheckBox(toolBar,
        DisplayQueryCallsCheck_id, wxT("Display Query Calls"));
    displayQueryCallsCheckBox->SetValue(mTraceFile.GetLoadQueryCalls());
    displayQueryCallsCheckBox->SetToolTip(displayQueryCallToolTip);
    toolBar->AddControl(displayQueryCallsCheckBox);
    toolBar->AddSeparator();

    // Filter: Just display the functions whose names match the filter string
    const wxString displayFilterToolTip = wxT("Just display the calls whose function names match the input regular expression string.\nJust apply to function names.");
    wxStaticText* displayFilterLabel = new wxStaticText(toolBar, -1, wxT("Filter : "));
    displayFilterLabel->SetToolTip(displayFilterToolTip);
    toolBar->AddControl(displayFilterLabel);
    const wxString displayFilterStr = wxString(mTraceFile.GetLoadFilter().c_str(), wxConvUTF8);
    wxTextCtrl *filterTextCtrl = new wxTextCtrl(toolBar,
        FilterText_id, displayFilterStr, wxDefaultPosition,
        wxSize(200, wxDefaultCoord), wxTE_PROCESS_ENTER);
    filterTextCtrl->SetToolTip(displayFilterToolTip);
    toolBar->AddControl(filterTextCtrl);
    toolBar->AddSeparator();

    // Search call by string
    const wxString searchCallToolTip = wxT("Search the next/previous call whose string representation match the input string.\nApply to the whole string representation of the call, including paratmeters and return value.");
    mpSearchCallCtrl = new wxSearchCtrl(toolBar, -1, wxT("glDraw"),
        wxDefaultPosition, wxSize(200, wxDefaultCoord), wxSUNKEN_BORDER );
    mpSearchCallCtrl->SetToolTip(searchCallToolTip);
    toolBar->AddControl(mpSearchCallCtrl);

    wxBitmapButton *upBtn = new wxBitmapButton(toolBar, SearchPreCallBtn_id, searchUpBmp);
    upBtn->SetToolTip(searchCallToolTip);
    wxBitmapButton *downBtn = new wxBitmapButton(toolBar, SearchNextCallBtn_id, searchDownBmp);
    downBtn->SetToolTip(searchCallToolTip);

    toolBar->AddControl(upBtn);
    toolBar->AddControl(downBtn);
    toolBar->AddSeparator();

    // 2. goto
    const wxString gotoCallToolTip = wxT("Directly jump to the call with specific call number, if it's loaded according to current filter.");
    mpGotoCallTextCtrl = new wxTextCtrl(toolBar, GotoCallText_id, wxT("0"),
        wxDefaultPosition, wxSize(60, wxDefaultCoord));
    mpGotoCallTextCtrl->SetToolTip(gotoCallToolTip);
    wxButton *gotoBtn = new wxButton(toolBar, GotoCallBtn_id, wxT("Goto"),
        wxDefaultPosition, wxSize(50, wxDefaultCoord));
    gotoBtn->SetToolTip(gotoCallToolTip);

    toolBar->AddControl(mpGotoCallTextCtrl);
    toolBar->AddControl(gotoBtn);
    toolBar->AddSeparator();

    // 3. memory consumption
    const wxString beginFrameToolTip = wxT("Set the begin frame index of memory consumption measurement.");
    wxStaticText* pFBStaticText = new wxStaticText(toolBar, MemConsumption_id, wxT("BeginFrame"));
    pFBStaticText->SetToolTip(beginFrameToolTip);
    mpMemFrameBegCtrl = new wxTextCtrl(toolBar, MemFrameBeg_id,
        wxString::Format(wxT("%d"), mFrameBeg),
        wxDefaultPosition, wxSize(60, wxDefaultCoord), wxTE_PROCESS_ENTER);
    mpMemFrameBegCtrl->SetToolTip(beginFrameToolTip);

    const wxString endFrameToolTip = wxT("Set the end frame index of memory consumption measurement.");
    wxStaticText* pFEStaticText = new wxStaticText(toolBar, MemConsumption_id, wxT("EndFrame"));
    pFEStaticText->SetToolTip(endFrameToolTip);
    mpMemFrameEndCtrl = new wxTextCtrl(toolBar, MemFrameEnd_id,
        wxString::Format(wxT("%d"), mFrameEnd),
        wxDefaultPosition, wxSize(60, wxDefaultCoord), wxTE_PROCESS_ENTER);
    mpMemFrameEndCtrl->SetToolTip(endFrameToolTip);

    const wxString memConsumptionToolTip = wxT("How much memory (in MB) is needed to load the frame range (including the begin frame and end frame).");
    mpMemConsumption = new wxStaticText(toolBar, MemConsumption_id, wxT("0"));
    mpMemConsumption->SetToolTip(memConsumptionToolTip);

    toolBar->AddControl(pFBStaticText);
    toolBar->AddControl(mpMemFrameBegCtrl);
    toolBar->AddControl(pFEStaticText);
    toolBar->AddControl(mpMemFrameEndCtrl);
    toolBar->AddControl(mpMemConsumption);
    toolBar->AddSeparator();

    toolBar->Realize();
}

void MainFrame::OnOpen(wxCommandEvent& event)
{
    wxFileDialog* pOpenDlg = new wxFileDialog(
        this, wxT("Choose a file to open"), wxEmptyString, wxEmptyString,
        wxT("Trace file (*.pat;*.pat.ra)|*.pat;*.pat.ra"),
        wxFD_OPEN, wxDefaultPosition);

    if (pOpenDlg->ShowModal() == wxID_OK)
    {
        wxString wxFilePath = pOpenDlg->GetPath();
        OpenFile(wxFilePath);
    }
}

bool MainFrame::OpenFile(const wxString &filepath)
{
    if (!mpFrameCallTree)
    {
        mpFrameCallTree = new FrameCallTree(this, mpSplitter, FrameCallTree_id,
            wxDefaultPosition, wxDefaultSize,
            wxTR_ROW_LINES|wxTR_HIDE_ROOT|wxTR_HAS_BUTTONS|wxTR_SINGLE|wxTR_HAS_VARIABLE_ROW_HEIGHT);
        wxSize size = GetClientSize();
        mpFrameCallTree->SetSize(0, 0, size.x, size.y);

        mpCallDetail = new CallDetail(mpSplitter, CallDetail_id);
        mpSplitter->SplitHorizontally(mpFrameCallTree, mpCallDetail);
        mpSplitter->Unsplit();
    }
    SetTitle(filepath);
    return mpFrameCallTree->LoadFrames(filepath);
}

void MainFrame::OnSize(wxSizeEvent& event)
{
    wxSize size = GetClientSize();
    if (mpFrameCallTree)
        mpFrameCallTree->SetSize(0, 0, size.x, size.y);

    event.Skip();
}

void MainFrame::OnAbout(wxCommandEvent& event)
{
    wxString msg;
    msg.Printf(wxT("Traceviewer version: %s. (PA team, ARM)"),
        "0.1");
    wxMessageBox(msg, wxT("About..."),
        wxOK | wxICON_INFORMATION, this);
}
void MainFrame::OnQuit(wxCommandEvent& event)
{
    // Destroy the frame
    Close();
}

void MainFrame::OnDisplayQueryCallsChanged(wxCommandEvent &event)
{
    mTraceFile.SetLoadQueryCalls(event.IsChecked());

    mpFrameCallTree->ReloadLoadedFrames();
}

void MainFrame::OnDisplayFilter(wxCommandEvent &event)
{
    const wxString filterStr = event.GetString();
    mTraceFile.SetLoadFilter((const char *)(filterStr.mb_str()));
    mpFrameCallTree->ReloadLoadedFrames();
}

void MainFrame::OnSearchNextCall(wxCommandEvent& event)
{
    if (mpFrameCallTree)
    {
        wxString strPattern = mpSearchCallCtrl->GetLineText(0);

        unsigned int curCall = mpFrameCallTree->GetCurrentCallNo();
        unsigned int selCall = mTraceFile.FindNext(curCall, strPattern.ToAscii());
        mpFrameCallTree->SelectCall(selCall);

        if (curCall == selCall)
            wxMessageBox(wxT("Reach the end of the trace file!"), wxT("Warning"), wxOK | wxICON_INFORMATION, this);
    }
}

void MainFrame::OnSearchPreCall(wxCommandEvent& event)
{
    if (mpFrameCallTree)
    {
        wxString strPattern = mpSearchCallCtrl->GetLineText(0);

        unsigned int curCall = mpFrameCallTree->GetCurrentCallNo();
        unsigned int selCall = mTraceFile.FindPrevious(curCall, strPattern.ToAscii());
        mpFrameCallTree->SelectCall(selCall);

        if (curCall == selCall)
            wxMessageBox(wxT("Reach the end of the trace file!"), wxT("Warning"), wxOK | wxICON_INFORMATION, this);
    }
}

void MainFrame::OnGotoOneCall(wxCommandEvent& event)
{
    if (mpFrameCallTree)
    {
        wxString strCallNo = mpGotoCallTextCtrl->GetLineText(0);
        if (strCallNo.IsNumber())
        {
            unsigned long callNo = 0;
            strCallNo.ToULong(&callNo);
            mpFrameCallTree->SelectCall(callNo);
        }
    }
}

void MainFrame::OnSetFrameBegin(wxCommandEvent& event)
{
    UpdateMemConsumption();
}

void MainFrame::OnSetFrameEnd(wxCommandEvent& event)
{
    UpdateMemConsumption();
}

void MainFrame::UpdateMemConsumption()
{
    if (!mpFrameCallTree)
        return;

    wxString strFrameNo = mpMemFrameBegCtrl->GetLineText(0);
    if (!strFrameNo.IsNumber())
        return;
    unsigned long frameNo = 0;
    strFrameNo.ToULong(&frameNo);
    mFrameBeg = frameNo;

    strFrameNo = mpMemFrameEndCtrl->GetLineText(0);
    if (!strFrameNo.IsNumber())
        return;
    strFrameNo.ToULong(&frameNo);
    mFrameEnd = frameNo;

    if (mFrameBeg < 0)
    {
        mFrameBeg = 0;
        mpMemFrameBegCtrl->SetValue(wxString::Format(wxT("%d"), mFrameBeg));
    }

    if (mFrameEnd >= int(mTraceFile.mFrames.size()))
    {
        mFrameEnd = mTraceFile.mFrames.size() - 1;
        mpMemFrameEndCtrl->SetValue(wxString::Format(wxT("%d"), mFrameEnd));
    }

    unsigned int totalBytes = 0;
    for (unsigned int fNo = mFrameBeg;
        (int)fNo <= mFrameEnd;
        ++fNo) {
        totalBytes += mTraceFile.mFrames[fNo]->mBytes;
    }

    const float fMem = totalBytes / (1024.0f*1024.0f);
    wxString strMem;
    strMem.Printf(wxT("%.1f MB"), fMem);
    mpMemConsumption->SetLabel(strMem);
}

void MainFrame::UpdateCallDetail(CallTM* call)
{
    if (!call)
    {
        mpSplitter->Unsplit();
        return;
    }

    mpSplitter->SplitHorizontally(mpFrameCallTree, mpCallDetail, -100);
    mpCallDetail->Update(call);
}


