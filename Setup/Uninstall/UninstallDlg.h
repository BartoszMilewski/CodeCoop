#if !defined (UNISTALLDLG_H)
#define UNINSTALLDLG_H
//------------------------------------
//  (c) Reliable Software, 1997 - 2005
//------------------------------------

#include <Win\Dialog.h>
#include <ctrl\Button.h>

class UninstallDlgCtrl : public Dialog::ControlHandler
{
public:
	UninstallDlgCtrl ();

    bool OnInitDialog () throw (Win::Exception);
    bool OnApply () throw ();

private:
	Win::Button	_uninstall;
	Win::Button	_cancel;
};

#endif
