#if !defined (FILEPROPERTIESDLG_H)
#define FILEPROPERTIESDLG_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "GlobalId.h"

#include <Ctrl/PropertySheet.h>
#include <Win/Dialog.h>
#include <Ctrl/Button.h>
#include <Ctrl/ListBox.h>

class FileProps;

class CheckedOutByPageHndlr : public PropPage::Handler
{
public:
	CheckedOutByPageHndlr (FileProps & dlgData);

	bool OnInitDialog () throw (Win::Exception);
	void OnApply (long & result) throw (Win::Exception);

private:
	Win::ListBox::Simple	_memberList;
	Win::CheckBox			_startNotifications;
	FileProps &				_dlgData;
};

class FilePropertyHndlrSet : public PropPage::HandlerSet
{
public:
	FilePropertyHndlrSet (FileProps & props);

private:
	CheckedOutByPageHndlr	_checkedOutByPageHndlr;
};


#endif
