//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include "precompiled.h"
#include "MoveFilesDlg.h"
#include "resource.h"
#include "InputSource.h"
#include "SelectIter.h"
#include "OutputSink.h"

MoveFilesData::MoveFilesData(WindowSeq & selection, std::string const & currentDir, std::string const & rootDir)
	: _targetDir(currentDir),
	  _rootDir(rootDir),
	  _names(selection)
{
}

bool MoveFilesData::Verify(std::string const & target)
{
	if (_targetDir == target)
	{
		TheOutput.Display("Target path must be different from current path");
		return false;
	}
	else if (!FilePath::IsValid(target))
	{
		TheOutput.Display("Invalid target path");
		return false;
	}
	else
	{
		FilePath targetPath = target;
		if (!targetPath.HasPrefix(_rootDir))
		{
			TheOutput.Display("Target directory must be inside the project");
			return false;
		}
	}
	return true;
}


MoveFilesCtrl::MoveFilesCtrl(MoveFilesData & data)
	: Dialog::ControlHandler (IDD_MOVE_FILES),
	  _data(data)
{
}

bool MoveFilesCtrl::OnInitDialog () throw (Win::Exception)
{
	_fileListing.Init(GetWindow(), IDC_LIST);
	_targetPath.Init(GetWindow(), IDC_EDIT);
	_targetPath.SetText(_data._targetDir);
	WindowSeq & names = _data._names;
	while (!names.AtEnd())
	{
		char const * name = names.GetName();
		_fileListing.AddItem(name);
		names.Advance();
	}
	return true;
}

bool MoveFilesCtrl::OnApply () throw ()
{
	std::string target(_targetPath.GetText());
	if (_data.Verify(target))
	{
		_data.SetTarget(target);
		EndOk ();
	}
	return true;
}

// Usage:
// co-op -Project_Visit 27 -Selection_Move "c:\Project\foo.txt" target:"c:\Project\Sub"
bool MoveFilesCtrl::GetDataFrom(NamedValues const & source)
{
	std::string target = source.GetValue("target");
	_data.SetTarget(target);
	return true;
}
