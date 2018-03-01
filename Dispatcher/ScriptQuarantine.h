#if !defined (SCRIPTQUARANTINE_H)
#define SCRIPTQUARANTINE_H
// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------

#include "Table.h"
#include <StringOp.h>

class ScriptQuarantine : public Table
{
public:
	bool HasInvitation() const;
	void RemoveInvitations();
	bool Find (std::string const & script) const
	{
		return _scripts.find (script) != _scripts.end ();
	}
	bool FindModuloChunkNumber (std::string const & script) const;
	void Insert (std::string const & script, std::string const & failureReason)
	{
		_scripts [script] = failureReason;
	}
	bool IsEmpty () const { return _scripts.empty (); }
	void Clear () { _scripts.clear (); }
	void Delete (std::string const & script)
	{
		_scripts.erase (script);
	}
	void Delete (NocaseSet const & toBeDeletedScripts);

	// Table interface
	void QueryUniqueNames (std::vector<std::string> & unames, Restriction const * restrict = 0);
	std::string	GetStringField (Column col, std::string const & uname) const;

private:
	NocaseMap<std::string> _scripts; // script filename --> reason of failure
};

#endif
