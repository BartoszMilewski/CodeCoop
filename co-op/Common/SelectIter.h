#if !defined (SELECTITER_H)
#define SELECTITER_H
//----------------------------------
// (c) Reliable Software 1998 - 2006
//----------------------------------

#include "GlobalId.h"
#include "FileTypes.h"
#include "Table.h"

class SelectionManager;
class RecordSet;
class UniqueName;

class SelectState
{
public:
	// fast default constructor
	SelectState ()
		: _value (0)
	{}
	explicit SelectState (bool isSelected)
		: _value (0)
	{
		if (isSelected)
			Select ();
	}
	SelectState (SelectState const & state) { _value = state._value; }
	void Select () { _bits._select = 1; }
	void DeSelect () { _bits._select = 0; }
	void SetRange (bool yes = true) { _bits._range = (yes? 1: 0); }

	bool IsSelected () const { return _bits._select != 0; }
	bool IsRange () const    { return _bits._range != 0; }
private:
	union
	{
		unsigned char _value;
		struct
		{
			unsigned char _select:1;
			unsigned char _range:1;
		} _bits;
	};
};

class WindowSeq
{
public:
	bool IsEmpty () const { return _rows.empty (); }
	bool AtEnd () const { return _cur == _rows.size (); }
	void Advance () { _cur++; }
	void Reset() { _cur = 0; }

	bool IsIncluded (GlobalId gid) const;

    // Access to data
    unsigned  GetRow () const { return _rows [_cur]; }
    void GetUniqueName (UniqueName & uname) const;
    GlobalId GetGlobalId () const;
	FileType GetType () const;
	unsigned long GetState () const;
    char const * GetName () const;
    int Count () const { return _rows.size (); }
	RecordSet const & GetRecordSet () const { return _recordSet; }
	virtual void ExpandRange () { /* Window sequencer doesn't know how to expand range */ }

protected:
    WindowSeq (SelectionManager * selectMan, Table::Id tableId);
    explicit WindowSeq (SelectionManager * selectMan);
protected:
    unsigned					_cur;
    std::vector<unsigned>		_rows;
    RecordSet const &			_recordSet;
};

class AllSeq : public WindowSeq
{
public:
    AllSeq (SelectionManager * selectMan, Table::Id tableId);
};

class SelectionSeq : public WindowSeq
{
public:
	explicit SelectionSeq(SelectionManager * selectMan);
    SelectionSeq (SelectionManager * selectMan, Table::Id tableId);
    SelectionSeq (SelectionManager * selectMan, Table::Id tableId, std::vector<std::string> const & names);

	void ExpandRange ();
};


#endif
