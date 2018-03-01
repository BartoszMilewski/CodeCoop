#if !defined (RECORDSET_H)
#define RECORDSET_H
// ----------------------------------
// (c) Reliable Software, 2001 - 2002
// ----------------------------------

#include "Table.h"

class Bookmark
{
public:
	Bookmark () : _uid (-1) {}

	int GetId () const { return _uid; }
	void SetId (int id) { _uid = id; }

	std::string const & GetName () const { return _uname; }
	void SetName (std::string const & name) { _uname = name; }

	TripleKey const & GetKey () const { return _ukey; }
	void SetKey (TripleKey const & key) { _ukey = key; }

private:
	// only one of these is valid in the same time
	int			_uid;
	std::string _uname;
	TripleKey	_ukey;
};

class RecordSet
{
public:
	virtual ~RecordSet () {}

    virtual unsigned int GetColCount () const = 0;
    virtual unsigned int GetRowCount () const = 0;
    virtual char const * GetColumnHeading  (unsigned int col) const = 0;
    virtual std::string GetFieldString (unsigned int row, unsigned int col) const = 0;

	virtual void GetBookmark (unsigned int row, Bookmark & bookmark) const = 0;
	virtual int GetRow (Bookmark const & bookmark) const = 0;
	virtual int GetId (unsigned int row) const;
	virtual std::string const & GetName (unsigned int row) const;
	virtual TripleKey const & GetKey (unsigned int row) const;
	virtual int CmpRows (unsigned int row1, unsigned int row2, int col) const = 0;

    virtual void GetImage (unsigned int item, int & imageId, int & overlay) const = 0;
    virtual void GetItemIcons (std::vector<int> & iconResIds) const = 0;

protected:
	int CompareInts (int i1, int i2) const { return i1 - i2; }

	std::vector<int>			_ids;
	std::vector<std::string>	_names;

	static std::string			_emptyStr;
	static TripleKey			_emptyKey;
};

#endif
