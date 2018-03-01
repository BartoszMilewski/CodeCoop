// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------
#include "precompiled.h"
#include "ScriptQuarantine.h"
#include "ScriptName.h"
#include "DispatcherCmd.h"

bool ScriptQuarantine::HasInvitation() const
{
	for (NocaseMap<std::string>::const_iterator it = _scripts.begin (); it != _scripts.end (); ++it)
		if (it->second == InvitationCmd::REQUIRES_ACTION)
			return true;

	return false;
}

void ScriptQuarantine::RemoveInvitations()
{
	NocaseMap<std::string>::const_iterator it = _scripts.begin ();
	while (it != _scripts.end ())
	{
		if (it->second == InvitationCmd::REQUIRES_ACTION)
			it = _scripts.erase(it);
		else
			++it;
	}
}

void ScriptQuarantine::Delete (NocaseSet const & toBeDeletedScripts)
{
	if (toBeDeletedScripts.empty ())
		return;

	for (NocaseMap<std::string>::iterator it = _scripts.begin (); it != _scripts.end (); /* 'it' forwarded manually */)
	{
		if (toBeDeletedScripts.find (it->first) == toBeDeletedScripts.end ())
		{
			++it;
		}
		else
		{
			it = _scripts.erase (it);
		}
	}
}

bool ScriptQuarantine::FindModuloChunkNumber (std::string const & script) const
{
	for (NocaseMap<std::string>::const_iterator it = _scripts.begin (); 
		 it != _scripts.end (); 
		 ++it)
	{
		if (ScriptFileName::AreEqualModuloChunkNumber (script, it->first))
			return true;
	}
	return false;
}

void ScriptQuarantine::QueryUniqueNames (
		std::vector<std::string> & unames, 
		Restriction const * restrict)
{
	Assert (restrict == 0);
	unames.reserve (_scripts.size ());
	for (NocaseMap<std::string>::const_iterator it = _scripts.begin ();
		 it != _scripts.end ();
		 ++it)
	{
		unames.push_back (it->first);
	}
}

std::string	ScriptQuarantine::GetStringField (Column col, std::string const & uname) const
{
	Assert (0 == _restriction);
	Assert (Table::colComment == col);
	NocaseMap<std::string>::const_iterator it = _scripts.find (uname);
	Assert (it != _scripts.end ());
	return it->second;
}
