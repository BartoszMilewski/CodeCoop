//------------------------------------
//  (c) Reliable Software, 2006 - 2007
//------------------------------------

#include "precompiled.h"
#include "Restorer.h"
#include "HistoryRange.h"
#include "History.h"
#include "PathFind.h"
#include "Workspace.h"
#include "Xml/XmlTree.h"

//
//	Script range - a list of scripts ordered by the predecessor relation
//
//		====================== <-- after file version
//		+--------------------+
//		| Youngest Script Id |
//		+--------------------+
//		|					 |
//		.					 .
//		.					 .
//		|					 |
//		+--------------------+
//		| Oldest Script Id	 |
//		+--------------------+
//		====================== <-- before file version
//

//
// One file restorer
//

Restorer::Restorer (History::Db const & history,
					PathFinder & pathFinder,
					History::Range const & scriptRange,
					GlobalId forkId,
					GlobalId fileGid)
	: _fileGid (fileGid),
	  _pathFinder (pathFinder),
	  _isReconstructed (false),
	  _createdByBranch (false),
	  _diffBackwards (IsBackwardDiff (history, scriptRange)),
	  _currentVersionSelected (scriptRange.IsFromCurrentVersion ())
{
	_diffArea.SetAreaId (Area::Compare);
	GidSet singleFileFilter;
	singleFileFilter.insert (_fileGid);
#if !defined (NDEBUG)
	bool rangeIsFromExecutedTrunk = history.IsExecuted (scriptRange.GetOldestId ()) && 
									history.IsExecuted (scriptRange.GetYoungestId ());
	bool rangeIsFromBranch = history.IsRejected (scriptRange.GetOldestId ()) &&
							 history.IsRejected (scriptRange.GetYoungestId ());
	bool rangeIsFromUnpackedTrunk = history.IsCandidateForExecution (scriptRange.GetOldestId ()) &&
									history.IsCandidateForExecution (scriptRange.GetYoungestId ());
	Assert (rangeIsFromExecutedTrunk || rangeIsFromBranch || rangeIsFromUnpackedTrunk);
#endif
	if (_diffBackwards) // <-Executed trunk script
	{
		// Base path: undo everything from current to youngest, exclusive
		History::Range trunkUndoRange;
		history.CreateRangeFromCurrentVersion (scriptRange.GetYoungestId (), singleFileFilter, trunkUndoRange);
		if (trunkUndoRange.GetOldestId () == scriptRange.GetYoungestId ())
			trunkUndoRange.RemoveOldestId ();	// Remove scriptRange.GetYoungestId () from the trunk undo range
		_basePath.AddUndoRange (history, trunkUndoRange, singleFileFilter);
		// Diff path: undo from youngest to oldest, inclusive
		_diffPath.AddUndoRange (history, scriptRange, singleFileFilter);
	}
	else // Not executed either trunk or branch script
	{
		Assert (history.IsRejected (scriptRange.GetYoungestId ()) ||
				history.IsCandidateForExecution (scriptRange.GetYoungestId ()));
		// Follow trunk or branch back to an executed script
		// 1. If in trunk, this will be current version
		// 2. If in branch, this will be trunk branch point
		History::Range toFirstExecutedScript;
		GlobalId firstExecutedScriptId = history.CreateRangeToFirstExecuted (scriptRange.GetOldestId (),
																			 singleFileFilter,
																			 toFirstExecutedScript);
		if (firstExecutedScriptId == gidInvalid ||			  // There are no executed scripts in the history
			history.IsCurrentVersion (firstExecutedScriptId)) // Dealing with future trunk scripts
		{
			Assert (history.IsCandidateForExecution (scriptRange.GetOldestId ()) ||
					history.IsRejected (scriptRange.GetOldestId ()));
			if (toFirstExecutedScript.GetYoungestId () == scriptRange.GetOldestId ())
				toFirstExecutedScript.RemoveYoungestId ();	// exclusive
			_basePath.AddRedoRange (history, toFirstExecutedScript, singleFileFilter);
		}
		else
		{
			// Branch range (Note: this branch may become a trunk if current version is to be rejected)
			Assert (history.IsBranchPoint (firstExecutedScriptId));
			// Base path: from current version to branchpoint (exclusive) and up the branch to oldest (exclusive)
			History::Range trunkRange;
			history.CreateRangeFromCurrentVersion (firstExecutedScriptId, // branch point
												   singleFileFilter,
												   trunkRange);
			if (trunkRange.GetOldestId () == firstExecutedScriptId)
				trunkRange.RemoveOldestId ();	// Remove the trunk branch point from trunk range
			if (toFirstExecutedScript.GetYoungestId () == scriptRange.GetOldestId ())
				toFirstExecutedScript.RemoveYoungestId ();	// Exclusive
			_basePath.AddUndoRange (history, trunkRange, singleFileFilter);
			_basePath.AddRedoRange (history, toFirstExecutedScript, singleFileFilter);
		}
		// Diff path: from oldest to youngest inclusive
		_diffPath.AddRedoRange (history, scriptRange, singleFileFilter);
	}
	_createdByBranch = CreatesItem ();
	if (!_createdByBranch && forkId != gidInvalid && forkId != scriptRange.GetOldestId ())
	{
		Lineage lineage;
		history.GetLineageStartingWith (lineage, forkId);
		for (Lineage::Sequencer lineageSeq (lineage); !lineageSeq.AtEnd (); lineageSeq.Advance ())
		{
			GlobalId scriptId = lineageSeq.GetScriptId ();
			if (history.IsFileChangedByScript (fileGid, scriptId))
			{
				// First script after fork id that changes restored file
				_createdByBranch = history.IsFileCreatedByScript (fileGid, scriptId);
				break;
			}
		}
	}
}

Restorer::~Restorer ()
{
	if (!_binaryFileTmpPath.empty ())
		File::DeleteNoEx (_binaryFileTmpPath);
	_baseArea.Cleanup (_pathFinder);
	_diffArea.Cleanup (_pathFinder);
}

bool Restorer::IsBackwardDiff (History::Db const & history, History::Range const & range) 
{
	return history.IsExecuted (range.GetOldestId ());
}

TmpProjectArea const & Restorer::GetAfterArea () const
{
	if (_diffBackwards)
		return _baseArea;
	else
		return _diffArea;
}

TmpProjectArea const & Restorer::GetBeforeArea () const
{
	if (_diffBackwards)
		return _diffArea;
	else
		return _baseArea;
}

TmpProjectArea & Restorer::GetBeforeArea ()
{
	if (_diffBackwards)
		return _diffArea;
	else
		return _baseArea;
}

UniqueName const & Restorer::GetAfterUniqueName () const
{
	FileData const & fd = GetLaterFileData ();
	return fd.GetUniqueName ();
}

UniqueName const & Restorer::GetBeforeUniqueName () const
{
	FileData const & fd = GetEarlierFileData ();
	if (fd.IsRenamedIn (Area::Original))
		return fd.GetUnameIn (Area::Original);
	else
		return fd.GetUniqueName ();
}

void Restorer::CreateDifferArgs (XML::Tree & xmlTree) const
{
	XML::Node * root = 0;
	TmpProjectArea const & afterArea = GetAfterArea ();
	TmpProjectArea const & beforeArea = GetBeforeArea ();
	XML::Node * fileNode = 0;
	if (afterArea.IsReconstructed (_fileGid) &&
		beforeArea.IsReconstructed (_fileGid))
	{
		root = xmlTree.SetRoot ("diff");
		// Before
		fileNode = root->AddEmptyChild ("file");
		fileNode->AddTransformAttribute ("path", _pathFinder.GetFullPath (_fileGid, beforeArea.GetAreaId ()));
		fileNode->AddAttribute ("edit", "no");
		fileNode->AddAttribute ("role", "before");
		std::string  dispPath = "[before] ";
		dispPath += GetFileName ();
		fileNode->AddTransformAttribute ("display-path", dispPath);

		// After
		fileNode = root->AddEmptyChild ("file");
		fileNode->AddTransformAttribute ("path", _pathFinder.GetFullPath (_fileGid, afterArea.GetAreaId ()));
		fileNode->AddAttribute ("edit", "no");
		fileNode->AddAttribute ("role", "after");
		std::string  dispPath2 = "[after] ";
		dispPath2 += GetFileName ();
		fileNode->AddTransformAttribute ("display-path", dispPath2);

	}
	else
	{
		// Call editor
		root = xmlTree.SetRoot ("edit");
		char const * path = 0;

		if (afterArea.IsReconstructed (_fileGid) || beforeArea.IsReconstructed (_fileGid))
		{
			fileNode = root->AddEmptyChild ("file");
			fileNode->AddAttribute ("edit", "no");
		}

		if (afterArea.IsReconstructed (_fileGid))
		{
			// After
			fileNode->AddAttribute ("role", "after");
			path = _pathFinder.GetFullPath (_fileGid, afterArea.GetAreaId ());
		}
		else if (beforeArea.IsReconstructed (_fileGid))
		{
			// Before
			fileNode->AddAttribute ("role", "before");
			path = _pathFinder.GetFullPath (_fileGid, beforeArea.GetAreaId ());
		}

		if (fileNode != 0)
		{
			// Add paths
			fileNode->AddTransformAttribute ("path", path);
			fileNode->AddTransformAttribute ("display-path", GetRootRelativePath ());
		}
	}

	// Current
	Workspace::HistoryItem const & item = GetLaterItem ();
	if (item.HasEffectiveSource ())
	{
		// Add source path only when file not created by the restorer
		char const * currentPath = _pathFinder.GetFullPath (_fileGid, Area::Project);
		if (File::Exists (currentPath))
		{
			fileNode = root->AddEmptyChild ("file");
			fileNode->AddTransformAttribute ("path", currentPath);
			fileNode->AddAttribute ("role", "current");
		}
	}
}

bool Restorer::IsFolder () const
{
	FileData const & fd = GetLaterFileData ();
	return fd.GetType ().IsFolder ();
}

bool Restorer::IsTextual () const
{
	// File has always been textual
	return GetFileTypeBefore ().IsTextual () && GetFileTypeAfter ().IsTextual ();
}

bool Restorer::IsBinary () const
{
	// File has always been binary
	return GetFileTypeBefore ().IsBinary () && GetFileTypeAfter ().IsBinary ();
}

FileType Restorer::GetFileTypeAfter () const
{
	FileData const & fd = GetLaterFileData ();
	return fd.GetType ();
}

FileType Restorer::GetFileTypeBefore () const
{
	FileData const & fd = GetEarlierFileData ();
	if (fd.IsTypeChangeIn (Area::Original))
		return fd.GetTypeIn (Area::Original);
	else
		return fd.GetType ();
}

bool Restorer::EditsItem () const
{
	if (IsFolder ())
		return false;

	Workspace::HistoryItem const & itemAfter = GetLaterItem ();
	Workspace::HistoryItem const & itemBefore = GetEarlierItem ();
	return itemBefore.HasIntendedSource () && itemAfter.HasIntendedTarget ();
}

bool Restorer::CreatesItem () const
{
	Workspace::HistoryItem const & item = GetEarlierItem ();
	return !item.HasIntendedSource () && item.HasIntendedTarget ();
}

bool Restorer::DeletesItem () const
{
	Workspace::HistoryItem const & item = GetLaterItem ();
	return item.HasIntendedSource () && !item.HasIntendedTarget ();
}

bool Restorer::IsAbsent () const
{
	Workspace::HistoryItem const & itemAfter = GetLaterItem ();
	Workspace::HistoryItem const & itemBefore = GetEarlierItem ();
	return !itemBefore.HasIntendedSource () && !itemAfter.HasIntendedTarget ();
}

FileData const & Restorer::GetEarlierFileData () const
{
	Workspace::HistoryItem const & item = GetEarlierItem ();
	return item.GetOriginalSourceFileData ();
}

std::string const & Restorer::GetBinaryFilePath ()
{
	Assert (!IsTextual ());
	if (_binaryFileTmpPath.empty ())
	{
		CreateTmpFilePath ();
		Assert (!_binaryFileTmpPath.empty ());
	}

	return _binaryFileTmpPath;
}

std::string Restorer::GetRootRelativePath () const
{
	Workspace::HistoryItem const & item = GetLaterItem ();
	UniqueName const & targetName = item.GetIntendedTarget ();
	return _pathFinder.GetRootRelativePath (targetName);
}

std::string Restorer::GetReferenceRootRelativePath ()
{
	Workspace::HistoryItem const & item = GetEarlierItem ();
	UniqueName const & referenceName = item.GetIntendedSource ();
	return _pathFinder.GetRootRelativePath (referenceName);
}

std::string const & Restorer::GetFileName () const
{
	Workspace::HistoryItem const & item = GetLaterItem ();
	UniqueName const & targetName = item.GetIntendedTarget ();
	return targetName.GetName ();
}

std::string Restorer::GetReferencePath ()
{
	TmpProjectArea & beforeArea = GetBeforeArea ();
	if (!beforeArea.IsReconstructed (_fileGid))
	{
		beforeArea.CreateEmptyFile (_fileGid, _pathFinder);
	}
	return _pathFinder.GetFullPath (_fileGid, beforeArea.GetAreaId ());
}

std::string Restorer::GetRestoredPath ()
{
	TmpProjectArea const & afterArea = GetAfterArea ();
	return _pathFinder.GetFullPath (_fileGid, afterArea.GetAreaId ());
}

void Restorer::CreateTmpFilePath ()
{
	std::string userFileName;
	Area::Location sourceArea;
	if (GetAfterArea ().IsReconstructed (_fileGid))
	{
		UniqueName const & afterUname = GetAfterUniqueName ();
		userFileName = afterUname.GetName ();
		sourceArea = GetAfterArea ().GetAreaId ();
	}
	else
	{
		Assert (GetBeforeArea ().IsReconstructed (_fileGid));
		UniqueName const & beforeUname = GetBeforeUniqueName ();
		userFileName = beforeUname.GetName ();
		sourceArea = GetBeforeArea ().GetAreaId ();
	}

	// Rename reconstructed file in the source area to the user file name
	std::string tgtPath (_pathFinder.GetSysFilePath (userFileName.c_str ()));
	if (File::Exists (tgtPath))
		tgtPath = File::CreateUniqueName (tgtPath.c_str (), "copy");
	char const * srcPath = _pathFinder.GetFullPath (_fileGid, sourceArea);
	File::Move (srcPath, tgtPath.c_str ());
	File::MakeReadOnly (tgtPath.c_str ());
	_binaryFileTmpPath = tgtPath;
}

FileData const & Restorer::GetLaterFileData () const
{
	Workspace::HistoryItem const & item = GetLaterItem ();
	return item.GetOriginalTargetFileData ();
}

Workspace::HistoryItem const & Restorer::GetLaterItem () const
{
	Assert (!_diffPath.IsEmpty ());
	Workspace::HistorySelection const * selection = 0;
	if (_diffBackwards)
	{
		selection = &_diffPath.GetFirstSegment ();
	}
	else
	{
		selection = &_diffPath.GetLastSegment ();
	}

	Assert (selection != 0);
	// Just one item - one file
	Assert (selection->size () == 1);
	Workspace::Selection::Sequencer selSeq (*selection);
	Assert (!selSeq.AtEnd ());
	return selSeq.GetHistoryItem ();
}

Workspace::HistoryItem const & Restorer::GetEarlierItem () const
{
	Assert (!_diffPath.IsEmpty ());
	Workspace::HistorySelection const * selection = 0;
	if (_diffBackwards)
	{
		selection = &_diffPath.GetLastSegment ();
	}
	else
	{
		selection = &_diffPath.GetFirstSegment ();
	}

	Assert (selection != 0);
	// Just one item - one file
	Assert (selection->size () == 1);
	Workspace::Selection::Sequencer selSeq (*selection);
	Assert (!selSeq.AtEnd ());
	return selSeq.GetHistoryItem ();
}

FileCmd const & Restorer::GetFileCmd () const
{
	Assert (CreatesItem ());
	Workspace::HistoryItem const & item = GetLaterItem ();
	Workspace::HistoryItem::ForwardCmdSequencer seq (item);
	Assert (seq.Count () == 1);
	return seq.GetCmd ();
}
