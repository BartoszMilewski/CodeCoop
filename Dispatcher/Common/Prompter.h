#if !defined (PROMPTER_H)
#define PROMPTER_H
// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------
#include "Global.h"
#include <Win/Message.h>

namespace Dialog
{
	class ControlHandler;
}

namespace PropPage
{
	class HandlerSet;
}

class Prompter
{
public:
	Prompter ()
		: _msgShowWindow (UM_SHOW_WINDOW),
		  _isOnUserCmd (0)
	{}

	void Init (Win::Dow::Handle appWin, bool * isOnUserCmd) 
	{ 
		Assert (isOnUserCmd != 0);
		_appWin = appWin;
		_isOnUserCmd = isOnUserCmd;
	}

	bool Prompt (std::string const & msg, char const * alert);
	bool GetData (Dialog::ControlHandler & ctrl, char const * alert = 0);
	bool GetSheetData (PropPage::HandlerSet & ctrlSet);
	bool GetWizardData (PropPage::HandlerSet & ctrlSet, char const * alert = 0);

private:
	bool IsOnUserCmd () const { return *_isOnUserCmd; }
private:
	Win::Dow::Handle		_appWin;
	bool				  *	_isOnUserCmd;
	Win::RegisteredMessage	_msgShowWindow;
};

//----------------
// Global prompter
//----------------

extern Prompter ThePrompter;

#endif
