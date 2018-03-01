//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "precompiled.h"
#include "SelectRootPathDlg.h"
#include "OutputSink.h"
#include "PathFind.h"
#include "BrowseForFolder.h"

#include <File/Drives.h>
#include <Dbg/Out.h>

bool SelectRootCtrl::OnInitDialog () throw (Win::Exception)
{
	_pathList.Init (GetWindow (), IDC_ROOT_PATH_LIST);
	_driveSelector.Init (GetWindow (), IDC_DRIVE_SELECTOR);
	_browseNetwork.Init (GetWindow (), IDC_BROWSE_NETWORK);

	_pathList.AddProportionalColumn (69, "Root path");
	_pathList.AddProportionalColumn (30, "Project");

	RefreshPathList (std::string ());

	WriteableDriveSeq drives;
	for (WriteableDriveSeq drives; !drives.AtEnd (); drives.Advance ())
	{
		_driveSelector.AddToList (drives.GetDriveString ());
	}

	Project::PathClassifier::ProjectSequencer projSeq = _dlgData.GetProjectSequencer ();
	Assert (!projSeq.AtEnd ());
	FullPathSeq pathSeq (projSeq.GetRootPath ());
	if (pathSeq.IsUNC ())
		_browseNetwork.Enable ();
	else
		_browseNetwork.Disable ();

	return true;
}

bool SelectRootCtrl::OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception)
{
	if (id == IDC_DRIVE_SELECTOR)
	{
		if (Win::ComboBox::IsEditChange (notifyCode))
		{
			RefreshPathList (_driveSelector.RetrieveTrimmedEditText ());
		}
		else if (Win::ComboBox::IsSelectionChange (notifyCode))
		{
			int idx = _driveSelector.GetSelectionIdx ();
			RefreshPathList (_driveSelector.RetrieveText (idx));
		}
	}
	else if (id == IDC_BROWSE_NETWORK)
	{
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			std::string newPathPrefix;
			if (BrowseForNetworkFolder (newPathPrefix, GetWindow (), "Select new \\\\server\\share"))
			{
				RefreshPathList (newPathPrefix);
				_dlgData.SetNewPrefix (newPathPrefix);
			}
		}
	}
	return true;
}

bool SelectRootCtrl::OnApply () throw ()
{
	std::string newPathPrefix (_dlgData.GetNewPrefix ());
	if (newPathPrefix.empty ())
	{
		newPathPrefix = _driveSelector.RetrieveTrimmedEditText ();
		if (newPathPrefix.empty ())
		{
			TheOutput.Display ("Please, select new drive.");
			return false;
		}
	}

	Project::PathClassifier::ProjectSequencer projSeq = _dlgData.GetProjectSequencer ();
	for (; !projSeq.AtEnd (); projSeq.Advance ())
	{
		FilePath newPath (newPathPrefix);
		for (FullPathSeq pathSeq (projSeq.GetRootPath ()); !pathSeq.AtEnd (); pathSeq.Advance ())
			newPath.DirDown (pathSeq.GetSegment ().c_str ());

		if (File::Exists (newPath.GetDir ()))
		{
			std::string info ("The following path:\n\n");
			info += newPath.GetDir ();
			info += "\n\nalready exists. Do you want to place the project ";
			info += projSeq.GetProjectName ();
			info += " there?\n\nWarning: During project repair the existing files may be overwritten.";
			Out::Answer userChoice = TheOutput.Prompt (info.c_str (),
													   Out::PromptStyle (Out::YesNo, Out::No, Out::Question),
													   GetWindow ());
			if (userChoice == Out::No)
				return false;
		}
	}

	_dlgData.SetNewPrefix (newPathPrefix);
	Dialog::ControlHandler::EndOk ();
	return true;
}

void SelectRootCtrl::RefreshPathList (std::string const & prefix)
{
	_pathList.ClearRows ();
	Project::PathClassifier::ProjectSequencer projSeq = _dlgData.GetProjectSequencer ();
	if (prefix.empty ())
	{
		for (; !projSeq.AtEnd (); projSeq.Advance ())
		{
			Win::ListView::Item item;
			item.SetText (projSeq.GetRootPath ());
			int row = _pathList.AppendItem (item);
			_pathList.AddSubItem (projSeq.GetProjectName (), row, 1);	// Second column
		}
	}
	else
	{
		for (; !projSeq.AtEnd (); projSeq.Advance ())
		{
			FilePath newPath (prefix);
			for (FullPathSeq pathSeq (projSeq.GetRootPath ()); !pathSeq.AtEnd (); pathSeq.Advance ())
				newPath.DirDown (pathSeq.GetSegment ().c_str ());

			Win::ListView::Item item;
			item.SetText (newPath.GetDir ());
			int row = _pathList.AppendItem (item);
			_pathList.AddSubItem (projSeq.GetProjectName (), row, 1);	// Second column
		}
	}
}