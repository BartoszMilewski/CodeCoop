#if !defined (SELECTFILEDLG_H)
#define SELECTFILEDLG_H
//----------------------------------
// (c) Reliable Software 2005
//----------------------------------

#include <File/Path.h>
#include <Ctrl/Button.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Static.h>
#include <Win/Dialog.h>

class SelectFileCtrl : public Dialog::ControlHandler
{
public:
	SelectFileCtrl (std::string const & fileCaption, std::string const & browseCaption);

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

protected:
	Win::Edit		_path;
	Win::Edit		_fileName;
	Win::CheckBox	_overwriteExisting;

private:
	Win::Button		_browse;
	Win::StaticText	_fileFrame;
	std::string		_fileCaption;
	std::string		_browseCaption;
};

#endif
