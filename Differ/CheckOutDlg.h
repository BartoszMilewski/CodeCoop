#if !defined (CHECKOUTDLG_H)
#define CHECKOUTDLG_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include <Win/Dialog.h>
#include <Ctrl/Button.h>

class CheckOutCtrl : public Dialog::ControlHandler
{
public:
    CheckOutCtrl (bool & dontAsk);

    bool OnInitDialog () throw (Win::Exception);
    bool OnApply () throw ();

private:
	Win::CheckBox	_dontAskCheck;
    bool & _dontAsk;
};

#endif
