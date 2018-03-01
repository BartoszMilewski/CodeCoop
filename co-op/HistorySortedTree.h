#if !defined (HISTORYSORTEDTREE_H)
#define HISTORYSORTEDTREE_H
//------------------------------------
//  (c) Reliable Software, 2003 - 2007
//------------------------------------

#include "XArray.h"
#include "XLong.h"
#include "Transactable.h"
#include "GlobalId.h"
#include "Serialize.h"
#include "HistoryNode.h"
#include "HistoryPolicy.h"
#include "Lineage.h"
#include "ScriptKind.h"

#include <Dbg/Assert.h>

namespace Progress { class Meter; }
class MemoryLog;
namespace Mailbox
{
	class Agent;
}

namespace History
{
	enum
	{
		npos = TransactableArray<Node>::npos
	};

	// SortedTree is a serialized tree with the following properties:
	// - Serialization is depth-first
	// - Winning branch in each node is serialized last
	class SortedTree : public TransactableContainer
	{
	public:
		class Sequencer;			// Iterates all nodes in the reverse chronological order, stops after iterating over first interesting node
		class XSequencer;			// Iterates all nodes in the reverse chronological order
		class UnitSequencer;		// Iterates nodes of a given unit in the reverse chronological order, stops after iterating over first interesting node
		class XUnitSequencer;		// Iterates nodes of a given unit in the reverse chronological order, stops after iterating over first interesting node
		class ForwardSequencer;		// Iterates all nodes in the chronological order
		class FwdUnitSequencer;		// Iterates nodes of a given unit in the chronological order
		class XFwdUnitSequencer;	// Iterates nodes of a given unit in the chronological order from a given script id
		class XWriteSequencer;		// Iterates all nodes in the reverse chronological order
		class UISequencer;			// Iterates all nodes in the reverse chronological order, stops at node with archive bit set
		class FullSequencer;		// Iterates all nodes in the reverse chronological order
		class XFullSequencer;		// Iterates all nodes in the reverse chronological order
		class PredecessorSequencer;	// Iterates over node predecessors in the reverse chronological order
		class XPredecessorSequencer;// Iterates over node predecessors in the reverse chronological order

		friend class Sequencer;
		friend class XSequencer;
		friend class UnitSequencer;
		friend class XUnitSequencer;
		friend class ForwardSequencer;
		friend class XFwdUnitSequencer;
		friend class XWriteSequencer;
		friend class UISequencer;
		friend class FullSequencer;
		friend class XFullSequencer;
		friend class PredecessorSequencer;
		friend class XPredecessorSequencer;

		class XImportInserter;
		class XFullSynchInserter;

		friend class XImportInserter;
		friend class XFullSynchInserter;


	public:
		SortedTree (Unit::Type unitType, ScriptKind scriptKind)
			: _unitType (unitType),
			  _scriptKind (scriptKind)
		{
			AddTransactableMember (_nodes);
			AddTransactableMember (_firstInterestingScriptId);
		}

		void SetPolicy (std::unique_ptr<History::Policy> policy) { _policy = std::move(policy); }

		bool HasDuplicates () const;
		bool IsCreationMarkerLinked() const;
		void GetMissingScripts (Unit::ScriptList & missingScripts) const;
		bool HasMissingScripts () const;
		bool CanArchive (GlobalId scriptId) const;
		bool IsTreeRoot (GlobalId scriptId) const;
		unsigned int size () const { return _nodes.Count (); }
		unsigned int xsize () const { return _nodes.XCount (); }
		Unit::Type GetUnitType () const { return _unitType; }
		GlobalId GetFirstInterestingScriptId () const;
		GlobalId XGetFirstInterestingScriptId () const;
		void XSetFirstInterestingScriptId (GlobalId id) { _firstInterestingScriptId.XSet (id); }
		unsigned int XActualSize () const { return _nodes.XActualCount (); }
		bool XHasArchiveMarkers () const;

		Node const * GetNode (unsigned int idx) const { return _nodes.Get (idx); }
		Node const * XGetNode (unsigned int idx) const { return _nodes.XGet (idx); }
		Node * XGetEditNode (unsigned int idx) { return _nodes.XGetEdit (idx); }
		Node const * FindNode (GlobalId scriptId) const;
		Node const * XFindNode (GlobalId scriptId) const;
		bool XProcessLineage (UnitLineage::Sequencer & lineageSeq,
							  bool & rootIdFound,
							  Mailbox::Agent & agent);
		bool XInsertNewNode (std::unique_ptr<Node> newNode);
		void XForceInsert (std::unique_ptr<Node> newNode); // Will be used to insert labels in history
		void XMarkDeepFork (GlobalId scriptId);

		void push_back (std::unique_ptr<Node> newNode) { _nodes.XAppend (std::move(newNode)); }
		void XDeleteRecentNode (GlobalId scriptId);
		void XDeleteNodeByIdx (unsigned idx);
		bool XSubstituteNode (std::unique_ptr<Node> newNode);
		void XConfirmAllOlderNodes (XSequencer & seq);
		void XConfirmOlderPredecessors (XSequencer const & seq);

		// Conversion from version 4.2 to 4.5
		void XConvertNodeState (Progress::Meter * meter, MemoryLog & log);
		void XDeleteBranch (unsigned recursionLevel, unsigned idx, std::multimap<GlobalId, unsigned> const & successorMap, MemoryLog * log);
		void XInitLastArchivable (MemoryLog * log);
		void XRemoveTopRejected ();
		bool XPrecedes (GlobalId precederId, GlobalId followerId) const;

		// Serializable interface
		void Serialize (Serializer & out) const;
		void Deserialize (Deserializer & in, int version);

	private:
		unsigned int FindIdx (GlobalId scriptId) const;
		unsigned int XFindIdx (GlobalId scriptId) const;
		unsigned int XInsertNode (std::unique_ptr<Node> newNode);
		bool XHasMissingPredecessors (unsigned int startIdx) const;
		void XForceExecution (unsigned int newNodeIdx);
		static bool IsCandidateForFis (Node const * node)
		{
			return node->IsAckListEmpty () && node->IsExecuted ();
		}
		static bool NoMoreCandidatesForFis (Node const * node)
		{
			return (!node->IsAckListEmpty () && !node->IsRejected ())	// Unconfirmed trunk script or
				|| node->IsMissing ()									// Missing trunk script or
				|| node->IsCandidateForExecution ()						// Not executed yet trunk script or
				|| node->IsToBeRejected ();								// To be rejected trunk script
		}
	private:
		TransactableArray<Node>	_nodes;
		XLong					_firstInterestingScriptId;
		// Volatile
		Unit::Type				_unitType;
		ScriptKind				_scriptKind;
		std::unique_ptr<History::Policy>	_policy;
	};

	// REVISIT: create sequencer interface (const and non-const sequencers) taking different advance and stop policies.
	class SortedTree::Sequencer	// Iterates all nodes in the reverse chronological order, stops after iterating over first interesting node
	{
	public:
		Sequencer (SortedTree const & tree, GlobalId startingId = gidInvalid);

		void Advance ();
		bool AtEnd () const { return _atEnd; }
		Node const * GetNode () const { return _nodes.Get (_curIdx); }
		unsigned int GetIdx () const { return _curIdx; }

	private:
		TransactableArray<Node> const &	_nodes;
		unsigned int					_curIdx;
		GlobalId						_firstInterestingScriptId;
		bool							_atEnd;
	};

	class SortedTree::XSequencer	// Iterates all nodes in the reverse chronological order, stops after iterating over first interesting node
	{
	public:
		XSequencer (SortedTree & tree, GlobalId startingId = gidInvalid);

		void Advance ();
		bool AtEnd () const { return _atEnd; }
		Node const * GetNode () const { return _nodes.XGet (_curIdx); }
		Node * GetNodeEdit () { return _nodes.XGetEdit (_curIdx); }
		unsigned int GetIdx () const { return _curIdx; }
	private:
		void SkipDeleted ();
	private:
		TransactableArray<Node> &	_nodes;
		unsigned int				_curIdx;
		GlobalId					_firstInterestingScriptId;
		bool						_atEnd;
	};

	class SortedTree::UnitSequencer	// Iterates nodes of a given unit in reverse chronological order, stops after iterating over first interesting node		
	{
	public:
		UnitSequencer (SortedTree const & tree, GlobalId unitId)
			: _nodes (tree._nodes),
			  _curIdx (_nodes.Count ()),
			  _firstInterestingScriptId (tree._firstInterestingScriptId.Get ()),
			  _unitId (unitId),
			  _atEnd (false)
		{
			// patch earlier bug
			if (unitId != gidInvalid)
				_firstInterestingScriptId = 0;
			Skip ();
		}

		void Advance ();
		bool AtEnd () const { return _atEnd; }
		Node const * GetNode () const { return _nodes.Get (_curIdx); }
		unsigned int GetIdx () const { return _curIdx; }

	private:
		void Skip ();

	private:
		TransactableArray<Node> const &	_nodes;
		unsigned int					_curIdx;
		GlobalId						_firstInterestingScriptId;
		GlobalId						_unitId;
		bool							_atEnd;
	};


	class SortedTree::XUnitSequencer	// Iterates nodes of a given unit in reverse chronological order, stops after iterating over first interesting node		
	{
	public:
		XUnitSequencer (SortedTree & tree, GlobalId unitId)
			: _nodes (tree._nodes),
			  _curIdx (_nodes.XCount ()),
			  _firstInterestingScriptId (tree._firstInterestingScriptId.XGet ()),
			  _unitId (unitId),
			  _atEnd (false)
		{
			// patch earlier bug
			if (unitId != gidInvalid)
				_firstInterestingScriptId = 0;
			Skip ();
		}

		void Seek (GlobalId scriptId);
		void Advance ();
		bool AtEnd () const { return _atEnd; }
		Node const * GetNode () const { return _nodes.XGet (_curIdx); }
		Node * GetNodeEdit () { return _nodes.XGetEdit (_curIdx); }
		unsigned int GetIdx () const { return _curIdx; }

	private:
		void Skip ();

	private:
		TransactableArray<Node>		&	_nodes;
		unsigned int					_curIdx;
		GlobalId						_firstInterestingScriptId;
		GlobalId						_unitId;
		bool							_atEnd;
	};

	class SortedTree::ForwardSequencer	// Iterates all nodes in chronological order
	{
	public:
		ForwardSequencer (SortedTree const & tree)
			: _nodes (tree._nodes), 
			  _curIdx (0)
		{}

		void Advance ()
		{
			Assert (!AtEnd ());
			++_curIdx;
		}
		bool AtEnd () const { return _curIdx == _nodes.Count (); }
		Node const * GetNode () const { return _nodes.Get (_curIdx); }
		unsigned int GetIdx () const { return _curIdx; }

	private:
		TransactableArray<Node> const &	_nodes;
		unsigned int					_curIdx;
	};
	
	class SortedTree::FwdUnitSequencer	// Iterates nodes of a given unit id in chronological order starting from a given script id
	{
	public:
		FwdUnitSequencer (SortedTree const & tree, GlobalId startScriptId, GlobalId unitId = gidInvalid);

		void Advance ();
		bool AtEnd () const { return _curIdx == _nodes.Count (); }
		Node const * GetNode () const { return _nodes.Get (_curIdx); }
		unsigned int GetIdx () const { return _curIdx; }
		GlobalId GetUnitId () const { return _unitId; }
	private:
		TransactableArray<Node>  const &	_nodes;
		unsigned int						_curIdx;
		GlobalId							_unitId;
	};

	class SortedTree::XFwdUnitSequencer	// Iterates nodes of a given unit id in chronological order starting from a given script id
	{
	public:
		XFwdUnitSequencer (SortedTree & tree, GlobalId startScriptId, GlobalId unitId = gidInvalid);
		XFwdUnitSequencer (SortedTree & tree, UnitLineage::Sequencer & seq);

		void Advance ();
		bool AtEnd () const { return _curIdx == _nodes.XCount (); }
		bool IsTreeEmpty () const { return _isTreeEmpty; }
		Node const * GetNode () const { return _nodes.XGet (_curIdx); }
		Node * GetNodeEdit () { return _nodes.XGetEdit (_curIdx); }
		unsigned int GetIdx () const { return _curIdx; }
		GlobalId GetUnitId () const { return _unitId; }
		void MarkDeleted () { _nodes.XMarkDeleted (_curIdx); }

	private:
		TransactableArray<Node> &	_nodes;
		unsigned int				_curIdx;
		GlobalId					_unitId;
		bool						_isTreeEmpty;
	};

	class SortedTree::XWriteSequencer	// Iterates all nodes in reverse chronological order		
	{
	public:
		XWriteSequencer (SortedTree & tree)
			: _nodes (tree._nodes), 
			  _curIdx (_nodes.XCount ())
		{
			if (!AtEnd ())
				Advance ();
		}

		void Advance ();
		bool AtEnd () const { return _curIdx == npos; }
		Node const * GetNode () const { return _nodes.XGet (_curIdx); }
		Node * GetNodeEdit () const { return _nodes.XGetEdit (_curIdx); }
		unsigned int GetIdx () const { return _curIdx; }
		void MarkDeleted () { _nodes.XMarkDeleted (_curIdx); }
		bool Seek (unsigned int position)
		{
			if (position < _nodes.XCount ())
			{
				_curIdx = position;
				return true;
			}
			return false;
		}

	private:
		TransactableArray<Node> &	_nodes;
		unsigned int				_curIdx;
	};

	class SortedTree::UISequencer	// Iterator all nodes in the reverse chronological order, stops at node with archive bit set
	{
	public:
		UISequencer (SortedTree const & tree)
			: _nodes (tree._nodes), 
			  _curIdx (_nodes.Count () == 0 ? npos : _nodes.Count () - 1),
			  _prevIdx (_curIdx)
		{}

		void Advance ()
		{
			Assert (!AtEnd ());
			_prevIdx = _curIdx;
			if (_curIdx != 0)
				_curIdx--;
			else
				_curIdx = npos;
			Assert (_curIdx == npos || _nodes.Get (_curIdx) != 0);
		}
		bool AtEnd () const { return _curIdx == npos || _nodes.Get (_prevIdx)->IsArchive (); }
		Node const * GetNode () const { return _nodes.Get (_curIdx); }

	private:
		TransactableArray<Node> const &	_nodes;
		unsigned int					_curIdx;
		unsigned int					_prevIdx;
	};

	class SortedTree::FullSequencer	// Iterates all nodes in the reverse chronological order
	{
	public:
		FullSequencer (SortedTree const & tree, GlobalId startingId = gidInvalid);

		void Advance ();
		bool AtEnd () const { return _curIdx == npos; }
		Node const * GetNode () const { return _nodes.Get (_curIdx); }
		unsigned int GetIdx () const { return _curIdx; }

	private:
		TransactableArray<Node> const &	_nodes;
		unsigned int					_curIdx;
	};

	class SortedTree::XFullSequencer	// Iterates all nodes in the reverse chronological order		
	{
	public:
		XFullSequencer (SortedTree & tree, GlobalId startingId = gidInvalid);

		void Advance ();
		bool AtEnd () const { return _curIdx == npos; }
		Node const * GetNode () const { return _nodes.XGet (_curIdx); }
		Node * GetNodeEdit () const { return _nodes.XGetEdit (_curIdx); }
		unsigned int GetIdx () const { return _curIdx; }

	private:
		TransactableArray<Node> &	_nodes;
		unsigned int				_curIdx;

	};

	class SortedTree::XImportInserter
	{
	public:
		XImportInserter (SortedTree & tree)
			: _nodes (tree._nodes), 
			  _originalCount (_nodes.XCount ())
		{
			Assert (_originalCount > 0);
		}

		void push_back (std::unique_ptr<Node> newNode) { _nodes.XAppend (std::move(newNode)); }
		void Finalize ();

	private:
		TransactableArray<Node> &	_nodes;
		unsigned int				_originalCount;
	};

	class SortedTree::XPredecessorSequencer
	{
	public:
		XPredecessorSequencer (SortedTree const & tree, unsigned int startIdx)
			: _nodes (tree._nodes), 
			  _curIdx (startIdx),
			  _nextPredecessorId (_nodes.XGet (_curIdx)->GetPredecessorId ())
		{
			Advance ();
		}

		void Advance ();
		bool AtEnd () const { return _curIdx == npos; }

		Node const * GetNode () const { return _nodes.XGet (_curIdx); }
		unsigned int GetIdx () const { return _curIdx; }

	private:
		TransactableArray<Node> const &	_nodes;
		unsigned int				_curIdx;
		GlobalId					_nextPredecessorId;
	};

	class SortedTree::PredecessorSequencer
	{
	public:
		PredecessorSequencer (SortedTree const & tree, GlobalId startScript);

		void Advance ();
		bool AtEnd () const { return _curIdx == 0; }

		Node const * GetNode () const { return _nodes.Get (_curIdx); }

	private:
		TransactableArray<Node> const &	_nodes;
		unsigned int					_curIdx;
		GlobalId						_nextPredecessorId;
	};
}

#endif
