#if !defined (REMOTEHUB_H)
#define REMOTEHUB_H
// ---------------------------
// (c) Reliable Software, 2002
// ---------------------------

#include "Table.h"
#include "Transport.h"
#include <StringOp.h>

class Catalog;

class RemoteHubList : public Table
{
public:
	typedef NocaseMap<Transport>::const_iterator Iterator;
public:
	RemoteHubList (Catalog & catalog);
	bool IsKnown (std::string const & hubId) const;
	void GetAskTransport (std::string const & hubId, Transport & transport);
	void Add (std::string const & hubId, Transport const & transport);
	bool VerifyAdd (std::string const & hubId);
	void AddReplaceHubId (std::string const & oldHubId,
						  std::string const & newHubId,
						  Transport const & transport);
	Transport const & GetInterClusterTransport (std::string const & hubId) const;
	void UpdateInterClusterTransport (std::string const & hubId, Transport const & newTransport);
	void Delete (std::string const & hubId);

	// Table interface
	void QueryUniqueNames (std::vector<std::string> & unames, Restriction const * restrict = 0);
    int	GetNumericField (Column col, std::string const & uname) const;
    std::string	GetStringField (Column col, std::string const & uname) const;
	Iterator begin () const { return _hubMap.begin (); }
	Iterator end () const { return _hubMap.end (); }
private:
	Catalog & _catalog;
	NocaseMap<Transport>	_hubMap;
};

#endif
