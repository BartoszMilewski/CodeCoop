#if !defined (SCRIPTFILELIST_H)
#define SCRIPTFILELIST_H
//---------------------------
// (c) Reliable Software 2000-2003
//---------------------------
#include "ScriptStatus.h"
#include <Sys/Synchro.h>
#include <File/Path.h>
#include <StringOp.h>

// maps script file name (not path) to its status
// concurrently accessed by the producer and the consumer

class ScriptFileList
{
private:
    typedef NocaseMap<ScriptStatus>::iterator iterator;
    typedef NocaseMap<ScriptStatus>::const_iterator const_iterator;
public:
	// locks the list for the duration of sequencing
	class ConstSequencer
	{
	public:
		ConstSequencer (ScriptFileList const & scriptList)
			: _lock (scriptList._critSect),
			  _it (scriptList.begin ()),
			  _end (scriptList.end ())
		{
		}
		bool AtEnd () const { return _it == _end; }
		void Advance ()
		{
			Assert (!AtEnd ());
			++_it; 
		}
		std::string const & GetName () const { return _it->first; }
		ScriptStatus const & GetStatus () const { return _it->second; }
	private:
		Win::Lock		_lock;
		const_iterator	_it;
		const_iterator	_end;
	};

public:
	friend class ScriptFileList::ConstSequencer;

	ScriptFileList (FilePath const & path)
		: _path (path), _uiStatusChanged(false)
	{}
	void ChangePath (FilePath const & path);
	bool IsSameMailbox (char const * path) const;
	void GetSetDifference(std::set<std::string> const & inProgressList, std::vector<std::string> & toDoList);
	void clear ();
	int  size () const;
	bool Insert (std::string const & name);

	// Status is manipulated through ScriptTicket
	ScriptStatus GetStatus (std::string const & name) const;
	void SetStatus(std::string const & name, ScriptStatus status);
	void SetUiStatus (std::string const & name, ScriptStatus::Dispatch::Bits status);
	bool AcceptUiStatusChange();

	void PrepareForRefresh ();
	void PruneDeletedScripts (NocaseSet & deletedScripts);
private:
    iterator		begin () { return _scriptFiles.begin (); }
    iterator		end	  () { return _scriptFiles.end (); }
    const_iterator	begin () const { return _scriptFiles.begin (); }
    const_iterator	end   () const { return _scriptFiles.end (); }
private:
	mutable Win::CritSection	_critSect;
	bool						_uiStatusChanged;
    NocaseMap<ScriptStatus>		_scriptFiles;
	FilePath					_path;
};

#endif
