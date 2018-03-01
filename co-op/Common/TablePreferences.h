#if !defined (TABLEPREFERENCES_H)
#define TABLEPREFERENCES_H
//------------------------------------
//  (c) Reliable Software, 2005 - 2006
//------------------------------------

#include "PreferencesStorage.h"
#include "ColumnInfo.h"

#include <NamedBool.h>
#include <StringOp.h>

class TablePreferences : public Preferences::Storage
{
public:
	typedef std::map<std::string, unsigned int> WidthMap;

public:
	TablePreferences (Preferences::Storage const & storage,
					  std::string const & tableName,
					  Column::Info const  * columnInfo);
	~TablePreferences ();

	void Verify ();

	void SetFilter (NocaseSet const & filter) { _filter = filter; }
	void SetOptions (NamedBool const & options) { _options = options; }
	void SetSortColumn (unsigned int colIdx);
	void SetColumnWidth (unsigned int colIdx, unsigned int width);
	void ChangeCurrentSortOrder ();

	NocaseSet const & GetFilter () const { return _filter; }
	NamedBool const & GetOptions () const { return _options; }
	void GetSortInfo (unsigned int & colIdx, SortType & sortOrder) const;
	unsigned int GetSortColumnIdx () const { return _sortColumnIdx; }
	SortType GetSortOrder () const { return _sortOrder; }
	unsigned int GetSecondarySortColumnIdx () const { return _secondarySortColumnIdx; }
	SortType GetSecondarySortOrder () const { return _secondarySortOrder; }
	std::string const GetColumnHeading (unsigned int colIdx) const;
	Win::Report::ColAlignment GetColumnAlignment (unsigned int colIdx) const;
	unsigned int GetColumnWidth (std::string const & columnName) const;
	unsigned int GetEmptyTableColumnWidth () const { return _emptyTableColumnWidth; }

private:
	static char const *	_sortOrderValueName;
	static char const *	_sortIndexValueName;
	static char const *	_secondarySortOrderValueName;
	static char const *	_secondarySortIndexValueName;
	static char const *	_extFilterValueName;
	static char const *	_optionsValueName;

private:
	Column::Info const * FindDefaultInfo (std::string const & columnHeading) const;

private:
	Column::Info const *	_columnInfo;	// Default column information
	unsigned int			_columnCount;
	WidthMap				_currentWidth;	// Maps column name to the column current width
	unsigned int			_emptyTableColumnWidth;
	SortType				_sortOrder;		// Current sort order
	SortType				_secondarySortOrder;	// Current sort order
	unsigned int			_sortColumnIdx;
	unsigned int			_secondarySortColumnIdx;
	NocaseSet				_filter;
	NamedBool				_options;
};

#endif
