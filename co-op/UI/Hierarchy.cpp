//----------------------------------
// (c) Reliable Software 2002 - 2008
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "Hierarchy.h"
#include "HierarchyView.h"
#include "RecordSet.h"
#include <File/Vpath.h>

class Refresher
{
public:
	Refresher (Hierarchy & hierarchy)
		: _tableProv (hierarchy._tableProv), 
		  _tableId (hierarchy._tableId),
		  _restriction (hierarchy._restriction),
		  _view (hierarchy._view),
		  _selection (&hierarchy._tree)
	{
		_gidPath.push_back (gidRoot);
		_restriction.SetFolder (_path, _gidPath.back ());
	}
	std::unique_ptr<RecordSet> GetRecordSet ()
	{
		std::unique_ptr<RecordSet> recordSet = _tableProv.Query (_tableId, _restriction);
		if (recordSet->GetTableId () != Table::emptyTableId)
			return recordSet;
		else
			return std::unique_ptr<RecordSet> ();
	}
	void DirDown (std::string const & name, GlobalId gid)
	{
		_path.DirDown (name);
		_gidPath.push_back (gid);
		_restriction.SetFolder (_path, _gidPath.back ());
	}
	void DirUp ()
	{
		_path.DirUp ();
		_gidPath.pop_back ();
		_restriction.SetFolder (_path, _gidPath.back ());
	}
	Hierarchy::NodeInfo * GetSelection () const { return _selection; }
	void ResetSelection (Hierarchy::NodeInfo * selection) { _selection = selection; }
	HierarchyView &	GetView () const { return _view; }

private:
	File::Vpath				_path;
	std::vector<GlobalId>	_gidPath;
	Hierarchy::NodeInfo *	_selection;

	TableProvider &		_tableProv;
	Table::Id			_tableId;
	Restriction			_restriction;
    HierarchyView &		_view;
};

void Hierarchy::NodeInfo::GetPath (File::Vpath & path) const
{
	if (_parent != 0)
	{
		_parent->GetPath (path);
		path.DirDown (_name);
	}
}

void Hierarchy::NodeInfo::GetUname (UniqueName & name) const
{
	if (_parent != 0)
	{
		GlobalId idParent = _parent->GetGlobalId ();
		if (idParent != gidInvalid)
		{
			name.Init (idParent, _name);
		}
		else
		{
			_parent->GetUname (name);
			name.Down (_name.c_str ());
		}
	}
}

void Hierarchy::NodeInfo::SortChildren ()
{
	dbg << "--> NodeInfo::SortChildren" << std::endl;
	std::sort (begin (), end (), LessPtr);
	dbg << "<-- NodeInfo::SortChildren" << std::endl;
}

Hierarchy::NodeInfo * Hierarchy::NodeInfo::Find (Tree::NodeHandle node) 
{
	if (GetNode () == node)
		return this;
	for (ChildIter it = begin (); it != end (); ++it)
	{
		Hierarchy::NodeInfo * result = (*it)->Find (node);
		if (result != 0)
			return result;
	}
	return 0;
}

Hierarchy::NodeInfo * Hierarchy::NodeInfo::Find (
	Vpath::iterator pathIter,
	Vpath::iterator endIter)
{
	NodeInfo * node = FindChild (*pathIter);
	Assert (node != 0);
	++pathIter;
	if (pathIter == endIter)
		return node;
	return node->Find (pathIter, endIter);
}

Hierarchy::NodeInfo * Hierarchy::NodeInfo::FindChild (std::string const & name)
{
	for (ChildIter it = begin (); it != end (); ++it)
	{
		if (IsNocaseEqual ((*it)->GetName (), name))
			return *it;
	}
	return 0;
}

// returns -1 if it doesn't contain
// otherwise it returns the number of levels down from this to node
int Hierarchy::NodeInfo::Contains (Hierarchy::NodeInfo * node)
{
	if (node == 0)
		return -1;
	if (node == this)
		return 0;

	int i = Contains (node->GetParent ());
	return (i != -1)? i + 1: -1;
}

void Hierarchy::NodeInfo::Collapse ()
{
	SetExpanded (false);
	// recursively destroys
	_children.clear ();
}

void Hierarchy::NodeInfo::SwapInChildren (auto_vector<NodeInfo> & children)
{
	_children.swap (children);
	for (ChildIter it = begin (); it != end (); ++it)
		(*it)->SetParent (this);
}

void Hierarchy::NodeInfo::Refresh (Refresher & refresher)
{
	dbg << "--> Hierarchy::NodeInfo::Refresh" << std::endl;
	{	// List children from record set
		std::unique_ptr<RecordSet> recordSet = refresher.GetRecordSet ();
		if (recordSet.get () == 0)
			return;

		unsigned childCount = recordSet->RowCount ();

		if (IsExpanded ())
		{
			// create a temporary vector of (actual) children
			auto_vector<NodeInfo> children;
			for (unsigned i = 0; i < childCount; ++i)
			{
				std::string name (recordSet->GetStringField (i, 0));
				GlobalId gid = recordSet->GetGlobalId (i);

				std::unique_ptr<Hierarchy::NodeInfo> actualChild (
					new Hierarchy::NodeInfo (this, gid, name));
				// copy expansion attribute and grand-children from the original
				ChildIter it;
				for (it = begin(); it != end(); ++it)
					if (IsNocaseEqual(name, (*it)->GetName()))
						break;
				if (it != end ()) // this child existed in the old tree
				{
					if ((*it)->IsSelected ())
					{
						actualChild->Select ();
						refresher.ResetSelection (actualChild.get ());
					}
					actualChild->SetExpanded ((*it)->IsExpanded ());
					actualChild->SwapInChildren ((*it)->_children);
				}
				children.push_back (std::move(actualChild));
			}
			_children.swap (children);
			SortChildren ();
		}
		else
			_children.clear ();

		SetHasChildren (childCount != 0);
		// Add myself to view
		Tree::NodeHandle node;
		NodeInfo * parent = GetParent ();
		if (parent == 0) // I am root
			node = refresher.GetView ().AddRoot (recordSet->GetRootName (), childCount != 0);
		else
		{
			Tree::Node descriptor (parent->GetNode ());
			descriptor.SetHasChildren (childCount != 0);
			descriptor.SetText (GetName ().c_str ());
			if (GetGlobalId () != gidInvalid)
				descriptor.SetIcon (HierarchyView::imgClosed, HierarchyView::imgOpen);
			else
				descriptor.SetIcon (HierarchyView::imgGreyClosed, HierarchyView::imgGreyOpen);
			node = refresher.GetView ().AppendChild (descriptor);
		}
		ResetNode (node);
	}

	// recurse
	for (ChildIter it = begin (); it != end (); ++it)
	{
		refresher.DirDown ((*it)->GetName (), (*it)->GetGlobalId ());
		(*it)->Refresh (refresher);
		refresher.DirUp ();
	}

	// Finally, expand if necessary
	if (IsExpanded () && HasChildren ())
	{
		refresher.GetView ().Expand (GetNode ());
	}
	dbg << "<-- Hierarchy::NodeInfo::Refresh" << std::endl;
}

//----------
// Hierarchy
//----------

Hierarchy::Hierarchy (TableProvider & tableProv, Table::Id tableId, HierarchyView & view)
	: _isInitialized (false),
	  _tableProv (tableProv),
	  _tableId (tableId),
	  _view (view),
	  _selection (&_tree)
{
	_tree.Select ();
	_restriction.Set ("FoldersOnly");
	_tree.SetGlobalId (gidRoot);
}

bool Hierarchy::GetSelection (GlobalId & gid, std::string & name)
{
	if (_selection == 0)
		return false;
	gid = _selection->GetGlobalId ();
	name = _selection->GetName ();
	return true;
}

void Hierarchy::Clear (bool forGood)
{
	Select (&_tree);
	_tree.Collapse ();
	_isInitialized = !forGood;
	_view.ClearAll ();
}

std::unique_ptr<RecordSet> Hierarchy::Rebuild ()
{
	dbg << "--> Hierarchy::Rebuild" << std::endl;
	Win::RedrawLock lock (_view);
	_view.ClearAll ();
	if (!_isInitialized)
	{
		_tree.SetExpanded ();
		_isInitialized = true;
	}
	Refresher refresher (*this);
	_tree.Refresh (refresher);
	if (_view.GetItemCount () != 0)
	{
		_selection = refresher.GetSelection ();
		Assert (_selection != 0);
		Tree::NodeHandle node = _selection->GetNode ();
		_view.Select (node);
		File::Vpath path;
		return OnSelect (node, path);
	}
	dbg << "<-- Hierarchy::Rebuild" << std::endl;
	return std::unique_ptr<RecordSet> ();
}

void Hierarchy::GoDown (std::string name)
{
	if (!_selection->IsExpanded ())
		Expand (_selection->GetNode ());
	NodeInfo * info = _selection->FindChild (name);
	Assert (info != 0);
	Select (info);
	_view.Select (info->GetNode ());
}

void Hierarchy::GoUp ()
{
	Select (_selection->GetParent ());
	_view.Select (_selection->GetNode ());
}

void Hierarchy::GoToRoot ()
{
	Select (&_tree);
	_view.Select (_selection->GetNode ());
}

void Hierarchy::GoTo(Vpath const & vpath)
{
	GoToRoot();
	for (Vpath::const_iterator it = vpath.begin(); it != vpath.end(); ++it)
		GoDown(*it);
}

std::unique_ptr<RecordSet> Hierarchy::OnSelect (Tree::NodeHandle node, File::Vpath & path)
{
	NodeInfo * info = _tree.Find (node);
	Assert (info != 0);
	Select (info);
	info->GetPath (path);

	std::vector<UniqueName> names;
	File::Vpath parentPath;
	NodeInfo * parent = info->GetParent();
	if (parent)
	{
		parent->GetPath (parentPath);
		UniqueName uname;
		info->GetUname (uname);
		names.push_back (uname);
	}
	else
	{
		// Project root selected
		UniqueName root;
		names.push_back (root);
	}
	Restriction restriction (&names, 0);
	if (parent)
		restriction.SetFolder (parentPath, parent->GetGlobalId ());
	else
		restriction.SetFolder (parentPath, gidRoot);
	return _tableProv.Query (_tableId, restriction);
}

void Hierarchy::Select (Hierarchy::NodeInfo * selection)
{
	Assert (_selection->IsSelected ());
	_selection->Select (false);
	_selection = selection;
	_selection->Select ();
}

// return true if expansion successful
bool Hierarchy::Expand (Tree::NodeHandle node)
{
	Assert (_view != 0);
	NodeInfo * parentNodeInfo = _tree.Find (node);
	Assert (parentNodeInfo != 0);
	if (parentNodeInfo->IsExpanded ())
		return parentNodeInfo->HasChildren ();

	File::Vpath path;
	parentNodeInfo->GetPath (path);
	_restriction.SetFolder (path, parentNodeInfo->GetGlobalId ());

	{ // Replenish current node
		// Currently, there can be only one observer of the table,
		// don't make this record set the observer!
		std::unique_ptr<RecordSet> recordSet (_tableProv.Query (_tableId, _restriction));
		unsigned childCount = recordSet->RowCount ();
		for (unsigned i = 0; i < childCount; ++i)
		{
			std::string name (recordSet->GetStringField (i, 0));
			GlobalId gid = recordSet->GetGlobalId (i);
			std::unique_ptr<Hierarchy::NodeInfo> childInfo (
				new Hierarchy::NodeInfo (parentNodeInfo, gid, name));
			parentNodeInfo->AddChild (std::move(childInfo));
		}
		parentNodeInfo->SetExpanded (true);
		if (childCount == 0)
		{
			parentNodeInfo->SetHasChildren (false);
			_view.SetChildless (parentNodeInfo->GetNode ());
			return false;
		}
		else
		{
			parentNodeInfo->SortChildren ();
		}
	}
	// Create visible nodes
	Tree::Node childItem (node);
	GlobalId gidParent = parentNodeInfo->GetGlobalId ();
	for (NodeInfo::ChildIter it = parentNodeInfo->begin (); it != parentNodeInfo->end (); ++it)
	{
		GlobalId gid = (*it)->GetGlobalId ();
		std::string const & name = (*it)->GetName ();
		childItem.SetText (name.c_str ());
		if ((*it)->GetGlobalId () != gidInvalid)
			childItem.SetIcon (HierarchyView::imgClosed, HierarchyView::imgOpen);
		else
			childItem.SetIcon (HierarchyView::imgGreyClosed, HierarchyView::imgGreyOpen);
		path.DirDown (name);
		_restriction.SetFolder (path, gidParent);
		path.DirUp ();
		std::unique_ptr<RecordSet> recordSet (_tableProv.Query (_tableId, _restriction));
		bool hasChildren = (recordSet->RowCount () != 0);
		childItem.SetHasChildren (hasChildren);

		Tree::NodeHandle node = _view.AppendChild (childItem);
		(*it)->ResetNode (node);
		(*it)->SetHasChildren (hasChildren);
	}
	return true;
}

// returns # of levels selection went up
int Hierarchy::Collapse (Tree::NodeHandle node)
{
	Assert (_view != 0);
	_view.Collapse (node, true); // forget}
	NodeInfo * nodeInfo = _tree.Find (node);
	Assert (nodeInfo != 0);
	int levelsUp = nodeInfo->Contains (_selection);
	nodeInfo->Collapse ();
	if (levelsUp != -1)
	{
		_selection = nodeInfo;
		_selection->Select ();
		return levelsUp;
	}
	else
		return 0;
}

bool Hierarchy::IsSelected (Tree::NodeHandle node)
{
	NodeInfo * nodeInfo = _tree.Find (node);
	return _selection == nodeInfo;
}

void Hierarchy::RetrieveNodePath (Tree::NodeHandle node, File::Vpath & path)
{
	NodeInfo * info = _tree.Find (node);
	Assert (info != 0);
	info->GetPath (path);
}

std::ostream& operator<<(std::ostream& os, Hierarchy::NodeInfo const & node)
{
	for (Hierarchy::NodeInfo::ConstChildIter it = node.begin(); it != node.end(); ++it)
	{
		Hierarchy::NodeInfo const * child = *it;
		dbg << "    " << child->GetName() << std::endl;
	}
	return os;
}
