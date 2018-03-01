#if !defined (ADDFILESDLG_H)
#define ADDFILESDLG_H
//-------------------------------------
//  (c) Reliable Software, 1998 -- 2005
//-------------------------------------

#include <Win/Dialog.h>
#include <Ctrl/Static.h>
#include <Ctrl/Button.h>
#include <Ctrl/ListBox.h>
#include <Ctrl/ListView.h>

class FolderContents;

// Windows WM_NOTIFY handler for extension list view

class AddFilesCtrl;

class ExtensionListHandler : public Notify::ListViewHandler
{
public:
	explicit ExtensionListHandler (AddFilesCtrl & ctrl);

	bool OnItemChanged (Win::ListView::ItemState & state) throw ();

private:
	AddFilesCtrl & _ctrl;
};

class AddFilesCtrl : public Dialog::ControlHandler
{
public:
    AddFilesCtrl (FolderContents * contents);

	Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ();
    bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();
	bool OnCancel () throw ();
	void ShowExtension (int item);
	void ChangeExtensionSelection (int item);

private:
	void SetCount ();
	void DisplayFiles ();
	int  DisplaySelection ();
	void RememberCurrentSelection ();

private:
	Win::StaticText		_count;
	Win::StaticText		_extensionFrame;
	Win::ReportListing	_extensionView;
	Win::ListBox::MultiSel	_fileView;
	Win::Button			_selectAllExt;
	Win::Button			_deselectAllExt;
	Win::Button			_selectAllFiles;
	Win::Button			_deselectAllFiles;

	FolderContents *	_contents;
	int					_curExtensionItem;
	int					_initFileSelectionCount;
	bool				_ignoreNotifications;
	std::vector<std::string>	_extensionList;
	ExtensionListHandler		_notifyHandler;
};

#endif
