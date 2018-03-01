#if !defined (HISTORICALFILES_H)
#define HISTORICALFILES_H
//------------------------------------
//  (c) Reliable Software, 2006 - 2007
//------------------------------------

#include "Table.h"
#include "GlobalId.h"
#include "Restorer.h"
#include "HistoryRange.h"
#include "HistoryPath.h"
#include "TmpProjectArea.h"
#include "UniqueName.h"
#include "FileTypes.h"
#include "MergeStatus.h"
#include "MergeExec.h"

namespace Workspace
{
	class HistorySelection;
	class HistoryItem;
}
namespace History
{
	class Db;
}
class FeedbackManager;
class FileFilter;
class PathFinder;
class FileData;
class AttributeMerge;
namespace Progress
{
	class Meter;
}
class Catalog;
class FileIndex;
class Directory;
class SccProxyEx;

class TargetStatus
{
public:
	TargetStatus () {}
	TargetStatus (unsigned long long value)
		: _bits (value)
	{}

	void SetControlled (bool bit)		{ _bits.set (Controlled, bit); }
	void SetControlledByPath (bool bit)	{ _bits.set (ControlledByPath, bit); }
	void SetIsPresentByPath (bool bit)	{ _bits.set (PresentByPath, bit); }
	void SetDifferentGid (bool bit)		{ _bits.set (DifferentGid, bit); }
	void SetDeleted (bool bit)			{ _bits.set (Deleted, bit); }
	void SetDeletedByPath (bool bit)	{ _bits.set (DeletedByPath, bit); }
	void SetCheckedOut (bool bit)		{ _bits.set (CheckedOut, bit); }
	void SetCheckedOutByPath (bool bit)	{ _bits.set (CheckedOutByPath, bit); }
	void SetParentControlled (bool bit)	{ _bits.set (ParentControlledByPath, bit); }
	void SetUnknown (bool bit)			{ _bits.set (Unknown, bit); }

	unsigned long GetValue () const { return _bits.to_ulong (); }

	// By GID
	bool IsControlled () const			{ return _bits.test (Controlled); }
	bool IsDeleted () const				{ return _bits.test (Deleted); }
	bool IsCheckedOut () const			{ return _bits.test (CheckedOut); }
	// By Path
	bool IsPresentByPath () const		{ return _bits.test (PresentByPath); }
	bool IsControlledByPath () const	{ return _bits.test (ControlledByPath); }
	bool IsDeletedByPath () const		{ return _bits.test (DeletedByPath); }
	bool HasDifferentGid () const		{ return _bits.test (DifferentGid); }
	bool IsCheckedOutByPath () const	{ return _bits.test (CheckedOutByPath); }
	bool IsParentControlled () const	{ return _bits.test (ParentControlledByPath); }
	bool IsUnknown () const				{ return _bits.test (Unknown); }

private:
	enum
	{
		// By GID
		Controlled,			// We have database record for the file/folder
		Deleted,			// Already deleted or to be deleted
		CheckedOut,
		// By Path
		PresentByPath,
		ControlledByPath,
		DeletedByPath,
		DifferentGid,
		CheckedOutByPath,
		ParentControlledByPath,
		Unknown
	};

private:
	std::bitset<std::numeric_limits<unsigned long>::digits>	_bits;
};

class TargetData
{
public:
	TargetData () {}
	TargetData (std::string const & targetPath,
				FileType targetType,
				TargetStatus targetStatus)
		: _targetPath (targetPath),
		  _targetType (targetType),
		  _targetStatus (targetStatus)
	{}

	void SetMergeStatus (MergeStatus status) { _mergeStatus = status; }
	void SetTargetPath (std::string const & path) { _targetPath = path; }
	void SetTargetType (FileType type) { _targetType = type; }

	std::string const & GetTargetPath () const { return _targetPath; }
	FileType GetTargetType () const { return _targetType; }
	TargetStatus GetTargetStatus () const { return _targetStatus; }
	MergeStatus GetMergeStatus () const { return _mergeStatus; }

private:
	std::string		_targetPath;
	FileType		_targetType;
	TargetStatus	_targetStatus;
	MergeStatus		_mergeStatus;
};

class HistoricalFiles
{
public:
	HistoricalFiles (History::Db const & history,
					 PathFinder & pathFinder,
					 Catalog & catalog);
	~HistoricalFiles ();
	void Clear (bool force = false);
	// Returns true when scripts re-loaded
	bool ReloadScripts (History::Range const & range,
						GidSet const & fileFilter,
						bool extendedRange,
						Progress::Meter & meter);
	bool IsExtendedRange () const { return _extendedRange; }
	bool RangeHasMissingPredecessors () const { return _rangeHasMissingPredecessors; }
	bool RangeHasRejectedScripts () const { return _rangeHasRejectedScripts; }
	bool RangeIsAllRejected () const { return _rangeIsAllRejected; }
	bool RangeIncludesMostRecentVersion () const;
	bool RangeFollowsTrunkBranchPoint () const;
	bool IsEmpty () const;

	unsigned RangeSize () const { return _range.Size (); }
	GlobalId GetFirstFileId () const;

	Restorer & GetRestorer (GlobalId gid);
	std::unique_ptr<Workspace::HistorySelection> GetRevertSelection (Progress::Meter & meter) const;
	std::unique_ptr<Workspace::HistorySelection> GetRevertSelection (GidList const & files,
																   Progress::Meter & meter) const;
	void QueryTargetData (Progress::Meter & progressMeter,
						  FileIndex const & fileIndex,
						  Directory const & directory);
	bool QueryTargetData (GlobalId gid,
						  FileIndex const & fileIndex,
						  Directory const & directory);
	TargetStatus GetTargetPath (FileIndex const & fileIndex,
								Directory const & directory,
								GlobalId gid,
								std::string const & sourcePath,
								std::string & targetPath,
								FileType & targetType);
	bool HasTargetData (GlobalId gid) const;
	TargetData & GetTargetData (GlobalId gid);
	MergeStatus GetMergeStatus (GlobalId gid);
	int GetThisProjectId () const { return _thisProjectId; }
	int GetTargetProjectId () const { return _targetProjectId; }
	char const * GetTargetFullPath (std::string const & targetRelativePath);
	GlobalId GetFirstBranchId () const { return _firstBranchId; }
	GlobalId GetForkId () const { return _forkId; }
	Workspace::HistoryItem const & GetFileItem (GlobalId gid) const;
	void GetFileList (GidList & files) const;
	void ClearTargetData () { _targetFiles.clear (); }

	void SetTargetData (GlobalId gid,
						std::string const & targetPath,
						FileType targeType,
						TargetStatus targetStatus);
	void SetMergeStatus (GlobalId gid, MergeStatus status);

	bool IsTargetProjectSet () const { return _targetProjectId != -1; }
	bool IsLocalMerge () const { return IsTargetProjectSet () && _targetProjectId == _thisProjectId; }
	bool IsAttributeMergeNeeded (GlobalId gid);
	bool IsAncestorMerge () const { return _isAncestorMerge; }
	bool IsCumulativeMerge () const { return _isCumulativeMerge; }
	bool IsSelectiveMerge () const { return !_isAncestorMerge && !_isCumulativeMerge; }

	std::unique_ptr<MergeExec> GetMergeExec (GlobalId gid, bool autoMerge = false);

private:
	void QueryLocalTargetData (Progress::Meter & progressMeter,
							   FileIndex const & fileIndex,
							   Directory const & directory);
	void QueryRemoteTargetData (Progress::Meter & progressMeter);
	void GetLocalTargetData (GlobalId gid,
							 FileIndex const & fileIndex,
							 Directory const & directory);
	bool GetRemoteTargetData (GlobalId gid, SccProxyEx & proxy);
	void UpdateMergeStatus (GlobalId gid);

protected:
	typedef std::map<GlobalId, Workspace::HistoryItem const *>::const_iterator FileSequencer;
	typedef std::map<GlobalId, TargetData>::const_iterator TargetSequencer;

protected:
	History::Db const &				_history;
	PathFinder &					_pathFinder;
	Catalog &						_catalog;
	History::Range					_range;
	GidSet							_fileFilter;
	bool							_extendedRange;
	bool							_rangeHasMissingPredecessors;
	bool							_rangeHasRejectedScripts;
	bool							_rangeIsAllRejected;
	bool							_isAncestorMerge;
	bool							_isCumulativeMerge;
	std::unique_ptr<Workspace::HistorySelection>			_fileDiaryList;
	std::map<GlobalId, Workspace::HistoryItem const *>	_fileDiaryMap;	// For quick lookup in display selection
	auto_vector<Restorer>			_restorers;
	std::map<GlobalId, unsigned>	_restorerMap;
	int								_targetProjectId;
	FilePath						_targetProjectRoot;
	int								_thisProjectId;
	GlobalId						_forkId;
	GlobalId						_firstBranchId;
	std::map<GlobalId, TargetData>	_targetFiles;
};

class ScriptDetails : public HistoricalFiles, public Table
{
public:
	ScriptDetails (History::Db const & history, PathFinder & pathFinder, Catalog & catalog)
		: HistoricalFiles (history, pathFinder, catalog)
	{
		_isAncestorMerge = true;
	}

	// Table interface
	void QueryUniqueIds (Restriction const & restrict, GidList & ids) const;
	bool IsValid () const { return true; }
	Table::Id GetId () const { return Table::scriptDetailsTableId; }
	std::string	GetStringField (Column col, GlobalId gid) const;
	std::string	GetStringField (Column col, UniqueName const & uname) const { return std::string (); }
	GlobalId GetIdField (Column col, UniqueName const & uname) const { return gidInvalid; }
	GlobalId GetIdField (Column col, GlobalId gid) const;
	unsigned long GetNumericField (Column col, GlobalId gid) const;
	std::string	GetCaption (Restriction const & restrict) const; 

	void SetThisProjectId (int projectId);
	void SetTargetProject (int targetProjectId, GlobalId forkId);
	void ClearTargetProject ();
};

class MergeDetails : public HistoricalFiles, public Table
{
public:
	MergeDetails (History::Db const & history, PathFinder & pathFinder, Catalog & catalog)
		: HistoricalFiles (history, pathFinder, catalog)
	{}
	~MergeDetails ();

	// Table interface
	void QueryUniqueIds (Restriction const & restrict, GidList & ids) const;
	bool IsValid () const { return true; }
	Table::Id GetId () const { return Table::mergeDetailsTableId; }
	std::string	GetStringField (Column col, GlobalId gid) const;
	std::string	GetStringField (Column col, UniqueName const & uname) const { return std::string (); }
	GlobalId GetIdField (Column col, UniqueName const & uname) const { return gidInvalid; }
	GlobalId GetIdField (Column col, GlobalId gid) const;
	unsigned long GetNumericField (Column col, GlobalId gid) const;
	std::string	GetCaption (Restriction const & restrict) const; 

	void SetAncestorMerge (bool flag) { _isAncestorMerge = flag; }
	void SetCumulativeMerge (bool flag) { _isCumulativeMerge = flag; }

	void SetThisProjectId (int projectId);
	void SetTargetProject (int targetProjectId, GlobalId forkId);
	void ClearTargetProject (bool saveHints = false);
	void UpdateTargetHints (std::string const & projectName, bool removeHint = false);

private:
	void ReadTargetHints ();
	void SaveTargetHints ();
	void GetHintList (std::string & list, std::string const & currentTarget) const;

private:
	std::list<std::string>	_branchHints;
	std::string _thisProjectName; // valid only when _thisProjectId != -1
};

#endif
