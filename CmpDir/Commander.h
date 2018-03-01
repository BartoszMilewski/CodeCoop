#if !defined (CMPDIRCOMMANDER_H)
#define CMPDIRCOMMANDER_H
//------------------------------------
//  (c) Reliable Software, 2002
//------------------------------------

#include <Ctrl/Command.h>
class CmpDirCtrl;
class View;

class Commander
{
public:
	Commander (CmpDirCtrl & ctrl)
		: _ctrl (ctrl), _view (0)
	{}
	void ConnectView (View * view) { _view = view; }
	void Program_Exit ();
	Cmd::Status can_Program_Exit () const { return Cmd::Enabled; }
	void View_Old ();
	Cmd::Status can_View_Old () const { return Cmd::Enabled; }
	void View_New ();
	Cmd::Status can_View_New () const { return Cmd::Enabled; }
	void View_Diff ();
	Cmd::Status can_View_Diff () const { return Cmd::Enabled; }
	void Directory_Up ();
	Cmd::Status can_Directory_Up () const;
	void Selection_View ();
	Cmd::Status can_Selection_View () const;
	void Directory_Refresh ();
private:
	CmpDirCtrl & _ctrl;
	View	   * _view;
};

#endif
