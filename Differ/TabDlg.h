#if !defined (TABDLG_H)
#define TABDLG_H
//---------------------------
// (c) Reliable Software 2005
//---------------------------

#include <Win/Dialog.h>
#include <Ctrl/Edit.h>

class TabSizeCtrl : public Dialog::ControlHandler
{
public:
	TabSizeCtrl (unsigned & tabSize);

	bool OnInitDialog () throw (Win::Exception);
	bool OnApply () throw ();

private:
	unsigned &	_tabSize;
	Win::Edit	_edit;
};


#endif
