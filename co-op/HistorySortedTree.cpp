//------------------------------------
//  (c) Reliable Software, 2003 - 2008
//------------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "HistorySortedTree.h"
#include "HistoryTraversal.h"
#include "MemoryLog.h"
#include "MailboxHelper.h"
#include "AckBox.h"

#include <Ctrl/ProgressMeter.h>
#include <Ex/Winex.h>
#include <Dbg/Out.h>

#include <iomanip>

using namespace History;

SortedTree::Sequencer::Sequencer (SortedTree const & tree, GlobalId startingId)
	: _nodes (tree._nodes), 
	  _curIdx (_nodes.Count () == 0 ? npos : _nodes.Count () - 1),
	  _firstInterestingScriptId (tree._firstInterestingScriptId.Get ()),
	  _atEnd (false)
{
	// patch earlier bug
	if (tree.GetUnitType () == Unit::Member)
		_firstInterestingScriptId = 0;
	if (_curIdx == npos)
		_atEnd = true;
	else if (startingId != gidInvalid)
	{
		while (!AtEnd () && GetNode ()->GetScriptId () != startingId)
			Advance ();
	}
}

void SortedTree::Sequencer::Advance ()
{
	Assert (!AtEnd ());
	if (_curIdx == 0)
	{
		_atEnd = true;
	}
	else
	{
		Node const * node = GetNode ();
		Assert (node != 0);
		if (node->GetScriptId () == _firstInterestingScriptId)
			_atEnd = true;
		else
			--_curIdx;
	}
}

SortedTree::XSequencer::XSequencer (SortedTree & tree, GlobalId startingId)
	: _nodes (tree._nodes),
	  _curIdx (_nodes.XCount () == 0 ? npos : _nodes.XCount () - 1),
	  _firstInterestingScriptId (tree._firstInterestingScriptId.XGet ()),
	  _atEnd (false)
{
	// patch earlier bug
	if (tree.GetUnitType () == Unit::Member)
		_firstInterestingScriptId = 0;
	Assert (_curIdx != npos);
	SkipDeleted ();

	if (startingId != gidInvalid)
	{
		while (!AtEnd () && GetNode ()->GetScriptId () != startingId)
			Advance ();
	}
}

void SortedTree::XSequencer::Advance ()
{
	Assert (!AtEnd ());
	Node const * node = 0;
	if (_curIdx == 0)
	{
		_atEnd = true;
	}
	else
	{
		node = GetNode ();
		if (node != 0 && node->GetScriptId () == _firstInterestingScriptId)
		{
			_atEnd = true;
		}
		else
		{
			--_curIdx;
			SkipDeleted ();
		}
	}
	Assert (AtEnd () || node != 0);
}

void SortedTree::XSequencer::SkipDeleted ()
{
	// Skip deleted nodes
	for ( ;; )
	{
		if (_curIdx == npos)
		{
			_atEnd = true;
			break;
		}

		if (GetNode () != 0)
			break;
		--_curIdx;
	}
}

void SortedTree::UnitSequencer::Advance ()
{
	Assert (!AtEnd ());
    Node const * node = nullptr;
	if (_curIdx == 0)
	{
		_atEnd = true;
	}
	else
	{
		node = GetNode ();
		if (node->GetScriptId () == _firstInterestingScriptId)
		{
			Assert (node->IsChanging (_unitId));
			_atEnd = true;
		}
		else
		{
			Skip ();
		}
	}
    Assert(AtEnd() || node != nullptr && node->IsChanging(_unitId));
}

void SortedTree::UnitSequencer::Skip ()
{
    Node const * node = nullptr;
	for ( ;; )
	{
		if (_curIdx == 0)
		{
			_atEnd = true;
			break;
		}
		--_curIdx;

		node = GetNode ();
		if (node->IsChanging (_unitId))
			break;

		if (node->GetScriptId () == _firstInterestingScriptId)
		{
			_atEnd = true;
			break;
		}
	}
    Assert(AtEnd() || node != nullptr && node->IsChanging(_unitId));
}

void SortedTree::XUnitSequencer::Advance ()
{
	Assert (!AtEnd ());
    Node const * node = nullptr;
	if (_curIdx == 0)
	{
		_atEnd = true;
	}
	else
	{
		node = GetNode ();
		if (node != 0 && node->GetScriptId () == _firstInterestingScriptId)
		{
			Assert (node->IsChanging (_unitId));
			_atEnd = true;
		}
		else
		{
			Skip ();
		}
	}
    Assert(AtEnd() || node != nullptr && node->IsChanging(_unitId));
}

void SortedTree::XUnitSequencer::Skip ()
{
    Node const * node = nullptr;
	for ( ;; )
	{
		if (_curIdx == 0)
		{
			_atEnd = true;
			break;
		}
		--_curIdx;

		node = GetNode ();
        if (node == nullptr)
			continue;	// Skip deleted nodes

		if (node->IsChanging (_unitId))
			break;

		if (node->GetScriptId () == _firstInterestingScriptId)
		{
			_atEnd = true;
			break;
		}
	}
    Assert(AtEnd() || node != nullptr && node->IsChanging(_unitId));
}

void SortedTree::XUnitSequencer::Seek (GlobalId scriptId)
{
	while (!AtEnd () && GetNode ()->GetScriptId () != scriptId)
		Advance ();
}

SortedTree::FwdUnitSequencer::FwdUnitSequencer (SortedTree const & tree,
												GlobalId startScriptId,
												GlobalId unitId)
	: _nodes (tree._nodes), 
	  _curIdx (npos),
	  _unitId (unitId)
{
	if (startScriptId == gidInvalid)
	{
		// Find first, in chronological order, script changing given unit id
		Advance ();
	}
	else
	{
		// Find given script going in reverse chronological order
		_curIdx = tree.FindIdx (startScriptId);
		if (_curIdx == npos)
			_curIdx = _nodes.Count ();	// Start script not found -- sequencer is at end
	}
}

void SortedTree::FwdUnitSequencer::Advance ()
{
	Assert (!AtEnd ());
	// Let the note decide if we are skipping
    Node const * node = nullptr;
	if (_curIdx == npos)
		_curIdx = 0;
	else
		++_curIdx;
	while (!AtEnd ())
	{
		node = _nodes.Get (_curIdx);
		Assert (node != 0);
		if (node->IsChanging (_unitId))
			break;
		++_curIdx;
	}
    Assert(AtEnd() || node != nullptr && node->IsChanging(_unitId));
}

SortedTree::XFwdUnitSequencer::XFwdUnitSequencer (SortedTree & tree, GlobalId startScriptId, GlobalId unitId)
	: _nodes (tree._nodes),
	  _curIdx (npos),
	  _unitId (unitId),
	  _isTreeEmpty (false)
{
	// Find first, in chronological order, script changing given unit id
	Advance ();
	if (AtEnd ())
	{
		_isTreeEmpty = true;
	}
	else if (startScriptId != gidInvalid)
	{
		while (!AtEnd () && GetNode ()->GetScriptId () != startScriptId)
			Advance ();
	}
	Assert (AtEnd () || GetNode ()->IsChanging (_unitId) && (GetNode ()->GetScriptId () == startScriptId || startScriptId == gidInvalid));
}

// Preconditions: seq not at end
// All scripts in the seq modify the same unitId
SortedTree::XFwdUnitSequencer::XFwdUnitSequencer (SortedTree & tree, UnitLineage::Sequencer & seq)
	: _nodes (tree._nodes),
	  _curIdx (npos),
	  _unitId (seq.GetUnitId ()),
	  _isTreeEmpty (false)
{
	Assert (!seq.AtEnd ());
	// Find first, in chronological order, script changing given unit id
	Advance ();
	if (AtEnd ())
	{
		_isTreeEmpty = true;
	}
	else
	{
		// Go through lineage script ids and find the first match in the tree
		do
		{
			// Try this one
			GlobalId scriptId = seq.GetScriptId ();
			// Maybe we're lucky?
			if (AtEnd () || GetNode ()->GetScriptId () == scriptId)
			{
				break;
			}
			else
			{
				// Position a new sequencer at our starting point
				// and try to find the script id
				SortedTree::XFwdUnitSequencer copySeq (*this);
				do
				{
					copySeq.Advance ();
				} while (!copySeq.AtEnd () && copySeq.GetNode ()->GetScriptId () != scriptId);

				if (!copySeq.AtEnd ())
				{
					 // found it!
					_curIdx = copySeq.GetIdx ();
					break;
				}
			}
			// try next script id from the lineage
			seq.Advance ();
		} while (!seq.AtEnd ());

		if (seq.AtEnd ())
			_curIdx = _nodes.XCount ();	// No lineage script ids present in the tree
	}
	Assert (AtEnd () || (GetNode ()->IsChanging (_unitId) && seq.GetScriptId () == GetNode ()->GetScriptId ()));
}

void SortedTree::XFwdUnitSequencer::Advance ()
{
	Assert (!AtEnd ());
	// Let the note decide if we are skipping
    Node const * node = nullptr;
	if (_curIdx == npos)
		_curIdx = 0;
	else
		++_curIdx;
	while (!AtEnd ())
	{
		node = _nodes.XGet (_curIdx);
		if (node != 0 && node->IsChanging (_unitId))
			break;
		++_curIdx;
	}
    Assert(AtEnd() || node != nullptr && node->IsChanging(_unitId));
}

void SortedTree::XWriteSequencer::Advance ()
{
	Assert (!AtEnd ());
	Node const * node = 0;
	// Skip deleted nodes
	--_curIdx;
	while (!AtEnd ())
	{
		node = _nodes.XGet (_curIdx);
		if (node != 0)
			break;
		--_curIdx;
	}
	Assert (AtEnd () || node != 0);
}

SortedTree::FullSequencer::FullSequencer (SortedTree const & tree, GlobalId startingId)
	: _nodes (tree._nodes),
	  _curIdx (_nodes.Count ())
{
	Advance ();
	if (startingId != gidInvalid)
	{
		while (!AtEnd () && GetNode ()->GetScriptId () != startingId)
			Advance ();
	}
}

void SortedTree::FullSequencer::Advance ()
{
	Assert (!AtEnd ());
	if (_curIdx == 0)
	{
		_curIdx = npos;
		Assert (AtEnd ());
	}
	else
	{
		--_curIdx;
		Assert (_nodes.Get (_curIdx) != 0);
	}
}

SortedTree::XFullSequencer::XFullSequencer (SortedTree & tree, GlobalId startingId)
	: _nodes (tree._nodes), 
	  _curIdx (_nodes.XCount ())
{
	Advance ();
	if (startingId != gidInvalid)
	{
		while (!AtEnd () && GetNode ()->GetScriptId () != startingId)
			Advance ();
	}
}

void SortedTree::XFullSequencer::Advance ()
{
	Assert (!AtEnd ());
	Node const * node = 0;
	if (_curIdx == 0)
		_curIdx = npos;
	else
		--_curIdx;
	// Skip deleted nodes
	while (!AtEnd ())
	{
		node = _nodes.XGet (_curIdx);
		if (node != 0)
			break;
		--_curIdx;
	}
	Assert (AtEnd () || node != 0);
}

void SortedTree::XPredecessorSequencer::Advance ()
{
	Assert (!AtEnd ());
	if (_nextPredecessorId == gidInvalid)
	{
		// No more predecessors -- sequencer is at end
		_curIdx = npos;
		return;
	}
	Node const * node = 0;
	// Skip deleted nodes
	--_curIdx;
	while (!AtEnd ())
	{
		node = _nodes.XGet (_curIdx);
		if (node != 0 && node->GetScriptId () == _nextPredecessorId)
		{
			_nextPredecessorId = node->GetPredecessorId ();
			break;
		}
		--_curIdx;
	}
}

SortedTree::PredecessorSequencer::PredecessorSequencer (SortedTree const & tree, GlobalId startScript)
	: _nodes (tree._nodes),
	  _curIdx (tree._nodes.Count ()),
	  _nextPredecessorId (gidInvalid)
{
	Assert (!AtEnd ());
	Assert (startScript != gidInvalid);
	--_curIdx;
    Node const * node = nullptr;
	// Find starting script in the tree
	while (!AtEnd ())
	{
		node = _nodes.Get (_curIdx);
		Assert (node != 0);
		if (node->GetScriptId () == startScript)
			break;
		--_curIdx;
	}
    Assert(AtEnd() || node != nullptr && node->GetScriptId() == startScript);

	if (!AtEnd ())
	{
		Assert (node != 0);
		_nextPredecessorId = node->GetPredecessorId ();
	}
	Assert (AtEnd () || (node->GetScriptId () == startScript && _nextPredecessorId != gidInvalid));
}

void SortedTree::PredecessorSequencer::Advance ()
{
	Assert (!AtEnd ());
	--_curIdx;
	while (!AtEnd ())
	{
		Node const * node = _nodes.Get (_curIdx);
		Assert (node != 0);
		if (node->GetScriptId () == _nextPredecessorId)
		{
			_nextPredecessorId = node->GetPredecessorId ();
			break;
		}
		--_curIdx;
	}
	Assert (AtEnd () || _nextPredecessorId != gidInvalid || _curIdx == 1);
}

void SortedTree::Serialize (Serializer & out) const
{
	_nodes.Serialize (out);
	_firstInterestingScriptId.Serialize (out);
}

void SortedTree::Deserialize (Deserializer & in, int version)
{
	_nodes.Deserialize (in, version);
	if (version > 41)
		_firstInterestingScriptId.Deserialize (in, version);
	if (version < 50)
	{
		// Convert tree index to script id
		unsigned int curIdx = _firstInterestingScriptId.XGet ();
		_firstInterestingScriptId.XSet (gidMarker);
		if (0 < curIdx && curIdx < _nodes.XCount ())
		{
			Node const * node = _nodes.XGet (curIdx);
			if (node->IsAckListEmpty () && (node->IsExecuted () || node->IsRejected ()))
				_firstInterestingScriptId.XSet (node->GetScriptId ());
		}
	}
	else if (_firstInterestingScriptId.XGet () == gidInvalid)
	{
		_firstInterestingScriptId.XSet (gidMarker);
	}
}

bool SortedTree::HasDuplicates () const
{
	std::set<GlobalId> scriptSet;
	for (TransactableArray<Node>::const_iterator it = _nodes.begin (); it != _nodes.end (); ++it)
	{
		std::set<GlobalId>::const_iterator setIt = scriptSet.find ((*it)->GetScriptId ());
		if (setIt != scriptSet.end ())
			return true;
		scriptSet.insert ((*it)->GetScriptId ());
	}
	return false;
}

bool SortedTree::IsCreationMarkerLinked() const
{
	TransactableArray<Node>::const_iterator it = _nodes.begin (); 
	Assume (it != _nodes.end (), "History tree is empty");
	Node const * firstNode = *it;

	if (!firstNode->IsMilestone()) // missing creation marker
		return false; 

	GlobalId firstId = firstNode->GetScriptId();
	++it;
	if (it != _nodes.end())
	{
		Node const * secondNode = *it;
		GlobalId predId = secondNode->GetPredecessorId();
		if (predId != firstId)
			return false;
	}
	// else: nothing to be linked to, we're fine
	return true;
}

void SortedTree::GetMissingScripts (Unit::ScriptList & missingScripts) const
{
	for (Sequencer seq (*this); !seq.AtEnd (); seq.Advance ())
	{
		Node const * node = seq.GetNode ();
		if (node->IsMissing () && node->CanRequestResend ())
			missingScripts.push_back (Unit::ScriptId (node->GetScriptId (), _unitType));
	}
}

bool SortedTree::HasMissingScripts () const
{
	for (Sequencer seq (*this); !seq.AtEnd (); seq.Advance ())
	{
		Node const * node = seq.GetNode ();
		if (node->IsMissing () && node->CanRequestResend())
			return true;
	}
	return false;
}


class XIsArchiveMarker : public std::unary_function<Node const *, bool>
{
public:
	bool operator () (Node const * node) const
	{
		return node != 0 ? node->IsArchive () : false;
	}
};

bool SortedTree::XHasArchiveMarkers () const
{
	TransactableArray<Node>::const_iterator iter = std::find_if (_nodes.xbegin (), _nodes.xend (), XIsArchiveMarker ());
	return iter != _nodes.xend ();
}

bool SortedTree::CanArchive (GlobalId scriptId) const
{
	// Position Sequencer at scriptId. Sequencer stops at FIS marker.
	// If Sequencer is at end then we can archive scriptId (this
	// script is older then the FIS marker).
	Sequencer seq (*this, scriptId);
	return seq.AtEnd ();
}

bool SortedTree::IsTreeRoot (GlobalId scriptId) const
{
	Node const * node = FindNode (scriptId);
	if (node != 0)
	{
		return node->GetPredecessorId () == gidInvalid;
	}
	return false;
}

GlobalId SortedTree::GetFirstInterestingScriptId () const
{
	GlobalId fis = _firstInterestingScriptId.Get ();
	return fis == gidMarker ? gidInvalid : fis;
}

GlobalId SortedTree::XGetFirstInterestingScriptId () const
{
	GlobalId fis = _firstInterestingScriptId.XGet ();
	return fis == gidMarker ? gidInvalid : fis;
}

class IsEqualId : public std::unary_function<Node const *, bool>
{
public:
	IsEqualId (GlobalId gid)
		: _gid (gid)
	{}
	bool operator () (Node const * node) const
	{
		return node->GetScriptId () == _gid;
	}
private:
	GlobalId	_gid;
};

unsigned int SortedTree::FindIdx (GlobalId scriptId) const
{
	TransactableArray<Node>::const_iterator iter = std::find_if (_nodes.begin (), _nodes.end (), IsEqualId (scriptId));
	if (iter != _nodes.end ())
	{
		return iter - _nodes.begin ();
	}
	return npos;
}

class XIsEqualId : public std::unary_function<Node const *, bool>
{
public:
	XIsEqualId (GlobalId gid)
		: _gid (gid)
	{}
	bool operator () (Node const * node) const
	{
		return node != 0 ? node->GetScriptId () == _gid : false;
	}
private:
	GlobalId	_gid;
};

unsigned int SortedTree::XFindIdx (GlobalId scriptId) const
{
	TransactableArray<Node>::const_iterator iter = std::find_if (_nodes.xbegin (), _nodes.xend (), XIsEqualId (scriptId));
	if (iter != _nodes.xend ())
	{
		return iter - _nodes.xbegin ();
	}
	return npos;
}

Node const * SortedTree::FindNode (GlobalId scriptId) const
{
	unsigned int idx = FindIdx (scriptId);
	if (idx != npos)
		return _nodes.Get (idx);

	return 0;
}

Node const * SortedTree::XFindNode (GlobalId scriptId) const
{
	unsigned int idx = XFindIdx (scriptId);
	if (idx != npos)
		return _nodes.XGet (idx);

	return 0;
}

// Lineage sequencer is positioned at root id -- first non-prehistoric script id in the lineage
// Return false if processed lineage is illegal (contains illegal script id sequence)
bool SortedTree::XProcessLineage (UnitLineage::Sequencer & lineageSeq,
								  bool & rootIdFound,
								  Mailbox::Agent & agent)
{
	dbg << "--> History::SortedTree::XProcessLineage" << std::endl;
	Assume (!lineageSeq.AtEnd (), "SortedTree::XProcessLineage");
	Assert (lineageSeq.GetUnitType () == GetUnitType ());
	GlobalId rootId = lineageSeq.GetScriptId ();
	unsigned int historyForkIdx = npos;
	// Revisit: this should always be different!
	Assert (rootId != agent.GetKnownMissingId ());
	if (rootId != agent.GetKnownMissingId ())
	{
		// Note: the XFwdUnitSequencer constructor will position lineageSeq at matching script id or at end.
		XFwdUnitSequencer treeSeq (*this, lineageSeq);
		if (treeSeq.AtEnd () && !treeSeq.IsTreeEmpty ())
		{
			// There are some scripts recorded for this unit id in the tree
			// and none of the script lineage ids found in the tree.
			dbg << "<-- History::SortedTree::XProcessLineage -- lineage doesn't connect with the tree" << std::endl;
			rootIdFound = false;	// Lineage doesn't connect with the tree -- cannot process lineage
			return true;	
		}
		else if (!treeSeq.AtEnd ())
		{
			Assert (!treeSeq.IsTreeEmpty ());
			// Some script lineage id found in the tree
			// Walk the lineage's historic part and compare it with the recorded
			// history, until one of the sequencers hits end
			rootIdFound = true;
			AckBox & ackBox = agent.GetAckBox ();
			historyForkIdx = treeSeq.GetIdx ();
			treeSeq.Advance ();
			lineageSeq.Advance ();
			while (!treeSeq.AtEnd () && !lineageSeq.AtEnd ())
			{
				GlobalId lineageScriptId = lineageSeq.GetScriptId ();
				Node const * node = treeSeq.GetNode ();
				// Skip other branches
				while (node->GetScriptId () != lineageScriptId)
				{
					dbg << "    Skipping node: " << *node << std::endl;
					treeSeq.Advance ();
					if (treeSeq.AtEnd ())
					{
						break;
					}
					node = treeSeq.GetNode ();
				}
				Assert (node->GetScriptId () == lineageScriptId || treeSeq.AtEnd ());
				if (treeSeq.AtEnd ())
					break;

				dbg << "    Matching node: " << *node << std::endl;
				GlobalIdPack nodePack (node->GetScriptId ());
				UserId scriptAuthorId = nodePack.GetUserId ();
				UserId incomingScriptSenderId = agent.GetIncomingScriptSenderId ();
				if (scriptAuthorId == agent.GetMyId ())
				{
					// I am the script author
					if (node->IsAckListEmpty ())
					{
						// In my history the script is already confirmed.
						// Re-send "make reference" to the user listing this script as tentative.
						Unit::ScriptId scriptId (node->GetScriptId (), _unitType);
						ackBox.RememberMakeRef (incomingScriptSenderId, scriptId);
						dbg << "    Sending make ref to " << std::hex << incomingScriptSenderId << std::endl;
					}
				}
				else if (!node->IsMissing ())
				{
					// Some other project member is the script author.
					// We can send acknowledgement to the script author or to the
					// user listing this script as unconfirmed.
					Unit::ScriptId scriptId (node->GetScriptId (), _unitType);
					GlobalId ackRecipientId = node->IsDefect () ? incomingScriptSenderId : scriptAuthorId;
					ackBox.RememberAck (ackRecipientId, scriptId, false);	// Can send ack, but don't have to
					dbg << "    Can send ack to " << std::hex << ackRecipientId << std::endl;
				}
				historyForkIdx = treeSeq.GetIdx ();
				lineageSeq.Advance ();
				treeSeq.Advance ();
			}
			Assert (treeSeq.AtEnd () || lineageSeq.AtEnd ());
		}
		else
		{
			// Logical unit tree is empty.
			rootIdFound = false;
			return true;
		}

		Assert (lineageSeq.AtEnd () || historyForkIdx != npos);
		// Insert missing script placeholders
		bool canRequestResend = _policy->XCanRequestResend (lineageSeq.GetUnitId ());
		while (!lineageSeq.AtEnd ())
		{
			GlobalIdPack lineageScriptId = lineageSeq.GetScriptId ();
			GlobalIdPack predecessorId = _nodes.XGet (historyForkIdx)->GetScriptId ();
			if (lineageScriptId == predecessorId)
				return false; // Somebody sent us a bad lineage!

			UserId missingScriptAuthorId = GlobalIdPack (lineageScriptId).GetUserId ();
			if (missingScriptAuthorId == agent.GetMyId ())
				return false; // Somebody sent us a bad lineage!

			dbg << "    Inserting missing script (lineage): " << lineageScriptId << " (" << predecessorId << ")" << std::endl;
			std::unique_ptr<MissingNode> missingNode (
				new MissingNode (lineageScriptId, predecessorId, lineageSeq.GetUnitId (), agent, canRequestResend));
			historyForkIdx = XInsertNode (std::move(missingNode));
			lineageSeq.Advance ();
		}
	}

	// We know the id of the missing script when we receive its chunk
	if (agent.HasKnownMissingId ())
	{
		Node const * node = XFindNode (agent.GetKnownMissingId ());
		if (node == 0)
		{
			unsigned int predecessorIdx = historyForkIdx == npos ? 0 : historyForkIdx;
			GlobalIdPack predecessorId = _nodes.XGet (predecessorIdx)->GetScriptId ();
			dbg << "    Inserting missing script (chunk): " << GlobalIdPack (agent.GetKnownMissingId ()) << " (" << predecessorId << ")" << std::endl;
			bool canRequestResend = _policy->XCanRequestResend (agent.GetKnownMissingUnitId ());
			std::unique_ptr<MissingNode> missingNode (new MissingNode (predecessorId, agent, canRequestResend));
			XInsertNode (std::move(missingNode));
		}
	}
	dbg << "<-- History::SortedTree::XProcessLineage" << std::endl;
	return true;
}

#if !defined (NDEBUG)
class XIsValidGlobalId : public std::unary_function<Node const *, bool>
{
public:
	bool operator () (Node const * node) const
	{
		return node != 0 ? node->GetScriptId () != gidInvalid : false;
	}
};
#endif

// Returns true when inserted node doesn't have missing predecessors
bool SortedTree::XInsertNewNode (std::unique_ptr<Node> newNode)
{
	if (newNode->GetScriptId () == gidInvalid)
	{
		// Node with script id set to gidInvalid is appended at tree end
		_nodes.XAppend (std::move(newNode));
		return true;
	}

	Assert (newNode->GetScriptId () != gidInvalid);
	Assert (XFindNode (newNode->GetScriptId ()) == 0);
	GlobalId predecessorId = newNode->GetPredecessorId ();
	if (predecessorId == gidInvalid)
	{
		// First node in the tree or defect script (both don't specify predecessor id)
		if (_nodes.XCount () == 0)
		{
			// The first node in the tree doesn't have predecessor
			_nodes.XAppend (std::move(newNode));
			return true;
		}
		else
		{
			// Tree is not empty. Position unit sequencer on the last node changing given unit id.
			// Iterates nodes of a given unit in reverse chronological order, stops at FIS marker node
			XUnitSequencer seq (*this, newNode->GetUnitId (_unitType));
			// Skip temporary nodes (nodes with gidInvalid script ids).
			while (!seq.AtEnd ())
			{
				Node const * node = seq.GetNode ();
				if (node->GetScriptId () != gidInvalid)
					break;

				seq.Advance ();
			}

			if (seq.AtEnd ())
			{
				// The first node in the tree for this unit id doesn't have predecessor
				_nodes.XAppend (std::move(newNode));
				return true;
			}
			else if (_policy->XCanTerminateTree (*newNode))
			{
				// Terminating tree means attaching node to the tree end even when the new node
				// doesn't specify its predecessor
				Node const * lastBranchNode = seq.GetNode ();
				newNode->SetPredecessorId (lastBranchNode->GetScriptId ());
				// Continue inserting the new node
			}
			else
			{
				// New node doesn't specify its predecessor and the tree is not empty
				// and policy doesn't allow for the tree termination - we cannot insert such a node.
				std::ostringstream out;
				out << *newNode;
				out << "; unit id: " << std::hex << newNode->GetUnitId (_unitType) << std::endl;
				out << "Tree node: " << *seq.GetNode ();
				throw Win::InternalException ("Illegal script insertion in the history. "
											  "Please, contact support@relisoft.com",
											  out.str ().c_str ());
			}
		}
	}

	Assert (newNode->GetPredecessorId () != gidInvalid);
	unsigned int newNodeIdx = XInsertNode (std::move(newNode));
	Assume (newNodeIdx != 0, GlobalIdPack (newNode->GetScriptId ()).ToBracketedString ().c_str ());

	if (_scriptKind.IsMergeable ())
	{
		// Tree contains mergeable scripts.
		Node const * newNode = _nodes.XGet (newNodeIdx);
		if (newNode->IsRejected ())
			XForceExecution (newNodeIdx);
	}

	return !XHasMissingPredecessors (newNodeIdx);
}

// Stops when the sequencer is positioned at point of insertion
// or AtEnd, when new node should be appended to the three
class InsertGatherer: public Gatherer
{
public:
	InsertGatherer (GlobalId newScriptId)
		: _parentNode (0),
		  _makeBranchingPoint (false),
		  _keepRejecting (false),
		  _keepSkipping (false)
	{
		GlobalIdPack thisId (newScriptId);
		_userId = thisId.GetUserId (); // for priority comparison
	}
	bool ProcessNode (SortedTree::XFwdUnitSequencer & seq)
	{
		if (_keepSkipping)
			return true;

		if (_keepRejecting)
		{
			Reject (seq);
			return true;
		}

		if (_parentNode == 0) // we must be processing the parent node
		{
			_parentNode = seq.GetNodeEdit ();
			if (!_parentNode->IsBranchPoint ())
				_makeBranchingPoint = true; // but only if more nodes follow
			return true;
		}
		else if (_makeBranchingPoint) // we must be processing second node
		{
			// there is at least one node after parent
			// so parent must be marked a branching point
			_makeBranchingPoint = false;
			_parentNode->SetBranchPoint (true);
			_stack.push_back (_parentNode->GetScriptId ());
		}

		// this code is only executed for beginnings of branches stemming from _parentNode
		Assert (!_keepRejecting && !_keepSkipping);
		Node const * node = seq.GetNode ();
		if (node->IsHigherPriority (_userId)) // new script loses
		{
			return false; // insert the loser right where we are!
		}
		// our script wins
		Assert (!node->IsHigherPriority (_userId));
		if (node->IsRejected () || node->IsToBeRejected ())
		{
			_keepSkipping = true;
		}
		else
		{
			// we hit the main branch
			_keepRejecting = true; // reject the whole tree
			Reject (seq);
		}
		Assert (_keepRejecting || _keepSkipping);
		return true;
	}
	void Push (GlobalId branchId)
	{
		_stack.push_back (branchId);
	}
	bool BackTo (GlobalId branchId)
	{
		while (_stack.size () != 0 && _stack.back () != branchId)
		{
			_stack.pop_back ();
		}
		if (_stack.size () == 0)
			return false;
		if (_stack.size () == 1) // we are at the original branching point
			_keepSkipping = false;
		return true;
	}

	bool ConflictDetected () const { return _keepRejecting; }

private:
	void Reject (SortedTree::XFwdUnitSequencer & seq)
	{
		Node * node = seq.GetNodeEdit ();
		node->SetRejected (true);
	}

private:
	Node *		_parentNode;
	bool		_makeBranchingPoint;
	bool		_keepRejecting;
	bool		_keepSkipping;
	GlobalId	_userId; // user ID of the inserted
	std::vector<GlobalId> _stack;
};

// Returns newNode index; skips rejected nodes
unsigned int SortedTree::XInsertNode (std::unique_ptr<Node> newNode)
{
	GlobalId unitId = newNode->GetUnitId (_unitType);
	if (_unitType == Unit::Member)
	{
		// Make sure the node is not already there (corruption of lineages)
		XFwdUnitSequencer seq (*this, newNode->GetScriptId (), unitId);
		if (!seq.AtEnd ())
			_nodes.XMarkDeleted (seq.GetIdx ());
	}
	XFwdUnitSequencer treeSeq (*this, newNode->GetPredecessorId (), unitId);
	Assume (!treeSeq.AtEnd (), GlobalIdPack (newNode->GetPredecessorId ()).ToBracketedString ().c_str ());
	InsertGatherer insertionPointFinder (newNode->GetScriptId ());
	XTraverseTree (treeSeq, insertionPointFinder);
	unsigned int insertionIdx = npos;
	// If the insertion point is in the middle of history,
	// the script must be rejected (the winning branch always follows rejected ones)
	if (treeSeq.AtEnd ())
	{
		// Append new node to the current active branch
		_nodes.XAppend (std::move(newNode));
		insertionIdx = _nodes.XCount () - 1;
		GlobalId fis = XGetFirstInterestingScriptId ();
		if (insertionPointFinder.ConflictDetected () && fis != 0)
		{
			// Verify FIS marker -- FIS marker has to point to the confirmed executed script.
			// In rare situations it is possible for the observer turning to voting member, to check-in
			// script that will reject already confirmed part of the history (observer didn't receive a lot of scripts).
			// We have to check if current FIS marker points to the executed confirmed script. If not,
			// then we have to move FIS back in time to the first executed confirmed script.
			XSetFirstInterestingScriptId (0); // so that the sequencer doesn't stop immediately
			XUnitSequencer seq (*this, unitId);
			Node const * node = seq.GetNode ();
			while (!node->IsExecuted () || !node->IsAckListEmpty ())
			{
				seq.Advance ();
				if (seq.AtEnd ())
				{
					Win::ClearError ();
					throw Win::Exception ("Your history is corrupted. Contact support@relisoft.com.");
				}
				node = seq.GetNode ();
			}
			Assert (node->IsExecuted () && node->IsAckListEmpty ());
			XSetFirstInterestingScriptId (node->GetScriptId ());
		}
		Assume (insertionIdx != npos, ::ToString (_nodes.XCount ()).c_str ());
	}
	else
	{
		// Insert new node in the rejected branch
		newNode->SetRejected (true);
		Assert (XFindNode (newNode->GetPredecessorId ())->IsBranchPoint () ||
				XFindNode (newNode->GetPredecessorId ())->IsRejected ()    ||
				XFindNode (newNode->GetPredecessorId ())->IsToBeRejected ());
		insertionIdx = treeSeq.GetIdx ();
		Assume (insertionIdx != npos, GlobalIdPack (newNode->GetScriptId ()).ToBracketedString ().c_str ());
		_nodes.XInsert (insertionIdx, std::move(newNode));
	}
	return insertionIdx;
}

void SortedTree::XForceInsert (std::unique_ptr<Node> newNode)
{
	GlobalId predecessorId = newNode->GetPredecessorId ();
	// GlobalId thisScriptId = newNode->GetScriptId ();
	unsigned idx = 0;
	if (predecessorId != gidInvalid)
	{
		idx = XFindIdx (predecessorId);
		Assume (idx != npos, "Inserting history node: predecessor not found!");
		++idx;
	}
	_nodes.XInsert (idx, std::move(newNode));
}

void SortedTree::XMarkDeepFork (GlobalId scriptId)
{
	unsigned idx = XFindIdx (scriptId);
	Assume (idx != npos, GlobalIdPack (scriptId).ToBracketedString ().c_str ());
	Node * node = _nodes.XGetEdit (idx);
	node->SetDeepFork (true);
}

// May change first interesting script id (FIS marker)
void SortedTree::XConfirmAllOlderNodes (XSequencer & seq)
{
	if (seq.AtEnd ())
		return;

	seq.Advance ();
	while (!seq.AtEnd ())
	{
		Node const * node = seq.GetNode ();
		if (node->IsAckListEmpty ())
			break;	// Stop at first confirmed node changing the same unit

		Node * editedNode = seq.GetNodeEdit ();
		editedNode->ClearAckList ();
		seq.Advance ();
	}
	Assert (seq.AtEnd () || seq.GetNode ()->IsAckListEmpty ());

	// Set _firstInterestingScriptId. Start from the current first interesting script id
	// and iterate history froward until we find node in one of the following states:
	//	- not confirmed (acknowledgment list is not empty)
	//	- missing
	//	- candidate for execution
	//	- to be rejected
	GlobalId candidateForFis = gidInvalid;
	GlobalId startId = XGetFirstInterestingScriptId ();
	XFwdUnitSequencer searchSeq (*this, startId);
	Assert (!searchSeq.AtEnd ());
	
	for (searchSeq.Advance (); !searchSeq.AtEnd (); searchSeq.Advance ())
	{
		Node const * node = searchSeq.GetNode ();
		if (IsCandidateForFis (node))
		{
			Assert (!node->IsMissing ());
			candidateForFis = node->GetScriptId ();
		}
		else if (NoMoreCandidatesForFis (node))
		{
			break;	// Stop at first unconfirmed, missing, incoming or to be rejected node
		}
	}
	if (candidateForFis != gidInvalid)
		_firstInterestingScriptId.XSet (candidateForFis);
}

// Doesn't change the first interesting script id (FIS marker)
void SortedTree::XConfirmOlderPredecessors (XSequencer const & startSeq)
{
	std::vector<unsigned int> unconfirmedPredecessors;
	for (XPredecessorSequencer seq (*this, startSeq.GetIdx ()); !seq.AtEnd (); seq.Advance ())
	{
		Node const * node = seq.GetNode ();
		if (node->IsAckListEmpty ())
			break;	// Stop at first confirmed predecessor

		unconfirmedPredecessors.push_back (seq.GetIdx ());
	}

	for (std::vector<unsigned int>::const_iterator iter = unconfirmedPredecessors.begin (); iter != unconfirmedPredecessors.end (); ++iter)
	{
		unsigned int idx = *iter;
		Node * node = _nodes.XGetEdit (idx);
		node->ClearAckList ();
	}
}

// Returns true if there are missing scripts in the branch before startIdx
bool SortedTree::XHasMissingPredecessors (unsigned int startIdx) const
{
	Assert (0 <= startIdx && startIdx < _nodes.XCount ());
	for (XPredecessorSequencer seq (*this, startIdx); !seq.AtEnd (); seq.Advance ())
	{
		Node const * node = seq.GetNode ();
		if (node->IsMissing ())
			return true;
		if (node->IsExecuted ())
			break;	// Stop at first executed node
	}
	return false;
}

void SortedTree::XDeleteRecentNode (GlobalId scriptId)
{
	// Find deleted script
	unsigned int deletedNodeIdx = XFindIdx (scriptId);
	Assert (deletedNodeIdx != 0);
	Node const * deletedNode = _nodes.XGet (deletedNodeIdx);
	Assume (deletedNode->IsCandidateForExecution () || deletedNode->IsMissing (),
			GlobalIdPack (scriptId).ToBracketedString ().c_str ());
	if (deletedNode->IsMissing ())
	{
		Assume (deletedNodeIdx == _nodes.XCount () - 1, ::ToString (_nodes.XCount ()).c_str ());
		// Deleting missing node at tree end.
		// Find out if deleting script makes some branch accepted.

		// Find predecessor of deleted node - if it is a branch point
		// then we have to 'un-reject' some branch.
		Node const * predecessorNode = XFindNode (deletedNode->GetPredecessorId ());
		Assert (predecessorNode != 0);
		if (predecessorNode->IsBranchPoint ())
		{
			// Start with the last node before the removed one.
			// It belongs to the branch of highest priority
			unsigned int currentIdx = deletedNodeIdx - 1;
			Node const * currentNode = _nodes.XGet (currentIdx);
			// Walk the tree in predecessor order and clear the rejected bit until
			// we reach the predecessor node of the rejected node--the branch point.
			while (currentNode->GetScriptId () != deletedNode->GetPredecessorId ())
			{
				Assume (currentNode->IsRejected () || currentNode->IsToBeRejected () || currentNode->IsMissing (),
						GlobalIdPack (currentNode->GetScriptId ()).ToBracketedString ().c_str ());
				Node * currentNodeEdit = _nodes.XGetEdit (currentIdx);
				currentNodeEdit->SetRejected (false);
				Assume (currentNodeEdit->IsCandidateForExecution () || currentNodeEdit->IsExecuted () || currentNodeEdit->IsMissing (),
						GlobalIdPack (currentNodeEdit->GetScriptId ()).ToBracketedString ().c_str ());
				if (currentNodeEdit->GetPredecessorId () == gidInvalid)
					break;	// Tree root

				currentIdx = XFindIdx (currentNodeEdit->GetPredecessorId ());
				currentNode = _nodes.XGet (currentIdx);
			}
		}
		_nodes.XMarkDeleted (deletedNodeIdx);
	}
	else
	{
		Assume (deletedNode->IsCandidateForExecution (), GlobalIdPack (deletedNode->GetScriptId ()).ToBracketedString ().c_str ());
		// Deleting incoming script -- mark it as missing
		Node * node = _nodes.XGetEdit (deletedNodeIdx);
		node->SetMissing (true);
	}
}

void SortedTree::XDeleteNodeByIdx (unsigned deletedNodeIdx)
{
	Assert (deletedNodeIdx != 0);
	_nodes.XMarkDeleted (deletedNodeIdx);
}

// Returns true when substituted node doesn't have missing predecessors
bool SortedTree::XSubstituteNode (std::unique_ptr<Node> newNode)
{
	unsigned int idx = XFindIdx (newNode->GetScriptId ());
	Assert (idx != npos);
	Assert (_nodes.XGet (idx)->IsMissing () ||
			_nodes.XGet (idx)->IsInventory () ||
			_nodes.XGet (idx)->IsCandidateForExecution ());
	Assume (_nodes.XGet (idx)->GetPredecessorId () == newNode->GetPredecessorId () || newNode->IsInventory (),
			GlobalIdPack (newNode->GetScriptId ()).ToBracketedString ().c_str ());
	_nodes.XSubstitute (idx, std::move(newNode));
	if (_scriptKind.IsMergeable ())
		XForceExecution (idx);

	return !XHasMissingPredecessors (idx);
}

void SortedTree::XForceExecution (unsigned int newNodeIdx)
{
	Assert (_scriptKind.IsMergeable ());
	// Tree contains mergeable scripts.
	Node const * newNode = _nodes.XGet (newNodeIdx);
	Assert (newNode->IsForceExec ());
	// Force execution of all nodes following the new node in the tree of a given unit.
	for (XFwdUnitSequencer seq (*this, newNode->GetScriptId (), newNode->GetUnitId (_unitType));
		 !seq.AtEnd ();
		 seq.Advance ())
	{
		Node const * node = seq.GetNode ();
		if (!node->IsMissing ())
		{
			Node * forcedNode = seq.GetNodeEdit ();
			forcedNode->SetForceExec (true);
		}
	}
}

void SortedTree::XConvertNodeState (Progress::Meter * meter, MemoryLog & log)
{
	log << "Converting history nodes" << std::endl;
	for (unsigned int i = 0; i < _nodes.XCount (); ++i)
	{
		Node * node = _nodes.XGetEdit (i);
		log << std::setw (4) << std::setfill (' ') << std::left << i << ": ";
		if (!node->ConvertState ())
		{
			log << "*deleted" << *_nodes.XGet (i) << std::endl;
			Assert (i > 0);
			_nodes.XMarkDeleted (i);	// Delete placeholder
		}
		else if (node->IsRejected ())
		{
			log << "*rejected: " << *_nodes.XGet (i) << std::endl;
		}
		else
		{
			log << *_nodes.XGet (i) << std::endl;
		}
		meter->StepIt ();
	}
	log << "Done converting history nodes" << std::endl;
}

void SortedTree::XDeleteBranch (unsigned recursionLevel, unsigned idx, std::multimap<GlobalId, unsigned> const & successorMap, MemoryLog * log)
{
	History::Node const * startNode = _nodes.XGet (idx);
	Assert (startNode != 0);
	if (log)
	{
		for (unsigned i = 0; i < recursionLevel; ++i)
			*log << "> ";
		*log << "Delete: " << *startNode << std::endl;
	}
	GlobalId gid = startNode->GetScriptId ();
	typedef std::multimap<GlobalId, unsigned>::const_iterator MmIter;
	std::pair<MmIter, MmIter> range = successorMap.equal_range (gid);
	MmIter it = range.first;
	while (it != range.second)
	{
		History::Node const * node = _nodes.XGet (it->second);
		if (node != 0)
			XDeleteBranch (recursionLevel + 1, it->second, successorMap, log);
		++it;
	}
	_nodes.XMarkDeleted (idx);
}

void SortedTree::XInitLastArchivable (MemoryLog * log)
{
	unsigned lastConfirmedIdx = 0;
	if (log) *log << "Finding first interesting script" << std::endl;
	for (unsigned int i = 0; i < _nodes.XCount (); ++i)
	{
		Node const * node = _nodes.XGet (i);
		if (node != 0)
		{
			if (node->IsRejected ())
			{
				if (log) *log << "Deleting orphaned rejected node: " << *node << std::endl;
				_nodes.XMarkDeleted (i);
			}
			else if (node->IsAckListEmpty ())
			{
				lastConfirmedIdx = i;
			}
		}
	}
	if (lastConfirmedIdx != 0)
	{
		if (log) *log << "First interesting node: " << *_nodes.XGet (lastConfirmedIdx) << std::endl;
		_firstInterestingScriptId.XSet (_nodes.XGet (lastConfirmedIdx)->GetScriptId ());
	}
	else
		_firstInterestingScriptId.XSet (gidInvalid);
}

void SortedTree::XRemoveTopRejected ()
{
	Assert (_nodes.XCount () != 0);
	for (unsigned i = _nodes.XCount () - 1; i != 0; --i)
	{
		Node const * node = _nodes.XGet (i);
		if (node != 0)
		{
			if (!node->IsRejected () && !node->IsToBeRejected ())
				break; // we're done
			_nodes.XMarkDeleted (i);
		}
	}
}

bool SortedTree::XPrecedes (GlobalId precederId, GlobalId followerId) const
{
	Assert (precederId != gidInvalid);
	Assert (followerId != gidInvalid);
	unsigned precederIdx = XFindIdx (precederId);
	Assert (precederIdx != npos);
	unsigned followerIdx = XFindIdx (followerId);
	Assert (followerIdx != npos);
	return precederIdx < followerIdx;
}

void SortedTree::XImportInserter::Finalize ()
{
	GidSet importedScripts;
	for (unsigned int i = _originalCount; i < _nodes.XCount (); ++i)
	{
		Node const * node = _nodes.XGet (i);
		importedScripts.insert (node->GetScriptId ());
	}
	// After importing history our history has the following layout
	//
	//		+  <-- last imported script (replaces first set change script from our original history)
	//		|
	//		.
	//		.
	//		|
	//		*  <-- imported initial file inventory
	//		+  <-- project creation marker from the imported history
	//		#  <-- last script received before the history was imported
	//		$
	//		.
	//		.
	//		$
	//		*  <-- our initial file inventory
	//		+  <-- our original project creation marker
	//
	// After finalizing import our history will have the following layout:
	//
	//		#  <-- last script received before the history was imported
	//		$
	//		.
	//		.
	//		$
	//		+  <-- last imported script (replaces first set change script from our original history)
	//		|
	//		.
	//		.
	//		|
	//		*  <-- imported initial file inventory
	//		+  <-- project creation marker from the imported history

	//	Here we mark the importing user's project files script as deleted
	//	(we no longer need it because we've imported an older one).
	//	As in History::Db::VerifyImport, we need to handle the case where the
	//	first script in history was a label rather than project files
	unsigned int copyIdx = 0;
	Node const * node = _nodes.XGet (copyIdx);
	if (node->IsMilestone ())
	{
		//	The first script was the label that was inserted when we first
		//	created the enlistment - it will not exist in the imported
		//	history.
		_nodes.XMarkDeleted (copyIdx);
		++copyIdx;
		node = _nodes.XGet (copyIdx);
	}
	Assert ((node->GetScriptId () == _nodes.XGet (_nodes.XCount () - 1)->GetScriptId ()) ||	// Regulat history import
			 node->IsInventory ());	// Creating new project from the history
	_nodes.XMarkDeleted (copyIdx);
	for (++copyIdx; copyIdx < _originalCount; ++copyIdx)
	{
		node = _nodes.XGet (copyIdx);
		if (importedScripts.find (node->GetScriptId ()) != importedScripts.end ())
			throw Win::Exception ("History import failed -- duplicate script detected.  Please contact support@relisoft.com");
		//	We only append those old scripts that are not in the imported
		//	history.  We made sure not to import overlapping lineage scripts,
		//	so we copy those.
		std::unique_ptr<Node> copyOfNote (new Node (*node));
		_nodes.XAppend (std::move(copyOfNote));
		_nodes.XMarkDeleted (copyIdx);
	}
	if (_nodes.XActualCount () == 0)
		throw Win::Exception ("History import failed -- empty history.  Please contact support@relisoft.com");
}
