#if !defined (TABLEVIEW_H)
#define TABLEVIEW_H
//-----------------------------------------
// (c) Reliable Software 1998-2002
// ----------------------------------------

#include "Table.h"
#include "View.h"
#include "RecordSet.h"
#include "Settings.h"
#include <Graph/ImageList.h>

class ItemView;

class TableView
{
public:
    TableView (ItemView & itemView, 
			   TableProvider & tableProv, 
			   char const * tableName, 
			   const unsigned int colWidths [],
			   const unsigned int colCount);

	bool Show (Restriction const * restrict = 0);
	void Hide ();
    void Refresh ();
	void ReSort (unsigned int col);
    void SavePreferences ();
	void Invalidate () { _needsRefreshing = true; }

    void GetImage (unsigned int item, int & image, int & overlay) const;
	std::string const & GetCaption () { return _caption; }
	Win::Dow::Handle GetWindow () { return _itemView.ToNative (); }

    bool HasSelection () const;
    void GetSelectedRows (std::vector<int> & rows) const;
	RecordSet const * GetRecordSet () const { return _recordSet.get (); }
	Restriction const & GetRestriction () const { return _restriction; }

	bool IsInteresting ();

private:
	void UpdateRecordSet ();
	void RebuildView ();
	void UpdateViewData ();
	void SaveState ();
	void RestoreState ();
	bool Sort ();
	unsigned int RecordSetIdx2ViewRow (unsigned int item) const;

    void BindItemIcons ();
    void UpdateIcon (unsigned int item);

	TableProvider &			 _tableProv;
	char const *			 _tableName;
	std::string				 _caption;
	Restriction				 _restriction;
    std::unique_ptr<RecordSet> _recordSet;
	bool					 _needsRefreshing;

	ItemView &				_itemView;
    ViewSettings			_viewSettings;
	ImageList::AutoHandle	_images;

    std::vector<unsigned int>	_sortVector;
	std::vector<Bookmark>		_selectedRows;
	unsigned int  				_bottomItem;

private:
	class SortPredicate
	{
	public:
		SortPredicate (RecordSet const * recordSet, int col)
			: _recordSet (recordSet), _col (col)
		{}
		bool operator () (unsigned int row1, unsigned int row2) const;
	private:
		RecordSet const * _recordSet;
		int				  _col;
	};
};

#endif
