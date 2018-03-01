#if !defined (FOLDERMAN_H)
#define FOLDERMAN_H
//----------------------------------
// (c) Reliable Software 1998 - 2005
// ---------------------------------

#include "resource.h"

#include <Win/Dialog.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Static.h>

class FilePath;

namespace FolderMan
{
    bool CopyMaterialize (char const * src, FilePath const & destFolder, 
                          char const * filename, bool & driveNotReady, bool quiet = false);
    bool CreateFolder (char const * path, bool needsConfirmation = true, bool quiet = false);
}

class CopyFailedData
{
    friend class CopyFailedCtrl;
public:
    CopyFailedData (char const * what, char const * why, char const * script, char const * dest)
        : _what (what), _why (why), _script (script), _dest (dest)
    {}
private:
    std::string const _what; 
	std::string const _why;
	std::string const _script;
	std::string const _dest;
};

class CopyFailedCtrl : public Dialog::ControlHandler
{
public:
    CopyFailedCtrl (CopyFailedData * data)
		: Dialog::ControlHandler (IDD_COPY_FAILED),
		  _dlgData (data)
	{}
    bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);

private:
    Win::StaticText   _what;
    Win::StaticText   _why;
    Win::EditReadOnly _script;
    Win::EditReadOnly _dest;

    CopyFailedData *_dlgData;
};

#endif
