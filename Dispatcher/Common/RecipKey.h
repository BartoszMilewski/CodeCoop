#if !defined (RECIPKEY_H)
#define RECIPKEY_H
// ---------------------------
// (c) Reliable Software, 2001
// ---------------------------
#include "Table.h"

// instantiation of TripleKey for AddressDb table

class RecipientKey : public TripleKey
{
public:
	RecipientKey () {}
	RecipientKey (TripleKey const & k) : TripleKey (k) {}
	RecipientKey (std::string const & hubId, std::string const & userId, bool isLocal)
	{
		_str1 = hubId;
		_str2 = userId;
		_flag = isLocal;
	}
	std::string const & GetHubId () const { return _str1; }
	std::string const & GetUserId () const { return _str2; }
	bool IsLocal () const { return _flag; }
};

#endif
