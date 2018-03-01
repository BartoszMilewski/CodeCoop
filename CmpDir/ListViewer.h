#if !defined (LIST_H)
#define LIST_H
//------------------------------------
//  (c) Reliable Software, 2002
//------------------------------------
#include "DataPortal.h"
#include <Ctrl/ListView.h>
#include <Graph/ImageList.h>
#include <list>

// Purpose: Display a list of files and folders
//          Display full paths of directories being compared
// Knows: About files and folders, their icons, names, and states
//        Lets Windows keep the list of file/folder names

class FileListView: public Win::ReportListing
{
	friend class View;
public:
	FileListView (Win::Dow::Handle winParent, int id);
	void Initialize (int colWidthPct);
	void Size (int x, int y, int width, int height);
	long CalcColWidthPct ();

	int GetCount () const { return _items.size (); }
	void Clear (bool isTwoCol);
	Data::Item const & GetItem (unsigned i) const
	{
		Assert (i < _items.size ());
		std::list<Data::Item>::const_iterator it = _items.begin ();
		for (unsigned j = 0; j < i; ++j)
			++it;
		Assert (it != _items.end ());
		return *it;
	}
private:
	friend class FileViewSequencer;
	typedef std::list<Data::Item>::iterator Iterator;

	Iterator InsertItem (int pos, Iterator it, Data::Item const & item);
	void UpdateItem (int pos, Iterator it, Data::Item const & item);
	void AppendItem (Data::Item const & item);
	Iterator begin () { return _items.begin (); }
	Iterator end () { return _items.end (); }
	static void InitItem (ListView::Item & item, Data::Item const & data);
	void UpdateStateCol (int pos, Data::Item const & data);
private:
	long					_colWidthPct;
	ImageList::AutoHandle	_images;
	// cache all the items
	std::list<Data::Item>	_items;
};

class FileViewSequencer
{
public:
	FileViewSequencer (FileListView & listView)
		: _listView (listView), _i (0), _it (_listView.begin ())
	{}
	bool AtEnd () const { return _it == _listView.end (); }
	void Advance () { ++_i; ++_it; }
	Data::Item const & GetItem () { return *_it; }
	void Insert (Data::Item const & item)
	{
		_it = _listView.InsertItem (_i, _it, item);
	}
	void Set (Data::Item const & item)
	{
		_listView.UpdateItem (_i, _it, item);
	}
	void Append (Data::Item const & it)
	{
		_listView.AppendItem (it);
	}
private:
	int				_i;
	FileListView &	_listView;
	FileListView::Iterator _it;
};

#endif
