#if !defined (CONFLICT_H)
#define CONFLICT_H
//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "GlobalId.h"
#include "ProjectPath.h"
#include "FileTypes.h"
#include "HistoryRange.h"

class ScriptHeader;
class DataBase;
namespace History
{
	class Db;
}
class Model;
namespace Progress
{
	class Meter;
}
namespace Workspace
{
	class HistorySelection;
}

class ConflictDetector
{
public:
	ConflictDetector (DataBase const & dataBase, History::Db const & history, ScriptHeader const & incomingHdr);

	bool IsConflict () const { return _conflictRange.Size () != 0; }
	bool FilesLeftCheckedOut () const { return !_toBeLeftCheckedOut.empty (); }
	bool AreMyScriptsRejected () const { return _myScriptsAreRejected; }
	bool AreMyLocalEditsInMergeConflict () const { return _myLocalEditsInMergeConflict; }
	Workspace::HistorySelection & GetSelection () const { return *_selection; }
	std::string const & GetConflictVersion () const { return _conflictVersion; }
	bool IsAutoMerge () const { return _isAutoMerge; }
	void SetAutoMerge (bool flag) { _isAutoMerge = flag; }

	class ScriptIterator
	{
	public:
		ScriptIterator (ConflictDetector const & detector)
			: _cur (detector._conflictRange.begin ()),
			  _end (detector._conflictRange.end ()),
			  _history (detector._history)
		{}

		bool AtEnd () const { return _cur == _end; }
		void Advance () { ++_cur; }

		GlobalId GetScriptId () const { return *_cur; }

	private:
		GidList::const_iterator	_cur;
		GidList::const_iterator	_end;
		History::Db const &		_history;
	};

	class FileIterator
	{
	public:
		FileIterator (ConflictDetector const & detector);

		bool AtEnd () const { return _cur == _end; }
		void Advance () { ++_cur; }

		GlobalId GetGlobalId () const { return *_cur; }
		bool IsToBeLeftCheckedOut () const;

	private:
		GidList						_files;
		GidList::const_iterator		_cur;
		GidList::const_iterator		_end;
		GidSet const &				_toBeLeftCheckedOut;
	};

	friend class ScriptIterator;
	friend class FileIterator;

private:
	bool										_myScriptsAreRejected;
	bool										_myLocalEditsInMergeConflict;
	bool										_isAutoMerge;
	std::string									_conflictVersion;
	DataBase const &							_dataBase;
	History::Db const &							_history;
	History::Range								_conflictRange;
	std::unique_ptr<Workspace::HistorySelection>	_selection;
	GidSet										_toBeLeftCheckedOut;
};

#endif
