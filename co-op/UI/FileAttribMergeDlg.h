#if !defined (FILEATTRIBMERGEDLG_H)
#define FILEATTRIBMERGEDLG_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include <Win/Dialog.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Button.h>
#include <Ctrl/ComboBox.h>
#include <File/Path.h>

class AttributeMerge;

class FileAttribMergeCtrl : public Dialog::ControlHandler
{
public:
	FileAttribMergeCtrl (AttributeMerge & dlgData);

    bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

private:
	static char const *	_rootName;
	AttributeMerge &	_dlgData;
	Win::Edit		_currentName;
	Win::Edit		_currentPath;
	Win::Edit		_currentType;
	Win::Edit		_branchName;
	Win::Edit		_branchPath;
	Win::Edit		_branchType;
	Win::Edit		_mergeName;
	Win::Edit		_mergePath;
	Win::ComboBox	_mergeType;
};

#endif
