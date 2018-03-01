#if !defined (REVERTDLG_H)
#define REVERTDLG_H
//------------------------------------
//  (c) Reliable Software, 1998 - 2006
//------------------------------------

#include <Win/Dialog.h>
#include <Ctrl/Static.h>
#include <Ctrl/Edit.h>

class VersionInfo;
class FileFilter;

class RevertData
{
public:
	RevertData (VersionInfo const & info, bool fileRestore);

	char const * GetRevertVersion () const { return _version.c_str (); }
	bool IsFileRestore () const { return _fileRestore; }

private:
	bool			_fileRestore;
	std::string		_version;
};

class RevertCtrl : public Dialog::ControlHandler
{
public:
    RevertCtrl (RevertData const & data);

    bool OnInitDialog () throw (Win::Exception);
    bool OnApply () throw ();

private:
	Win::StaticText		_versionFrame;
	Win::EditReadOnly	_version;
	Win::StaticText		_revertDetails;
    RevertData const &	_dlgData;
};

#endif
