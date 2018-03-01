//------------------------------------
//  (c) Reliable Software, 2005 - 2006
//------------------------------------

#include "precompiled.h"
#include "TreeBrowser.h"
#include "HierarchyView.h"
#include "Predicate.h"

#include <Ctrl/ProgressBar.h>
#include <Ctrl/StatusBar.h>

TreeBrowser::TreeBrowser (TableProvider & tableProv, Table::Id tableId, HierarchyView & view)
	: WidgetBrowser (tableProv, tableId),
	  _hierarchyView (view),
	  _hierarchy (tableProv, tableId, view)
{
}

bool TreeBrowser::Show (FeedbackManager & feedback)
{
	_hierarchy.Rebuild ();
	return false;
}

void TreeBrowser::Hide ()
{
	// REVISIT: implementation
}

void TreeBrowser::OnFocus ()
{
	_hierarchyView.SetFocus ();
}

void TreeBrowser::Invalidate ()
{
	// REVISIT: implementation
}

bool TreeBrowser::IsActive () const
{
	return _tableProv.SupportsHierarchy (_tableId);
}

bool TreeBrowser::IsCurrentDir (Tree::NodeHandle node)
{
	return _recordSet.get () != 0 && _recordSet->IsValid () && _hierarchy.IsSelected (node);
}

void TreeBrowser::OnSelect (Tree::NodeHandle node, File::Vpath & path)
{
	_recordSet = _hierarchy.OnSelect (node, path);
}

bool TreeBrowser::FindIfSelected (std::function<bool(long, long)> predicate) const
{
	if (_recordSet.get () != 0 && _recordSet->IsValid ())
	{
		return predicate (_recordSet->GetState (0), _recordSet->GetType (0));
	}
	return false;
}

void TreeBrowser::GetSelectedRows (std::vector<unsigned> & rows) const
{
	rows.push_back (0);
}

void TreeBrowser::GetAllRows (std::vector<unsigned> & rows) const
{
	rows.push_back (0);
}

int TreeBrowser::SelectIf (NamePredicate const & predicate)
{
	// REVISIT: implementation
	return -1;
}

void TreeBrowser::SelectAll ()
{
	// REVISIT: implementation
}

void TreeBrowser::ScrollToItem (unsigned iItem)
{
	// REVISIT: implementation
}

void TreeBrowser::InPlaceEdit (unsigned row)
{
	// REVISIT: implementation
}

void TreeBrowser::BeginNewItemEdit ()
{
	// REVISIT: implementation
}

void TreeBrowser::AbortNewItemEdit ()
{
	// REVISIT: implementation
}

// Notification messages from TreeView
void TreeBrowser::CopyField (Tree::NodeHandle node, char * buf, unsigned bufLen) const
{
	if (bufLen > 4)
		strcpy (buf, "node");
}

void TreeBrowser::GetImage (Tree::NodeHandle node, int & image, int & selImage) const
{
	image = HierarchyView::imgClosed;
	selImage = HierarchyView::imgOpen;
}

int  TreeBrowser::GetChildCount (Tree::NodeHandle node) const
{
	return 1;
}

void TreeBrowser::UpdateAll ()
{
	throw Win::Exception ("TreeBrowser::UpdateAll should not be called");
}

void TreeBrowser::Update (std::string const & topic)
{
	Assert (topic == "tree");
	if (_tableProv.SupportsHierarchy (_tableId))
		_recordSet = _hierarchy.Rebuild ();
}
