#include <GLES2/gl2.h>

#include <ui/frame_call_tree.h>
#include <ui/texture_frame.hpp>

// Order of event procession:
// 1. When single click of left button happens:
//     Only EVT_TREE_SEL_CHANGED event is emitted;
// 2. When double clicks of left button happens:
//     EVT_TREE_SEL_CHANGED is emitted before EVT_LEFT_DCLICK
BEGIN_EVENT_TABLE(FrameCallTree, wxTreeCtrl)
    EVT_TREE_ITEM_EXPANDING(FrameCallTree_id, FrameCallTree::OnItemExpanding)
    EVT_TREE_ITEM_COLLAPSING(FrameCallTree_id, FrameCallTree::OnItemCollapsing)
    EVT_TREE_SEL_CHANGED(FrameCallTree_id, FrameCallTree::OnSelChanged)
    EVT_LEFT_DCLICK(FrameCallTree::OnLeftDoubleClick)
END_EVENT_TABLE()



FrameCallTree::FrameCallTree(MainFrame* mainFrame, wxWindow *parent, const wxWindowID id,
    const wxPoint& pos, const wxSize& size,
    long style)
    : wxTreeCtrl(parent, id, pos, size, style),
      mpCurSel(NULL),
      mpMainFrame(mainFrame)
{

}

FrameCallTree::~FrameCallTree()
{

}

bool FrameCallTree::LoadFrames(const wxString &filepath)
{
    if (mpMainFrame->mTraceFile.Open(filepath.ToAscii()) == false)
        return false;

    mpCurSel = new FrameCallItemData(RootItem);
    wxTreeItemId rootId = AddRoot(wxT(""), -1, -1, mpCurSel);
    SetItemText(rootId, filepath);

    wxString wsStr;
    wxTreeItemId frameId;



    for (unsigned int fNo = 0;
        fNo < mpMainFrame->mTraceFile.mFrames.size();
        ++fNo)
    {
        wsStr.Printf(wxT("Frame %d (call num: %d) (size: %dk)"), fNo,
            mpMainFrame->mTraceFile.mFrames[fNo]->GetCallCount(), mpMainFrame->mTraceFile.mFrames[fNo]->mBytes/1024+1);
        FrameCallItemData* newFrameData = new FrameCallItemData(FrameItem);
        newFrameData->mpFrameTM = mpMainFrame->mTraceFile.mFrames[fNo];
        frameId = AppendItem(rootId, wxT(""), -1, -1, newFrameData);
        SetItemHasChildren(frameId, true);
        SetItemText(frameId, wsStr);        
    }

    return true;
}

unsigned int FrameCallTree::GetCurrentCallNo()
{
    if (!mpCurSel)
        return 0;

    switch (mpCurSel->mType) {
    case FrameItem:
        return mpCurSel->mpFrameTM->mFirstCallOfThisFrame;
    case CallItem:
        return mpCurSel->mpCallTM->mCallNo;
    default:
        return 0;
    }
}

void FrameCallTree::SelectCall(unsigned int callNo)
{
    int frIdx = mpMainFrame->mTraceFile.GetFrameIdx(callNo);
    if (frIdx == -1)
        return;

    wxTreeItemId rootId = GetRootItem();

    wxTreeItemIdValue cockie;
    wxTreeItemId frId = GetFirstChild(rootId, cockie);
    for (int i = 0; i < frIdx; ++i)
        frId = GetNextSibling(frId);

    Expand(frId);
    FrameCallItemData* pData = static_cast<FrameCallItemData*>(GetItemData(frId));
    FrameTM* frameTM = pData->mpFrameTM;

    wxTreeItemId callId = GetFirstChild(frId, cockie);
    for (unsigned int i = 0; i < frameTM->GetLoadedCallCount(); ++i) {
        CallTM& callTM = *frameTM->mCalls[i];
        if (callTM.mCallNo == callNo) {
            SelectItem(callId, true);
            break;
        }
        callId = GetNextSibling(callId);
    }

}

void FrameCallTree::OnItemExpanding(wxTreeEvent& event)
{
    const wxTreeItemId item = event.GetItem();
    FrameCallItemData* pData = dynamic_cast<FrameCallItemData*>(GetItemData(item));
    if (pData && pData->IsValid() && pData->mType == FrameItem) {
        LoadFrame(item, pData->mpFrameTM);
    }
}

void FrameCallTree::OnItemCollapsing(wxTreeEvent& event)
{
    wxTreeItemId item = event.GetItem();
    FrameCallItemData* pData = dynamic_cast<FrameCallItemData*>(GetItemData(item));
    if (pData && pData->IsValid() && pData->mType == FrameItem) {
        FrameTM* frameTM = pData->mpFrameTM;
        frameTM->UnloadCalls();

        DeleteChildren(item);
    }
    
    mpMainFrame->loadProgressUpdate(0); // reset loading bar
}

void FrameCallTree::OnSelChanged(wxTreeEvent& event)
{
    wxTreeItemId item = event.GetItem();
    FrameCallItemData* pData = static_cast<FrameCallItemData*>(GetItemData(item));
    mpCurSel = pData;

    assert(wxGetApp().mpMainFrame);

    if (mpCurSel->mType != CallItem)
        wxGetApp().mpMainFrame->UpdateCallDetail(NULL);
    else
        wxGetApp().mpMainFrame->UpdateCallDetail(mpCurSel->mpCallTM);
}

void FrameCallTree::ReloadLoadedFrames()
{
    wxTreeItemIdValue treeItemId;
    const wxTreeItemId rootId = GetRootItem();
    wxTreeItemId childId = GetFirstChild(rootId, treeItemId);
    while (childId.IsOk())
    {
        if (IsExpanded(childId))
        {
            FrameCallItemData* pData = dynamic_cast<FrameCallItemData*>(GetItemData(childId));
            if (pData && pData->mType == FrameItem) {
                FrameTM* frameTM = pData->mpFrameTM;
                if (frameTM)
                {
                    frameTM->UnloadCalls();
                }
                DeleteChildren(childId);

                LoadFrame(childId, frameTM);
            }
        }

        childId = GetNextChild(rootId, treeItemId);
    }
}

void FrameCallTree::LoadFrame(wxTreeItemId item, FrameTM *frame)
{
    if (!item.IsOk() || !frame)
        return;

    frame->LoadCalls(
        mpMainFrame->mTraceFile.mpInFileRA,
        mpMainFrame->mTraceFile.GetLoadQueryCalls(),
        mpMainFrame->mTraceFile.GetLoadFilter());

    wxString wsStr;
    const unsigned int loadedCallCnt = frame->mCalls.size();
    mpMainFrame->loadProgressStart( loadedCallCnt );
    unsigned int oldProgressCnt = 0;
    
    for (unsigned int i = 0; i < loadedCallCnt; ++i){
        CallTM *callTM = frame->mCalls[i];

        // use sprintf instead of wxString.Printf because it doesnt support %s for c_str or std::string
        const size_t bufSize = 512;
        char buffer[bufSize];
        snprintf(buffer, bufSize, "[%d][%d] %s", callTM->mTid, callTM->mCallNo, callTM->ToStr().c_str());
        wsStr = wxString::FromAscii(buffer);    
        
        FrameCallItemData* newCallData = new FrameCallItemData(CallItem);
        newCallData->mpCallTM = callTM;
        wxTreeItemId cId = AppendItem(item, wsStr, -1, -1, newCallData);

        unsigned int bkc = callTM->mBkColor;
        unsigned int tc = callTM->mTxtColor;
        wxColour bk(RED_COM(bkc), GREEN_COM(bkc), BLUE_COM(bkc));
        wxColour t(RED_COM(tc), GREEN_COM(tc), BLUE_COM(tc));
        SetItemBackgroundColour(cId, bk);
        SetItemTextColour(cId, t);

        if ( loadedCallCnt > 0 ) {
            const int progress_interval = (loadedCallCnt) / 10; // update progress 10 times
            if (i > oldProgressCnt) {
                oldProgressCnt += progress_interval;
                mpMainFrame->loadProgressUpdate(i);
            }
        }
        
    }
    
    mpMainFrame->loadProgressUpdate(loadedCallCnt); // finish!
    
}

void FrameCallTree::OnLeftDoubleClick(wxMouseEvent& event)
{
    // Get the tree item from the mouse position
    const wxPoint pos = event.GetPosition();
    int flags = 0;
    const wxTreeItemId item = HitTest(pos, flags);

    FrameCallItemData* pData = dynamic_cast<FrameCallItemData*>(GetItemData(item));
    if (pData && pData->IsValid())
    {
        if (pData->mType == CallItem)
        {
            const CallTM *call = pData->mpCallTM;
            const std::string callName = call->Name();

            if (callName == "glTexImage2D")
            {
                // Create a dummy image
                const pat::Image image(640, 480, GL_RGB, GL_UNSIGNED_BYTE, 640 * 480 * 3, NULL);

                // Create a window to display the texture
                TextureFrame *viewer = new TextureFrame(this, image);
                viewer->Show(true);
            }
        }
        else if (pData->mType == FrameItem)
        {
            if (pData->mpFrameTM->IsLoaded())
                Collapse(item);
            else
                Expand(item);
        }
    }
}

