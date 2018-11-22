#ifndef _TRACEVIEW_FRAME_CALL_TREE_H_
#define _TRACEVIEW_FRAME_CALL_TREE_H_

#include "app.h"

#include <wx/wx.h>
#include <wx/treectrl.h>


#include <ui/main_frame.h>

#include <common/trace_model.hpp>




using namespace common;

enum FrameCallType
{
    RootItem = 1,
    FrameItem,
    CallItem,
};


class FrameCallItemData : public wxTreeItemData
{
public:
    FrameCallItemData(FrameCallType typeVal)
    : mType(typeVal), mpFrameTM(NULL)
    {
    }

    bool IsValid() const { return mpFrameTM != NULL; }

    FrameCallType       mType;
    union {
        FrameTM*            mpFrameTM;
        CallTM*             mpCallTM;
    };
};


class FrameCallTree : public wxTreeCtrl
{
public:
    FrameCallTree(MainFrame* mainFrame, 
				  wxWindow *parent, const wxWindowID id,
        const wxPoint& pos, const wxSize& size,
        long style);
    virtual ~FrameCallTree();

    bool LoadFrames(const wxString & filepath);
    unsigned int GetCurrentCallNo();
    void SelectCall(unsigned int callNo);

    // Reload all loaded frames with new display filter settings
    void ReloadLoadedFrames();

private:
    void OnItemExpanding(wxTreeEvent& event);
    void OnItemCollapsing(wxTreeEvent& event);
    void OnSelChanged(wxTreeEvent& event);
    void OnLeftDoubleClick(wxMouseEvent& event);
    void LoadFrame(wxTreeItemId item, FrameTM *frame);

    FrameCallItemData*      mpCurSel;
	MainFrame				*mpMainFrame;
    DECLARE_EVENT_TABLE()
};

#endif
