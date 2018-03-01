#if !defined (SELECTIONMAN_H)
#define SELECTIONMAN_H
//----------------------------------
// (c) Reliable Software 1998 - 2006
//----------------------------------

#include "SelectIter.h"
#include "GlobalId.h"
#include "Predicate.h"
#include "GidPredicate.h"

#include <iosfwd>

namespace History
{
	class Range;
}
class RecordSet;
class PathSequencer;

typedef std::set<std::string>::const_iterator string_iter;

class SelectionManager
{
public:
	virtual ~SelectionManager () {}
	virtual void SetSelection (PathSequencer & sequencer) = 0;
	virtual void SetSelection (GidList const & ids) = 0;
	virtual void SelectAll () = 0;
	virtual void SelectIds (GidList const & ids, Table::Id tableId) {}
	virtual void SelectItems (std::vector<unsigned> const & items, Table::Id tableId) {}
	virtual void SetRange (History::Range const & range) {}
	virtual RecordSet const * GetRecordSet (Table::Id tableId) const = 0;
	virtual RecordSet const * GetCurRecordSet () const { return 0; }
	virtual RecordSet const * GetMainRecordSet () const { return 0; }
	virtual std::string GetInputText () const { return std::string (); }
	virtual void DumpRecordSet (std::ostream & out, Table::Id tableId, bool allRows) = 0;
	virtual void VerifyRecordSet () const = 0;
	virtual void GetRows (std::vector<unsigned> & rows, Table::Id tableId) const = 0;
	virtual void GetSelectedRows (std::vector<unsigned> & rows) const = 0;
	virtual void GetSelectedRows (std::vector<unsigned> & rows, Table::Id tableId) const = 0;
	virtual void BeginDrag (Win::Dow::Handle win, unsigned id) {}
	virtual void EndDrag () {}
	virtual void Clear () = 0;
	virtual unsigned int SelCount () const = 0;
	virtual bool IsDefaultSelection () const = 0;

	// Find methods

	virtual bool FindIfSome (std::function<bool(long, long)> predicate) const { return false; }
	virtual bool FindIfSelected (std::function<bool(long, long)> predicate) const { return false; }
	virtual bool FindIfSelected (Table::Id tableId, std::function<bool(long, long)> predicate) const { return false; }
	virtual bool FindIfSelected (GidPredicate const & predicate) const { return false; }
};

#endif
