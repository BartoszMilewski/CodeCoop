//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "precompiled.h"
#include "MergeExec.h"
#include "Model.h"
#include "HistoricalFiles.h"
#include "OutputSink.h"
#include "Prompter.h"
#include "ProjectPath.h"
#include "Database.h"
#include "AttributeMerge.h"
#include "FileAttribMergeDlg.h"
#include "MergerProxy.h"
#include "Catalog.h"
#include "CmdLineSelection.h"
#include <XML/XmlTree.h>

MergeExec::MergeExec (HistoricalFiles & historicalFiles, GlobalId fileGid, bool autoMerge, bool localMerge)
	: _historicalFiles (historicalFiles),
	  _restorer (historicalFiles.GetRestorer (fileGid)),
	  _targetData (historicalFiles.GetTargetData (fileGid)),
	  _fileGid (fileGid),
	  _autoMerge (autoMerge),
	  _localMerge (localMerge)
{}

//Returns true when content identical or auto merge without conflicts
bool MergeExec::MergeContent (MergerProxy & merger)
{
	char const * targetFullPath = _historicalFiles.GetTargetFullPath (_targetData.GetTargetPath ());
	if (!File::Exists (targetFullPath))
		throw Win::InternalException ("Cannot merge file content, because the target file doesn't exist", targetFullPath);

	std::string restoredPath = _restorer.GetRestoredPath ();
	if (File::IsContentsEqual (restoredPath.c_str (), targetFullPath))
		return true;

	std::string referencePath = _restorer.GetReferencePath ();
	if ((_autoMerge || !_restorer.IsTextual ()) // in auto merge and for binary files, try trivial
		&& File::Exists (referencePath.c_str ())
		&& File::IsContentsEqual (referencePath.c_str (), targetFullPath))
	{
		// No edits at target - copy source to target
		merger.Checkout (targetFullPath, _fileGid);
		merger.CopyFile (restoredPath, targetFullPath, _autoMerge);
		return true;
	}

	if (!_restorer.IsTextual ())
	{
		std::string info ("Code Co-op cannot merge the following file:\n\n");
		info += _restorer.GetRootRelativePath ();
		info += "because it is a binary file.";
		TheOutput.Display (info.c_str ());
		return false;
	}

	merger.Checkout (targetFullPath, _fileGid);

	std::string srcTitle;
	std::string targetTitle;
	std::string ancestorTitle ("Common Ancestor Version");
	if (_historicalFiles.IsLocalMerge ())
	{
		srcTitle = "Script Version";
		targetTitle = "Current Version";
	}
	else
	{
		Catalog catalog;
		srcTitle = '\'' + catalog.GetProjectName (_historicalFiles.GetThisProjectId ()) + "\' Version";
		targetTitle = '\'' + catalog.GetProjectName (_historicalFiles.GetTargetProjectId ()) + "\' Version";
		if (_historicalFiles.IsSelectiveMerge ())
			ancestorTitle = '\'' + catalog.GetProjectName (_historicalFiles.GetThisProjectId ()) + "\' Previous Version";
	}

	XML::Tree xmlArgs;
	XML::Node * root = xmlArgs.SetRoot ("merge");
	XML::Node * child = 0;
	// Current file
	child = root->AddEmptyChild ("file");
	child->AddAttribute ("role", "result");
	child->AddTransformAttribute ("path", targetFullPath);
	child->AddAttribute ("edit", "yes");
	// Target file
	child = root->AddEmptyChild ("file");
	child->AddAttribute ("role", "target");
	child->AddTransformAttribute ("path", targetFullPath);
	child->AddTransformAttribute ("display-path", targetTitle);
	child->AddAttribute ("edit", "no");
	// Source file
	child = root->AddEmptyChild ("file");
	child->AddAttribute ("role", "source");
	child->AddTransformAttribute ("path", restoredPath);
	child->AddTransformAttribute ("display-path", srcTitle);
	child->AddAttribute ("edit", "no");
	// Reference file
	child = root->AddEmptyChild ("file");
	child->AddAttribute ("role", "reference");
	child->AddTransformAttribute ("path", referencePath);
	child->AddTransformAttribute ("display-path", ancestorTitle);
	child->AddAttribute ("edit", "no");

	if (_autoMerge)
	{
		root->AddAttribute ("auto", "true");
		return ::ExecuteAutoMerger (xmlArgs);
	}
	else
		::ExecuteMerger (xmlArgs);
	return false;
}

//Returns true when target path is recorded in the project database
bool MergeExec::IsTargetPathRecorded (FileIndex const & fileIndex, Directory const & directory) const
{
	TargetStatus status = _targetData.GetTargetStatus ();
	if (_localMerge && !status.IsParentControlled ())
	{
		PathParser parser (directory);
		UniqueName const * uname =
			parser.Convert (_historicalFiles.GetTargetFullPath (_targetData.GetTargetPath ()));
		Assert (uname != 0 && !uname->IsRootName ());
		if (!uname->IsNormalized () && !fileIndex.CanReviveFolder (*uname))
		{
			std::string info ("The following folder\n\n");
			PathSplitter splitter (_targetData.GetTargetPath ());
			info += splitter.GetDir ();
			info += "\n\ndoesn't exist in the target. You have to merge the folder first.";
			TheOutput.Display (info.c_str ());
			return false;
		}
	}

	return true;
}

bool MergeDeleteExec::VerifyMerge (FileIndex const & fileIndex, Directory const & directory)
{
	if (_autoMerge)
		return true;

	Out::Answer result;
	if (_restorer.IsFolder ())
	{
		std::string info ("The following folder\n\n");
		info += _targetData.GetTargetPath ();
		info += "\n\nis deleted at source.\n"
				"Would you like to delete it, and all its contents, at target too?",
		result = TheOutput.Prompt (info.c_str (),
								   Out::PromptStyle (Out::YesNo, Out::No, Out::Question));
	}
	else
	{
		std::string info ("The following file\n\n");
		info += _targetData.GetTargetPath ();
		info += "\n\nis deleted at source.\n"
				"Would you like to delete it at target too?";
		if (_targetData.GetTargetStatus ().IsCheckedOut ())
			info += "\n\nWarning: The target file is checked out. You will lose your edits!";

		result = TheOutput.Prompt (info.c_str (),
								   Out::PromptStyle (Out::YesNo, Out::No, Out::Question));
	}
	return result != Out::No;
}

bool MergeCreateExec::VerifyMerge (FileIndex const & fileIndex, Directory const & directory)
{
	if (!IsTargetPathRecorded (fileIndex, directory))
		return false;

	if (_autoMerge)
		return true;

	std::string info ("The following ");
	if (_restorer.IsFolder ())
		info += "folder\n\n";
	else
		info += "file\n\n";
	info += _restorer.GetRootRelativePath ();
	info += "\n\nis new at source and does not exist at target.\n"
			"Would you like to create it there?";

	Out::Answer result = TheOutput.Prompt (info.c_str (),
										   Out::PromptStyle (Out::YesNo, Out::No, Out::Question));
	return result != Out::No;
}

bool MergeReCreateExec::VerifyMerge (FileIndex const & fileIndex, Directory const & directory)
{
	if (!IsTargetPathRecorded (fileIndex, directory))
		return false;

	if (_autoMerge)
	{
		TargetStatus targetStatus = _targetData.GetTargetStatus ();
		return !targetStatus.HasDifferentGid ();
	}

	std::string info ("The following ");
	if (_restorer.IsFolder ())
		info += "folder\n\n";
	else
		info += "file\n\n";
	info += _targetData.GetTargetPath ();
	info += "\n\nis deleted at target.\n"
			"Would you like to add it back?";

	Out::Answer result = TheOutput.Prompt (info.c_str (),
										   Out::PromptStyle (Out::YesNo, Out::No, Out::Question));
	return result != Out::No;
}

bool MergeContentsExec::VerifyMerge (FileIndex const & fileIndex, Directory const & directory)
{
	if (_historicalFiles.IsAttributeMergeNeeded (_fileGid))
	{
		AttributeMerge attributes (_restorer, _targetData);
		if (_autoMerge)
		{
			if (attributes.IsConflict ())
				return false;
		}
		else
		{
			FileAttribMergeCtrl dlgCtrl (attributes);
			if (!ThePrompter.GetData (dlgCtrl))
				return false;	// User doesn't want to continue with content merge
		}
		SetAttributeMerge (attributes);
	}
	return true;
}

void MergeContentsExec::SetAttributeMerge (AttributeMerge const & attrib)
{
	// Check file type
	_attributeMergeNeeded = attrib.IsAttribMergeNeeded ();
	if (_attributeMergeNeeded)
	{
		// Remember merged attributes
		FilePath path (attrib.GetFinalPath ());
		_newTargetPath.assign (path.GetFilePath (attrib.GetFinalName ()));
		_newTargetType = attrib.GetFinalType ();
	}
}

bool MergeCreateExec::DoMerge (MergerProxy & merger)
{
	TargetStatus targetStatus = _targetData.GetTargetStatus ();
	
	Assert (!targetStatus.IsControlled () 
			&& (!targetStatus.IsControlledByPath () || targetStatus.IsDeletedByPath ()));
	// Creation is always by path!
	char const * targetFullPath = _historicalFiles.GetTargetFullPath (_targetData.GetTargetPath ());
	if (!targetStatus.IsPresentByPath ())
	{
		if (_restorer.IsFolder ())
			merger.MaterializeFolderPath (targetFullPath);
		else
			merger.CopyFile (_restorer.GetRestoredPath (), targetFullPath, _autoMerge);
	}

	merger.AddFile (targetFullPath, _fileGid, _restorer.GetFileTypeAfter ());

	if (!_restorer.IsFolder () && targetStatus.IsPresentByPath ())
		MergeContent (merger);

	return true;
}

bool MergeReCreateExec::DoMerge (MergerProxy & merger)
{
	TargetStatus targetStatus = _targetData.GetTargetStatus ();
	Assert (targetStatus.IsDeleted ());
	// Re-creation is always by path!
	char const * targetFullPath = _historicalFiles.GetTargetFullPath (_targetData.GetTargetPath ());
	if (!targetStatus.IsPresentByPath ())
	{
		if (_restorer.IsFolder ())
			merger.MaterializeFolderPath (targetFullPath);
		else
			merger.CopyFile (_restorer.GetRestoredPath (), targetFullPath, _autoMerge);
	}

	merger.ReCreateFile (targetFullPath, _fileGid, _restorer.GetFileTypeAfter ());
	
	if (!_restorer.IsFolder () && targetStatus.IsPresentByPath ())
		MergeContent (merger);

	return true;
}

bool MergeDeleteExec::DoMerge (MergerProxy & merger)
{
	Assert (_restorer.DeletesItem ());
	Assert (!_targetData.GetTargetStatus ().IsDeleted ());
	char const * targetFullPath = _historicalFiles.GetTargetFullPath (_targetData.GetTargetPath ());
	merger.Delete (targetFullPath);
	return true;
}

bool MergeContentsExec::DoMerge (MergerProxy & merger)
{
	if (_attributeMergeNeeded)
	{
		merger.MergeAttributes (_historicalFiles.GetTargetFullPath (_targetData.GetTargetPath ()),
								_historicalFiles.GetTargetFullPath (_newTargetPath),
								_newTargetType);
		_targetData.SetTargetPath (_newTargetPath);
		_targetData.SetTargetType (_newTargetType);
	}

	return MergeContent (merger);
}
