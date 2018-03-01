//----------------------------------
// (c) Reliable Software 2004 - 2007
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "HistoryTraversal.h"
#include "ScriptHeader.h"

namespace History
{
	void XTraverseTree (SortedTree::XFwdUnitSequencer & seq, Gatherer & gatherer)
	{
		if (seq.AtEnd ())
			return;
		Node const * currentNode = seq.GetNode ();
		Assert (currentNode != 0);
		while (gatherer.ProcessNode (seq))
		{
			GlobalId curId = currentNode->GetScriptId ();
			if (currentNode->IsBranchPoint ())
				gatherer.Push (curId); // descend into subtree
			seq.Advance ();
			if (seq.AtEnd ())
			{
				// Pop the whole stack
				gatherer.BackTo (gidInvalid);
				break;
			}
			currentNode = seq.GetNode ();
			Assert (currentNode != 0);
			GlobalId predecessorId = currentNode->GetPredecessorId ();
			if (predecessorId != curId)
			{
				// This node belongs to a new branch
				if (!gatherer.BackTo (predecessorId))
					break;
			}
		}
	}

	bool SideLineageGatherer::ProcessNode (SortedTree::XFwdUnitSequencer & seq)
	{
		Node const * node = seq.GetNode ();
		Assert (node != 0);
		GlobalId scriptId = node->GetScriptId ();
		// Minimal side lineage contains only unconfirmed script ids.
		// Maximal side lineage contains all (confirmed and unconfirmed) script ids.
		if (node->IsAckListEmpty () && !node->IsMissing () && _sideLineageType == UnitLineage::Minimal)
		{
			// For minimal side lineages remember last reference script id (last confirmed script id).
			_lastReferenceId = scriptId;
		}
		else
		{
			if (!_stack.empty ())
				_stack.back ().push_back (scriptId);
			else
				_stem.push_back (scriptId);
		}

		return true;
	}

	void SideLineageGatherer::Push (GlobalId branchPtId)
	{
		_stack.push_back (Segment (branchPtId, _lastReferenceId == branchPtId));
	}

	bool SideLineageGatherer::BackTo (GlobalId branchId)
	{
		Lineage lin;
		lin.PushId (_lastReferenceId);
		lin.Append (_stem.begin (), _stem.end ());
		Accumulate (lin);
		if (lin.Count () > 1)
		{
			std::unique_ptr<UnitLineage> sideLin (new UnitLineage (lin, _unitType, _unitId));
			// Sanity check: can't have equal scriptIds following each other
			Lineage::Sequencer seq (*sideLin);
			GlobalId prevId = seq.GetScriptId ();
			seq.Advance ();
			while (!seq.AtEnd ())
			{
				Assume (seq.GetScriptId () != prevId, GlobalIdPack (prevId).ToBracketedString ().c_str ());
				prevId = seq.GetScriptId ();
				seq.Advance ();
			}
			_hdr.AddSideLineage (std::move(sideLin));
		}
		while (!_stack.empty () && _stack.back ().GetBranchPtId () != branchId)
		{
			_stack.pop_back ();
		}
		if (_stack.empty ())
			return false;
		_stack.back ().clear ();
		if (_stack.back ().IsBranchPtConfirmed ())
			_lastReferenceId = _stack.back ().GetBranchPtId ();
		return true;
	}

	void SideLineageGatherer::Accumulate (Lineage & lin)
	{
		std::vector<Segment>::const_iterator stackIt = _stack.begin ();
		std::vector<Segment>::const_iterator stackEnd = _stack.end ();
		while (stackIt != stackEnd)
		{
			lin.Append (stackIt->begin (), stackIt->end ());
			++stackIt;
		}
	}

	RepairGatherer::RepairGatherer (SortedTree & tree) 
		: _tree (tree),
		  _doRemove (false),
		  _treeChanged (false)
	{
		// this is the stem
		_stack.push_back (Segment (gidInvalid, false));
	}

	bool RepairGatherer::ProcessNode (SortedTree::XFwdUnitSequencer & seq)
	{
		Node const * node = seq.GetNode ();
		Assert (node != 0);
		dbg << "RepairGatherer::ProcessNode: " << *node << std::endl;
		GlobalId scriptId = node->GetScriptId ();

		GidSet::const_iterator iter = _visited.find (scriptId);
		if (iter != _visited.end ())
		{
			dbg << "Duplicate node found" << std::endl;
			// Duplicate node found
			if (!_stack.back ().IsEmpty ())
			{
				// Duplicate inside the current segment
				Push (node->GetPredecessorId ());
			}
			_doRemove = true;
			_stack.back ().SetDoRemove (true);
		}

		if (!_doRemove)
			_visited.insert (scriptId);

		_stack.back ().push_back (seq.GetIdx ());
		return true;
	}

	void RepairGatherer::Push (GlobalId branchPtId)
	{
		bool doRemove = _stack.back ().IsDoRemove ();
		_stack.push_back (Segment (branchPtId, doRemove));
		dbg << "RepairGatherer::Push - branch id = " << GlobalIdPack (branchPtId).ToString () << "; do remove = " << (doRemove ? "true" : "false") << std::endl;
	}

	class IsEqualBranchId : public std::unary_function<RepairGatherer::Segment, bool>
	{
	public:
		IsEqualBranchId (GlobalId gid)
			: _branchId (gid)
		{}

		bool operator() (RepairGatherer::Segment & segment) const
		{
			return segment.GetBranchPtId () == _branchId;
		}
	private:
		GlobalId	_branchId;
	};

	bool RepairGatherer::BackTo (GlobalId branchId)
	{
		dbg << "RepairGatherer::BackTo - branch id = " << GlobalIdPack (branchId).ToString () << std::endl;
		std::vector<Segment>::const_iterator iter = 
			std::find_if (_stack.begin (), _stack.end (), IsEqualBranchId (branchId));
		Assert (branchId != gidInvalid || iter != _stack.end ());
		if (iter == _stack.end ())
			return true;	// Branch id not found among segments on the stack

		while (!_stack.empty () && _stack.back ().GetBranchPtId () != branchId)
		{
			if (_stack.back ().IsDoRemove ())
			{
				// Remove all segment scripts
				for (IdxIterator iter = _stack.back ().begin ();
					iter != _stack.back ().end ();
					++iter)
				{
					Node const * node = _tree.XGetNode (*iter);
					dbg << "Deleting node: " << *node << std::endl; 
					_tree.XDeleteNodeByIdx (*iter);
				}
				_treeChanged = true;
			}
			else
			{
				_doRemove = false;
			}
			_stack.pop_back ();
		}
		if (branchId == gidInvalid)
			return false;

		_stack.back ().clear ();
		return true;
	}
}
