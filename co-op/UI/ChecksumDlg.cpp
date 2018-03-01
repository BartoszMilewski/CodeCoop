//------------------------------------
//  (c) Reliable Software, 1999 - 2007
//------------------------------------

#include "precompiled.h"
#include "ChecksumDlg.h"
#include "resource.h"

#include <File/Path.h>

ChecksumMismatchCtrl::ChecksumMismatchCtrl (ChecksumMismatchData & data)
	: Dialog::ControlHandler (IDD_CHECKSUM_MISMATCH),
	  _dlgData (data)
{}

bool ChecksumMismatchCtrl::OnInitDialog () throw (Win::Exception)
{
	_fileList.Init (GetWindow (), IDC_CHECKSUM_FILES);
	_repair.Init (GetWindow (), IDC_CHECKSUM_REPAIR);
	_advanced.Init (GetWindow (), IDC_CHECKSUM_ADVANCED);

	_fileList.AddProportionalColumn (36, "Name");
	_fileList.AddProportionalColumn (50, "Path");
	_fileList.AddProportionalColumn (12, "Id");

	// Fill in file list -- column 0 - file name; column 1 - project relative path;
	//                      column 2 - file global id
	for (VerificationReport::Sequencer seq = _dlgData.GetSequencer (); !seq.AtEnd (); seq.Advance ())
	{
		PathSplitter splitter (_dlgData.GetRelativePath (seq.Get ()));
		std::string fileName = splitter.GetFileName ();
		char const * fileExt = splitter.GetExtension ();
		fileName += fileExt;
		GlobalIdPack gid (seq.Get ());
		int row = _fileList.AppendItem (fileName.c_str ());
		FilePath coopPath (splitter.GetDir ());
		_fileList.AddSubItem (coopPath.GetDir (), row, 1);
		_fileList.AddSubItem (gid.ToString (), row, 2);
	}
	if (_dlgData.IsRepair ())
		_repair.Check ();
	std::string advancedText;
	if (_dlgData.IsCheckout ())
	{
		advancedText += "Remove file from project (for advanced users). ";
	}
	else
	{
		advancedText += "Ignore and continue operation (for advanced users). ";
	}
	advancedText += "This will make file(s) unrecoverable. "
					"Code Co-op will not be able to reverse this operation. "
					"Also, if you receive a script that changes an unrecoverable file, you will not be able to execute it.";
	_advanced.SetText (advancedText.c_str ());
	if (_dlgData.IsAdvanced ())
		_advanced.Check ();
	return true;
}

bool ChecksumMismatchCtrl::OnApply () throw ()
{
    _dlgData.SetRepair (_repair.IsChecked ());
	_dlgData.SetAdvanced (_advanced.IsChecked ());
	EndOk ();
    return true;
}
