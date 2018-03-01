#if !defined (SELECTITER_H)
#define SELECTITER_H
// ---------------------------------
// (c) Reliable Software, 1999, 2000
// ---------------------------------

class SelectionMan;
class RecordSet;
class Restriction;
class TripleKey;

class SelectionSeq
{
public:
	SelectionSeq (SelectionMan const * selMan, char const * tableName);
	void Advance ();
	bool AtEnd () const { return _rows.size () == _cur; }
	
	Restriction const & GetRestriction () const { return _restriction; }
	int GetId () const;
	std::string const & GetName () const;
	TripleKey const & GetKey () const;

private:
	RecordSet const *	_recordSet;
	Restriction const & _restriction;
    std::vector<int>	_rows;    
    unsigned int		_cur;
};

#endif
