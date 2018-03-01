//------------------------------------
//  (c) Reliable Software, 1999 - 2008
//------------------------------------

#include "precompiled.h"
#include "DiagDlg.h"
#include "resource.h"
#include "ClusterRecipient.h"
#include "Config.h"
#include "OutputSink.h"
#include "Registry.h"
#include "EmailMan.h"

#include <Ex/Error.h>

//
// Helper class switching buttons for the test duration
//

ButtonSwitch::ButtonSwitch (Win::Button & ok, Win::Button & start, Win::Button & stop)
	: _ok (ok),
	  _start (start),
	  _stop (stop)
{
	_ok.Disable ();
	_stop.Enable ();
	_stop.SetFocus ();
	_start.Disable ();
}

ButtonSwitch::~ButtonSwitch ()
{
	_start.Enable ();
	_start.SetFocus ();
	_stop.Disable ();
	_ok.Enable ();
}

//
// Test file used during forwarding test
//

char const * TestFile::_name = "Test.txt";

TestFile::TestFile ()
{
	FileSerializer out (GetFullPath ());
	out.PutBytes ("0123456789", 10);
}

TestFile::~TestFile ()
{
	File::Delete (GetFullPath ());
}

//
// Diagnostics dialog data
//

bool DiagData::IsStandalone () const
{
	return _config.GetTopology ().IsStandalone ();
}

bool DiagData::IsHubOrPeer () const
{
	return _config.GetTopology().IsHubOrPeer ();
}

bool DiagData::IsTmpHub () const
{
	return _config.GetTopology().IsTemporaryHub ();
}

bool DiagData::IsRemoteSat () const
{
	return _config.GetTopology().IsRemoteSatellite ();
}

Transport const & DiagData::GetTransportToHub () const
{
	return _config.GetActiveTransportToHub ();
}

Transport const & DiagData::GetInterClusterTransportToMe () const
{
	return _config.GetInterClusterTransportToMe ();
}

//
// Diagnostics dialog controller
//

DiagCtrl::DiagCtrl (DiagData & data)
	: Dialog::ControlHandler (IDD_DIAGNOSTICS),
	  _dlgData (data),
	  _feedback (_status) // _status is not yet initialised !
{}

bool DiagCtrl::OnInitDialog () throw (Win::Exception)
{
	_email.Init (GetWindow (), IDC_TEST_EMAIL);
	_forwarding.Init (GetWindow (), IDC_TEST_FORWARD);
	_status.Init (GetWindow (), IDC_TEST_STATUS);
	_progressBar.Init (GetWindow (), IDC_TEST_PROGRESS);
	_start.Init (GetWindow (), IDC_TEST_START);
	_stop.Init (GetWindow (), IDC_TEST_STOP);
	_ok.Init (GetWindow (), Out::OK);
	_diagProgress.Init (&_progressBar, _dlgData.GetMsgPrepro ());
	_status.SetReadonly (true);

	if (_dlgData.IsHubOrPeer () || _dlgData.IsTmpHub () || _dlgData.IsRemoteSat ())
	{
		_email.Check ();
	}
	else
	{
		_forwarding.Check ();
	}
	_stop.Disable ();
	_start.SetFocus ();
	return true;
}

bool DiagCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_TEST_START:
		if (_start.IsClicked (notifyCode))
		{
			_feedback.Clear ();
			if (_email.IsChecked ())
			{
				ButtonSwitch btnSwitch (_ok, _start, _stop);
				Transport const & incomingTransport = _dlgData.GetInterClusterTransportToMe ();
				if (!incomingTransport.IsEmail ())
				{
					TheOutput.Display ("Configuration: Your e-mail address was not set", Out::Error);
					return true;
				}
				Email::Diagnostics emailDiag (incomingTransport.GetRoute (), _feedback, _diagProgress);
				Email::Manager & emailMan = _dlgData.GetEmailManager ();
				Email::Status status = emailDiag.Run (emailMan);
				if (status != Email::NotTested)
					emailMan.SetEmailStatus (status);
				_diagProgress.Clear ();
			}
			else
			{
				Assert (_forwarding.IsChecked ());
				ButtonSwitch btnSwitch (_ok, _start, _stop);
				if (_dlgData.IsStandalone ())
				{
					ResString info (GetWindow (), IDS_STANDALONE_INFO);
					_feedback.Display (info);
					_feedback.Display ("This machine is currently configured as standalone.");
				}
				else if (_dlgData.IsHubOrPeer ())
				{
					TestHubForwarding ();
				}
				else
				{
					TestSatForwarding ();
				}
				_diagProgress.Clear ();
			}
			return true;
		}
		break;
	case IDC_TEST_STOP:
		if (_stop.IsClicked (notifyCode))
		{
			_diagProgress.Cancel ();
			return true;
		}
		break;
	}
    return false;
}

bool DiagCtrl::OnApply () throw ()
{
	EndOk ();
	return true;
}

// these are actual forwarding paths used with network transport
class FwdPathDictionary
{
	friend class FwdPathDictionaryIter;

public:
	void Add (FilePath const & fwdPath, char const * project) ;
	int size () const { return _dictionary.size (); }

private:
	typedef std::map<FilePath, std::vector<std::string>, DirPathLess>::const_iterator DictIter;
	typedef std::map<FilePath, std::vector<std::string>, DirPathLess> const & DictRef;

private:
	std::map<FilePath, std::vector<std::string>, DirPathLess> _dictionary;
};

void FwdPathDictionary::Add (FilePath const & fwdPath, char const * project) 
{
	std::map<FilePath, std::vector<std::string>, DirPathLess>::iterator iter 
		= _dictionary.find (fwdPath);
	if (iter == _dictionary.end ())
	{
		// Forwarding path seen for the first time
		std::vector<std::string> projectList;
		projectList.push_back (project);
		_dictionary.insert (std::make_pair (fwdPath, projectList));
	}
	else
	{
		// Forwarding path already known -- add project name to the list
		iter->second.push_back (project);
	}
}

class FwdPathDictionaryIter
{
public:
	FwdPathDictionaryIter (FwdPathDictionary const & dict)
		: _dict (dict._dictionary)
	{
		_cur = _dict.begin ();
	}

	bool AtEnd () const { return _cur == _dict.end (); }
	void Advance () { ++_cur; }

	FilePath const & GetFwdPath () const { return _cur->first; }
	std::vector<std::string> const & GetProjectList () const { return _cur->second; }

private:
	FwdPathDictionary::DictRef	_dict;
	FwdPathDictionary::DictIter	_cur;
};

void DiagCtrl::TestHubForwarding ()
{
	ClusterRecipientList const & clusterRecip = _dlgData.GetClusterRecipients ();
	bool userAbort = false;
	bool errorsDetected = false;
	// Create forwarding path dictionary
	FwdPathDictionary dictionary;
	for (ClusterRecipientList::const_iterator recip = clusterRecip.begin (); recip != clusterRecip.end (); ++recip)
	{
		if (recip->IsRemoved ())
			continue;
		Transport const & transport = recip->GetTransport ();
		if (transport.IsNetwork ())
		{
			// Remove appended file name if any
			std::string const & route = recip->GetTransport ().GetRoute ();
			std::string projName;
			UserIdPack pack (recip->GetUserId ().c_str ());
			projName += recip->GetProjectName ();
			projName += " (User id ";
			projName += pack.GetUserIdString ();
			projName += ")";
			dictionary.Add (route, projName.c_str ());
		}
	}
	if (dictionary.size () == 0)
	{
		// No cluster recipient on this hub
		_feedback.Display ("This machine does not have any active satellite recipients.");
		return;
	}
	// Create test file
	TestFile testFile;
	char const * src = testFile.GetFullPath ();
	// Copy test file to each forward folder, see if it is there and delete it
	_diagProgress.SetRange (0, dictionary.size (), 1);
	for (FwdPathDictionaryIter iter (dictionary); !iter.AtEnd (); iter.Advance ())
	{
		// Display what is tested
		std::string infoPath;
		infoPath += "Testing '";
		infoPath += iter.GetFwdPath ().GetDir ();
		infoPath += "' forwarding path";
		_feedback.Display (infoPath.c_str ());
		_feedback.Display ("used by the following project(s):");
		std::vector<std::string> const & projectList = iter.GetProjectList ();
		for (std::vector<std::string>::const_iterator proj = projectList.begin (); 
			proj != projectList.end (); 
			++proj)
		{
			std::string projInfo;
			projInfo += "    ";
			projInfo += *proj;
			_feedback.Display (projInfo.c_str ());
		}
		// Step progress meter and check if test was canceled
		_diagProgress.StepIt ();
		if (_diagProgress.WasCanceled ())
		{
			userAbort = true;
			break;
		}
		char const * dst = iter.GetFwdPath ().GetFilePath (testFile.GetName ());
		// Copy test file
		try
		{
			File::Copy (src, dst);
			if (File::Exists (dst))
			{
				File::Delete (dst);
				_feedback.Display ("PASSED");
			}
			else
			{
				_feedback.Display ("FAILED -- test file not found in the forwarding folder");
				errorsDetected = true;
			}
		}
		catch (Win::Exception e)
		{
			std::string info;
			SysMsg err (e.GetError ());
			info += "FAILED -- ";
			info += err.Text ();
			_feedback.Display (info.c_str ());
			errorsDetected = true;
		}
		catch ( ... )
		{
			Win::ClearError ();
			_feedback.Display ("FAILED -- unknown error");
			errorsDetected = true;
		}
	}
	if (userAbort)
	{
		_feedback.Display ("Forwarding test aborted by the user");
	}
	else
	{
		if (errorsDetected)
			_feedback.Display ("Forwarding test -- FAILED");
		else
			_feedback.Display ("Forwarding test -- PASSED");
	}
}

void DiagCtrl::TestSatForwarding ()
{
	Transport const & hubTransport = _dlgData.GetTransportToHub ();
	if (!hubTransport.IsNetwork ())
		return;

	// Create test file
	TestFile testFile;
	char const * src = testFile.GetFullPath ();
	_diagProgress.SetRange (0, 2, 1);
	// Step progress meter and check if test was canceled
	_diagProgress.StepIt ();
	if (_diagProgress.WasCanceled ())
	{
		_feedback.Display ("Forwarding test aborted by the user");
		return;
	}
	FilePath hubFwdFolder (hubTransport.GetRoute ());
	_feedback.Display ("Testing hub forwarding path:");
	_feedback.Display (hubFwdFolder.GetDir ());
	char const * dst = hubFwdFolder.GetFilePath (testFile.GetName ());
	// Copy test file
	try
	{
		File::Copy (src, dst);
		if (File::Exists (dst))
		{
			File::Delete (dst);
			_feedback.Display ("Hub forwarding test -- PASSED");
		}
		else
		{
			_feedback.Display ("FAILED -- test file not found in the hub forwarding folder");
		}
	}
	catch (Win::Exception e)
	{
		std::string info;
		SysMsg err (e.GetError ());
		info += "FAILED -- ";
		info += err.Text ();
		_feedback.Display (info.c_str ());
	}
	catch ( ... )
	{
		Win::ClearError ();
		_feedback.Display ("FAILED -- unknown error");
	}
	_diagProgress.StepIt ();
}
