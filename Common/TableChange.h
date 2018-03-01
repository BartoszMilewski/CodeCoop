#if !defined (TABLECHANGE_H)
#define TABLECHANGE_H
//----------------------------------
//  (c) Reliable Software, 2005
//----------------------------------
#include "GlobalId.h"
#include "Bookmark.h"
#include <iosfwd>

enum ChangeKind
{
	changeEdit,
	changeAdd,
	changeRemove,
	changeAll
};

#if !defined (NDEBUG)
std::ostream & operator<<(std::ostream & os, ChangeKind kind);
#endif

class TableChange : public Bookmark
{
	friend class RowChange;

public:
	TableChange (ChangeKind kind, GlobalId gid)
		: _kind (kind),
		  Bookmark (gid)
	{}
	TableChange (ChangeKind kind, char const * name)
		: _kind (kind),
		  Bookmark (name)
	{}
	TableChange (ChangeKind kind, GlobalId gid, char const * name)
		: _kind (kind),
		  Bookmark (gid, name)
	{}

	bool IsEdit () const { return _kind == changeEdit; }
	bool IsAdd () const { return _kind == changeAdd; }
	bool IsDelete () const { return _kind == changeRemove; }

private:
	ChangeKind	_kind;
};

class RowChange
{
public:
	RowChange (TableChange change, unsigned int row)
		: _kind (change._kind),
		  _row (row)
	{}

	bool IsEdit () const { return _kind == changeEdit; }
	bool IsAdd () const { return _kind == changeAdd; }
	bool IsDelete () const { return _kind == changeRemove; }
	bool IsAll () const { return _kind == changeAll; }

	unsigned int GetRow () const { return _row; }

private:
	ChangeKind		_kind;
	unsigned int	_row;
};

#endif
