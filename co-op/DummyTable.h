#if !defined (DUMMYTABLE_H)
#define DUMMYTABLE_H
//------------------------------------
//  (c) Reliable Software, 2000 - 2005
//------------------------------------

#include "Table.h"
#include "GlobalId.h"
#include "Global.h"

class DummyTable : public Table
{
public:
	DummyTable ()
	{}

	// Table interface
	Table::Id GetId () const { return Table::emptyTableId; }
	bool IsValid () const { return true; }
    std::string	GetStringField (Column col, GlobalId gid) const { return _info; }
    std::string	GetStringField (Column col, UniqueName const & uname) const { return _info; }
    GlobalId GetIdField (Column col, UniqueName const & uname) const { return gidInvalid; }
    GlobalId GetIdField (Column col, GlobalId gid) const { return gidInvalid; }
	unsigned long GetNumericField (Column col, GlobalId gid) const 
	{ Assert (!"GetNumericField called on DummyTable"); return 0; }

	void Init (std::string const & info) { _info = info; }

private:
	std::string	_info;
};

#endif
