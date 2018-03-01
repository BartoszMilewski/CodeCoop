#if !defined (SELECTIONMAN_H)
#define SELECTIONMAN_H
// ---------------------------
// (c) Reliable Software, 2001
// ---------------------------

#include <vector>

class RecordSet;
class Restriction;

class SelectionMan
{
public:
    virtual bool HasSelection () const = 0;
    virtual void GetSelectedRows (std::vector<int> & rows) const = 0;
	virtual RecordSet const * GetRecordSet (char const * tableName) const = 0;
	virtual Restriction const & GetRestriction () const = 0;
};


#endif
