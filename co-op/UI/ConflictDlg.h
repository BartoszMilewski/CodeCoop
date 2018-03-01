#if !defined (CONFLICTDLG_H)
#define CONFLICTDLG_H
//------------------------------------
//  (c) Reliable Software, 1998 - 2007
//------------------------------------

#include <Ctrl/Static.h>
#include <Ctrl/Button.h>
#include <Win/Dialog.h>

class ConflictDetector;

class ScriptConflictCtrl : public Dialog::ControlHandler
{
public:
    ScriptConflictCtrl (ConflictDetector & detector);

    bool OnInitDialog () throw (Win::Exception);
	bool OnApply () throw ();

private:
	Win::StaticText		_conflictVersion;
	Win::RadioButton	_autoMerge;
	Win::RadioButton	_executeOnly;
	Win::RadioButton	_postpone;
    ConflictDetector &	_dlgData;
};

class ScriptConflictDlg
{
public:
	bool Show (ConflictDetector & detector);
};

#endif
