// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------
#include "precompiled.h"
#include "ProjectInviteDlg.h"
#include "JoinProjectData.h"
#include "BrowseForFolder.h"
#include "OutputSink.h"

#include <File/Path.h>

bool ProjectInviteCtrl::OnInitDialog () throw (Win::Exception)
{
	_invitation.Init (GetWindow (), IDC_EDIT);
	_path.Init (GetWindow (), IDC_PATH);
	_browse.Init (GetWindow (), IDC_BROWSE);
	_reject.Init (GetWindow (), IDC_REJECT);

	std::string msg = "You have been invited to the project \"";
	msg += _data.GetProject ().GetProjectName ();
	msg += "\"\r\nby \"";
	msg += _adminName;
	msg += "\" <";
	msg += _data.GetAdminHubId ();
	msg += ">.";
	msg += "\r\n";
	if (_data.IsObserver ())
		msg += "You will have an Observer status in this project.";
	_invitation.SetText (msg.c_str ());

	return true;
}

bool ProjectInviteCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_BROWSE:
		{
			std::string folder;
			if (BrowseForProjectRoot (folder, GetWindow (), false))	// Joining
				_path.SetString (folder);
		}
		return true;
	case IDC_REJECT:
		_isReject = true;
		EndOk ();
		return true;
	};
	return false;
}

bool ProjectInviteCtrl::OnApply () throw ()
{
	std::string rootPath = _path.GetTrimmedString ();
	unsigned len = rootPath.length();
	for (unsigned i = 0; i < len; ++i)
		if (rootPath[i] == '/')
			rootPath[i] = '\\';
	_data.GetProject ().SetRootPath (rootPath);

	if (_data.IsValid ())
		EndOk ();
	else
		_data.DisplayErrors (GetWindow ());

	return true;
}
