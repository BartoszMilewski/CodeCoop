#if !defined (RECORDSET_H)
#define RECORDSET_H
//----------------------------------
// (c) Reliable Software 1998 - 2008
//----------------------------------

#include "Table.h"
#include "Observer.h"

#include <iosfwd>

class SelectState;
class TablePreferences;
namespace Column
{
	struct Info;
}

enum DrawStyle { DrawNormal, DrawGreyed, DrawBold, DrawHilite };

//
// Record Set
//

class RecordSet : public TransactionObserver, public Notifier
{
public:
    RecordSet (Table const & table)
        : _table (table),
		  _startRowCount (0)
    {
		_table.Attach (this);
	}
    virtual ~RecordSet ()
	{
		// idempotent
		_table.Detach (this);
	}

	std::string const & GetRootName () const { return _rootName; }
    GlobalId GetGlobalId (unsigned int row) const;
    char const * GetName (unsigned int row) const;
	Table::Id GetTableId () const { return _table.GetId (); }
	bool IsDummy () const { return GetTableId () == Table::emptyTableId; }
	virtual unsigned long GetState (unsigned int row) const { return 0; }
	virtual unsigned long GetType (unsigned int row) const { return 0; }

	// Column information
    unsigned int  ColCount () const { return _columns.size (); }

	virtual void CopyStringField (unsigned int row, unsigned int col, unsigned int bufLen, char * buf) const { buf [0] = '\0'; }
	virtual std::string GetStringField (unsigned int row, unsigned int col) const { return std::string (); }
    virtual void GetImage (unsigned int row, SelectState state, int & iImage, int & iOverlay) const;
	virtual bool SupportsDrawStyles () const { return false; }
	virtual DrawStyle GetStyle (unsigned int row) const { return DrawNormal; }
    virtual void Refresh (unsigned int row) {}

    unsigned int RowCount () const;
	Bookmark GetBookmark (unsigned int row) const;
	unsigned int GetRow (Bookmark const & selection) const;
	virtual bool IsEmpty () const;
	virtual DegreeOfInterest HowInteresting () const;
	bool IsValid () const;

	void DumpColHeaders (std::ostream  & out, char fieldSeparator = '\t') const;
	void DumpRow (std::ostream  & out, unsigned int row, char fieldSeparator = '\t') const;
	virtual void DumpField (std::ostream  & out, unsigned int row, unsigned int col) const {}

	virtual bool IsEqual (RecordSet const * recordSet) const { return false; }
    virtual bool IsDefaultSelection () const { return false; }
	virtual unsigned GetDefaultSelectionRow () const { return 0; }
    virtual int CmpRows (unsigned int row1, unsigned int row2, unsigned int col) const { return 0; }
    virtual GlobalId GetParentId (unsigned int row) const { return gidInvalid; }
    virtual void BeginNewItemEdit () {}
    virtual void AbortNewItemEdit () {}
	virtual void Verify () const {}

	//
	// TransactionObserver interface
	//

	RowChange TranslateNotification (TableChange const & change);
	void StartUpdate ();
	void AbortUpdate ();
	void CommitUpdate (bool delayRefresh);
	void ExternalUpdate (char const * topic);

protected:
	virtual int CmpNames (unsigned int row1, unsigned int row2) const;
    int CmpStates (unsigned int row1, unsigned int row2) const;
    int CmpGids (unsigned int row1, unsigned int row2) const;
	virtual void PushRow (GlobalId gid, std::string const & name);
	virtual void PopRows (int oldSize);
	void InitColumnHeaders (Column::Info const * columnInfo);

protected:
    Table const &   _table;
	std::string		_rootName;
	unsigned int	_startRowCount;

    GidList						_ids;
    std::vector<std::string>	_names;
    std::vector<std::string>	_states;
	std::vector<std::string>	_columns;
};

#endif
