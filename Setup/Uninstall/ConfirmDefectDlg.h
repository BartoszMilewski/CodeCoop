#if !defined (CONFIRMDEFECTDLG_H)
#define CONFIRMDEFECTDLG_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include <Win\Dialog.h>
#include <Ctrl\Button.h>

class ConfirmDefectDlgCtrl : public Dialog::ControlHandler
{
public:
	ConfirmDefectDlgCtrl ();

    bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception);

	bool IsDefectFromAllProjects () const { return _doDefect; }

private:
	Win::Button	_dontDefect;
	bool		_doDefect;
};

#endif
