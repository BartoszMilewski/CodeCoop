//---------------------------
// (c) Reliable Software 2000-2003
//---------------------------
#include "precompiled.h"
#include "ScriptFileList.h"

void ScriptFileList::ChangePath (FilePath const & path)
{
	Win::Lock lock (_critSect);
	_scriptFiles.clear ();
	_path = path;
}

void ScriptFileList::clear () 
{
	Win::Lock lock (_critSect);
	_scriptFiles.clear ();
}

int ScriptFileList::size () const 
{
	Win::Lock lock (_critSect);
	return _scriptFiles.size (); 
}

bool ScriptFileList::IsSameMailbox (char const * path) const
{
	return strcmp (_path.GetDir (), path) == 0;
}

void ScriptFileList::GetSetDifference(std::set<std::string> const & inProgressSet, std::vector<std::string> & newFiles)
{
	Win::Lock lock (_critSect);
	for (iterator it = begin(); it != end(); ++it)
	{
		std::string const & name = it->first;
		if (inProgressSet.find(name) == inProgressSet.end())
			newFiles.push_back(name);
		else
			it->second.SetInProgress();
	}
}

// Returns true, if element is added
bool ScriptFileList::Insert (std::string const & name)
{
	Win::Lock lock (_critSect);
	iterator result = _scriptFiles.find (name);
	if (result == _scriptFiles.end ())
	{
		ScriptStatus status (ScriptStatus::Direction::Out);
		status.SetPresent ();
		_scriptFiles.insert (std::make_pair (name, status));
		return true;
	}
	else
	{
		result->second.SetPresent ();
		return false;
	}
}

ScriptStatus ScriptFileList::GetStatus (std::string const & name) const
{
	Win::Lock lock (_critSect);
	const_iterator result = _scriptFiles.find (name);
	if (result == _scriptFiles.end ())
	{
		// if script not found return ScriptStatus
		// with default values: Direction::Unknown, Read::Absent, Dispatch::ToBeDone
		return ScriptStatus ();
	}
	else
		return result->second;
}

void ScriptFileList::SetStatus (std::string const & name, ScriptStatus status)
{
	Win::Lock lock (_critSect);
	iterator result = _scriptFiles.find (name);
	if (result != _scriptFiles.end ())
	{
		result->second.Set(status);
	}
}

void ScriptFileList::SetUiStatus (std::string const & name, 
								  ScriptStatus::Dispatch::Bits status)
{
	Win::Lock lock (_critSect);
	iterator result = _scriptFiles.find (name);
	if (result != _scriptFiles.end ())
	{
		// set flag _statusChanged, reset it when UI updated
		if (result->second.SetDispatchStatus (status))
			_uiStatusChanged = true;
	}
}

bool ScriptFileList::AcceptUiStatusChange()
{
	Win::Lock lock (_critSect);
	bool isChanged = _uiStatusChanged;
	_uiStatusChanged = false;
	return isChanged;
}

// clear read status
void ScriptFileList::PrepareForRefresh ()
{
	Win::Lock lock (_critSect);
	for (iterator it = _scriptFiles.begin (); it != _scriptFiles.end (); )
	{
		it->second.ClearReadStatus ();
		it->second.ClearInProgress ();
		++it;
	}
}

void ScriptFileList::PruneDeletedScripts (NocaseSet & deletedScripts)
{
	Win::Lock lock (_critSect);
	for (iterator it = _scriptFiles.begin (); it != _scriptFiles.end (); /* 'it' is forwarded manually */)
	{
		if (it->second.GetReadStatus () == ScriptStatus::Read::Absent)
		{
			deletedScripts.insert (it->first);
			it = _scriptFiles.erase (it);
		}
		else
		{
			++it;
		}
	}
}
