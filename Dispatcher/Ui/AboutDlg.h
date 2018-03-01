#if !defined (ABOUTDLG_H)
#define ABOUTDLG_H
// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------
#include <Win/Dialog.h>

class AboutCtrl : public Dialog::ControlHandler
{
public:
	AboutCtrl ();

	bool OnInitDialog () throw (Win::Exception);
	bool OnApply () throw ();
};

#endif
