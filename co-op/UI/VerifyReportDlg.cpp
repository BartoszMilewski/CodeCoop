//------------------------------------
//  (c) Reliable Software, 1999 - 2008
//------------------------------------

#include "precompiled.h"
#include "VerifyReportDlg.h"
#include "ProjectPath.h"
#include "Resource.h"

#include <File/Path.h>

VerifyReportCtrl::VerifyReportCtrl (VerificationReport const & report,
									std::string const & projectName,
									Project::Path	& relativePath)
	: Dialog::ControlHandler (IDD_VERIFY_PROJECT_REPORT),
	  _report (report),
	  _projectName (projectName),
	  _projectPath (relativePath)
{}

bool VerifyReportCtrl::OnInitDialog () throw (Win::Exception)
{
	_fileListing.Init (GetWindow (), IDC_MISSING_FILE_LIST);

	_fileListing.AddProportionalColumn (22, "Name");
	_fileListing.AddProportionalColumn (9, "State");
	_fileListing.AddProportionalColumn (36, "Path");
	_fileListing.AddProportionalColumn (8, "Id");
	_fileListing.AddProportionalColumn (25, "Code Co-op Action");

	Display (_report.GetSequencer (VerificationReport::MissingFolder), "In", "Create folder");
	Display (_report.GetSequencer (VerificationReport::MissingNew), "New", "Remove from project");	
	Display (_report.GetSequencer (VerificationReport::MissingCheckedout), "Out", "Un-checkout");
	Display (_report.GetSequencer (VerificationReport::PreservedLocalEdits), "Out", "Restore local edits");
	Display (_report.GetSequencer (VerificationReport::IllegalName), "In", "Correct unique name");
	Display (_report.GetSequencer (VerificationReport::AbsentFolder), "In", "Correct folder state");
	Display (_report.GetSequencer (VerificationReport::Corrupted), "In", "Repair file");
	Display (_report.GetSequencer (VerificationReport::MissingReadOnlyAttribute), "In", "Make read only");

	std::string caption ("Verification report for the project ");
	caption += _projectName;
	GetWindow ().SetText (caption.c_str ());
	return true;
}

void VerifyReportCtrl::Display (VerificationReport::Sequencer seq,
								std::string const & state,
								std::string const & action)
{
	// Fill in file list -- column 0 - file name;
	//						column 1 - file state;
	//						column 2 - project relative path;
	//                      column 3 - file global id;
	//						column 4 - Code Co-op action
	for (; !seq.AtEnd (); seq.Advance ())
	{
		GlobalId gid = seq.Get ();
		PathSplitter splitter (_projectPath.MakePath (gid));
		std::string fileName = splitter.GetFileName ();
		fileName += splitter.GetExtension ();
		GlobalIdPack pack (gid);
		int row = _fileListing.AppendItem (fileName.c_str ());
		_fileListing.AddSubItem (state, row, 1);
		FilePath coopPath (splitter.GetDir ());
		_fileListing.AddSubItem (coopPath.GetDir (), row, 2);
		_fileListing.AddSubItem (pack.ToString (), row, 3);
		_fileListing.AddSubItem (action, row, 4);
	}
}
