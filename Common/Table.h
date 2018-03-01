#if !defined TABLE_H
#define TABLE_H
//----------------------------------
// (c) Reliable Software 1997 - 2006
//----------------------------------

#include "GlobalId.h"
#include "FileFilter.h"
#include "UniqueName.h"
#include "Bookmark.h"
#include "Observer.h"

#include <NamedBool.h>
#include <StringOp.h>

class RecordSet;
class TransactionObserver;
class FileTime;

const int maxColName = 100;

enum SortType { sortAscend, sortDescend, sortNone };
enum DegreeOfInterest { NotInteresting, Interesting, VeryInteresting };

// Named bools are
// "SpecificFolder"
// "FileFiltering"
// "HideNonProject"
// "FoldersOnly"

class Restriction: public NamedBool
{
 	friend class FileFilterSeq;

public:
    Restriction () 
        : _sortCol (-1), 
          _type (sortNone),
		  _names (0),
		  _ids (0),
		  _folderGid (gidInvalid)
    {}
	Restriction (std::vector<UniqueName> const * names, GidList const * ids)
        : _sortCol (-1), 
          _type (sortNone),
		  _names (names),
		  _ids (ids),
		  _folderGid (gidInvalid)
	{}

	void Clear ()
	{
		_sortCol = -1;
		_type = sortNone;
		ClearFilters ();
		ClearAll ();
	}
	void ClearFilters ()
	{
		_extFilter.clear ();
		_fileFilter.reset ();
	}
	void SetSort (int col, SortType type) 
    { 
        _sortCol = col; 
        _type = type;
    }
    SortType GetSortCol (unsigned int & col) const 
    { 
        col = _sortCol; 
        return _type;
    }
	bool IsFilteredOut (char const * ext) const
	{
		return _extFilter.find (ext) != _extFilter.end ();
	}
	NocaseSet const & GetExtensionFilter () const { return _extFilter; }
	void SetExtensionFilter (NocaseSet const & newFilter) { _extFilter = newFilter; }
	bool HasExtensionFilter () const { return !_extFilter.empty (); }

	// listing particular folder
	void SetFolder (File::Vpath const & folderPath, GlobalId folderGid)
	{
		_folderPath = folderPath;
		_folderGid = folderGid;
		Set ("SpecificFolder");
	}
	File::Vpath const & GetFolderPath () const { return _folderPath; }
	GlobalId GetFolderGid () const { return _folderGid; }

	void SetFileFilter (std::unique_ptr<FileFilter> filter)
	{
		Assert (filter.get () != 0);
		if (filter->IsFilteringOn ())
		{
			_fileFilter = std::move(filter);
		}
		else
		{
			// empty filter means no filtering at all
			_fileFilter.reset ();
		}
	}
	FileFilter const * GetFileFilter () const { return _fileFilter.get (); }
	bool IsFileVisible (GlobalId fileGid) const
	{
		Assert (_fileFilter.get () != 0);
		return _fileFilter->IsIncluded (fileGid);
	}
	bool IsScriptVisible (GlobalId scriptGid) const
	{
		Assert (_fileFilter.get () != 0);
		return _fileFilter->IsScriptIncluded (scriptGid);
	}
	bool IsFilterOn () const { return _fileFilter.get () != 0; }
	bool IsScriptCommentFilterOn () const
	{
		return IsFilterOn () && _fileFilter->IsScriptFilterOn ();
	}
	bool IsChangedFileFilterOn () const
	{
		return IsFilterOn () && _fileFilter->IsFileFilterOn ();
	}
	void SetInterestingItems (GidSet const & itemIds)
	{
		_interestingItems = itemIds;
	}
	bool IsInteresting (GlobalId gid) const
	{
		return _interestingItems.empty () ||
			   _interestingItems.find (gid) != _interestingItems.end ();
	}
	// If this restriction has been pre-filled
	void SetPreSelectedIds (GidList const * ids) { _ids = ids; }
	typedef std::vector<UniqueName>::const_iterator UnameIter;
	bool HasFiles () const { return _names != 0 && !_names->empty (); }
	UnameIter BeginFiles () const { return _names->begin (); }
	UnameIter EndFiles () const { return _names->end (); }
	bool HasIds () const { return _ids != 0 && !_ids->empty (); }
	GidList const & GetPreSelectedIds () const { return *_ids; }

private:
    unsigned int					_sortCol;
    SortType						_type;
	NocaseSet						_extFilter;		// Eliminate files with extensions from the filter
	std::shared_ptr<FileFilter>		_fileFilter;    // shared, not unique: to make Restriction copyable
	std::vector<UniqueName> const *	_names;			// Pre-selected names
	GidList const *					_ids;			// Pre-selected ids
	File::Vpath						_folderPath;	// if listing particular folder
	GlobalId						_folderGid;
	GidSet							_interestingItems;
};

//
// Table -- data source for display
//

class Table: public TransactionNotifier
{
private:
	static char const * const _name []; // indexed by Id below
public:
	enum Id
	{
		WikiDirectoryId,
		projectTableId,
		folderTableId,
		checkInTableId,
		mailboxTableId,
		synchTableId,
		historyTableId,
		scriptDetailsTableId,
		mergeDetailsTableId,
		emptyTableId
	};

	enum Column
    {
        colName,        // char const *
        colId,          // GlobalId
        colState,       // unsigned long
        colStateName,   // char const *
        colParentId,    // GlobalId
        colOrigin,      // GlobalId
        colType,        // unsigned long
		colTypeName,	// char const *
        colParentPath,  // char const *
        colTimeStamp,   // char const * or FileTime
		colFrom,		// char const *
		colVersion,		// char const * - report column -- full script comment
		colSize,		// unsigned long
		colReadOnly,	// unsigned long -- 1 -- read-only; 0 -- read-write
		colTargetPath,	// char const *
		colInvalid = -1	// Unspecified column
    };

	enum Row
	{
		rowInvalid = -1	// Unspecified row
	};

public:
	virtual ~Table () {}

	virtual std::string GetRootName () const { return std::string (); }
    virtual void QueryUniqueIds (Restriction const & restrict, GidList & ids) const {};
	virtual void QueryUniqueNames (Restriction const & restrict, 
								   std::vector<std::string> & names, 
								   GidList & parentIds) const {}
	virtual void QueryUniqueNames (Restriction const & restrict, 
								   std::vector<std::string> & names) const {}
	virtual Id GetId () const = 0;
	virtual bool IsValid () const = 0;
    virtual std::string	GetStringField (Column col, GlobalId gid) const = 0;
    virtual std::string	GetStringField (Column col, UniqueName const & uname) const = 0;
    virtual GlobalId GetIdField (Column col, UniqueName const & uname) const = 0;
    virtual GlobalId GetIdField (Column col, GlobalId gid) const = 0;
    virtual unsigned long GetNumericField (Column col, UniqueName const & uname) const { return 0; }
    virtual unsigned long GetNumericField (Column col, GlobalId gid) const= 0;
    virtual bool GetBinaryField (Column col, void * buf, unsigned int bufLen, GlobalId gid) const { return false; }
    virtual bool GetBinaryField (Column col, void * buf, unsigned int bufLen, UniqueName const & uname) const { return false; }

    virtual std::string	GetCaption (Restriction const & restrict) const 
	{
		return std::string ();
	}

	virtual void ClearFileCache () const {}

	static char const * GetName (Table::Id tableId)
	{
		Assert (0 <= tableId && tableId <= Table::emptyTableId);
		return _name [tableId];
	}
};

class TableProvider
{
	friend class TableBrowser;

public:
	virtual ~TableProvider () {}

    virtual std::unique_ptr<RecordSet> Query (Table::Id tableId, Restriction const & restrict) = 0;
    virtual std::string QueryCaption (Table::Id tableId, Restriction const & restrict) const = 0;
	virtual bool IsEmpty (Table::Id tableId) const = 0;
	virtual DegreeOfInterest HowInteresting (Table::Id tableId) const = 0;
	virtual bool SupportsHierarchy (Table::Id tableId) const = 0;

private:
	virtual Table const & GetTable (Table::Id tableId) const = 0;
};

#endif
