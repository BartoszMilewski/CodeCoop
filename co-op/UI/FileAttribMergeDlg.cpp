//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "precompiled.h"
#include "FileAttribMergeDlg.h"
#include "AttributeMerge.h"
#include "FileTypes.h"
#include "resource.h"
#include "OutputSink.h"

#include <Com/Shell.h>

char const * FileAttribMergeCtrl::_rootName = "<Project Root>";

FileAttribMergeCtrl::FileAttribMergeCtrl (AttributeMerge & dlgData)
	: Dialog::ControlHandler (IDD_MERGE_FILE_ATTRIBUTES),
	  _dlgData (dlgData)
{}

bool FileAttribMergeCtrl::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin = GetWindow ();
	_currentName.Init (dlgWin, IDC_CURRENT_NAME);
	_currentPath.Init (dlgWin, IDC_CURRENT_PATH);
	_currentType.Init (dlgWin, IDC_CURRENT_TYPE);
	_branchName.Init (dlgWin, IDC_BRANCH_NAME);
	_branchPath.Init (dlgWin, IDC_BRANCH_PATH);
	_branchType.Init (dlgWin, IDC_BRANCH_TYPE);
	_mergeName.Init (dlgWin, IDC_MERGE_NAME);
	_mergePath.Init (dlgWin, IDC_MERGE_PATH);
	_mergeType.Init (dlgWin, IDC_MERGE_TYPE);

	_currentName.SetText (_dlgData.GetSourceName ());
	std::string const & currentPath = _dlgData.GetSourcePath ();
	if (currentPath.empty ())
		_currentPath.SetText (_rootName);
	else
		_currentPath.SetText (currentPath);
	_currentType.SetText (_dlgData.GetSourceTypeName ());

	_branchName.SetText (_dlgData.GetTargetName ());
	std::string const & branchPath = _dlgData.GetTargetPath ();

	if (branchPath.empty ())
		_branchPath.SetText (_rootName);
	else
		_branchPath.SetText (branchPath);
	_branchType.SetText (_dlgData.GetTargetTypeName ());

	std::string const & finalPath = _dlgData.GetFinalPath ();
	if (finalPath.empty ())
		_mergePath.SetText (_rootName);
	else
		_mergePath.SetText (finalPath);

	// Defaults are set in dlg data
	_mergeName.SetText (_dlgData.GetFinalName ());
	_mergeType.SetEditText (_dlgData.GetFinalTypeName ());

	HeaderFile header;

	int idx = _mergeType.AddToList (header.GetName ());
	_mergeType.SetItemData (idx, header.GetValue ());
	SourceFile source;
	idx = _mergeType.AddToList (source.GetName ());
	_mergeType.SetItemData (idx, source.GetValue ());
	TextFile text;
	idx = _mergeType.AddToList (text.GetName ());
	_mergeType.SetItemData (idx, text.GetValue ());
	BinaryFile binary;
	idx = _mergeType.AddToList (binary.GetName ());
	_mergeType.SetItemData (idx, binary.GetValue ());

	return true;
}

bool FileAttribMergeCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_USE_CURRENT_NAME:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_mergeName.SetText (_currentName.GetString ());
			return true;
		}
		break;

	case IDC_USE_CURRENT_PATH:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_mergePath.SetText (_currentPath.GetString ());
			return true;
		}
		break;

	case IDC_USE_CURRENT_TYPE:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_mergeType.SetEditText (_currentType.GetString ().c_str ());
			return true;
		}
		break;

	case IDC_USE_BRANCH_NAME:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_mergeName.SetText (_branchName.GetString ());
			return true;
		}
		break;

	case IDC_USE_BRANCH_PATH:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_mergePath.SetText (_branchPath.GetString ());
			return true;
		}
		break;

	case IDC_USE_BRANCH_TYPE:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_mergeType.SetEditText (_branchType.GetString ().c_str ());
			return true;
		}
		break;
	}

    return false;
}

bool FileAttribMergeCtrl::OnApply () throw ()
{
	std::string mergePath = _mergePath.GetString ();
	if (mergePath == _rootName)
		mergePath.clear ();

	_dlgData.SetFinalPath (mergePath.c_str ());
	_dlgData.SetFinalName (_mergeName.GetTrimmedString ().c_str ());
	std::string typeName = _mergeType.RetrieveEditText ();
	int idx = _mergeType.FindString (typeName.c_str ());
	Assert (idx != -1);
	_dlgData.SetFinalType (FileType (_mergeType.GetItemData (idx)));
	EndOk ();
	return true;
}
