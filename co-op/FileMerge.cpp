//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "precompiled.h"
#undef DBG_LOGGING
#define DBG_LOGGING false

#include "Model.h"
#include "AltDiffer.h"
#include "AttributeMerge.h"
#include "PhysicalFile.h"
#include "HistoricalFiles.h"
#include "FeedbackMan.h"
#include "BuiltInMerger.h"
#include "OutputSink.h"
#include "Workspace.h"
#include "Transformer.h"
#include "FileList.h"
#include "FileTrans.h"
#include "CmdExec.h"
#include "CmdLineSelection.h"
#include "SccProxyEx.h"
#include "MergerProxy.h"
#include "CmdLineMaker.h"
#include "ActiveMerger.h"
#include "AppInfo.h"
#include <XML/XmlTree.h>

void Model::MergeAttributes (std::string const & currentPath,
							 std::string const & newPath,
							 FileType newType)
{
	PathParser parser (_directory);
	UniqueName const * uName = parser.Convert (currentPath.c_str ());
	if (uName == 0 || uName->IsRootName ())
		throw Win::InternalException ("Cannot merge file attributes - illegal current target path", currentPath.c_str ());

	FileData const * currentFd = _dataBase.FindProjectFileByName (*uName);
	if (currentFd == 0)
		throw Win::InternalException ("Cannot merge file attributes - file not controlled", currentPath.c_str ());

	GlobalId gid = currentFd->GetGlobalId ();
	FileType currentType = currentFd->GetType ();

	PathSplitter splitter1 (currentPath);
	std::string currentFolder (splitter1.GetDrive ());
	currentFolder += splitter1.GetDir ();
	std::string currentName (splitter1.GetFileName ());
	currentName += splitter1.GetExtension ();

	PathSplitter splitter2 (newPath);
	std::string newFolder (splitter2.GetDrive ());
	newFolder += splitter2.GetDir ();
	std::string newName (splitter2.GetFileName ());
	newName += splitter2.GetExtension ();

	if (currentName != newName ||
		!FilePath::IsEqualDir (currentFolder, newFolder))
	{
		uName = parser.Convert (newPath.c_str ());
		if (uName == 0 || uName->IsRootName ())
			throw Win::InternalException ("Cannot merge file attributes - illegal new target path", newPath.c_str ());
		RenameFile (gid, *uName);
	}

	if (!newType.IsEqual (currentType))
	{
		GidList files;
		files.push_back (gid);
		ChangeFileType (files, newType);
	}
}

void Model::MergeContent (PhysicalFile & file, Progress::Meter & meter)
{
	FileState state = file.GetState ();
	Assert (state.IsRelevantIn (Area::Original));
	Assert (state.IsPresentIn (Area::Original));
	Assert (state.IsPresentIn (Area::Project));
	Assert (state.IsPresentIn (Area::Synch));
	// Find out if we can merge contents -- for textual files
	// we know how to merge contents. For binary files we don't know
	// how to merge contents.
	if (file.IsTextual ())
	{
		if (state.IsPresentIn (Area::Reference))
		{
			XML::Tree xmlArgs;
			XML::Node * root = xmlArgs.SetRoot ("merge");
			root->AddAttribute ("auto", "true");
			XML::Node * child = 0;
			// Current file
			std::string const & targetPath = file.GetFullPath (Area::Project);
			child = root->AddEmptyChild ("file");
			child->AddAttribute ("role", "result");
			child->AddTransformAttribute ("path", targetPath);
			child->AddAttribute ("edit", "yes");

			child = root->AddEmptyChild ("file");
			child->AddAttribute ("role", "source");
			child->AddTransformAttribute ("path", file.GetFullPath (Area::Synch));
			child->AddAttribute ("edit", "no");

			child = root->AddEmptyChild ("file");
			child->AddAttribute ("role", "target");
			child->AddTransformAttribute ("path", targetPath);
			child->AddAttribute ("edit", "no");

			child = root->AddEmptyChild ("file");
			child->AddAttribute ("role", "reference");
			child->AddTransformAttribute ("path", file.GetFullPath (Area::Reference));
			child->AddAttribute ("edit", "no");

			::ExecuteAutoMerger (xmlArgs, &_pendingAutoMergers, file.GetGlobalId ());
		}
		// Else do nothing, because both synch and user are adding (restoring)
		// the same file. Keep user version in project area
	}
	else
	{
		Assert (file.IsBinary ());
		// Tell the user what we did during transaction.
		// We don't know how to merge binary files, so we
		// just preserve local user edits by making a copy
		// of the binary file in the project area.
		std::string projectPath (file.GetFullPath (Area::Project));
		UniqueName const & currentUname = file.GetUniqueName ();
		PathSplitter splitter (currentUname.GetName ());
		std::string nameWithSuffix (splitter.GetFileName ());
		nameWithSuffix += " pre-merge";
		nameWithSuffix += splitter.GetExtension ();
		UniqueName preMergeUname (currentUname.GetParentId (), nameWithSuffix);
		std::string preMergePath (_pathFinder.GetFullPath (preMergeUname));
		Assert (!preMergePath.empty ());
		std::string info ("Code Co-op cannot auto-merge binary files. Your local changes have been preserved as:\n\n    ");
		info += preMergePath;
		info += "\n\nThe file modified by the synch is:\n\n    ";
		info += projectPath;
		info += "\n\nAfter accepting the synch, you may merge these two files manually.";
		TheOutput.Display (info.c_str ());
	}
}

void Model::OnAutoMergeCompleted (ActiveMerger const * merger, bool success)
{
	GlobalId mergedFileGid = _pendingAutoMergers.FindErase (merger);
	if ((mergedFileGid != gidInvalid) && success)
	{
		// Clear merge conflict bit
		Transaction xact (*this, _pathFinder);
		Transformer trans (_dataBase, mergedFileGid);
		trans.SetMergeConflict (false);
		_synchArea.Notify (changeEdit, mergedFileGid);
		xact.Commit ();
	}
}

void Model::SetMergeTarget (int targetProjectId, GlobalId forkScriptId)
{
	Assert (targetProjectId != -1);
	_mergedFiles.SetTargetProject (targetProjectId, forkScriptId);
}

void Model::GetInterestingScripts (GidSet & interestingScripts) const
{
	Assert (_mergedFiles.IsTargetProjectSet () && !_mergedFiles.IsLocalMerge ());
	interestingScripts.insert (gidInvalid); // make current version interesting
	_history.GetItemsYoungerThen (_mergedFiles.GetForkId (), interestingScripts);
}

void Model::PrepareMerge (Progress::Meter & progressMeter, HistoricalFiles & historicalFiles)
{
	dbg << "--> Prepare Merge" << std::endl;
	if (historicalFiles.IsEmpty ())
		return;

	if (!historicalFiles.IsTargetProjectSet ())
		return;

	bool userCancel = false;
	try
	{
		GidList files;
		historicalFiles.GetFileList (files);
		progressMeter.SetRange (0, files.size ());
		progressMeter.SetActivity ("Re-creating file(s) at source.");

		dbg << "   Creating restorers for all files" << std::endl;
		for (GidList::const_iterator iter = files.begin (); iter != files.end (); ++iter)
		{
			Restorer & restorer = historicalFiles.GetRestorer (*iter);
			progressMeter.SetActivity (restorer.GetRootRelativePath ().c_str ());
			if (!restorer.IsReconstructed ())
				Reconstruct (restorer);

			progressMeter.StepAndCheck ();
		}
		dbg << "   QueryTargetData" << std::endl;
		historicalFiles.QueryTargetData (progressMeter, _dataBase, _directory);
	}
	catch (Win::Exception ex)
	{
		if (ex.GetMessage () != 0)
			throw ex;

		userCancel = true;
	}

	if (userCancel)
		historicalFiles.ClearTargetData ();
	dbg << "<-- Prepare Merge" << std::endl;
}

bool Model::PrepareMerge (GlobalId gid, HistoricalFiles & historicalFiles)
{
	Restorer & restorer = historicalFiles.GetRestorer (gid);
	if (!restorer.IsReconstructed ())
		Reconstruct (restorer);

	return historicalFiles.QueryTargetData (gid, _dataBase, _directory);
}

TargetStatus Model::GetTargetPath (GlobalId gid,
								   std::string const & sourcePath,
								   std::string & targetPath,
								   FileType & targetType)
{
	return _mergedFiles.GetTargetPath (_dataBase,
									   _directory,
									   gid,
									   sourcePath,
									   targetPath,
									   targetType);
}
