#if !defined (HIERARCHY_H)
#define HIERARCHY_H
//----------------------------------
// (c) Reliable Software 2002 - 2008
//----------------------------------

#include "Table.h"

#include <Ctrl/Tree.h>
#include <auto_vector.h>

class HierarchyView;
class Vpath;

class Hierarchy
{
	friend class Refresher;
	class NodeInfo;
	friend std::ostream& operator<<(std::ostream& os, Hierarchy::NodeInfo const & node);

	class NodeInfo
	{
	public:
		typedef auto_vector<NodeInfo>::iterator ChildIter;
		typedef auto_vector<NodeInfo>::const_iterator ConstChildIter;

	private:
		// predicate
		static bool LessPtr (Hierarchy::NodeInfo const * node1, Hierarchy::NodeInfo const * node2)
		{
			return *node1 < *node2;
		}

	public:
		NodeInfo () : _parent (0), _gid (gidInvalid) {}

		NodeInfo (NodeInfo * parent, GlobalId gid, std::string const & name, Tree::NodeHandle node = Tree::NodeHandle ())
			: _gid (gid), _name (name), _node (node), _parent (parent)
		{}
		Tree::NodeHandle GetNode () const { return _node; }
		void ResetNode (Tree::NodeHandle node = Tree::NodeHandle ()) { _node = node; }
		void SetGlobalId (GlobalId gid) { _gid = gid; }
		std::string const & GetName () const { return _name; }
		GlobalId GetGlobalId () const { return _gid; }
		void SetParent (NodeInfo * parent) { _parent = parent; }
		NodeInfo * GetParent () const { return _parent; }
		// State
		bool HasChildren () const { return _state.test (Children); }
		void SetHasChildren (bool val = true) { _state.set (Children, val); }
		bool IsExpanded () const { return _state.test (Expanded); }
		void SetExpanded (bool val = true) { _state.set (Expanded, val); }
		bool IsSelected () const { return _state.test (Selected); }
		void Select (bool val = true) { _state.set (Selected, val); }

		void AddChild (std::unique_ptr<NodeInfo> child)
		{
			_state.set (Children);
			_children.push_back (std::move(child));
		}
		void SwapInChildren (auto_vector<NodeInfo> & children);
		NodeInfo * FindChild (std::string const & name);
		bool operator< (NodeInfo const & node) const
		{
			return IsNocaseLess (_name, node.GetName ());
		}
		ChildIter begin () { return _children.begin (); }
		ChildIter end () { return _children.end (); }
		ConstChildIter begin () const { return _children.begin (); }
		ConstChildIter end () const { return _children.end (); }
		void SortChildren ();
		// recursive methods
		NodeInfo * Find (Tree::NodeHandle node);
		NodeInfo * Find (	Vpath::iterator pathIter,
							Vpath::iterator endIter);
		void GetPath (File::Vpath & path) const;
		void GetUname (UniqueName & name) const;
		void Collapse ();
		int  Contains (NodeInfo * node);
		void Refresh (Refresher & refresher);

	private:
		enum States { Children = 0, Expanded, Selected };
		std::bitset<32>		_state;
		Tree::NodeHandle	_node;
		std::string			_name;
		GlobalId			_gid;
		NodeInfo *			_parent;
		auto_vector<NodeInfo> _children;
	};

	// Predicate
	class EqualName
	{
	public:
		EqualName (std::string const & name)
			: _name (name)
		{}
		bool operator () (Hierarchy::NodeInfo * info)
		{
			return IsNocaseEqual (_name, info->GetName ());
		}
	private:
		std::string const & _name;
	};

public:
	Hierarchy (TableProvider & tableProv, Table::Id tableId, HierarchyView & view);

	std::unique_ptr<RecordSet> Rebuild ();
	void Clear (bool forGood);
	bool GetSelection (GlobalId & gid, std::string & name);
	std::unique_ptr<RecordSet> OnSelect (Tree::NodeHandle node, File::Vpath & path);
	void GoDown (std::string name);
	void GoUp ();
	void GoToRoot ();
	void GoTo(Vpath const & vpath);
	bool Expand (Tree::NodeHandle node);
	// returns # of levels selection went up
	int Collapse (Tree::NodeHandle node);
	bool IsSelected (Tree::NodeHandle node);
	void RetrieveNodePath (Tree::NodeHandle node, File::Vpath & path);

private:
	void Select (NodeInfo * node);

private:
	bool				_isInitialized;
	TableProvider &		_tableProv;
	Table::Id			_tableId;
	Restriction			_restriction;
    HierarchyView &		_view;

	NodeInfo			_tree;
	NodeInfo *			_selection;
};

std::ostream& operator<<(std::ostream& os, Hierarchy::NodeInfo const & node);

#endif
