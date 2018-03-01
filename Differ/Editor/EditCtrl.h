#if !defined (EDITCTRL_H)
#define EDITCTRL_H
//
// (c) Reliable Software 1997-2006
//

#include "EditPane.h"

#include <Ctrl/Scroll.h>
#include <Graph/Cursor.h>
#include <Win/Win.h>
#include <Win/Controller.h>

class EditController : public Win::Controller
{

public:
    EditController (bool readOnlyWin);
    ~EditController ();
	void MakeEditable (bool val) { _isReadOnlyWin = !val; }

	bool OnCreate (Win::CreateData const * create, bool & success) throw ();
    bool OnDestroy () throw ();
	bool OnFocus (Win::Dow::Handle winPrev) throw ();
	bool OnSize (int width, int height, int flag) throw ();
	bool OnMouseActivate (Win::HitTest hitTest, Win::MouseActiveAction & activate) throw ();
	bool OnVScroll (int code, int pos, Win::Dow::Handle winCtrl) throw ();
	bool OnHScroll (int code, int pos, Win::Dow::Handle winCtrl) throw ();
	bool OnCommand (int cmdId, bool isAccel) throw ();
	bool OnUserMessage (Win::UserMessage & msg) throw ();

private:
	void SynchScroll (int offset, int targetPara);
	void UpdScrollBars ();
	void SetScrollBar (int bar, int newPos);
	bool CheckOut ();

private:

    // User Interface
	Win::Dow::Handle	_hwndParent;
    Win::Dow::Handle	_marginWin;
    Win::Dow::Handle	_editPaneWin;
	EditPaneController	_editPaneCtrl;

    int					_marginSize;
	int					_cx;
	int					_cy;

    VTxtScrollBar		_vScroll;
    HTxtScrollBar		_hScroll;

	bool				_isReadOnlyWin;	// True if controlled window doesn't allow edits
};

#endif
