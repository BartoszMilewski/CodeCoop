#if !defined (TREEBROWSER_H)
#define TREEBROWSER_H
//------------------------------------
//  (c) Reliable Software, 2005 - 2006
//------------------------------------

#include "WidgetBrowser.h"
#include "Hierarchy.h"
#include <Ctrl/Tree.h>

class HierarchyView;
class TableProvider;
class FeedbackManager;
namespace Win
{
	class StatusBar;
	class ProgressBar;
}

class TreeBrowser: public WidgetBrowser
{
public:
	TreeBrowser (TableProvider & tableProv, Table::Id tableId, HierarchyView & view);
	Hierarchy & GetHierarchy () { return _hierarchy; }
	bool IsActive () const;
	bool IsCurrentDir (Tree::NodeHandle node);

	// Widget browser interface

	bool Show (FeedbackManager & feedback);
	void Hide ();
	void OnFocus ();
	void Invalidate ();
	void Clear (bool forGood) { _hierarchy.Clear (forGood); }
	void SetRestrictionFlag (std::string const & name, bool val);
	unsigned SelCount () const { return 1; }
	bool FindIfSelected (std::function<bool(long, long)> predicate) const;
	void GetSelectedRows (std::vector<unsigned> & rows) const;
	void GetAllRows (std::vector<unsigned> & rows) const;
	int SelectIf (NamePredicate const & predicate);
	void SelectAll ();
	void ScrollToItem (unsigned iItem);
	void InPlaceEdit (unsigned row);
	void BeginNewItemEdit ();
	void AbortNewItemEdit ();

	// Notification messages from TreeView
	void OnSelect (Tree::NodeHandle node, File::Vpath & path);
	void CopyField (Tree::NodeHandle node, char * buf, unsigned bufLen) const;
	void GetImage (Tree::NodeHandle node, int & image, int & selImage) const;
	int  GetChildCount (Tree::NodeHandle node) const;
	// Observer
	void UpdateAll ();
	void Update (std::string const & topic);

private:
	Hierarchy			_hierarchy;
    HierarchyView &		_hierarchyView;
};

#endif
