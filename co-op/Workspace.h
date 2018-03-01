#if !defined (WORKSPACE_H)
#define WORKSPACE_H
//------------------------------------
//  (c) Reliable Software, 2001 - 2007
//------------------------------------

#include "GlobalId.h"
#include "UniqueName.h"
#include "FileTypes.h"
#include "FileTag.h"
#include "ScriptCommandList.h"
#include "VerificationReport.h"

#include <Dbg/Assert.h>

#include <auto_vector.h>

class DataBase;
class Directory;
class CommandList;
class WindowSeq;
class FileData;
class FileFilter;
namespace History
{
	class Db;
	class Range;
}
class FileTyper;
class FileCmd;
class Lineage;
namespace Progress
{
	class Meter;
}
class PathFinder;
class PathParser;
class FileChanges;

namespace Workspace
{
	class Item;
	class Selection;
	class TopologicalSorter;

	// Group of checked out items that are not selected
	// Use only under transaction
	class XGroup
	{
	public:
		XGroup (DataBase const & dataBase, Selection & selection);

		bool empty () const { return _workgroup.empty (); }

		FileData const * IsUsing (UniqueName const & name) const;
		FileData const * WasUsing (UniqueName const & name) const;
		FileData const * IsFolderContentsPresent (GlobalId gidFolder) const;

	private:
		typedef std::vector<FileData const *>::const_iterator Iterator;

	private:
		std::vector<FileData const *>	_workgroup;
	};

	enum Operation
	{
		Undefined,
		Add,		// Add new name to the project name space
		Move,		// Change name and/or position in the project tree
		Edit,		// Change file contents and/or file type
		Delete,		// Remove a name from the project name space and file from disk
		Remove,		// Remove a name from the project name space but leave file on disk
		Resolve,	// Current file/folder name cannot be used. Change it to “Previous …” name
		Checkout,
		Uncheckout,
		Checkin,
		Undo,		// Undo changes recorded in the history
		Redo,		// Redo changes recorded in the history
		Synch		// Synchronize with incoming script
	};

	class Item
	{
	public:
		virtual ~Item () {}

		void SetOperation (Operation operation) { _operation = operation; }
		void SetTargetParentItem (Item const * item) { _targetFolderItem = item; }
		void SetSourceParentItem (Item const * item) { _sourceFolderItem = item; }
		void SetOverwrite (bool flag) { _overwriteInProject = flag; }

		Operation GetOperation () const { return _operation; }
		Item const * GetTargetParentItem () const { return _targetFolderItem; }
		Item const * GetSourceParentItem () const { return _sourceFolderItem; }
		bool IsOverwriteInProject () const { return _overwriteInProject; }
		bool IsTargetParentInSelection () const { return GetTargetParentItem () != 0; }
		bool IsSourceParentInSelection () const { return GetSourceParentItem () != 0; }
		void GetNonContentsFileChanges (FileChanges & changes) const;

		virtual void SetTargetType (FileType type)
		{
			Assert (!"Workspace::Item::SetTargetType should never be called");
		}
		virtual void SetTargetName (std::string const & name)
		{
			Assert (!"Workspace::Item::SetTargetName should never be called");
		}
		virtual void SetItemGid (GlobalId gid)
		{
			Assert (!"Workspace::Item::SetTargetGid should never be called");
		}
		virtual void SetKeepCheckedOut (bool flag)
		{
			Assert (!"Workspace::Item::SetKeepCheckedOut should never be called");
		}

		// Returns true if item has source folder in the project name space
		virtual bool HasEffectiveSource () const
		{
			Assert (!"Workspace::Item::HasEffectiveSource should never be called");
			return false;
		}
		// Returns true if item has target folder in the project name space
		virtual bool HasEffectiveTarget () const
		{
			Assert (!"Workspace::Item::HasEffectiveTarget should never be called");
			return false;
		}
		virtual bool HasIntendedSource () const
		{
			Assert (!"Workspace::Item::HasIntendedSource should never be called");
			return false;
		}
		virtual bool HasIntendedTarget () const
		{
			Assert (!"Workspace::Item::HasIntendedTarget should never be called");
			return false;
		}
		// Returns true if item has been moved in the project name space.
		// Moving item in the project space means cut/paste or rename.
		virtual bool HasBeenMoved () const
		{
			Assert (!"Workspace::Item::HasBeenMoved should never be called");
			return false;
		}
		bool HasBeenDeleted() const { return !HasIntendedTarget(); }
		virtual bool HasBeenChangedType () const
		{
			Assert (!"Workspace::Item::HasBeenChangedType should never be called");
			return false;
		}
		virtual bool IsFolder () const
		{
			Assert (!"Workspace::Item::IsFolder should never be called");
			return false;
		}
		virtual bool IsKeepCheckedOut () const
		{
			Assert (!"Workspace::Item::IsKeepCheckOut should never be called");
			return false;
		}
		virtual bool IsChanged (PathFinder & pathFinder) const
		{
			Assert (!"Workspace::Item::IsChanged should never be called");
			return false;
		}

		virtual std::string const & GetEffectiveTargetName () const = 0;
		virtual FileType GetEffectiveTargetType () const = 0;

		virtual GlobalId GetItemGid () const
		{
			Assert (!"Workspace::Item::GetItemGid should never be called");
			return gidInvalid;
		}
		// Can be called only when HasEffectiveTarget returns true
		virtual UniqueName const & GetEffectiveTarget () const
		{
			Assert (!"Workspace::Item::GetEffectiveTarget should never be called");
			return *static_cast<UniqueName const *>(0);
		}
		// Can be called only when HasEffectiveSource returns true
		virtual UniqueName const & GetEffectiveSource () const
		{
			Assert (!"Workspace::Item::GetEffectiveSource should never be called");
			return *static_cast<UniqueName const *>(0);
		}
		virtual UniqueName const & GetIntendedTarget () const
		{
			Assert (!"Workspace::Item::GetIntendedTarget should never be called");
			return *static_cast<UniqueName const *>(0);
		}
		virtual FileType GetEffectiveSourceType () const
		{
			Assert (!"Workspace::Item::GetEffectiveSourceType should never be called");
			return *static_cast<FileType const *>(0);
		}
		
	protected:
		Item (Operation operation = Undefined)
			: _operation (operation),
			  _sourceFolderItem (0),
			  _targetFolderItem (0),
			  _overwriteInProject (true)
		{}

		static std::string NormalizeTargetName (UniqueName const & uname);

	protected:
		Operation		_operation;
		Item const *	_sourceFolderItem;
		Item const *	_targetFolderItem;
		bool			_overwriteInProject;	// Overwrite file with the same name in the project -- default true
	};

	// Only used in unit tests
	class TestItem: public Item
	{
	public:
		TestItem(GlobalId gid = gidInvalid, bool isFolder = false, bool isDeleted = false)
			: _gid(gid),
			  _isFolder(isFolder),
			  _isDeleted(isDeleted)
		{}
		std::string const & GetEffectiveTargetName () const 
		{ 
			Assert(!"called"); 
			return _dummy; 
		}
		FileType GetEffectiveTargetType () const  { Assert(!"called"); return FileType(); }
		GlobalId GetItemGid () const
		{
			return _gid;
		}
		bool IsFolder() const { return _isFolder; }
		bool HasIntendedTarget () const { return !_isDeleted; }
	private:
		GlobalId _gid;
		bool _isFolder;
		bool _isDeleted;
		std::string _dummy;
	};

	// Represents file/folder present in the project name space -- currently controlled
	class ExistingItem : public Item
	{
	public:
		ExistingItem (FileData const * fileData, Operation operation)
			: Item (operation),
			  _fileData (fileData),
			  _isRestored (false),
			  _keepCheckedOut (false)
		{}

		void SetTargetType (FileType type) { _targetType = type; }
		void SetKeepCheckedOut (bool flag) { _keepCheckedOut = flag; }
		void SetRestored (bool flag) { _isRestored = flag; }

		bool HasEffectiveSource () const;
		bool HasEffectiveTarget () const;
		bool HasIntendedSource () const;
		bool HasIntendedTarget () const;
		bool HasBeenMoved () const;
		bool HasBeenChangedType () const;
		bool IsFolder () const;
		bool IsKeepCheckedOut () const { return _keepCheckedOut; }
		bool IsChanged (PathFinder & pathFinder) const;
		bool IsRestored () const { return _isRestored; }
		bool IsRecoverable () const;

		UniqueName const & GetEffectiveTarget () const;
		UniqueName const & GetEffectiveSource () const;
		UniqueName const & GetIntendedTarget () const;
		UniqueName const & GetIntendedSource () const;

		std::string const & GetEffectiveTargetName () const;
		FileType GetEffectiveTargetType () const;
		FileType GetEffectiveSourceType () const;

		GlobalId GetItemGid () const;

		FileData const & GetFileData () const { return *_fileData; }

	protected:
		FileData const *	_fileData;
		FileType			_targetType;
		bool				_isRestored;
		bool				_keepCheckedOut;
	};

	// Represents file/folder being added to the project name space
	class ToBeAddedItem : public Item
	{
	public:
		ToBeAddedItem (UniqueName const * uname, FileType fileType = FileType ());
		ToBeAddedItem (UniqueName const & uname, Item const * targetFolder);

		bool HasEffectiveSource () const { return false;	}
		bool HasEffectiveTarget () const { return true; }
		bool HasIntendedSource () const { return false; }
		bool HasIntendedTarget () const { return true; }
		bool HasBeenMoved () const { return false; }
		bool HasBeenChangedType () const { return false; }
		bool IsFolder () const { return _targetType.IsFolder (); }

		void SetTargetType (FileType type) { _targetType = type; }
		void SetTargetName (std::string const & name) { _targetName = name; }
		void SetItemGid (GlobalId gid)
		{
			Assert (_itemGid == gidInvalid);
			_itemGid = gid;
		}
		
		std::string const & GetEffectiveTargetName () const { return _targetName; }
		FileType GetEffectiveTargetType () const { return _targetType; }
		UniqueName const & GetEffectiveTarget () const { return _targetUname; }
		UniqueName const & GetIntendedTarget () const { return _targetUname; }
		GlobalId GetItemGid () const { return _itemGid; }

	private:
		std::string	_targetName;
		FileType	_targetType;
		GlobalId	_itemGid;
		UniqueName	_targetUname;
	};

	// Represents file being moved in the project name space
	class ToBeMovedItem : public ExistingItem
	{
	public:
		ToBeMovedItem (FileData const * fd, UniqueName const & targetUname);

		bool HasEffectiveSource () const { return true;	}
		bool HasEffectiveTarget () const { return true; }
		bool HasIntendedSource () const { return true; }
		bool HasIntendedTarget () const { return true; }
		bool HasBeenMoved () const { return true; }

		std::string const & GetEffectiveTargetName () const { return _targetName; }
		UniqueName const & GetEffectiveTarget () const { return _targetUname; }
		UniqueName const & GetIntendedTarget () const { return _targetUname; }
		UniqueName const & GetEffectiveSource () const { return ExistingItem::GetEffectiveSource (); }

	private:
		std::string _targetName;
		UniqueName	_targetUname;
	};

	// Represents file/folder changed by script
	class ScriptItem : public ExistingItem
	{
	public:
		ScriptItem (FileCmd const & cmd);

		bool HasEffectiveSource () const;
		bool HasEffectiveTarget () const;
		bool HasIntendedSource () const;
		bool HasIntendedTarget () const;

		FileData const & GetScriptFileData () const { return *_fileData; }
		FileCmd const & GetFileCmd () const { return _fileCmd; }

	private:
		FileCmd const &   _fileCmd;
	};

	// Represents file/folder changed by scripts stored in the history
	class HistoryItem : public Item
	{
	public:
		HistoryItem (std::unique_ptr<FileCmd> cmd)
		{
			AddCmd (std::move(cmd));
		}

		bool HasEffectiveSource () const;
		bool HasEffectiveTarget () const;
		bool HasIntendedSource () const;
		bool HasIntendedTarget () const;
		bool HasBeenMoved () const;
		bool HasBeenChangedType () const ;
		bool IsFolder () const;
		bool IsRecoverable () const;

		std::string const & GetEffectiveTargetName () const;
		FileType GetEffectiveTargetType () const;
		FileType GetEffectiveSourceType () const;
		UniqueName const & GetEffectiveTarget () const;
		UniqueName const & GetIntendedSource () const;
		UniqueName const & GetIntendedTarget () const;
		GlobalId GetItemGid () const;
		UniqueName const & GetEffectiveSource () const;
		std::string const & GetLastName () const;
		FileData const & GetEffectiveTargetFileData () const;
		FileData const & GetEffectiveSourceFileData () const;
		FileData const & GetOriginalTargetFileData () const;
		FileData const & GetOriginalSourceFileData () const;

		void AddCmd (std::unique_ptr<FileCmd> cmd)
		{
			_editCmds.push_back (std::move(cmd));
		}

	public:
		class BackwardCmdSequencer
		{
		public:
			BackwardCmdSequencer (HistoryItem const & item)
				: _seq (item._editCmds),
				  _count (item._editCmds.size ())
			{}

			bool AtEnd () const { return _seq.AtEnd (); }
			void Advance () { _seq.Advance (); }
			FileCmd const & GetCmd () const { return _seq.GetFileCmd (); }
			unsigned int Count () const { return _count; }

		private:
			CommandList::Sequencer	_seq;
			unsigned int			_count;
		};

		class ForwardCmdSequencer
		{
		public:
			ForwardCmdSequencer (HistoryItem const & item)
				: _seq (item._editCmds),
				  _count (item._editCmds.size ())
			{}

			bool AtEnd () const { return _seq.AtEnd (); }
			void Advance () { _seq.Advance (); }
			FileCmd const & GetCmd () const { return _seq.GetFileCmd (); }
			unsigned int Count () const { return _count; }

		private:
			CommandList::ReverseSequencer	_seq;
			unsigned int					_count;
		};

		friend class Workspace::HistoryItem::BackwardCmdSequencer;
		friend class Workspace::HistoryItem::ForwardCmdSequencer;

	private:
		CommandList	_editCmds;	// Edit commands are stored in the
								// reverse chronological order.
								// _editCmds.front () -- yields the most recent change
								// _editCmds.back () - yields the oldest change
	};

	class Selection
	{
	public:
		Selection (GidList const & files, DataBase const & dataBase, Operation operation);
		Selection (GidList const & files, DataBase const & dataBase, UniqueName const & targetName);
		Selection (auto_vector<FileTag> const & fileClipboard, DataBase const & dataBase, Directory const & folder);
		Selection (std::vector<std::string> const & files, PathParser & pathParser);
		Selection (std::string const & file, PathParser & pathParser);

		void AddContents (DataBase const & dataBase, bool recursive);
		virtual void Extend (DataBase const & dataBase);
		virtual void XMerge (XGroup const & workgroup);
		bool IsIncluded (GlobalId gid) const { return _items.IsIncluded (gid); }
		Item const & FindItem (GlobalId gid) const { return _items.Find (gid); }
		void Sort ();

		void SetOperation (Operation operation);
		void SetOverwrite (bool flag);
		void SetType (FileTyper & fileTyper, PathFinder & pathFinder);
		void SetType (FileType type);
		unsigned int size () const { return _sortVector.size (); }
		bool empty () const { return size () == 0; }

	// protected: // needed for unit test
		class ItemVector
		{
		public:

			void push_back (std::unique_ptr<Item> item);
			//void erase (unsigned int i);
			void Erase(GidSet const & eraseSet);
			void clear() { _items.clear(); _folderGid2Item.clear(); _itemSet.clear(); }

			Item const * operator [] (unsigned int i) const { return _items [i]; }
			Item * operator [] (unsigned int i) { return _items [i]; }
			ExistingItem * GetExistingItem (unsigned int i) { return dynamic_cast<ExistingItem *> (operator [](i)); }
			HistoryItem * GetHistoryItem (unsigned int i) { return dynamic_cast<HistoryItem *> (operator [](i)); }
			ScriptItem * GetScriptItem (unsigned int i) { return dynamic_cast<ScriptItem *> (operator [](i)); }
			unsigned int size () const { return _items.size (); }
			Item const * IsFolderIncluded (GlobalId folderGid) const;
			bool IsIncluded (GlobalId gid) const { return _itemSet.find (gid) != _itemSet.end (); }
			Item const & Find (GlobalId gid) const;

			typedef auto_vector<Item>::const_iterator Iterator;
			Iterator begin () const { return _items.begin (); }
			Iterator end () const { return _items.end (); }
			Item * back () { return _items.back (); }

		private:
			auto_vector<Item>				_items;			// Selected items
			std::map<GlobalId, Item const*>	_folderGid2Item;// For easy folder lookup in the selection
			GidSet							_itemSet;		// For easy item lookup in the selection
		};

	public:
		class Sequencer
		{
		public:
			Sequencer (Selection const & selection)
				: _cur (selection._sortVector.begin ()),
				  _end (selection._sortVector.end ())
			{}

			bool AtEnd () const { return _cur == _end; }
			void Advance () { ++_cur; }

			Item const & GetItem () const { return **_cur; }
			Item const * GetItemPtr () const { return *_cur; }
			Item & GetItemForEdit () const { return *(const_cast<Item *>(GetItemPtr ())); }
			ExistingItem const & GetExistingItem () { return reinterpret_cast<ExistingItem const &> (GetItem ()); }
			HistoryItem const & GetHistoryItem () { return reinterpret_cast<HistoryItem const &> (GetItem ()); }
			ScriptItem const & GetScriptItem () { return reinterpret_cast<ScriptItem const &>(GetItem ()); }

		private:
			std::vector<Item const *>::const_iterator	_cur;
			std::vector<Item const *>::const_iterator	_end;
		};

		friend class Workspace::Selection::Sequencer;
		friend class Workspace::TopologicalSorter;

	protected:
		Selection () {}

		void LinkItems ();
		void AddResolveItem (FileData const * fd);
		void AddFolderContents (Item const * folderItem,
								DataBase const & dataBase,
								bool recursive = true);
		Item const * AddFoldersIfNecessary (GlobalId folderGid, DataBase const & dataBase); 
		Item const * ParseUname (UniqueName const & folderUname,
								 std::map<UniqueName, Item const *> & nonProjectFolders); 

	protected:
		static char	const *			_incompleteOriginalSelection;
		ItemVector					_items;			// Selected items
		std::vector<Item const *>	_sortVector;	// Sequencing order
	};

	class SubSelection : public Selection
	{
	public:
		SubSelection (Selection const & selection, GidSet const & gidSubset);
	};

	class CheckinSelection : public Selection
	{
	public:
		CheckinSelection (GidList const & files, DataBase const & dataBase);

		void Extend (DataBase const & dataBase);
		void XMerge (XGroup const & workgroup);

		void KeepCheckedOut ();
		bool DetectChanges (PathFinder & pathFinder) const;
		bool IsComplete (DataBase const & dataBase) const;

	private:
		void AddCheckedOutFolderContents (Item const * folderItem, DataBase const & dataBase);
		void Incomplete (FileData const * missing, char const * problem);
	};

	class HistorySelection : public Selection
	{
	public:
		void Extend (DataBase const & dataBase);
		void AddCommand (std::unique_ptr<FileCmd> cmd);
		bool IsRecoverable (GlobalId gid) const;
		void Erase(GidSet const & eraseSet);
	protected:
		HistorySelection () {}

		void Init (bool isForward);

	protected:
		typedef std::map<GlobalId, HistoryItem *>::const_iterator GidIterator;
		std::map<GlobalId, HistoryItem *>	_gid2Idx;	// For easy item lookup in the selection
	};

	class HistoryRangeSelection : public HistorySelection
	{
	public:
		HistoryRangeSelection (History::Db const & history,
							   History::Range const & range,
							   bool isForward,
							   Progress::Meter & meter);
	};

	class FilteredHistoryRangeSelection : public HistorySelection
	{
	public:
		FilteredHistoryRangeSelection (History::Db const & history,
									   History::Range const & range,
									   GidSet const & preSelectedGids,
									   bool isForward,
									   Progress::Meter & meter);
	};

	class RepairHistorySelection : public HistorySelection
	{
	public:
		RepairHistorySelection (History::Db const & history,
								VerificationReport::Sequencer corruptedFiles,
								GidSet & unrecoverableFiles,
								Progress::Meter & meter);
	};

	class BlameHistorySelection : public HistorySelection
	{
	public:
		BlameHistorySelection (History::Db const & history,
							   GlobalId fileGid,
							   Progress::Meter & meter);

		GidList const & GetScriptIds () const { return _scriptIds; }

	private:
		GidList	_scriptIds;
	};

	class ScriptSelection : public Selection
	{
	public:
		ScriptSelection (CommandList const & cmdList);

		void Extend (DataBase const & dataBase);

		void MarkRestored (HistorySelection const & conflict);
	};

	class TopologicalSorter
	{
	public:
		TopologicalSorter (Selection::ItemVector const & selection);

		void Sort (std::vector<Item const *> & sortVector);

	private:
		class SortItem
		{
		public:
			SortItem (Item const * selectionItem)
				: _selectionItem (selectionItem),
				  _predecessorCount (0)
			{}

			void IncPredecessorCount ()	{ ++_predecessorCount; }
			void DecPredecessorCount ()
			{
				if (_predecessorCount != 0)
					--_predecessorCount;
			}
			void AddSuccessor (SortItem * item) { _successors.push_back (item); }

			Item const * GetSelectionItem () const { return _selectionItem; }
			bool HasNoPredecessors () const { return _predecessorCount == 0; }
			std::vector<SortItem *> const & GetSucessors () const { return _successors; }

		private:
			Item const *			_selectionItem;
			unsigned int			_predecessorCount;
			std::vector<SortItem *>	_successors;
		};

	private:
		std::vector<SortItem>	_sortItems;
	};

};

std::ostream& operator<<(std::ostream& os, Workspace::Operation operation);
std::ostream& operator<<(std::ostream& os, Workspace::Item const & item);
std::ostream& operator<<(std::ostream& os, Workspace::Selection const & selection);

#endif
