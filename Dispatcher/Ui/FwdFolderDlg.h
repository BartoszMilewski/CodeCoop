#if !defined (FWDFOLDERDLG_H)
#define FWDFOLDERDLG_H
//----------------------------------
// (c) Reliable Software 1998 - 2005
// ---------------------------------

#include "Transport.h"

#include <Win/Dialog.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Button.h>
#include <File/Path.h>
#include <Win/Win.h>

class FwdFolderData
{
public:
    FwdFolderData (char const * hubId,
				   char const * userId,
				   char const * project,
				   char const * comment)
        : _hubId (hubId),
		  _userId (userId),
		  _project (project),
		  _comment (comment)
    {}

	std::string const & GetHubId () const { return _hubId; }
	std::string const & GetUserId () const { return _userId; }
	std::string const & GetProjectName () const { return _project; }
	std::string const & GetComment () const { return _comment; }
	Transport const & GetTransport () const { return _transport; }

	void SetTransport (Transport const & transport) { _transport = transport; }

private:
	std::string	_hubId;
    std::string	_userId;
    std::string	_project;
	std::string	_comment;
	Transport	_transport;
};

class FwdFolderCtrl : public Dialog::ControlHandler
{
public:
    FwdFolderCtrl (FwdFolderData & data, bool isJoin = false);

    bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();
	bool OnCancel () throw ();

private:
	Win::EditReadOnly	_hubId;
    Win::EditReadOnly	_userId;
    Win::EditReadOnly	_name;
	Win::EditReadOnly	_comment;
    Win::Edit			_transport;
    Win::Button			_browse;

    FwdFolderData &		_dlgData;
};

#endif
