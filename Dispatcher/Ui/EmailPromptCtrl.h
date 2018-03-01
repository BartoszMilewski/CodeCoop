#if !defined (EMAILPROMPTCTRL_H)
#define EMAILPROMPTCTRL_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include <Win/Dialog.h>
#include <Ctrl/Edit.h>

class EmailPromptCtrl : public Dialog::ControlHandler
{
public:
	EmailPromptCtrl (std::string & dlgData);

	bool OnInitDialog () throw (Win::Exception);
	bool OnApply () throw ();

private:
	Win::Edit			_email;

	std::string &		_dlgData;
};

#endif
