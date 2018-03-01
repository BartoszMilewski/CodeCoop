//------------------------------------
//  (c) Reliable Software, 2006 - 2007
//------------------------------------

#include "precompiled.h"
#include "HistoricalFiles.h"
#include "History.h"
#include "Workspace.h"
#include "FeedbackMan.h"
#include "PathFind.h"
#include "VersionInfo.h"
#include "AttributeMerge.h"
#include "Catalog.h"
#include "RegKeys.h"
#include "SccProxyEx.h"
#include "Database.h"
#include "Directory.h"
#include "CmdLineSelection.h"
#include "UiStrings.h"

#include <Ctrl/ProgressBar.h>


//
// Historical files
//

HistoricalFiles::HistoricalFiles (History::Db const & history,
								  PathFinder & pathFinder,
								  Catalog & catalog)
	: _history (history),
	  _pathFinder (pathFinder),
	  _catalog (catalog),
	  _extendedRange (false),
	  _rangeHasMissingPredecessors (false),
	  _targetProjectId (-1),
	  _thisProjectId (-1),
	  _forkId (gidInvalid),
	  _firstBranchId (gidInvalid),
	  _rangeHasRejectedScripts (false),
	  _rangeIsAllRejected (false),
	  _isAncestorMerge (false),
	  _isCumulativeMerge (false)
{}

HistoricalFiles::~HistoricalFiles ()
{}

bool HistoricalFiles::IsEmpty () const
{
	return _fileDiaryMap.size () == 0;
}

bool HistoricalFiles::RangeIncludesMostRecentVersion () const
{
	return _history.MostRecentScriptId () == _range.GetYoungestId ();
}

bool HistoricalFiles::RangeFollowsTrunkBranchPoint () const
{
	GlobalId trunkBranchPoint = _history.GetTrunkBranchPointId (_range.GetOldestId ());
	GlobalId rangePredecessor = _history.GetPredecessorId (_range.GetOldestId ());
	return trunkBranchPoint == rangePredecessor;
}

GlobalId HistoricalFiles::GetFirstFileId () const
{
	if (_fileDiaryList.get () == 0)
		return gidInvalid;;

	Workspace::Selection::Sequencer seq (*_fileDiaryList);
	Assert (!seq.AtEnd ());
	Workspace::Item const & item = seq.GetItem ();
	return item.GetItemGid ();
}

void HistoricalFiles::Clear (bool force)
{
	_range.Clear ();
	_fileFilter.clear ();
	_fileDiaryList.reset ();
	_fileDiaryMap.clear ();
	_extendedRange = false;
	_restorers.clear ();
	_restorerMap.clear ();
	_targetFiles.clear ();
	_rangeHasRejectedScripts = false;
	_rangeIsAllRejected = false;
	_rangeHasMissingPredecessors = false;

	if (force)
	{
		_targetProjectId  = -1;
		_thisProjectId =  -1;
		_forkId = gidInvalid;
		_firstBranchId = gidInvalid;
		_targetProjectRoot.Clear ();
	}
}

// Returns true when scripts re-loaded
bool HistoricalFiles::ReloadScripts (History::Range const & range,
									 GidSet const & fileFilter,
									 bool extendedRange,
									 Progress::Meter & meter)
{
	if (_range.IsEqual (range) 
		&& _extendedRange == extendedRange
		&& _fileFilter == fileFilter)
		return false;

	Clear ();
	_extendedRange = extendedRange;

	if (range.Size () == 0)
		return true;

	if (range.GetOldestId () == gidInvalid)
	{
		Assert (range.Size () == 1);
		// Current project version - nothing to load
		return true;
	}

	if (_history.GetPredecessorId (range.GetOldestId ()) == gidInvalid)
		return true;	// The project creation marker is the last selected script - nothing to show

	_range = range;
	_fileFilter = fileFilter;
	_rangeHasMissingPredecessors = _history.HasMissingPredecessors (_range.GetOldestId ());
	_rangeIsAllRejected = true;
	if (!_rangeHasMissingPredecessors)
	{
		// Check if there are missing scripts in the range
		for (GidList::const_iterator iter = _range.begin (); iter != _range.end (); ++iter)
		{
			GlobalId gid = *iter;
			if (_history.IsMissing (gid))
			{
				_rangeHasMissingPredecessors = true;
				break;
			}
			if (_history.IsRejected (gid))
				_rangeHasRejectedScripts = true;
			else
				_rangeIsAllRejected = false;
		}
	}

	if (_rangeHasMissingPredecessors)
		_rangeIsAllRejected = false;

	if (_fileFilter.empty ())
	{
		// Show all files changed by the selected scripts
		_fileDiaryList.reset (new Workspace::HistoryRangeSelection (
			_history,
			_range,
			true,	// Display selection is always forward
			meter));
	}
	else
	{
		// Show only files present in the filter
		_fileDiaryList.reset (new Workspace::FilteredHistoryRangeSelection (
			_history,
			_range,
			_fileFilter,
			true,	// Display selection is always forward
			meter));
	}

	for (Workspace::Selection::Sequencer seq (*_fileDiaryList); !seq.AtEnd (); seq.Advance ())
	{
		Workspace::Item const & item = seq.GetItem ();
		GlobalId gid = item.GetItemGid ();
		_fileDiaryMap [gid] = &seq.GetHistoryItem ();
	}
	return true;
}

Restorer & HistoricalFiles::GetRestorer (GlobalId gid)
{
	Assert (!RangeHasMissingPredecessors ());
	std::map<GlobalId, unsigned>::const_iterator iter = _restorerMap.find (gid);
	if (iter != _restorerMap.end ())
		return *_restorers [iter->second];

	std::unique_ptr<Restorer> newRestorer (new Restorer (_history, _pathFinder, _range, _forkId, gid));
	_restorers.push_back (std::move(newRestorer));
	unsigned newRestorerIdx = _restorers.size () - 1;
	_restorerMap [gid] = newRestorerIdx;
	return *_restorers [newRestorerIdx];
}

std::unique_ptr<Workspace::HistorySelection> HistoricalFiles::GetRevertSelection (
	Progress::Meter & meter) const
{
	Assert (!_rangeHasMissingPredecessors);
	Assert (RangeIncludesMostRecentVersion () || !_fileFilter.empty ());
	Assert (!_history.IsRejected (_range.GetOldestId ()));
	std::unique_ptr<Workspace::HistorySelection> selection;
	if (_fileFilter.empty ())
	{
		// Revert all files changed by the selected scripts
		selection.reset (new Workspace::HistoryRangeSelection (
			_history,
			_range,
			false,	// Backward selection
			meter));
	}
	else
	{
		// Revert only files present in the filter
		selection.reset (new Workspace::FilteredHistoryRangeSelection (
			_history,
			_range,
			_fileFilter,
			false,	// Backward selection
			meter));
	}
	return selection;
}

std::unique_ptr<Workspace::HistorySelection> HistoricalFiles::GetRevertSelection (
	GidList const & files,
	Progress::Meter & meter) const
{
	Assert (!_rangeHasMissingPredecessors);
	Assert (RangeIncludesMostRecentVersion () || !_fileFilter.empty ());
	Assert (!_history.IsRejected (_range.GetOldestId ()));
	GidSet fileFilter (files.begin (), files.end ());
	std::unique_ptr<Workspace::HistorySelection> selection
		(new Workspace::FilteredHistoryRangeSelection (
			_history,
			_range,
			fileFilter,
			false,	// Backward selection
			meter));
	return selection;
}

void HistoricalFiles::QueryTargetData (Progress::Meter & progressMeter,
									   FileIndex const & fileIndex,
									   Directory const & directory)
{
	Assert (IsTargetProjectSet ());
	progressMeter.SetRange (0, _fileDiaryMap.size ());
	progressMeter.SetActivity ("Checking file/folder status at target.");
	if (IsLocalMerge ())
		QueryLocalTargetData (progressMeter, fileIndex, directory);
	else
		QueryRemoteTargetData (progressMeter);
}

void HistoricalFiles::QueryLocalTargetData (Progress::Meter & progressMeter,
											FileIndex const & fileIndex,
											Directory const & directory)
{
	for (FileSequencer seq = _fileDiaryMap.begin (); seq != _fileDiaryMap.end (); ++seq)
	{
		GlobalId gid = seq->first;
		Restorer & restorer = GetRestorer (gid);
		progressMeter.SetActivity (restorer.GetRootRelativePath ().c_str ());
		GetLocalTargetData (gid, fileIndex, directory);
		progressMeter.StepAndCheck ();
	}
}

void HistoricalFiles::QueryRemoteTargetData (Progress::Meter & progressMeter)
{
	SccProxyEx proxy;
	for (FileSequencer seq = _fileDiaryMap.begin (); seq != _fileDiaryMap.end (); ++seq)
	{
		GlobalId gid = seq->first;
		Restorer & restorer = GetRestorer (gid);
		progressMeter.SetActivity (restorer.GetRootRelativePath ().c_str ());
		GetRemoteTargetData (gid, proxy);
		progressMeter.StepAndCheck ();
	}
}

bool HistoricalFiles::QueryTargetData (GlobalId gid,
									   FileIndex const & fileIndex,
									   Directory const & directory)
{
	if (IsLocalMerge ())
	{
		GetLocalTargetData (gid, fileIndex, directory);
		return true;
	}

	SccProxyEx proxy;
	return GetRemoteTargetData (gid, proxy);
}

void HistoricalFiles::GetLocalTargetData (GlobalId gid,
										  FileIndex const & fileIndex,
										  Directory const & directory)
{
	Restorer & restorer = GetRestorer (gid);
	std::string targetPath;
	FileType targetType;
	TargetStatus targetStatus;
	if (restorer.IsAbsent ())
	{
		targetType = restorer.GetFileTypeAfter ();
	}
	else
	{
		targetStatus = GetTargetPath (fileIndex,
									  directory,
									  gid,
									  restorer.GetRootRelativePath (),
									  targetPath,
									  targetType);
	}
	SetTargetData (gid, targetPath, targetType, targetStatus);
}

bool HistoricalFiles::GetRemoteTargetData (GlobalId gid,
										   SccProxyEx & proxy)
{
	Restorer & restorer = GetRestorer (gid);
	if (restorer.IsAbsent ())
	{
		SetTargetData (gid, "", restorer.GetFileTypeAfter (), TargetStatus ());
		return true;
	}
	else
	{
		std::string targetPath;
		unsigned long statusValue;
		unsigned long typeValue;
		if (proxy.GetTargetPath (GetTargetProjectId (),
								 restorer.IsCreatedByBranch () ? gidInvalid : gid,
								 restorer.GetRootRelativePath (),
								 targetPath,
								 typeValue,
								 statusValue))
		{
			SetTargetData (gid, targetPath, FileType (typeValue), TargetStatus (statusValue));
			return true;
		}

	}

	return false;
}

TargetStatus HistoricalFiles::GetTargetPath (FileIndex const & fileIndex,
											 Directory const & directory,
											 GlobalId gid,
											 std::string const & sourcePath,
										     std::string & targetPath,
										     FileType & targetType)
{
	FileData const * fileDataByGid = 0;
	if (gid != gidInvalid)
		fileDataByGid = fileIndex.FindByGid (gid);

	TargetStatus status;
	if (fileDataByGid != 0)
	{
		status.SetControlled (true);
		FileState state = fileDataByGid->GetState ();
		status.SetDeleted (state.IsNone () || state.IsToBeDeleted ());
		status.SetCheckedOut (state.IsRelevantIn (Area::Original));
		targetType = fileDataByGid->GetType ();
	}

	FilePath root (_pathFinder.GetProjectDir ());
	char const * targetSlotPath = root.GetFilePath (sourcePath);
	PathParser parser (directory);
	UniqueName const * slotUname = parser.Convert (targetSlotPath);
	Assert (slotUname != 0 && !slotUname->IsRootName ());
	FileData const * fileDataByPath = fileIndex.FindProjectFileByName (*slotUname);

	std::string fullTargetPath;
	if (fileDataByGid != 0 &&
		!fileDataByGid->GetState ().IsNone () &&
		!fileDataByGid->GetState ().IsToBeDeleted ())
	{
		fullTargetPath = _pathFinder.GetFullPath (gid, Area::Project);
		targetPath = _pathFinder.GetRootRelativePath (gid);
	}
	else
	{
		fullTargetPath = targetSlotPath;
		targetPath = sourcePath;
		if (fileDataByPath != 0)
			targetType = fileDataByPath->GetType ();
		else
			targetType = FileType ();
	}

	status.SetIsPresentByPath (File::Exists (targetSlotPath));

	if (fileDataByPath != 0)
	{
		status.SetControlledByPath (true);
		FileState state = fileDataByPath->GetState ();
		Assert (!state.IsNone ());
		status.SetDeletedByPath (state.IsToBeDeleted ());
		status.SetCheckedOutByPath (state.IsRelevantIn (Area::Original));
	}

	PathSplitter splitter (targetSlotPath);
	std::string parentPath (splitter.GetDrive ());
	parentPath += splitter.GetDir ();
	UniqueName const * parentUname = parser.Convert (parentPath.c_str ());
	Assert (parentUname != 0);
	if (parentUname->IsRootName ())
	{
		// Project root folder is item's parent
		status.SetParentControlled (true);
	}
	else
	{
		FileData const * parentFileDataByPath = fileIndex.FindProjectFileByName (*parentUname);
		if (parentFileDataByPath != 0)
		{
			status.SetParentControlled (true);
		}
	}

	if (fileDataByPath != 0)
	{
		status.SetDifferentGid (gid != fileDataByPath->GetGlobalId ());
	}

	return status;
}

bool HistoricalFiles::HasTargetData (GlobalId gid) const
{
	std::map<GlobalId, TargetData>::const_iterator seq = _targetFiles.find (gid);
	return (seq != _targetFiles.end ());
}

TargetData & HistoricalFiles::GetTargetData (GlobalId gid)
{
	std::map<GlobalId, TargetData>::iterator seq = _targetFiles.find (gid);
	Assert (seq != _targetFiles.end ());
	return seq->second;
}

MergeStatus HistoricalFiles::GetMergeStatus (GlobalId gid)
{
	return GetTargetData (gid).GetMergeStatus ();
}

Workspace::HistoryItem const & HistoricalFiles::GetFileItem (GlobalId gid) const
{
	FileSequencer seq = _fileDiaryMap.find (gid);
	Assert (seq != _fileDiaryMap.end ());
	return *seq->second;
}

void HistoricalFiles::GetFileList (GidList & files) const
{
	for (FileSequencer seq = _fileDiaryMap.begin (); seq != _fileDiaryMap.end (); ++seq)
	{
		files.push_back (seq->first);
	}
}

char const * HistoricalFiles::GetTargetFullPath (std::string const & targetRelativePath)
{
	return _targetProjectRoot.GetFilePath (targetRelativePath);
}

void HistoricalFiles::SetTargetData (GlobalId gid,
									 std::string const & path,
									 FileType targetType,
									 TargetStatus targetStatus)
{
	TargetData data (path, targetType, targetStatus);
	_targetFiles [gid] = data;
	UpdateMergeStatus (gid);
}

void HistoricalFiles::SetMergeStatus (GlobalId gid, MergeStatus status)
{
	TargetData & data = GetTargetData (gid);
	data.SetMergeStatus (status);
}

// Merge status is only used for display purposes
void HistoricalFiles::UpdateMergeStatus (GlobalId gid)
{
	MergeStatus mergeStatus;
	Restorer & restorer = GetRestorer (gid);
	TargetData & targetData = _targetFiles [gid];
	TargetStatus targetStatus = targetData.GetTargetStatus ();
	if (targetStatus.IsUnknown ())
	{
		targetData.SetMergeStatus (mergeStatus);
		return;
	}
	UniqueName const & uname = restorer.GetAfterUniqueName ();
	if (!_pathFinder.IsFileInProject (uname.GetParentId ()))
	{
		mergeStatus.SetMergeParent (true);
		targetData.SetMergeStatus (mergeStatus);
		return;
	}

	if (restorer.IsCreatedByBranch ())
	{
		// New at source
		if (restorer.IsFolder ())
		{
			if (targetStatus.IsControlledByPath () && !targetStatus.IsDeletedByPath ()&& targetStatus.IsPresentByPath () )
				mergeStatus.SetIdentical (true);
			else
				mergeStatus.SetCreated (true);
			targetData.SetMergeStatus (mergeStatus);
			return;
		}

		if (targetStatus.IsControlled ())
		{
			// Only possible in local case
			if (targetStatus.IsDeleted () && !targetStatus.HasDifferentGid ())
			{
				mergeStatus.SetCreated (true);
				targetData.SetMergeStatus (mergeStatus);
				return;
			}
		}
		else if (!targetStatus.IsPresentByPath () || !targetStatus.IsControlledByPath ())
		{
			mergeStatus.SetCreated (true);
			targetData.SetMergeStatus (mergeStatus);
			return;
		}
	}
	else if (restorer.DeletesItem ())
	{
		// Deleted at source
		if (targetStatus.IsControlled () && !targetStatus.IsDeleted () ||
			targetStatus.IsPresentByPath () && targetStatus.HasDifferentGid ())
		{
			mergeStatus.SetDeletedAtSource (true);
		}
		else
		{
			mergeStatus.SetIdentical (true);
		}
		targetData.SetMergeStatus (mergeStatus);
		return;
	}
	else if (restorer.EditsItem ())
	{
		// File existed before fork
		Assert (!restorer.IsFolder ());
		Assert (restorer.IsReconstructed ());
		if (targetStatus.IsDeleted ())
		{
			if (!targetStatus.IsPresentByPath () || !targetStatus.IsControlledByPath ())
			{
				mergeStatus.SetDeletedAtTarget (true);
				targetData.SetMergeStatus (mergeStatus);
				return;
			}
			// If it's present in its slot and controlled under a different GID
			// we treat it as the same file
		}
	}
	else
	{
		Assert (restorer.IsAbsent ());
		mergeStatus.SetAbsent (true);
		targetData.SetMergeStatus (mergeStatus);
		return;
	}

	FileType restoredType = restorer.GetFileTypeAfter ();
	std::string restoredFilePath = restorer.GetRestoredPath ();
	char const * targetFullPath = GetTargetFullPath (targetData.GetTargetPath ());
	if (!IsAttributeMergeNeeded (gid) &&
		File::IsContentsEqual (restoredFilePath.c_str (), targetFullPath))
	{
		mergeStatus.SetIdentical (true);
	}
	else
	{
		mergeStatus.SetDifferent (true);
	}
	targetData.SetMergeStatus (mergeStatus);
}

bool HistoricalFiles::IsAttributeMergeNeeded (GlobalId gid)
{
	Restorer & restorer = GetRestorer (gid);
	TargetData & targetData = _targetFiles [gid];
	std::string sourcePath = restorer.GetRootRelativePath ();
	if (sourcePath != targetData.GetTargetPath ())
		return true;
	FileType srcFileType = restorer.GetFileTypeAfter ();
	if (!srcFileType.IsEqual (targetData.GetTargetType ()))
		return true;
	return false;
}

std::unique_ptr<MergeExec> HistoricalFiles::GetMergeExec (GlobalId gid, bool autoMerge)
{
	std::unique_ptr<MergeExec> exec;
	Restorer & restorer = GetRestorer (gid);
	TargetData & targetData = _targetFiles [gid];
	Assert (!targetData.GetMergeStatus ().IsIdentical ());
	TargetStatus targetStatus = targetData.GetTargetStatus ();
	if (restorer.DeletesItem ()) 
	{
		Assert (!targetStatus.IsDeleted ());
		exec.reset (new MergeDeleteExec (*this, gid, autoMerge, IsLocalMerge ()));
	}
	else if (restorer.IsCreatedByBranch ())
	{
		if (targetStatus.IsControlled ())
		{
			Assert (IsLocalMerge ());
			if (targetStatus.IsDeleted () && !targetStatus.HasDifferentGid ())
				exec.reset (new MergeReCreateExec (*this, gid, autoMerge, IsLocalMerge ()));
			else
				exec.reset (new MergeContentsExec (*this, gid, autoMerge, IsLocalMerge ()));
		}
		else
		{
			// remote case, or local case when not controlled
			if (!targetStatus.IsControlledByPath () || targetStatus.IsDeletedByPath ())
			{
				exec.reset (new MergeCreateExec (*this, gid, autoMerge, IsLocalMerge ()));
			}
			else
			{
				// ctrlByPath && !delByPath
				exec.reset (new MergeContentsExec (*this, gid, autoMerge, IsLocalMerge ()));
			}
		}
	}
	else
	{
		Assert (restorer.EditsItem ());
		if (targetStatus.IsDeleted ())
		{
			if (targetStatus.IsControlledByPath () 
				&& targetStatus.HasDifferentGid ()
				&& !targetStatus.IsDeletedByPath ())
			{
				if (!targetStatus.IsPresentByPath ())
					throw Win::InternalException ("The file is absent from target branch.\n"
												  "Run Project>Repair",
												  restorer.GetRootRelativePath ().c_str ());
				// merge with an impostor file
				exec.reset (new MergeContentsExec (*this, gid, autoMerge, IsLocalMerge ()));
			}
			else
			{
				// There is no impostor file in the slot, we can safely recreate
				exec.reset (new MergeReCreateExec (*this, gid, autoMerge, IsLocalMerge ()));
			}
		}
		else
			exec.reset (new MergeContentsExec (*this, gid, autoMerge, IsLocalMerge ()));
	}
	return exec;
}

//
// Script details table
//

void ScriptDetails::QueryUniqueIds (Restriction const & restrict, GidList & ids) const
{
	GetFileList (ids);
}

GlobalId ScriptDetails::GetIdField (Column col, GlobalId gid) const
{
	Assert (col == colParentId);
	FileSequencer seq = _fileDiaryMap.find (gid);
	Assert (seq != _fileDiaryMap.end ());
	Workspace::HistoryItem const & item = *seq->second;
	if (item.HasEffectiveTarget ())
	{
		UniqueName const & target = item.GetEffectiveTarget ();
		return target.GetParentId ();
	}
	else if (item.HasEffectiveSource ())
	{
		UniqueName const & source = item.GetEffectiveSource ();
		return source.GetParentId ();
	}
	Assert (!item.HasEffectiveTarget () && !item.HasEffectiveSource ());
	// Item is created and deleted - return its last
	// parent id
	FileData const & fd = item.GetOriginalTargetFileData ();
	return fd.GetUniqueName ().GetParentId ();
}

unsigned long ScriptDetails::GetNumericField (Column col, GlobalId gid) const
{
	if (col == Table::colType)
	{
		FileSequencer seq = _fileDiaryMap.find (gid);
		Assert (seq != _fileDiaryMap.end ());
		Workspace::HistoryItem const & item = *seq->second;

		FileType type;
		if (item.HasEffectiveTarget ())
			type = item.GetEffectiveTargetType ();
		else
			type = item.GetEffectiveSourceType ();

		return type.GetValue ();
	}
	else
	{
		Assert (col == colState);
		TargetSequencer seq = _targetFiles.find (gid);
		if (seq != _targetFiles.end ())
		{
			TargetData const & target = seq->second;
			return target.GetMergeStatus ().GetValue ();
		}
	}

	return 0;
}

std::string	ScriptDetails::GetStringField (Column col, GlobalId gid) const
{
	if (col == Table::colParentPath)
	{
		std::string path (_pathFinder.GetRootRelativePath (gid));
		return path;
	}
	else
	{
		FileSequencer seq = _fileDiaryMap.find (gid);
		Assert (seq != _fileDiaryMap.end ());
		Workspace::HistoryItem const & item = *seq->second;

		if (col == Table::colName)
		{
			return item.GetLastName ();
		}
		else if (col == Table::colStateName)
		{
			if (item.HasEffectiveTarget ())
			{
				UniqueName const & target = item.GetEffectiveTarget ();
				if (item.HasEffectiveSource ())
				{
					UniqueName const & source = item.GetEffectiveSource ();
					if (source.IsEqual (target))
						return "Edited";
					else if (source.GetParentId () != target.GetParentId ())
						return "Moved";
					else
						return "Renamed";
				}
				else
				{
					return "Created";
				}
			}
			else
			{
				return "Deleted";
			}
		}
	}
	return std::string ();
}

std::string	ScriptDetails::GetCaption (Restriction const & restrict) const 
{
	if (_range.Size () == 1)
	{
		VersionInfo info;
		if (_history.RetrieveVersionInfo (_range.GetYoungestId (), info))
			return info.GetComment ();
	}
	return std::string ();
}

void ScriptDetails::SetThisProjectId (int projectId)
{
	_thisProjectId = projectId;
}

void ScriptDetails::SetTargetProject (int targetProjectId, GlobalId forkId)
{
	Assert (targetProjectId != -1);
	Assert (targetProjectId == _thisProjectId);
	Assert (forkId == gidInvalid);
	_targetProjectId = targetProjectId;
	_forkId = forkId;
	_targetProjectRoot.Change (_pathFinder.GetProjectDir ());
	_firstBranchId = gidInvalid;
}

void ScriptDetails::ClearTargetProject ()
{
	Assert (_thisProjectId != -1);
	_targetProjectId = -1;
}

//
// Merge details table
//

MergeDetails::~MergeDetails ()
{
	if (_thisProjectId != -1)
	{
		try
		{
			SaveTargetHints ();
		}
		catch (...)
		{}
	}
}

void MergeDetails::QueryUniqueIds (Restriction const & restrict, GidList & ids) const
{
	GetFileList (ids);
}

GlobalId MergeDetails::GetIdField (Column col, GlobalId gid) const
{
	Assert (col == colParentId);
	FileSequencer seq = _fileDiaryMap.find (gid);
	Assert (seq != _fileDiaryMap.end ());
	Workspace::HistoryItem const & item = *seq->second;
	if (item.HasEffectiveTarget ())
	{
		UniqueName const & target = item.GetEffectiveTarget ();
		return target.GetParentId ();
	}
	else if (item.HasEffectiveSource ())
	{
		UniqueName const & source = item.GetEffectiveSource ();
		return source.GetParentId ();
	}
	Assert (!item.HasEffectiveTarget () && !item.HasEffectiveSource ());
	// Item is created and deleted - return its last
	// parent id
	FileData const & fd = item.GetOriginalTargetFileData ();
	return fd.GetUniqueName ().GetParentId ();
}

std::string	MergeDetails::GetStringField (Column col, GlobalId gid) const
{
	if (col == Table::colParentPath)
	{
		std::string path (_pathFinder.GetRootRelativePath (gid));
		return path;
	}
	else if (col == Table::colName)
	{
		FileSequencer seq = _fileDiaryMap.find (gid);
		Assert (seq != _fileDiaryMap.end ());
		Workspace::HistoryItem const & item = *seq->second;
		return item.GetLastName ();
	}
	else
	{
		TargetSequencer seq = _targetFiles.find (gid);
		if (seq != _targetFiles.end ())
		{
			TargetData const & target = seq->second;
			if (col == Table::colStateName)
			{
				return target.GetMergeStatus ().GetStatusName ();
			}
			else
			{
				Assert (col == Table::colTargetPath);
				return target.GetTargetPath ();
			}
		}
		else
		{
			if (col == Table::colStateName)
				return "Unknown";

			return "Not available";
		}
	}
	return std::string ();
}

unsigned long MergeDetails::GetNumericField (Column col, GlobalId gid) const
{
	if (col == Table::colType)
	{
		FileSequencer seq = _fileDiaryMap.find (gid);
		Assert (seq != _fileDiaryMap.end ());
		Workspace::HistoryItem const & item = *seq->second;
		if (item.HasEffectiveTarget ())
		{
			FileType type = item.GetEffectiveTargetType ();
			return type.GetValue ();
		}
	}
	else if (col == Table::colState)
	{
		TargetSequencer seq = _targetFiles.find (gid);
		if (seq != _targetFiles.end ())
		{
			TargetData const & target = seq->second;
			return target.GetMergeStatus ().GetValue ();
		}
	}
	return 0;
}

std::string	MergeDetails::GetCaption (Restriction const & restrict) const
{
	std::string version;

	if (_range.Size () != 0)
	{
		VersionInfo info;
		if (_history.RetrieveVersionInfo (_range.GetYoungestId (), info))
			version = info.GetComment ();
	}

	std::string hintList;
	std::string currentTarget;
	if (IsTargetProjectSet ())
	{
		if (IsLocalMerge ())
			currentTarget = CurrentProject;
		else
			currentTarget = _catalog.GetProjectName (GetTargetProjectId ());
	}

	if (currentTarget.empty ())
		hintList = NoTarget;
	else
		hintList = currentTarget;

	GetHintList (hintList, currentTarget);
	if (!hintList.empty ())
		hintList += '\n';
	hintList += '*';		// <Select branch...> item marker

	bool isAncestor = IsAncestorMerge ();
	bool isCumulative = IsCumulativeMerge ();
	std::string mergeType;
	for (unsigned i = 0; MergeTypeEntries [i]._dispName != 0; ++i)
	{
		if (MergeTypeEntries [i]._isAncestor == isAncestor
			&& MergeTypeEntries [i]._isCumulative == isCumulative)
		{
			mergeType = MergeTypeEntries [i]._dispName;
			break;
		}
	}
	return (hintList + '|' + version + '|' + mergeType);
}

void MergeDetails::SetThisProjectId (int projectId)
{
	if (_thisProjectId != -1)
		ClearTargetProject (true);	// Save branch hints

	_thisProjectId = projectId;
	if (_thisProjectId != -1)
	{
		_thisProjectName = _catalog.GetProjectName (_thisProjectId);
		ReadTargetHints ();
	}
}

void MergeDetails::SetTargetProject (int targetProjectId, GlobalId forkId)
{
	Assert (targetProjectId != -1);
	_targetProjectId = targetProjectId;
	_forkId = forkId;
	if (IsLocalMerge ())
	{
		UpdateTargetHints (CurrentProject);
		_targetProjectRoot.Change (_pathFinder.GetProjectDir ());
		_firstBranchId = gidInvalid;
	}
	else
	{
		Assert (_forkId != gidInvalid);
		UpdateTargetHints (_catalog.GetProjectName (_targetProjectId));
		_targetProjectRoot.Change (_catalog.GetProjectSourcePath (targetProjectId));
		_firstBranchId = _history.GetFirstExecutedSuccessorId (_forkId);
	}
}

void MergeDetails::ClearTargetProject (bool saveHints)
{
	Assert (_thisProjectId != -1);
	if (saveHints)
		SaveTargetHints ();
	_targetProjectId = -1;
}

void MergeDetails::ReadTargetHints ()
{
	Assume (_thisProjectId != -1, "MergeDetails::ReadTargetHints");
	_branchHints.clear ();
	Registry::Branches branchesKey;
	MultiString hints;
	branchesKey.Key ().GetMultiString (_thisProjectName, hints);
	std::copy (hints.begin (), hints.end (), std::back_inserter (_branchHints));
}

void MergeDetails::SaveTargetHints ()
{
	Assume (_thisProjectId != -1, "MergeDetails::SaveTargetHints");
	std::list<std::string>::iterator iter =
		std::find (_branchHints.begin (), _branchHints.end (), CurrentProject);
	if (iter != _branchHints.end ())
		_branchHints.erase (iter);

	if (!_branchHints.empty ())
	{
		MultiString hints;
		std::copy (_branchHints.begin (), _branchHints.end (), std::back_inserter (hints));
		Registry::Branches branchesKey;
		branchesKey.Key ().SetValueMultiString (_thisProjectName, hints, true);	// Quiet
	}
}

// Project name could be "Current Project"
void MergeDetails::UpdateTargetHints (std::string const & projectName, bool removeHint)
{
	Assume (_thisProjectId != -1, "MergeDetails::UpdateTargetHints");
	std::list<std::string>::iterator iter =
		std::find (_branchHints.begin (), _branchHints.end (), projectName);
	if (iter != _branchHints.end ())
		_branchHints.erase (iter);

	if (removeHint)
		return;

	_branchHints.insert (_branchHints.begin (), projectName);
}

void MergeDetails::GetHintList (std::string & hints, std::string const & currentTarget) const
{
	for (std::list<std::string>::const_iterator iter = _branchHints.begin ();
		iter != _branchHints.end ();
		++iter)
	{
		std::string const & currentHint = *iter;
		if (currentHint != currentTarget)
		{
			if (!hints.empty ())
				hints += '\n';
			hints += *iter;
		}
	}
}
