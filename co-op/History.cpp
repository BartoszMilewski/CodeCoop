//----------------------------------
// (c) Reliable Software 1997 - 2008
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "History.h"
#include "PathFind.h"
#include "ProjectDb.h"
#include "HistoryScriptState.h"
#include "OutputSink.h"
#include "ExportedHistoryTrailer.h"
#include "FileFilter.h"
#include "HistoryFilter.h"
#include "HistoryTraversal.h"
#include "HistoryPolicy.h"
#include "VersionInfo.h"
#include "FeedbackMan.h"
#include "ScriptHeader.h"
#include "ScriptList.h"
#include "ScriptCommandList.h"
#include "ScriptIo.h"
#include "ScriptProps.h"
#include "MemoryLog.h"
#include "MailboxHelper.h"
#include "AckBox.h"
#include "ProjectOptions.h"
#include "HistoryRange.h"

#include <Ctrl/ProgressMeter.h>
#include <Ex/WinEx.h>

#include <locale>
#include <iomanip>

// Command Log

std::unique_ptr<ScriptCmd> History::Db::CmdLog::Retrieve (File::Offset logOffset, int version) const
{
    LogDeserializer in (_logPath.GetDir (), logOffset);
	std::unique_ptr<ScriptCmd> cmd  = ScriptCmd::DeserializeCmd (in, version);
	if (!cmd->Verify ())
		throw Win::Exception ("Error reading history log: possibly corrupt history database.");
    return cmd;
}

std::unique_ptr<FileCmd> History::Db::CmdLog::RetrieveFileCmd (File::Offset logOffset, int version) const
{
    LogDeserializer in (_logPath.GetDir (), logOffset);
	std::unique_ptr<FileCmd> cmd  = FileCmd::DeserializeCmd (in, version);
	if (!cmd->Verify ())
		throw Win::Exception ("Error reading file history log: possibly corrupt history database.");
    return cmd;
}

void History::Db::MissingScriptId::Serialize (Serializer & out) const
{
	out.PutLong (_scriptId);
	out.PutLong (_unitType);
	// Revisit: version > 55
	// out.PutLong (_unitId);
}

void History::Db::MissingScriptId::Deserialize (Deserializer & in, int version)
{
	_scriptId = in.GetLong ();
	_unitType = static_cast<Unit::Type>(in.GetLong ());
	if (version <= 55)
		_unitId = gidInvalid;
	else
		_unitId = in.GetLong ();
}


History::Db::HdrNote::HdrNote (ScriptHeader const & hdr)
	: _timeStamp (hdr.GetTimeStamp ()),
	  _lineage (hdr.GetLineage ()),
	  _comment (hdr.GetComment ())
{}

void History::Db::HdrNote::Serialize (Serializer& out) const
{
    out.PutLong (_timeStamp);
    _lineage.Serialize (out);
    _comment.Serialize (out);
}

void History::Db::HdrNote::Deserialize (Deserializer& in, int version)
{
    if (version < 13)
        _timeStamp = 0;
    else
        _timeStamp = in.GetLong ();
    _lineage.Deserialize (in, version);
    _comment.Deserialize (in, version);
	if (version < 39)
	{
		// Remove this script id from the script lineage
		_lineage.PopBack ();
	}
}

//
// HistoryCache
//

std::string History::Db::Cache::GetFullComment (GlobalId gid)
{
	Refresh (gid);
	return _hdr->GetComment ();
}

void History::Db::Cache::Refresh (GlobalId gid)
{
	Assert (gid != gidInvalid);
	if (gid == _lastGid)
		return;

	_lastGid = gidInvalid;
	_note = _history._setChanges.FindNode (gid);
	if (_note == 0)
	{
		std::string info ("Missing script ");
		info += GlobalIdPack (gid).ToBracketedString ();
		throw Win::Exception ("Corrupted Database: History", info.c_str ());
	}
	if (_note->IsMissing ())
	{
		_hdr.reset (0);
		std::string resendRequestRecipient = _history.GetLastResendRecipient (gid);
		std::string comment ("Code Co-op has asked ");
		comment += resendRequestRecipient;
		comment += " to re-send this script";
		_comment.Init (comment);
		_timeStamp.Reset (0);
	}
	else
	{
		_hdr = _history._hdrLog.Retrieve (_note->GetHdrLogOffset (), 
										  _note->GetScriptVersion ());
		_comment.Init (_hdr->GetComment ());
		_timeStamp.Reset (_hdr->GetTimeStamp ());
	}
	_lastGid = gid;
}

//
// History
//

char History::Db::_cmdLogName [] = "CmdLog.bin";
char History::Db::_hdrLogName [] = "NoteLog.bin";

History::Db::Db (Project::Db & dataBase)
	: _projectDb (dataBase),
	  _setChanges (Unit::Set, ScriptKindSetChange ()),
	  _membershipChanges (Unit::Member, ScriptKindEditMember ()),
#pragma warning (disable:4355)
	  _cache (*this),
#pragma warning (default:4355)
	  _memberCache (dataBase),
	  _isArchive (false),
	  _initialFileInventoryIsExecuted (false)
{
	std::unique_ptr<Policy> setPolicy (new Policy ());
	_setChanges.SetPolicy (std::move(setPolicy));
	std::unique_ptr<Policy> memberPolicy (new MemberPolicy (_projectDb));
	_membershipChanges.SetPolicy (std::move(memberPolicy));
    AddTransactableMember (_setChanges);
    AddTransactableMember (_membershipChanges);
	AddTransactableMember (_disconnectedScripts);
    AddTransactableMember (_cmdValidEnd);
    AddTransactableMember (_hdrValidEnd);
	AddTransactableMember (_nextScriptId);
}

void History::Db::InitPaths (SysPathFinder& pathFinder)
{
    _cmdLog.Init (pathFinder.GetSysFilePath (_cmdLogName));
    _hdrLog.Init (pathFinder.GetSysFilePath (_hdrLogName));
	_cache.Invalidate ();
	_memberCache.Invalidate ();
}

bool History::Db::Verify (History::Verification what) const
{
#if 0
	std::stringstream oss;
	SortedTree::ForwardSequencer seq(_setChanges);
	while (!seq.AtEnd())
	{
		Node const * node = seq.GetNode();
		DumpNode (oss, node);
		oss << std::endl;
		seq.Advance();
	}
	dbg << "==History Dump" << std::endl << oss.str () << std::endl;
#endif
	if (what == History::Membership)
		return !_membershipChanges.HasDuplicates ();
	if (what == History::Creation)
		return _setChanges.IsCreationMarkerLinked();
	return true;
}

void History::Db::XDoRepair (History::Verification what)
{
	if (what == History::Membership)
	{
		// Repair all membership trees
		GidList memberIds;
		XGetRecordedUnitIds (Unit::Member, memberIds);
		for (GidList::const_iterator iter = memberIds.begin (); iter != memberIds.end (); ++iter)
		{
			UserId id = *iter;
			RepairGatherer repairGatherer (_membershipChanges);
			History::SortedTree::XFwdUnitSequencer treeSeq (_membershipChanges, gidInvalid, id);
			XTraverseTree (treeSeq, repairGatherer);
			if (repairGatherer.TreeChanged ())
			{
				// Correct trunk nodes state bits
				History::SortedTree::XUnitSequencer seq (_membershipChanges, id);
				Assert (!seq.AtEnd ());
				for (History::SortedTree::XPredecessorSequencer predecessorSeq (_membershipChanges, seq.GetIdx ());
					 !predecessorSeq.AtEnd ();
					 predecessorSeq.Advance ())
				{
					History::Node * node = _membershipChanges.XGetEditNode (predecessorSeq.GetIdx ());
					node->SetRejected (false);
				}
				// Mark all nodes for forced execution
				History::SortedTree::XFwdUnitSequencer nodeSeq (_membershipChanges, gidInvalid, id);
				nodeSeq.Advance ();	// Skip first node that adds this user to the member database
				for ( ; !nodeSeq.AtEnd (); nodeSeq.Advance ())
				{
					History::Node const * node = nodeSeq.GetNode ();
					if (node->IsMissing ())
						continue;

					History::Node * editedNode = nodeSeq.GetNodeEdit ();
					editedNode->SetForceExec (true);
				}
			}
		}
		XRemoveDuplicatesInDifferentTrees ();
	}
	else if (what == History::Creation)
	{
		XRelinkTop();

		History::SortedTree::XFwdUnitSequencer fwdSeq (_setChanges, gidInvalid);
		History::Node const * firstNode = fwdSeq.GetNode();
		if (!firstNode->IsMilestone())
		{
			GlobalId scriptId = firstNode->GetScriptId() - 1;
			Assume (scriptId != gidInvalid, "Repair History: cannot create fake start script ID");
			XFixProjectCreationMarker(_projectDb.XProjectName(), scriptId);
			XRelinkTop();
		}
	}
}

void History::Db::XRelinkTop()
{
	History::SortedTree::XFwdUnitSequencer fwdSeq (_setChanges, gidInvalid);
	Assume (!fwdSeq.AtEnd(), "File history is empty");
	History::Node const * firstNode = fwdSeq.GetNode();
	GlobalId firstId = firstNode->GetScriptId();
	fwdSeq.Advance();
	if (fwdSeq.AtEnd())
		throw Win::Exception("File history is empty");
	History::Node * secondNode = fwdSeq.GetNodeEdit();
	secondNode->SetPredecessorId(firstId);
}

class IsMembershipScriptFrom : public std::unary_function<History::Db::MissingScriptId const *, bool>
{
public:
	IsMembershipScriptFrom (UserId id)
		: _id (id)
	{}

	bool operator () (History::Db::MissingScriptId const * id) const
	{
		GlobalIdPack idPack (id->Gid ());
		return id->Type () == Unit::Member && idPack.GetUserId () == _id;
	}
private:
	UserId _id;
};

bool History::Db::VerifyMembership () const
{
	GidList deadMembers;
	_projectDb.GetDeadMemberList (deadMembers);
	for (GidList::const_iterator it = deadMembers.begin (); it != deadMembers.end (); ++it)
	{
		History::SortedTree::FwdUnitSequencer nodeSeq (_membershipChanges, gidInvalid, *it);
		for ( ; !nodeSeq.AtEnd (); nodeSeq.Advance ())
		{
			History::Node const * node = nodeSeq.GetNode ();
			if (node->IsMissing () && node->CanRequestResend ())
				return false;
			if (!node->IsDefect () && !node->IsAckListEmpty ())
				return false;
		}

		if (std::find_if ( _disconnectedScripts.begin (), 
						_disconnectedScripts.end (), 
						IsMembershipScriptFrom (*it)) != _disconnectedScripts.end ())
		{
			return false;
		}
	}
	return true;
}

class XDuplicateFinder
{
public:
	void Find (History::SortedTree & tree)
	{
		std::map<GlobalId, std::pair<unsigned, bool> > visitedNodes;
		for (History::SortedTree::XFullSequencer seq (tree); !seq.AtEnd (); seq.Advance ())
		{
			History::Node const * node = seq.GetNode ();
			GlobalId scriptId = node->GetScriptId ();
			int nodeIdx = seq.GetIdx ();
			std::map<GlobalId, std::pair<unsigned, bool> >::iterator iter =
				visitedNodes.find (scriptId);
			if (iter == visitedNodes.end ())
			{
				// First time seen
				std::pair<int, bool> nodeIdxPair = std::make_pair (nodeIdx, false);
				visitedNodes.insert (std::make_pair(scriptId, nodeIdxPair));
			}
			else
			{
				// Already seen
				bool isRecordedAsDuplicate = iter->second.second;
				if (!isRecordedAsDuplicate)
				{
					_duplicates.insert (std::make_pair (iter->first, iter->second.first));
					iter->second.second = true;	// Recorded as duplicate.
				}
				_duplicates.insert (std::make_pair (scriptId, nodeIdx));
			}
		}
	}

private:
	typedef std::multimap<GlobalId, unsigned>::const_iterator DuplicateIter;

public:
	class NodeSequencer
	{
	public:
		NodeSequencer (DuplicateIter begin, DuplicateIter end)
			: _cur (begin),
			  _end (end)
		{}

		bool AtEnd () const { return _cur == _end; }
		void Advance ()
		{
			GlobalId prevKey = _cur->first;
			++_cur;
			Assert (_cur == _end || _cur->first == prevKey);
		}

		GlobalId GetScriptId () const { return _cur->first; }
		int GetNodeIndex () const { return _cur->second; }

	private:
		DuplicateIter	_cur;
		DuplicateIter	_end;
	};

	class Sequencer
	{
	public:
		Sequencer (XDuplicateFinder const & finder)
			: _duplicates (finder._duplicates)
		{
			if (_duplicates.empty ())
			{
				_curRangeStart = _duplicates.end ();
				_curRangeStop = _duplicates.end ();
			}
			else
			{
				DuplicateIter firstDuplicate = _duplicates.begin ();
				std::pair<DuplicateIter, DuplicateIter> range =
					_duplicates.equal_range (firstDuplicate->first);
				_curRangeStart = range.first;
				_curRangeStop = range.second;
				Assert (_curRangeStart != _curRangeStop);
			}
		}

		bool AtEnd () const { return _curRangeStart == _duplicates.end (); }
		void Advance ()
		{
			if (_curRangeStop == _duplicates.end ())
			{
				_curRangeStart = _duplicates.end ();
			}
			else
			{
				std::pair<DuplicateIter, DuplicateIter> range =
					_duplicates.equal_range (_curRangeStop->first);
				_curRangeStart = range.first;
				_curRangeStop = range.second;
			}
		}

		NodeSequencer GetNodeSequencer () const
		{
			NodeSequencer seq (_curRangeStart, _curRangeStop);
			return seq;
		}

	private:
		std::multimap<GlobalId, unsigned> const &	_duplicates;
		DuplicateIter								_curRangeStart;
		DuplicateIter								_curRangeStop;
	};

	friend class Sequencer;

private:
	std::multimap<GlobalId, unsigned>	_duplicates;
};

void History::Db::XRemoveDuplicatesInDifferentTrees ()
{
	dbg << "--> History::XRemoveDuplicatesInDifferentTrees" << std::endl;
	// Remove script duplicates from differnet member trees
	XDuplicateFinder duplicateFinder;
	duplicateFinder.Find (_membershipChanges);
	for (XDuplicateFinder::Sequencer seq (duplicateFinder); !seq.AtEnd (); seq.Advance ())
	{
		XDuplicateFinder::NodeSequencer nodeSeq = seq.GetNodeSequencer ();
		std::vector<unsigned> duplicateNodeIdx;
		while (!nodeSeq.AtEnd ())
		{
			unsigned nodeIdx = nodeSeq.GetNodeIndex ();
			History::Node const * node = _membershipChanges.XGetNode (nodeIdx);
			dbg << "Duplicate membership history node: " << *node << std::endl;
			if (node->IsMissing ())
			{
				// Always delete missing duplicate nodes
				_membershipChanges.XDeleteNodeByIdx (nodeIdx);
				dbg << "    MISSING - DELETED" << std::endl;
			}
			else
			{
				duplicateNodeIdx.push_back (nodeIdx);
			}
			nodeSeq.Advance ();
		}

		if (duplicateNodeIdx.size () > 1)
		{
			// There are non-missing duplicate nodes in the membership history
			// Remove duplicate nodes from defected member trees
			std::vector<unsigned> activeMemberNodeIdx;
			for (std::vector<unsigned>::const_iterator iter = duplicateNodeIdx.begin ();
				 iter != duplicateNodeIdx.end ();
				 ++iter)
			{
				unsigned nodeIdx = *iter;
				History::Node const * node = _membershipChanges.XGetNode (nodeIdx);
				UserId userId = node->GetUnitId (Unit::Member);
				if (_projectDb.XIsProjectMember (userId))
				{
					MemberState state = _projectDb.XGetMemberState (userId);
					if (state.IsDead ())
					{
						// Delete duplicate node.
						_membershipChanges.XDeleteNodeByIdx (nodeIdx);
						dbg << "    DEFECTED - DELETED" << std::endl;
					}
					else
					{
						activeMemberNodeIdx.push_back (nodeIdx);
					}
				}
				else
				{
					// Really strange - there is a member tree, but no member
					// description in the project database.
					// Delete duplicate node.
					_membershipChanges.XDeleteNodeByIdx (nodeIdx);
				}
			}

			if (activeMemberNodeIdx.size () > 1)
			{
				// There are active members duplicate nodes in the membership history.
				// This is really bad. We have the same script (identical script id)
				// changing different or the same member many times.
				std::string info;
				for (std::vector<unsigned>::const_iterator iter = activeMemberNodeIdx.begin ();
					 iter != activeMemberNodeIdx.end ();
					 ++iter)
				{
					History::Node const * node = _membershipChanges.XGetNode (*iter);
					dbg << "    DUPLICATE ACTIVE: " << *node << std::endl;
					info += GlobalIdPack (node->GetScriptId ()).ToString ();
					info += ": ";
					UserId userId = node->GetUnitId (Unit::Member);
					info += ::ToHexString (userId);
					info += "; ";
					if (_projectDb.XIsProjectMember (userId))
					{
						MemberState state = _projectDb.XGetMemberState (userId);
						info += state.GetDisplayName ();
					}
					else
					{
						info += "UNKNOWN";
					}
					info += "\n";
				}

				throw Win::InternalException ("Cannot repair membership history. Please, contact support@relisoft.com",
											  info.c_str ());
			}
		}
	}
	dbg << "<-- History::XRemoveDuplicatesInDifferentTrees" << std::endl;
}

void History::Db::XDoRepairMembership ()
{
	GidList deadMembers;
	_projectDb.XGetDeadMemberList (deadMembers);
	for (GidList::const_iterator it = deadMembers.begin (); it != deadMembers.end (); ++it)
	{
		XCleanupMemberTree (*it);
	}
}

void History::Db::LeaveProject ()
{
	// Clear volatile data
	_cache.Invalidate ();
	_memberCache.Invalidate ();
	_isArchive = false;
	_initialFileInventoryIsExecuted = false;
}

// Returns true if we have unpacked the full synch script
bool History::Db::XIsFullSyncUnpacked()
{
	History::SortedTree::XFwdUnitSequencer seq (_setChanges, gidInvalid);
	while (!seq.AtEnd ())
	{
		History::Node const * node = seq.GetNode ();
		if (node->IsInventory () && !node->IsMissing ())
			return true;

		seq.Advance ();
	}
	return false;
}

// Returns true if we have script in the history
bool History::Db::XIsRecorded (GlobalId scriptGid, History::ScriptState & scriptState, Unit::Type unitType)
{
	History::SortedTree & tree = XGetTree (unitType);
	History::Node const * node = tree.XFindNode (scriptGid);
	if (node != 0)
	{
		scriptState = ScriptState (node-> GetFlags ());
		return !node->IsMissing ();
	}
	return false;
}

// Returns script id of the most recent executed script
GlobalId History::Db::MostRecentScriptId () const
{
	History::Node const * node = nullptr;
	History::SortedTree::Sequencer seq (_setChanges);
	while (!seq.AtEnd ())
	{
		node = seq.GetNode ();
		if (node->IsExecuted () || node->IsToBeRejected ())
			break;
		seq.Advance ();
	}
    Assert(seq.AtEnd() && node == nullptr ||
		   !seq.AtEnd () && node != nullptr && (node->IsExecuted () || node->IsToBeRejected ()));
    if (node == nullptr)
		return gidInvalid;
	else
		return node->GetScriptId ();
}

// Returns script id of the most recent executed script
GlobalId History::Db::XMostRecentScriptId ()
{
    History::Node const * node = nullptr;
	History::SortedTree::XSequencer seq (_setChanges);
	while (!seq.AtEnd ())
	{
		node = seq.GetNode ();
		if (node->IsExecuted () || node->IsToBeRejected ())
			break;
		seq.Advance ();
	}
    Assert(seq.AtEnd() && node == nullptr ||
        !seq.AtEnd() && node != nullptr && (node->IsExecuted() || node->IsToBeRejected()));
    if (node == nullptr)
		return gidInvalid;
	else
		return node->GetScriptId ();
}

// Returns non-milestone predecessor id
GlobalId History::Db::GetPredecessorId (GlobalId scriptId, Unit::Type unitType) const
{
	History::SortedTree const & tree = GetTree (unitType);
	History::Node const * thisNode = tree.FindNode (scriptId);
	Assume (thisNode != 0, GlobalIdPack (scriptId).ToBracketedString ().c_str ());
	History::Node const * predecessorNode = tree.FindNode (thisNode->GetPredecessorId ());
	if (predecessorNode != 0)
	{
		while (predecessorNode->IsMilestone () && predecessorNode->GetPredecessorId () != gidInvalid)
		{
			// Skip milestone nodes
			predecessorNode = tree.FindNode (predecessorNode->GetPredecessorId ());
			Assert (predecessorNode != 0);
		}
		return predecessorNode->GetScriptId ();
	}
	return gidInvalid;
}

// Returns first executed successor id of the set change script
GlobalId History::Db::GetFirstExecutedSuccessorId (GlobalId scriptId) const
{
	Assert (scriptId != gidInvalid);
	History::SortedTree::FwdUnitSequencer seq (_setChanges, scriptId);
	Assert (!seq.AtEnd ());
	Assert (seq.GetNode ()->IsExecuted ());
	seq.Advance ();
	while (!seq.AtEnd ())
	{
		History::Node const * node = seq.GetNode ();
		if (node->GetPredecessorId () == scriptId && node->IsExecuted ())
			return node->GetScriptId ();
		seq.Advance ();
	}
	Assert (seq.AtEnd ());
	return gidInvalid;
}

// Returns trunk branch point id
GlobalId History::Db::GetTrunkBranchPointId (GlobalId rejectedScriptId) const
{
	Assert (_setChanges.FindNode (rejectedScriptId)->IsRejected ());
	for (History::SortedTree::PredecessorSequencer seq (_setChanges, rejectedScriptId);
		 !seq.AtEnd ();
		 seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		if (node->IsExecuted ())
		{
			Assert (node->IsBranchPoint ());
			return node->GetScriptId ();
		}
	}
	return gidInvalid;
}

void History::Db::RetrieveHeader (History::Node const * node, ScriptHeader & hdr) const
{
	Assert (node != 0);
	Assert (node->GetScriptVersion () > 23 && !node->IsMissing ());
	std::unique_ptr<HdrNote> hdrNote ;
	try
	{
		hdrNote = _hdrLog.Retrieve (node->GetHdrLogOffset (), node->GetScriptVersion ());
		hdr.AddScriptId (node->GetScriptId ());
		hdr.AddComment (hdrNote->GetComment ());
		hdr.AddMainLineage (hdrNote->GetLineage ());
		hdr.AddTimeStamp (hdrNote->GetTimeStamp ());
		hdr.SetProjectName (_projectDb.ProjectName ());
	}
	catch ( ... )
	{
		// Well, something went wrong while reading script header from the log.
		GlobalIdPack pack (node->GetScriptId ());
		std::string info ("Script id: ");
		info += pack.ToString ();
		if (hdrNote.get () != 0)
		{
			info += "\nScript comment:\n";
			info += hdrNote->GetComment ();
		}
		Win::ClearError ();
		throw Win::Exception ("Cannot retrieve script header from History.", info.c_str ());
	}
}

void History::Db::RetrieveCommandList (History::Node const * node, CommandList & cmdList) const
{
	Assert (node != 0);
	Assert (node->GetScriptVersion () > 23 && !node->IsMissing ());
	try
	{
		int scriptVersion = node->GetScriptVersion ();
		for (History::Node::CmdSequencer seq (*node); !seq.AtEnd (); seq.Advance ())
		{
			std::unique_ptr<ScriptCmd> cmd = _cmdLog.Retrieve (seq.GetCmd ()->GetLogOffset (), 
															 scriptVersion);
			cmdList.push_back (std::move(cmd));
		}
	}
	catch ( ... )
	{
		// Well, something went wrong while reading script from the history log.
		GlobalIdPack pack (node->GetScriptId ());
		std::string info ("Script id: ");
		info += pack.ToString ();
		Win::ClearError ();
		throw Win::Exception ("Cannot retrieve script commands from History.", info.c_str ());
	}
}

void History::Db::GetUnitLineage (Unit::Type unitType, GlobalId unitId, Lineage & lineage) const
{
	History::SortedTree const & tree = GetTree (unitType);
	GidList reverseLineage;
	// Collect reversed lineage for given unit.
	// Go back in time until we find confirmed executed script.
	// Skip rejected and to-be-rejected nodes (include missing,
	// candidates for execution and executed nodes).
	History::SortedTree::UnitSequencer seq (tree, unitId);
	for ( ; !seq.AtEnd (); seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		if (!node->IsRejected () && !node->IsToBeRejected ())
		{
			reverseLineage.push_back (node->GetScriptId ());
			if (node->IsAckListEmpty ())
				break;
		}
	}
	lineage.InitReverse (reverseLineage);
	Assert ((lineage.Count () != 0 && unitType == Unit::Set) || unitType == Unit::Member);
}


void History::Db::XGetRecordedUnitIds (Unit::Type type, GidList & unitIds) const
{
	if (type == Unit::Set)
		unitIds.push_back (gidInvalid);
	else if (type == Unit::Member)
		_projectDb.XGetHistoricalMemberList (unitIds);
}

void History::Db::XGetUnitLineage (Unit::Type unitType, GlobalId unitId, Lineage & lineage) 
{
	History::SortedTree & tree = XGetTree (unitType);
	GidList reverseLineage;
	// Collect reversed lineage for given unit.
	// Go back in time until we find confirmed executed script.
	// Skip rejected and to-be-rejected nodes (include missing,
	// candidates for execution and executed nodes).
	// XUnitSequencer iterates nodes of a given unit in reverse chronological order,
	// stops after iterating over first interesting script marker.
	// Note: membership tree doesn't have first interesting script marker.
	for (History::SortedTree::XUnitSequencer seq (tree, unitId); !seq.AtEnd (); seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		if (!node->IsRejected () && !node->IsToBeRejected ())
		{
			reverseLineage.push_back (node->GetScriptId ());
			if (node->IsAckListEmpty ())
				break;
		}
	}
	lineage.InitReverse (reverseLineage);
}

void History::Db::XGetMainSetLineage (Lineage & lineage) 
{
	GidList reverseLineage;
	// Collect reverse lineage in the set tree.
	// Go back in time until we find confirmed executed script.
	// Skip rejected, missing and candidates for execution nodes (include
	// nodes with executed bit set - IsExecuted or IsToBeRejected).
	for (History::SortedTree::XSequencer seq (_setChanges); !seq.AtEnd (); seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		if (node->IsExecuted () || node->IsToBeRejected ())
		{
			reverseLineage.push_back (node->GetScriptId ());
			if (node->IsAckListEmpty ())
				break;
		}
	}
	lineage.InitReverse (reverseLineage);
}

void History::Db::XGetLineages (ScriptHeader & hdr, UnitLineage::Type sideLineageType, bool includeReceivers)
{
	Assert (hdr.GetUnitType () == Unit::Set && hdr.GetModifiedUnitId () == gidInvalid ||
			hdr.GetUnitType () == Unit::Member && hdr.GetModifiedUnitId () != gidInvalid ||
			hdr.GetUnitType () == Unit::Ignore);
	if (hdr.IsData ())
	{
		// Add main lineage
		Lineage lineage;
		if (hdr.GetUnitType () == Unit::Set)
		{
			XGetMainSetLineage (lineage);
		}
		else
		{
			// Note: if we are changing a receiver, the script
			// will only be sent to him and to voting members.
			// No danger of leaking his membership data to other receivers.
			XGetUnitLineage (hdr.GetUnitType (), hdr.GetModifiedUnitId (), lineage);
		}
		// Unit::Set lineages never can be empty, while Unit:Member can be empty
		Assert (lineage.Count () != 0 || hdr.GetUnitType () == Unit::Member);
		hdr.AddMainLineage (lineage);
	}

	if (sideLineageType == UnitLineage::Empty)
		return;

	// Side lineages requested.  Different script kinds get different side lineage sets:
	//   - acknowledgement -- current lineage from the set change tree and all unconfirmed branches from the membership tree
	//   - set change -- all unconfirmed branches from the membership tree
	//   - membership update -- current lineage from the set change tree and all unconfirmed branches from the membership tree
	if (hdr.IsAck () || hdr.IsMembershipChange ())
	{
		// Add current set lineage to the side lineage list
		Lineage lineage;
		XGetUnitLineage (Unit::Set, gidInvalid, lineage);
		if (lineage.Count () > 1)
		{
			// Add set lineage only if it is longer then just the reference version
			std::unique_ptr<UnitLineage> sideLineage (new UnitLineage (lineage, Unit::Set, gidInvalid));
			hdr.AddSideLineage (std::move(sideLineage));
		}
	}

	GidList memberIds;
	XGetRecordedUnitIds (Unit::Member, memberIds);
	// Collect all unconfirmed or complete branches from the membership tree
	for (GidList::const_iterator iter = memberIds.begin (); iter != memberIds.end (); ++iter)
	{
		UserId id = *iter;
		MemberState memberState = _projectDb.XGetMemberState (id);
		if (!includeReceivers && memberState.IsReceiver() || !memberState.IsVerified ())
			continue;
		SideLineageGatherer sideLineageGatherer (Unit::Member, id, hdr, sideLineageType);
		History::SortedTree::XFwdUnitSequencer treeSeq (_membershipChanges, gidInvalid, id);
		XTraverseTree (treeSeq, sideLineageGatherer);
	}
}

bool History::Db::CheckCurrentLineage () const
{
	History::SortedTree::Sequencer seq (_setChanges);
    History::Node const * node = nullptr;
	// Skip incoming scripts
	while (!seq.AtEnd ())
	{
		node = seq.GetNode ();
		if (node->IsExecuted ())
			break;

		seq.Advance ();
	}
    Assert(seq.AtEnd() || node != nullptr && node->IsExecuted());
	if (seq.AtEnd ())
		return true;	// Only incoming scripts

	// Walk current lineage and check predecessor ids
	GlobalId currentPredecessorId = node->GetPredecessorId ();
	for (seq.Advance (); !seq.AtEnd (); seq.Advance ())
	{
		node = seq.GetNode ();
		if (node->IsRejected () || node->IsToBeRejected ())
			continue;		// Skip rejected scripts
		if (currentPredecessorId != node->GetScriptId ())
			return false;	// Wrong predecessor
		if (node->IsAckListEmpty ())
			break;			// Stop at first confirmed script
		currentPredecessorId = node->GetPredecessorId ();
	}
	Assert (seq.AtEnd () || node->IsAckListEmpty ());
	return true;	// Current lineage is ok
}

void History::Db::GetCurrentLineage (Lineage & lineage) const
{
    // Just call GetLineage with the refVersionId
    // Find last accepted version
	// Revisit: FullSequencer should be replaced with Sequencer, but because our
	// version 4.5 beta had problems with setting correctly first interesting script marker
	// we changed sequencer here to ignore FIS marker. When FIS is set incorrectly
	// then our lineages are too short.
	for (History::SortedTree::FullSequencer seq (_setChanges); !seq.AtEnd (); seq.Advance ())
    {
        History::Node const *node = seq.GetNode ();
        if (node->IsConfirmedScript ())
        {
			Assert (node->IsExecuted ());
            GetLineageStartingWith (lineage, node->GetScriptId ());
            break;
        }
    }
}

void History::Db::XDefect ()
{
	_cmdValidEnd.XSet (File::Offset (0, 0));
    _hdrValidEnd.XSet (File::Offset (0, 0));
}

void History::Db::XCleanupMemberTree (UserId userId)
{
	Assert (_projectDb.XGetMemberState (userId).IsDead ());

	unsigned count = _disconnectedScripts.XCount ();
	for (unsigned i = 0; i < count; ++i)
	{
		MissingScriptId const * script = _disconnectedScripts.XGet (i);
		if (script != 0 && script->Type () == Unit::Member)
		{
			GlobalIdPack gidPack (script->Gid ());
			if (gidPack.GetUserId () == userId)
			{
				_disconnectedScripts.XMarkDeleted (i);
			}
		}
	}

	History::SortedTree::XUnitSequencer seq (_membershipChanges, userId);
	if (seq.AtEnd ())
		return;

	// Skip defect node
	if (seq.GetNode ()->IsDefect ())
		seq.Advance ();
	// For every other node clear acknowledgement list
	// and if node is missing set don't request resend bit.
	while (!seq.AtEnd ())
	{
		History::Node const * node = seq.GetNode ();
		if (node->IsMissing ())
		{
			History::Node * editNode = seq.GetNodeEdit ();
			editNode->SetDontResend (true);
			editNode->ClearAckList ();
		}
		else if (!node->IsAckListEmpty ())
		{
			History::Node * editNode = seq.GetNodeEdit ();
			editNode->ClearAckList ();
		}
		seq.Advance ();
	}
}

void History::Db::GetTentativeScripts (ScriptList & fullSynch, Progress::Meter & meter) const
{
	dbg << "Tentative scripts" << std::endl;
    Lineage lineage;
    GetCurrentLineage (lineage);
	meter.SetActivity ("Adding tentative scripts to the full sync script");
	meter.SetRange (0, lineage.Count (), 1);
	// Lineage starts with reference id followed by tentative script ids
	Lineage::Sequencer seq (lineage);
	Assert (!seq.AtEnd ());
	seq.Advance (); // skip reference script, which is not tentative
	while (!seq.AtEnd ())
    {
        GlobalId scriptId = seq.GetScriptId ();
		std::unique_ptr<ScriptHeader> hdr (new ScriptHeader);
		std::unique_ptr<CommandList> cmdList (new CommandList);
        RetrieveScript (scriptId, *hdr, *cmdList);
		dbg << "    " << std::hex << hdr->ScriptId () << std::endl;
		fullSynch.push_back (std::move(hdr), std::move(cmdList));
		meter.StepIt ();
		seq.Advance ();
    }
	dbg << "end tentative scripts" << std::endl;
}

// mapRecentConfirmedScriptIds is initialized to contain all user ids -> gidInvalid
void History::Db::ScanTree (History::SortedTree const & tree, GlobalId startingId, std::map<UserId, GlobalId> & mostRecentConfirmedScriptIds) const
{
	dbg << "Last confirmed scripts starting with version: " << std::hex << startingId << std::endl;
	GidSet users;
	std::map<GlobalId, GlobalId>::iterator iter = mostRecentConfirmedScriptIds.begin ();
	for ( ; iter != mostRecentConfirmedScriptIds.end (); ++iter)
	{
		UserId userId = iter->first;
		users.insert (userId);
	}
	// Walk history notes in the reverse chronological order
	History::SortedTree::FullSequencer seq (tree, startingId);
	while (!seq.AtEnd () && !users.empty ())
	{
		History::Node const * node = seq.GetNode ();
		if (node->IsConfirmedScript () && !node->IsRejected ())
		{
			GlobalId scriptId = node->GetScriptId ();
			GlobalIdPack pack (scriptId);
			UserId userId = pack.GetUserId ();
			if (users.find (userId) != users.end ())
			{
				// Confirmed script from that user seen for the first time
				iter = mostRecentConfirmedScriptIds.find (userId);
				Assert (iter != mostRecentConfirmedScriptIds.end ());
				// Set most recent confirmed script id for that user
				if (iter->second == gidInvalid || iter->second < scriptId)
				{
					dbg << "    " << "User: " << std::hex << userId << " -> "
						<< std::hex << scriptId << std::endl;
					iter->second = scriptId;
				}
				users.erase (userId);
			}
		}
		seq.Advance ();
	}
}

void History::Db::GetMembershipScriptIds (GidList const & members, auto_vector<UnitLineage> & sideLineages)
{
	// Gather membership script ids for all members
	for (GidList::const_iterator iter = members.begin (); iter != members.end (); ++iter)
	{
		UserId userId = *iter;
		// Go through complete history of membership changes concerning the user with userId
		History::SortedTree::FwdUnitSequencer scriptSeq (_membershipChanges,
												gidInvalid, // Start from root
												userId);
		std::unique_ptr<UnitLineage> sideLineage (new UnitLineage (Unit::Member, userId));

		if (scriptSeq.AtEnd ())
		{
			MemberState userState = _projectDb.GetMemberState (userId);
			if (userState.IsVerified () && userState.IsDead ())
			{
				// Verified members can have empty history if the project is a branch
				continue;
			}
			else
			{
				// We don't have this member change history; assign invalid
				// script id, so receiving party will not store this script
				// in the history. However, script will be executed.
				sideLineage->PushId (gidInvalid);
			}
		}
		else
		{
			for ( ; !scriptSeq.AtEnd (); scriptSeq.Advance ())
			{
				History::Node const * node = scriptSeq.GetNode ();
				if (!node->IsMissing ())
				{
					sideLineage->PushId (node->GetScriptId ());
				}
			}
			if (sideLineage->Count () == 0)
			{
				// Only missing scripts recorded in this user history.
				// Assign invalid script id, so receiving party will not store this script
				// in the history. However, script will be executed.
				sideLineage->PushId (gidInvalid);
			}
		}
		sideLineages.push_back (std::move(sideLineage));
	}
}

void History::Db::GetMembershipScripts (ScriptList & scriptList, 
							   auto_vector<UnitLineage>::const_iterator sideLineagesSeq,
							   auto_vector<UnitLineage>::const_iterator sideLineagesEnd)
{
	while (sideLineagesSeq != sideLineagesEnd)
	{
		UnitLineage const & unitLineage = **sideLineagesSeq;
		Assert (unitLineage.GetUnitType () == Unit::Member);
		GlobalId userId = unitLineage.GetUnitId ();

		// Sequence scripts changing current user
		Lineage::Sequencer seq (unitLineage);
		GlobalId scriptId = seq.GetScriptId ();

		// First script for current userId gets normalized script comment.
		std::unique_ptr<ScriptHeader> firstHdr (new ScriptHeader (ScriptKindAddMember (),
																userId,
																_projectDb.ProjectName ()));
		firstHdr->SetScriptId (scriptId);
		firstHdr->AddComment ("Project member added by the full sync script");
		std::unique_ptr<MemberInfo> firstMemberInfo;
		if (scriptId == gidInvalid)
		{
			// Unconfirmed when full sync created
			firstMemberInfo = _projectDb.RetrieveMemberInfo (userId);
			firstMemberInfo->Pad (); // must be fixed size for resend to work!
		}
		else
		{
			std::unique_ptr<ScriptHeader> hdr (new ScriptHeader);
			std::unique_ptr<CommandList> cmdList (new CommandList);
			RetrieveScript (scriptId, *hdr, *cmdList, Unit::Member);
			Assert (cmdList->size () == 1);
			CommandList::Sequencer cmdSeq (*cmdList);
			if (hdr->IsAddMember ())
			{
				NewMemberCmd const & cmd = cmdSeq.GetAddMemberCmd ();
				firstMemberInfo.reset (new MemberInfo (cmd.GetMemberInfo ()));
			}
			else if (hdr->IsEditMember ()) // work around early bug
			{
				EditMemberCmd const & cmd = cmdSeq.GetEditMemberCmd ();
				firstMemberInfo.reset (new MemberInfo (cmd.GetNewMemberInfo ()));
			}
			else if (hdr->IsDefectOrRemove ())
			{
				// We have only defect script in this user membership change history
				DeleteMemberCmd const & cmd = cmdSeq.GetDeleteMemberCmd ();
				firstMemberInfo.reset (new MemberInfo (cmd.GetMemberInfo ()));
				firstHdr->AddScriptId (gidInvalid);	// Joinee should execute only this script
			}
			else
				throw Win::Exception ("Unexpected membership script type in history. Contact support@relisoft.com");
		}

		std::unique_ptr<CommandList> firstCmdList (new CommandList);
		std::unique_ptr<ScriptCmd> cmd (new NewMemberCmd (*firstMemberInfo));
		firstCmdList->push_back (std::move(cmd));
		scriptList.push_back (std::move(firstHdr), std::move(firstCmdList));

		seq.Advance ();
		// The rest of the scripts changing the current member
		while (!seq.AtEnd ())
		{
			GlobalId scriptId = seq.GetScriptId ();
			std::unique_ptr<ScriptHeader> hdr (new ScriptHeader);
			std::unique_ptr<CommandList> cmdList (new CommandList);
			RetrieveScript (scriptId, *hdr, *cmdList, Unit::Member);
			Assert (hdr->IsMembershipChange ());
			scriptList.push_back (std::move(hdr), std::move(cmdList));
			seq.Advance ();
		}
		++sideLineagesSeq;
	}
}

// The sequencer does not contain the "add joinee" script
void History::Db::PatchMembershipScripts (GidList const & members,
								ScriptList::EditSequencer & scriptList, 
								GlobalId setReferenceId)
{
	dbg << "Patch membership scripts" << std::endl;
	// Map project member ids to their most recent confirmed script ids
	std::map<UserId, GlobalId> mostRecentConfirmedScriptIds;
	for (GidList::const_iterator iter = members.begin (); iter != members.end (); ++iter)
	{
		UserId userId = *iter;
		mostRecentConfirmedScriptIds [userId] = gidInvalid;
	}
	// Scan history tree for most recent confirmed script ids created by project members
	ScanTree (_setChanges, setReferenceId, mostRecentConfirmedScriptIds);

	ScriptList::EditSequencer scriptSeq (scriptList);
	Assert (!scriptSeq.AtEnd ()); // At least the admin is on the list
	do
	{
		// First script for given member must be adding this member
		Assert (scriptSeq.GetCmdListSize () == 1);
		CommandList::EditSequencer const & cmdSeq = scriptSeq.GetCmdSequencer ();
		NewMemberCmd & addMemberCmd = cmdSeq.GetAddMemberCmd ();
		UserId userId = addMemberCmd.GetUserId ();
		dbg << "Patching user " << std::hex << userId << " - script " 
			<< std::hex << scriptSeq.GetHeader ().ScriptId () << std::endl;
		// Correct pre-historic script id for this project member
		std::map<UserId, GlobalId>::iterator scriptIdIter = mostRecentConfirmedScriptIds.find (userId);
		Assert (scriptIdIter != mostRecentConfirmedScriptIds.end ());
		GlobalId preHistoricId = scriptIdIter->second;
		if (preHistoricId != gidInvalid)
		{
			// We have found some confirmed script created by this user.
			if (GlobalIdPack (preHistoricId).GetOrdinal () == 0) // project creation maker
			{
				// That can only happen when the administrator (us) is not the project creator
				// This could be our marker, or it could be a marker from imported history
				preHistoricId = gidInvalid; // all scripts will be historic
			}
			else if (preHistoricId == setReferenceId)
			{
				// Prehistoric script id for that user is equal to
				// the the administrator's set-lineage reference id 
				// (this is is used to mark the initial file inventory for the joinee)
				dbg << "    most recent is equal to initial file inventory" << std::endl;
				GlobalIdPack prePack (preHistoricId);
				preHistoricId = GlobalIdPack (prePack.GetUserId (), prePack.GetOrdinal () - 1);
			}
		}
		else
		{
			preHistoricId = _projectDb.GetPreHistoricScriptId (userId);
		}

		dbg << "    prehistoric: " << std::hex << preHistoricId << std::endl;
		// We always set most recent script id to gidInvalid, so we can recreate
		// exactly the same full sync when re-send request arrives (in the mean time
		// given user may use many new script ids). [don't change order of these two!]
		addMemberCmd.ResetScriptMarkers ();
		addMemberCmd.SetPrehistoricScriptId (preHistoricId);

		// Skip other scripts changing this user is
		scriptSeq.Advance ();
		while (!scriptSeq.AtEnd ())
		{
			ScriptHeader const & hdr = scriptSeq.GetHeader ();
			if (hdr.GetModifiedUnitId () != userId)
				break;
			scriptSeq.Advance ();
		}
	} while (!scriptSeq.AtEnd ());

	dbg << "end patching" << std::endl;
}

void History::Db::MapMostRecentScripts (std::map<UserId, GlobalId> & userToMostRecent)
{
	History::SortedTree::ForwardSequencer seq (_setChanges);
	while (!seq.AtEnd ())
	{
		GlobalIdPack gidPack (seq.GetNode ()->GetScriptId ());
		userToMostRecent [gidPack.GetUserId ()] = gidPack;
		seq.Advance ();
	}
}

void History::Db::XRetrieveDefectedTrees (GidList const & hisDeadMembers, ScriptList & scriptList)
{
	GidList allRecordedDeadMembers;
	_projectDb.XGetDeadMemberList (allRecordedDeadMembers);
	if (allRecordedDeadMembers == hisDeadMembers)
		return;

	GidSet hisDeadMemberSet (hisDeadMembers.begin (), hisDeadMembers.end ());
	for (GidList::const_iterator iter = allRecordedDeadMembers.begin (); iter !=  allRecordedDeadMembers.end (); ++iter)
	{
		UserId deadMemberId = *iter;
		if (hisDeadMemberSet.find (deadMemberId) != hisDeadMemberSet.end ())
			continue; // he knows about this one

		for (History::SortedTree::XFwdUnitSequencer seq (_membershipChanges, gidInvalid, deadMemberId); !seq.AtEnd (); seq.Advance ())
		{
			History::Node const * node = seq.GetNode ();
			std::unique_ptr<ScriptHeader> hdr (new (ScriptHeader));
			std::unique_ptr<CommandList> cmdList (new (CommandList));
			if (XRetrieveScript (node->GetScriptId (), *hdr, *cmdList, Unit::Member))
				scriptList.push_back (std::move(hdr), std::move(cmdList));
		}
	}
}

void History::Db::PruneDeadMemberList(GidList & deadMemberList)
{
	GidList::iterator it = deadMemberList.begin();
	while (it != deadMemberList.end())
	{
		History::SortedTree::UnitSequencer seq(_membershipChanges, *it);
		while (!seq.AtEnd())
		{
			History::Node const * node = seq.GetNode();
			if (node->IsDefect())
				break;
			seq.Advance();
		}

		if (seq.AtEnd())
			it = deadMemberList.erase(it);
		else
			++it;
	}
}

void History::Db::GetReverseLineage (GidList & reverseLineage, GlobalId refVersionId) const
{
	reverseLineage.clear ();
	// Collect reverse lineage -- start from the most recent script in the history.
	// Skip all unpacked (not executed) or missing and collect all executed (ignoring rejected)
	// script id's until you reach the reference version in the history.
    // History iterator walks history nodes in the reverse chronological order
	// (ignores _lastArchiveable index and nodes with archive bit set)
	for (History::SortedTree::FullSequencer seq (_setChanges); !seq.AtEnd (); seq.Advance ())
    {
        History::Node const * node = seq.GetNode ();
		if (node->IsExecuted ())
		{
			GlobalId scriptId = node->GetScriptId ();
			reverseLineage.push_back (scriptId);
			if (scriptId == refVersionId)
            	break;
		}
    }
}

void History::Db::GetLineageStartingWith (Lineage & lineage, GlobalId refVersionId) const
{
	GidList reverseLineage;
	GetReverseLineage (reverseLineage, refVersionId);
	lineage.InitReverse (reverseLineage);
}

void History::Db::XGetReverseLineage (GidList & reverseLineage, GlobalId refVersionId)
{
	reverseLineage.clear ();
	// Collect reverse lineage -- start from the most recent script in the history.
	// Skip all unpacked (not executed) or missing and collect all executed (ignoring rejected)
	// script id's until you reach the reference version in the history.
	// History iterator walks history nodes in the reverse chronological order
	// (ignores _lastArchiveable index and nodes with archive bit set)
	for (History::SortedTree::XFullSequencer seq (_setChanges); !seq.AtEnd (); seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		if (node->IsExecuted ())
		{
			GlobalId scriptId = node->GetScriptId ();
			reverseLineage.push_back (scriptId);
			if (scriptId == refVersionId)
				break;
		}
	}
}

void History::Db::XGetLineageStartingWith (Lineage & lineage, GlobalId refVersionId)
{
	GidList reverseLineage;
	XGetReverseLineage (reverseLineage, refVersionId);
	lineage.InitReverse (reverseLineage);
}

std::string History::Db::RetrieveVersionDescription (GlobalId versionGid) const
{
	std::string versionDescr;
	if (versionGid == gidInvalid)
	{
		// Current version
		versionGid = MostRecentScriptId ();
	}
	GlobalIdPack pack (versionGid);
	History::Node const * node = _setChanges.FindNode (versionGid);
	if (node != 0)
	{
		// Format version description
		std::unique_ptr<HdrNote> hdr =
			_hdrLog.Retrieve (node->GetHdrLogOffset (), node->GetScriptVersion ());
		versionDescr += hdr->GetComment ();
		versionDescr += '\t';
		std::unique_ptr<MemberDescription> sender =
			_projectDb.RetrieveMemberDescription (pack.GetUserId ());
		versionDescr += sender->GetName ();
		versionDescr += '\t';
		StrTime timeStamp (hdr->GetTimeStamp ());
		versionDescr += timeStamp.GetString ();
		versionDescr += '\t';
		versionDescr += pack.ToString ();
		versionDescr += '\t';
		if (node->IsMilestone ())
			versionDescr += "Milestone";
		else
			versionDescr += "File";
	}
	else
	{
		versionDescr += "Version ";
		versionDescr += pack.ToString ();
		versionDescr += " not found in the project '";
		versionDescr += _projectDb.ProjectName ();
		versionDescr += "' history";
	}
	return versionDescr;
}

std::string History::Db::RetrieveLastRejectedScriptComment () const
{
	UserId thisUser = _projectDb.GetMyId ();
	std::string comment;
	int searchDepth = 0;
	for (History::SortedTree::UISequencer seq (_setChanges); !seq.AtEnd () && searchDepth < 50; seq.Advance (), searchDepth++)
    {
        History::Node const * node = seq.GetNode ();
        if (node->IsRejected ())
		{
			GlobalIdPack pack (node->GetScriptId ());
			if (thisUser == pack.GetUserId ())
			{
				// Our rejected script
				Assert (node->GetScriptVersion () >= 23);
				std::unique_ptr<HdrNote> hdr 
					= _hdrLog.Retrieve (node->GetHdrLogOffset (), node->GetScriptVersion ());
				comment.assign (hdr->GetComment ());
				break;
			}
		}
    }
	return comment;
}

bool History::Db::Export (std::string const & filePath, Progress::Meter & progressMeter) const
{
	FileSerializer out (filePath);
	std::vector<MemberInfo> memberList(_projectDb.RetrieveHistoricalMemberList());
	progressMeter.SetRange (0, _setChanges.size () + memberList.size () + 1);
	ExportedHistoryTrailer trailer (_projectDb.GetMyId (), _projectDb.ProjectName ());
	// Export scripts -- ignore all archives
	progressMeter.SetActivity ("Exporting scripts.");
	for (History::SortedTree::FullSequencer seq (_setChanges); !seq.AtEnd (); seq.Advance ())
	{
		unsigned int idx = seq.GetIdx ();
		History::Node const * node = seq.GetNode ();
		GlobalId scriptId = node->GetScriptId ();
		GlobalIdPack pack (scriptId);
		if (node->GetScriptVersion () < 23)
		{
			// Too old to read
			std::string info ("History cannot be exported because it contains very old\nscript ");
			info += pack.ToString ();
			info += ", whose format is no longer supported.";
			TheOutput.Display (info.c_str ());
			return false;
		}
		// Skip script if:
		if (node->IsCandidateForExecution () ||		// Is unpacked, but not executed or
			node->IsMissing ())						// Missing
			continue;

		ScriptHeader hdr;
		CommandList cmdList;
		RetrieveScript (node, hdr, cmdList);
		InitScriptHeader (node, Unit::Set, cmdList, hdr);
		trailer.RememberScript (scriptId, node->GetFlags (), out.GetPosition ());
		ScriptBuilder builder (0, &hdr); // no catalog needed
		builder.AddCommandList (&cmdList);
		builder.Save (out);
		progressMeter.StepIt ();
		if (progressMeter.WasCanceled ())
		{
			return false;
		}
	}
	// Export all project members -- past and present
	progressMeter.SetActivity ("Exporting project members.");
	typedef std::vector<MemberInfo>::const_iterator MemberIter;
	for (MemberIter mIter = memberList.begin (); mIter != memberList.end (); ++mIter)
	{
		trailer.RememberUser (mIter->Id (), out.GetPosition ());
		mIter->Serialize (out);
		progressMeter.StepIt ();
		if (progressMeter.WasCanceled ())
		{
			return false;
		}
	}
	progressMeter.SetActivity ("Saving exported history.");
	SerFileOffset trailerStart = out.GetPosition ();
	trailer.Save (out);
	progressMeter.StepIt ();
	if (progressMeter.WasCanceled ())
	{
		return false;
	}
	// Large format:
	// long version, File::Offset trailerStart, long signature
	out.PutLong (modelVersion);
	trailerStart.Serialize (out);
	out.PutLong ('EXHL');
	return true;
}

bool History::Db::VerifyImport (ExportedHistoryTrailer & trailer) const
{
	std::string info;
	std::string const & exporterProjectName = trailer.GetProjectName ();
	if (!IsFileNameEqual (exporterProjectName, _projectDb.ProjectName ()))
	{
		info += "Code Co-op cannot import history from the project '";
		info += exporterProjectName;
		info += "'\ninto history of the project '";
		info += _projectDb.ProjectName ();
		info += "'";
		TheOutput.Display (info.c_str ());
		return false;
	}
	if (!_projectDb.IsProjectMember (trailer.GetExporterId ()))
	{
		info += "Code Co-op cannot import history for the project '";
		info += _projectDb.ProjectName ();
		info += "'\nbecause it was exported by the user who is no longer a project member.";
		TheOutput.Display (info.c_str ());
		return false;
	}
	if (_projectDb.GetMyId () == trailer.GetExporterId ())
	{
		info += "The exported history file was created in this project instance.";
		info += "\nThere is no need to import this history, because it is identical";
		info += "\nwith the current project history.";
		TheOutput.Display (info.c_str ());
		return false;
	}
	//	We want to find the initial project files script (in the importing
	//	user's history).  This used to be the first script (early 3.x?), but
	//	now when we create a new enlistment we add a label to indicate the
	//	beginning of the history.

	History::SortedTree::ForwardSequencer historySeq (_setChanges);
	History::Node const * firstNode = historySeq.GetNode ();
	if (firstNode->IsMilestone ())
	{
		//	The first script was a label that was inserted when the enlistment
		//	was created - it will not exist in the imported history.
		//	Use the next script instead.
		//	Note that if we've already imported from an enlistment that didn't
		//	create the project the user id on the label won't be ours
		//	Assert ((firstNote->GetScriptId () >> 20) == _projectDb.GetMyId ());
		historySeq.Advance ();
		firstNode = historySeq.GetNode ();
	}
	GlobalId thisHistoryFirstScriptId = firstNode->GetScriptId ();
	bool isRejected = false;
	if (!trailer.IsScriptPresent (thisHistoryFirstScriptId, isRejected))
	{
		GlobalIdPack pack (thisHistoryFirstScriptId);
		info += "Code Co-op cannot import history for the project '";
		info += _projectDb.ProjectName ();
		info += "'\nbecause the first script present in this project ";
		info += pack.ToString ();
		info += "\nis not present in the exported history.";
		info += "\nThere is no connection between both histories.";
		TheOutput.Display (info.c_str ());
		return false;
	}
	int overlapLen = trailer.GetOverlapLength (thisHistoryFirstScriptId);

	// Revisit: Display are you sure when overlap length equals zero ?

	//	Here we make sure that scripts that we believe are overlapping in our
	//	history really do exist in the history we're importing.
	//	We start checking the overlap with our project files script (which may
	//	isn't necessarily script 0)
	for (; overlapLen > 0 && !historySeq.AtEnd (); --overlapLen, historySeq.Advance ())
	{
		History::Node const * node = historySeq.GetNode ();
		if (node->IsRelevantScript ())
		{
			// Lineage scripts in the overlapping have to match
			bool isRejected = false;
			bool isPresent = trailer.IsScriptPresent (node->GetScriptId (), isRejected);
			if (!isPresent || isRejected)
			{
				GlobalIdPack pack (node->GetScriptId ());
				info += "Code Co-op cannot import history for the project '";
				info += _projectDb.ProjectName ();
				info += "'\nbecause the overlapping history parts between this project history and the exported history do not match.";
				info += "\nThe script ";
				info += pack.ToString ();
				if (!isPresent)
					info += " is not present";
				else
					info += " is rejected";
				info += " in the exported history.";
				TheOutput.Display (info.c_str ());
				return false;
			}
		}
	}

	trailer.PrepareForImport (thisHistoryFirstScriptId);
	return trailer.GetScriptCount () > 1;
}

void History::Db::XImport (ExportedHistoryTrailer const & trailer,
				  std::string const & srcPath,
				  GidSet & changedFiles,
				  Progress::Meter & progressMeter,
				  bool resetFisMarker)
{
	// Step 1 -- remember current project history end
	History::SortedTree::XImportInserter importInserter (_setChanges);
	// Step 2 -- at current history end, add imported history
	//           without overlapping part
	progressMeter.SetActivity ("Importing scripts.");
	for (ImportedHistorySeq importSeq (trailer); !importSeq.AtEnd (); importSeq.Advance ())
	{
		progressMeter.StepAndCheck ();
		ScriptReader reader (srcPath);
		std::unique_ptr<ScriptHeader> hdr = reader.RetrieveScriptHeader (importSeq.GetScriptOffset ());
		std::unique_ptr<CommandList> cmdList = reader.RetrieveCommandList (importSeq.GetScriptOffset ());
		if (trailer.IsFromVersion40 ())
		{
			CommandList::Sequencer seq (*cmdList);
			if (!seq.AtEnd ())
			{
				ScriptCmd const & cmd = seq.GetCmd ();
				if (cmd.GetType () == typeUserCmd)
					continue;	// Skip membership placeholder scripts
			}
			if (GlobalIdPack (hdr->ScriptId ()).IsFromJoiningUser ())
			{
				Assert (hdr->IsFromVersion40 ());
				// Project creation marker from version 4.0 has lineage containing only one global id equal 0xfff00000.
				// When converting version 4.0 script header format to version 4.5 format we assign script id from the
				// lineage -- in version 4.0 the last script id in the lineage is the script id. In this particular case
				// this is 0xfff00000. In version 4.5 this script id is used only when project awaits full sync.
				// We have to use the real script id stored in the exported history trailer and fix script lineage to
				// contain reference id equal gidInvalid (no reference id)
				hdr->AddScriptId (importSeq.GetScriptId ());
				Lineage lineage;
				lineage.PushId (gidInvalid);
				hdr->AddMainLineage (lineage);
			}
		}
		hdr->Verify ();
		GlobalIdPack pack (hdr->ScriptId ());
		UserId sender = pack.GetUserId ();
		if (!_projectDb.XIsProjectMember (sender))
		{
			// Sender of the imported script is not present in our user database -- add it
			File::Offset userOffset = trailer.GetUserOffset (sender);
			if (userOffset != File::Offset::Invalid)
			{
				MemberInfo importedUser (srcPath, userOffset, trailer.GetImportVersion ());
				if (importedUser.State ().IsActive ())
				{
					// We don't know the sender of imported script. His/Hers state cannot
					// be active project member (observer or voting), because we should be knowing
					// all active project members. So this must be the mistake and we change
					// the state to defected.
					MemberState state = importedUser.State ();
					state.MakeDead ();
					importedUser.SetState (state);
				}
				_projectDb.XAddMember (importedUser);
			}
		}

		// Store imported script in the history
		GidList emptyAckList;
		std::unique_ptr<History::Node> newNode = XLogScript (*hdr, *cmdList, emptyAckList);
		newNode->SetState (importSeq.GetScriptFlags ());
		if (trailer.IsFromVersion40 ())
		{
			bool conversionSucceeded = newNode->ConvertState ();
			Assert (conversionSucceeded);
			std::string const & comment = hdr->GetComment ();
			if (comment.find ("Project files as of") != std::string::npos ||
				comment.find ("File(s) Added During") != std::string::npos)
			{
				// Found initial file inventory
				newNode->SetInventory (true);
			}
		}
		Assert (newNode->IsExecuted () || newNode->IsRejected ());
		// Record files changed by the imported script
		for (History::Node::CmdSequencer seq (*newNode); !seq.AtEnd (); seq.Advance ())
		{
			History::Node::Cmd const * cmd = seq.GetCmd ();
			GlobalId uid = cmd->GetUnitId ();
			if (uid != gidInvalid)
				changedFiles.insert (uid);
		}
		importInserter.push_back (std::move(newNode));
		Notify (changeAdd, hdr->ScriptId ());
	}
	// Step 3 -- copy current project history behind the imported history
	//           and delete original of current history.
	progressMeter.SetActivity ("Reorganizing history.");
	progressMeter.StepAndCheck ();
	// REVISIT: store old full sync in the snapshot list
	// REVISIT: implement overlapping part rejected branches merging
	importInserter.Finalize ();
	if (trailer.IsFromVersion40 ())
		XSetPredecessorIds ();

	if (resetFisMarker)
	{
		History::SortedTree::XSequencer seq (_setChanges);
		Assert (!seq.AtEnd ());
		History::Node const * node = seq.GetNode ();
		Assert (node->IsAckListEmpty ());
		_setChanges.XSetFirstInterestingScriptId (node->GetScriptId ());
	}
}

void History::Db::FindAllByComment (std::string const & keyword,
									GidList & scripts,
									GidSet & files) const
{
	// Keyword matching is case insensitive
	std::string keyUpper (keyword);
	std::transform (keyUpper.begin (), keyUpper.end (), keyUpper.begin (), ::ToUpper);
	for (History::SortedTree::UISequencer seq (_setChanges); !seq.AtEnd (); seq.Advance ())
    {
        History::Node const * node = seq.GetNode ();
		if (node->IsRelevantMilestone ())
		{
			// Include trunk milestones (labels) regardless of the comment
			scripts.push_back (node->GetScriptId ());
		}
		else
		{
			// Search script comment
			std::unique_ptr<HdrNote> hdr = _hdrLog.Retrieve (node->GetHdrLogOffset (),
														   node->GetScriptVersion ());
			std::string comment (hdr->GetComment ());
			std::transform (comment.begin (), comment.end (), comment.begin (), ::ToUpper);
			if (comment.find (keyUpper) != std::string::npos)
			{
				// Script comment contains keyword
				scripts.push_back (node->GetScriptId ());
				if (node->IsRelevantScript ())
				{
					// Remember changed files
					for (History::Node::CmdSequencer cmdSeq (*node); !cmdSeq.AtEnd (); cmdSeq.Advance ())
					{
						History::Node::Cmd const * cmd = cmdSeq.GetCmd ();
						files.insert (cmd->GetUnitId ());
					}
				}
			}
		}
    }
}

void History::Db::GetToBeRejectedScripts (History::Range & range) const
{
	for (History::SortedTree::FullSequencer seq (_setChanges); !seq.AtEnd (); seq.Advance ())
    {
        History::Node const * node = seq.GetNode ();
		if (node->IsToBeRejected ())
			range.AddScriptId (node->GetScriptId ());
		else if (node->IsExecuted ())
			break;
    }
}

void History::Db::GetItemsYoungerThen (GlobalId scriptId, GidSet & youngerItems) const
{
	// Return ids of all scripts (trunk or branch) created after the 'scriptId'
	for (History::SortedTree::FullSequencer seq (_setChanges); !seq.AtEnd (); seq.Advance ())
    {
        History::Node const * node = seq.GetNode ();
		if (node->GetScriptId () == scriptId)
			return;

		youngerItems.insert (node->GetScriptId ());
    }
}

bool History::Db::IsFileChangedByScript (GlobalId fileGid, GlobalId scriptGid) const
{
	History::Node const * node = _setChanges.FindNode (scriptGid);
	Assume (node != 0, GlobalIdPack (scriptGid).ToBracketedString ().c_str ());
	return node->Find (fileGid) != 0;
}

bool History::Db::IsFileCreatedByScript (GlobalId fileGid, GlobalId scriptGid) const
{
	History::Node const * node = _setChanges.FindNode (scriptGid);
	Assume (node != 0, GlobalIdPack (scriptGid).ToBracketedString ().c_str ());
	for (History::Node::CmdSequencer noteSeq (*node); !noteSeq.AtEnd (); noteSeq.Advance ())
	{
		History::Node::Cmd const * cmdNote = noteSeq.GetCmd ();
		if (fileGid == cmdNote->GetUnitId ())
		{
			std::unique_ptr<FileCmd> cmd = _cmdLog.RetrieveFileCmd (cmdNote->GetLogOffset (), node->GetScriptVersion ());
			return cmd->GetType () == typeWholeFile;
		}
	}
	return false;
}

void History::Db::GetFilesChangedByScript (GlobalId scriptId, GidSet & files) const
{
	History::Node const * node = _setChanges.FindNode (scriptId);
	Assume (node != 0, GlobalIdPack (scriptId).ToBracketedString ().c_str ());
	for (History::Node::CmdSequencer seq (*node); !seq.AtEnd (); seq.Advance ())
	{
		History::Node::Cmd const * cmd = seq.GetCmd ();
		Assert (cmd != 0);
		files.insert (cmd->GetUnitId ());
	}
}

void History::Db::XAddCheckinScript (ScriptHeader const & hdr, CommandList const & cmdList, AckBox & ackBox, bool isInventory)
{
	GlobalId checkinScriptId = hdr.ScriptId ();
	Assert (checkinScriptId != gidInvalid);
	std::unique_ptr<History::Node> newNode = XLogCheckinScript (hdr, cmdList);
	newNode->SetExecuted (true);
	newNode->SetInventory (isInventory);

	if (isInventory)
	{
		Assert (hdr.GetUnitType () == Unit::Set);
		Assert (_setChanges.xsize () <= 2);
		_initialFileInventoryIsExecuted = true;
		if (_setChanges.xsize () == 2)
		{
			// Replace current empty initial file inventory with this one
			Assert (cmdList.size () != 0);
			History::SortedTree::XSequencer seq (_setChanges);	// Walks history in the reverse historical order
			Assert (!seq.AtEnd ());
			History::Node const * currentInventory = seq.GetNode ();
			Assert (History::Node::CmdSequencer (*currentInventory).AtEnd ());
			Assert (currentInventory->IsMilestone ());
			// Preserve current inventory script id, predecessor id and header log offset
			newNode->SetScriptId (currentInventory->GetScriptId ());
			newNode->SetPredecessorId (currentInventory->GetPredecessorId ());
			newNode->SetHdrLogOffset (currentInventory->GetHdrLogOffset ());
			_setChanges.XSubstituteNode (std::move(newNode));
			return;
		}
	}

	Unit::Type unitType = hdr.GetUnitType ();
	bool newNodeIsConfirmed = newNode->IsAckListEmpty ();
	if (unitType == Unit::Set)
	{
		Notify (changeAdd, checkinScriptId);
	}
	else
	{
		Assert (unitType == Unit::Member);
		// Clear forced exec bit set during script header logging
		newNode->SetForceExec (false);
	}
	History::SortedTree & tree = XGetTree (unitType);
	tree.XInsertNewNode (std::move(newNode));
	if (newNodeIsConfirmed)
	{
		ackBox.RememberMakeRef (Unit::ScriptId (checkinScriptId, unitType));
		History::SortedTree::XSequencer seq (tree, checkinScriptId);
		if (unitType == Unit::Set)
			tree.XConfirmAllOlderNodes (seq);
		else
			tree.XConfirmOlderPredecessors (seq);
	}
}

void History::Db::XAddProjectCreationMarker (ScriptHeader const & hdr, CommandList const & cmdList)
{
	Assert (cmdList.size () == 0);
	GidList emptyAckList;
	std::unique_ptr<History::Node> newNode = XLogScript (hdr, cmdList, emptyAckList);
	newNode->SetExecuted (true);
	Assert (newNode->IsMilestone ());
	Assert (newNode->GetPredecessorId () == gidInvalid);
	Assert (_setChanges.xsize () == 0);
	_setChanges.XSetFirstInterestingScriptId (newNode->GetScriptId ());
	_setChanges.XInsertNewNode (std::move(newNode));
	Notify (changeAdd, hdr.ScriptId ());
}

void History::Db::XFixProjectCreationMarker(std::string const & projectName, GlobalId scriptId)
{
	std::string comment ("Project '");
	comment += projectName;
	comment += "' creation marker";
	ScriptHeader markerHdr (ScriptKindSetChange (), gidInvalid, projectName);
	markerHdr.SetScriptId (scriptId);
	markerHdr.AddComment (comment);
	CommandList emptyCmdList;
	GidList emptyAckList;
	std::unique_ptr<History::Node> newNode = XLogScript (markerHdr, emptyCmdList, emptyAckList);
	newNode->SetExecuted (true);

	Assert (newNode->IsMilestone ());
	Assert (newNode->GetPredecessorId () == gidInvalid);
	_setChanges.XForceInsert(std::move(newNode));
}

void History::Db::XDeleteScript (GlobalId scriptId)
{
	History::Node const * deletedNode = _setChanges.XFindNode (scriptId);
	if (deletedNode == 0)
	{
		// Script not found in the set changes tree
		unsigned count = _disconnectedScripts.XCount ();
		for (unsigned i = 0; i < count; ++i)
		{
			if (_disconnectedScripts.XGet (i)->Gid () == scriptId)
			{
				_disconnectedScripts.XMarkDeleted (i);
			}
		}
	}
	else
	{
		Assert (deletedNode->IsCandidateForExecution () || deletedNode->IsMissing ()); 
		_setChanges.XDeleteRecentNode (scriptId);
		Notify (changeRemove, scriptId);
	}

	// Find next script to be executed next
	XUpdateNextMarker ();
}

History::SortedTree const & History::Db::GetTree (Unit::Type unitType) const
{
	if (unitType == Unit::Set)
	{
		return _setChanges;
	}
	else
	{
		Assert (unitType == Unit::Member);
		return _membershipChanges;
	}
}

History::SortedTree & History::Db::XGetTree (Unit::Type unitType)
{
	if (unitType == Unit::Set)
	{
		return _setChanges;
	}
	else
	{
		Assert (unitType == Unit::Member);
		return _membershipChanges;
	}
}

History::Status History::Db::XProcessLineages (ScriptHeader const & hdr,
							 Mailbox::Agent & agent, 
							 bool processMainLineage)
{
	UserId senderId = GlobalIdPack (hdr.ScriptId ()).GetUserId ();
	History::Status scriptStatus = Connected;
	if (processMainLineage)
	{
		UnitLineage lineage (hdr.GetLineage (), hdr.GetUnitType (), hdr.GetModifiedUnitId ());
		scriptStatus = XProcessUnitLineage (senderId, lineage, agent);
	}
	if (scriptStatus == Prehistoric)
		return scriptStatus;

	// Revisit: passing known missing script id and unit id in the Mailbox::Agent is not a good idea
	agent.SetKnownMissingScript (gidInvalid, gidInvalid);
	// Now process side lineages
	for (ScriptHeader::SideLineageSequencer seq (hdr); !seq.AtEnd (); seq.Advance ())
	{
		UnitLineage const & lineage = seq.GetLineage ();
		XProcessUnitLineage (senderId, lineage, agent);
	}
	return scriptStatus;
}

History::Status History::Db::XProcessUnitLineage (UserId senderId, 
								UnitLineage const & lineage, 
								Mailbox::Agent & agent)
{
	if (lineage.Count () == 0)
		return Connected;	// Empty lineage connects with history

	Unit::Type unitType = lineage.GetUnitType ();
	GlobalId unitId = lineage.GetUnitId ();
	dbg << "--> History::Db::XProcessUnitLineage" << std::endl;
	dbg << "    Lineage: " << lineage << std::endl;
	Assert ((unitType == Unit::Set && unitId == gidInvalid) || (unitType == Unit::Member && unitId != gidInvalid));

	// Receivers ignore side lineages of other receivers (who are absent from their membership list)
	if (unitType == Unit::Member)
	{
		ThisUserAgent & thisUser = agent.GetThisUserAgent ();
		if (thisUser.IsReceiver ())
		{
			if (_projectDb.XIsProjectMember (unitId))
			{
				// I know the member whose lineage I'm processing
				if (unitId != _projectDb.XGetMyId () && _projectDb.XIsReceiver (unitId))
				{
					// I know other receiver - strange!
					Assert (!"I'm a receiver and I'm processing the lineage of other receiver!");
					return Prehistoric;
				}
			}
			else if (senderId != unitId)
			{
				return Prehistoric; // Ignore lineages of other receivers (which I shouldn't know)
			}
			Assert (_projectDb.XIsProjectMember (unitId) || senderId == unitId);
		}
	}

	GlobalId referenceId = lineage.GetReferenceId ();
	if (referenceId != gidInvalid 
		&& unitType == Unit::Set // no quick exit for membership scripts
		&& (_projectDb.XIsFromFuture (referenceId) == Tri::Yes))
	{
		dbg << "<-- History::Db::XProcessUnitLineage -- reference version is from the future" << std::endl;
		XRememberDisconnectedScript (referenceId, unitType, unitId);
		if (agent.HasKnownMissingId ())
		{
			// Note: this may happen only for main lineage. 
			// Side lienages have always known missing set to gidInvalid
			XRememberDisconnectedScript (agent.GetKnownMissingId (), unitType, unitId);
		}
		return Disconnected;
	}

	UnitLineage::Sequencer lineageSeq (lineage);
	Assert (!lineageSeq.AtEnd ());
	GlobalId rootId = lineageSeq.GetScriptId ();
	Assert (rootId == referenceId);
	bool isWholeTreeLineage = false;
	if (rootId == gidInvalid)
	{
		lineageSeq.Advance ();
		if (lineageSeq.AtEnd ())
			return Connected;	// Lineage has only gidInvalid as reference id -- assume lineage connects with our history

		rootId = lineageSeq.GetScriptId ();
		isWholeTreeLineage = true;
	}
	Assert (rootId != gidInvalid);
	Assert (!lineageSeq.AtEnd ());

	bool isPartialPrehistoric = false;
	bool isNotPrehistoric = false;
	if (unitType == Unit::Set)
	{
		Assert (std::find (lineageSeq.CurrentIter (), lineageSeq.EndIter (), gidInvalid) == lineageSeq.EndIter ());
		// Skip prehistoric part of the lineage.
		int ridx = lineage.Count () - 1;
		int rendIdx = lineageSeq.CurrentIter () - lineage.begin ();
		Assert (rendIdx == 0 || rendIdx == 1);
		Assert (ridx >= rendIdx);
		if (_projectDb.XIsPrehistoric (lineage [ridx]) == Tri::Yes)
		{
			dbg << "<-- History::Db::XProcessUnitLineage -- last script id in the lineage is prehistoric" << std::endl;
			return Prehistoric;
		}
		dbg << "    Incoming script lineage is not completely prehistoric" << std::endl;

		// Iterate over lineage backwards until first prehistoric script id is found.
		// Its follower is the root id from which farther processing should start.
		GlobalId rootIdCandidate = gidInvalid;
		do
		{
			GlobalId currentScriptId = lineage [ridx];
			Tri::State currentState = _projectDb.XIsPrehistoric (currentScriptId);
			if (currentState == Tri::Yes)
			{
				Assert (ridx != lineage.Count () - 1); // tested before entering loop
				isPartialPrehistoric = true;
				break;
			}
			else if (currentState == Tri::No)
			{
				isNotPrehistoric = true;
			}
			rootIdCandidate = currentScriptId;
			--ridx;
		} while (ridx >= rendIdx);

		Assert (rootIdCandidate != gidInvalid);

		rootId = rootIdCandidate;
		// Position lineage sequencer on the root id
		while (!lineageSeq.AtEnd () && lineageSeq.GetScriptId () != rootId)
			lineageSeq.Advance ();
		Assert (_projectDb.XIsPrehistoric (rootId) != Tri::Yes);
	}
	else
	{
		// Membership update scripts cannot be prehistoric, because
		// we keep the complete membership tree for each project member
		isNotPrehistoric = true;
	}

	dbg << "    Root id: " << GlobalIdPack (rootId) << std::endl;

	History::SortedTree & tree = XGetTree (unitType);

	if (agent.HasKnownMissingId ())
	{
		History::Node const * node = tree.XFindNode (agent.GetKnownMissingId ());
		if  (node != 0)
			return Connected; // we already know this script
	}

	History::Status status;
	bool rootIdFound;
	if (tree.XProcessLineage (lineageSeq, rootIdFound, agent))
	{
		if (rootIdFound)
		{
			status = Connected;
		}
		else if (isPartialPrehistoric)
		{
			dbg << "    Script lineage doesn't connect at any script id -- prehistoric" << std::endl;
			status = Prehistoric;
		}
		else if (!isNotPrehistoric)
		{
			// We couldn't determine the lineage status.
			// Find the oldest script from the root id's sender recorded in our history.
			// If root id is numerically smaller then the oldest script found in our history 
			// or we don't find any scripts from that sender
			// then the processed lineage is considered a prehistoric lineage. Otherwise
			// we assume that processed lineage is disconnected.
			UserId rootIdSender = GlobalIdPack (rootId).GetUserId ();
			History::SortedTree::XFwdUnitSequencer historySeq (tree, gidInvalid, unitId);
			History::Node const * node = 0;
			while (!historySeq.AtEnd ())
			{
				node = historySeq.GetNode ();
				UserId senderId = GlobalIdPack (node->GetScriptId ()).GetUserId ();
				if (senderId == rootIdSender)
					break;	// Found first script from the root id sender
				historySeq.Advance ();
			}
			Assert (historySeq.AtEnd () || node != 0);
			if (historySeq.AtEnd ())
			{
				dbg << "    Script lineage is prehistoric -- no scripts recored in the history from the root id sender" << std::endl;
				status = Prehistoric;
			}
			else if (rootId < node->GetScriptId ())
			{
				dbg << "    Script lineage is prehistoric -- root id smaller then the oldest script recorded in the history from the same sender" << std::endl;
				status = Prehistoric;
			}
			else
			{
				status = Disconnected;
				dbg << "    Script lineage doesn't connect at any script id -- disconnected (2)" << std::endl;
			}
		}
		else
		{
			Assert (!isPartialPrehistoric && isNotPrehistoric);
			status = Disconnected;
			dbg << "    Script lineage doesn't connect at any script id -- disconnected" << std::endl;
		}
	}
	else
	{
		// Processed lineage is illegal. Treat it as prehistoric
		status = Prehistoric;
	}

	if (status == Disconnected)
	{
		if (isWholeTreeLineage)
		{
			UnitLineage::Sequencer lineageSeq (lineage);
			while (!lineageSeq.AtEnd () && lineageSeq.GetScriptId () != rootId)
				lineageSeq.Advance ();
			while (!lineageSeq.AtEnd ())
			{
				XRememberDisconnectedScript (lineageSeq.GetScriptId (), unitType, unitId);
				lineageSeq.Advance ();
			}
		}
		else
		{
			XRememberDisconnectedScript (rootId, unitType, unitId);
		}
		if (agent.HasKnownMissingId ())
			XRememberDisconnectedScript (agent.GetKnownMissingId (), unitType, unitId);
	}
	dbg << "<-- History::Db::XProcessUnitLineage" << std::endl;
	return status;
}

class IsEqualMissingScriptId : public std::unary_function<History::Db::MissingScriptId const *, bool>
{
public:
	IsEqualMissingScriptId (GlobalId scriptId)
		: _id (scriptId)
	{}

	bool operator () (History::Db::MissingScriptId const * id) const
	{
		return id != 0 && _id == id->Gid ();
	}

private:
	GlobalId _id;
};

void History::Db::XRememberDisconnectedScript (GlobalId scriptId, Unit::Type unitType, GlobalId unitId)
{
	TransactableArray<MissingScriptId>::const_iterator it;
	it = std::find_if (_disconnectedScripts.xbegin (), _disconnectedScripts.xend (), 
						IsEqualMissingScriptId (scriptId));
	if (it == _disconnectedScripts.xend ())
	{
		dbg << "Adding disconnected missing script to history: " << std::hex << scriptId << std::endl;
		History::SortedTree & tree = XGetTree (unitType);
		History::Node const * node = tree.XFindNode (scriptId);
		if (node != 0)
		{
			std::string msg ("Disconnected script present in the history: ");
			msg += GlobalIdPack (scriptId).ToString ();
			msg += "; unit type: ";
			if (unitType == Unit::Set)
				msg += "file change";
			else
				msg += "membership change";
			throw Win::InternalException ("Unable to process incoming script - please contact support@relisoft.com", msg.c_str ());
		}

		// Don't add member scripts from dead members to disconnected list
		if (unitType == Unit::Member)
		{
			GlobalIdPack gidPack (scriptId);
			UserId uid = gidPack.GetUserId ();
			if (_projectDb.XIsProjectMember (uid))
			{
				MemberState state = _projectDb.XGetMemberState (uid);
				if (state.IsDead ())
					return;
			}
		}
		std::unique_ptr<MissingScriptId> id (new MissingScriptId (scriptId, unitType, unitId));
		_disconnectedScripts.XAppend (std::move(id));
	}
}

class AckGatherer: public History::Gatherer
{
public:
	AckGatherer (AckBox & ackBox, Unit::Type unitType)
		: _ackBox (ackBox),
		  _unitType (unitType),
		  _seenMissing (false)
	{}

	bool ProcessNode (History::SortedTree::XFwdUnitSequencer & seq)
	{
		History::Node const * node = seq.GetNode ();
		if (!node->IsMissing () && !_seenMissing)
		{
			Unit::ScriptId scriptId (node->GetScriptId (), _unitType);
			GlobalIdPack pack (node->GetScriptId ());
			_ackBox.RememberAck (pack.GetUserId (), scriptId);
		}
		else
		{
			// ignore all descendants
			_seenMissing = true;
		}
		return true;
	}
	void Push (GlobalId branchId)
	{
		_stack.push_back (std::make_pair (branchId, _seenMissing));
	}
	bool BackTo (GlobalId branchId)
	{
		while (_stack.size () != 0 && _stack.back ().first != branchId)
		{
			_stack.pop_back ();
		}
		if (_stack.size () == 0)
			return false;
		_seenMissing = _stack.back ().second;
		return true;
	}

private:
	AckBox &	_ackBox;
	Unit::Type	_unitType;

	bool		_seenMissing;
	// branch point / ignore flag
	std::vector<std::pair<GlobalId, bool> > _stack;
};

// Returns true when script inserted successfully
bool History::Db::XInsertIncomingScript (ScriptHeader const & hdr, CommandList const & cmdList, AckBox & ackBox)
{
	bool scriptInserted = true;	// Assume script inserted successfully
	Unit::Type unitType = hdr.GetUnitType ();
	History::SortedTree & tree = XGetTree (unitType);
	// Precondition: script connects with our history
	GlobalId scriptPredecessorId = hdr.GetLineage ().GetLastScriptId ();
	GlobalId incomingScriptId = hdr.ScriptId ();
	Assert (incomingScriptId != gidInvalid);
	Assert (scriptPredecessorId == gidInvalid || tree.XFindNode (scriptPredecessorId) != 0);
	History::Node const * node = tree.XFindNode (incomingScriptId);
	if  (node == 0)
	{
		// Script not recorded yet -- insert it to the history (not confirmed)
		std::unique_ptr<History::Node> newNode = XLogIncomingScript (hdr, cmdList, false, ackBox);
		Assert (hdr.ScriptId () == newNode->GetScriptId ());
		if (!tree.XInsertNewNode (std::move(newNode)))
		{
			// Inserted node has missing predecessors -- don't acknowledge it
			Unit::ScriptId id (incomingScriptId, unitType);
			GlobalIdPack pack (incomingScriptId);
			ackBox.RemoveAckForScript (pack.GetUserId (), id);
		}
		if (unitType == Unit::Set)
		{
			History::Node const * insertedNode = tree.XFindNode (incomingScriptId);
			Assume (insertedNode != 0, GlobalIdPack (incomingScriptId).ToBracketedString ().c_str ());
			if (insertedNode->IsRejected ())
			{
				// Incoming script was rejected without executing.
				// Don't acknowledge rejected set script, because
				// other project members still may have it as executed
				// in their histories. We don't want this script to become
				// confirmed and then rejected.
				Unit::ScriptId id (incomingScriptId, unitType);
				GlobalIdPack pack (incomingScriptId);
				ackBox.RemoveAckForScript (pack.GetUserId (), id);
			}
			Notify (changeAdd, incomingScriptId);	// Only set changes are displayed in the history view
	    }
	}
	else if (node->IsMissing ())
	{
		bool missingScriptIsRejected = node->IsRejected ();
		bool missingScriptNodeIsBranchPoint = node->IsBranchPoint ();
		Assume (node->GetPredecessorId () == scriptPredecessorId || scriptPredecessorId == gidInvalid,
				GlobalIdPack (scriptPredecessorId).ToBracketedString ().c_str ());
		// We have received missing script -- insert it and update history node
		// Replace placeholder node with the real thing
		std::unique_ptr<History::Node> newNode = XLogIncomingScript (hdr, cmdList, node->IsAckListEmpty (), ackBox);
		if (scriptPredecessorId == gidInvalid)
		{
			// Lineage doesn't specify predecessor -- copy predecessor from missing node
			newNode->SetPredecessorId (node->GetPredecessorId ());
		}
		newNode->SetRejected (missingScriptIsRejected);
		newNode->SetBranchPoint (missingScriptNodeIsBranchPoint);
		if (tree.XSubstituteNode (std::move(newNode)))
		{
			// Substituted node doesn't have missing predecessors.
			// Collect any delayed acknowledgments in this tree branch.
			History::SortedTree::XFwdUnitSequencer seq (tree, incomingScriptId);
			AckGatherer ackGatherer (ackBox, unitType);
			XTraverseTree (seq, ackGatherer);
		}
		else
		{
			// Substituted node has missing predecessors -- don't acknowledge it
			Unit::ScriptId id (incomingScriptId, unitType);
			GlobalIdPack pack (incomingScriptId);
			ackBox.RemoveAckForScript (pack.GetUserId (), id);
		}

		if (unitType == Unit::Set)
		{
			// When this script was missing script was not displayed in the history view, only in the mailbox view.
			// Add it now to the history view.
			Notify (changeAdd, incomingScriptId);
		}
	}
	else
	{
		// We already have this script recorded in the history.
		// Check if this is the same script as the incoming one.
		// If two different scripts in this project use the same script id then we have serious problem.
		scriptInserted = XIsDuplicateScript (node, cmdList);
	}
	// Check if the incoming script was listed as missing script from the future
	dbg << "    Removing incoming script from the disconnected list." << std::endl;
	XRemoveDisconnectedScript (incomingScriptId, unitType);
	return scriptInserted;
}

void History::Db::XInsertLabelScript (ScriptHeader const & hdr)
{
	GlobalId predecessorId = hdr.GetLineage ().GetLastScriptId ();
	GlobalId thisScriptId = hdr.ScriptId ();
	Assert (thisScriptId != gidInvalid);
	Assert (_setChanges.XFindNode (predecessorId) != 0);
	Assert (_setChanges.XFindNode (thisScriptId) == 0);
	CommandList emptyCmdList;
	GidList emptyAckList;
	std::unique_ptr<History::Node> newNode = XLogScript (hdr, emptyCmdList, emptyAckList);
	newNode->SetExecuted (true);
	Assert (hdr.ScriptId () == newNode->GetScriptId ());
	_setChanges.XForceInsert (std::move(newNode));

	// find the script whose predecessor is the predecessor of this script
	// and change its predecessor to this script ID
	History::SortedTree::XFwdUnitSequencer seq (_setChanges, predecessorId);
	Assert (!seq.AtEnd ());
	seq.Advance ();
	Assert (seq.GetNode ()->GetScriptId () == thisScriptId);
	seq.Advance ();
	while (!seq.AtEnd ())
	{
		History::Node const * node = seq.GetNode ();
		if (node->GetPredecessorId () == predecessorId && node->IsExecuted ())
		{
			History::Node * node = seq.GetNodeEdit ();
			node->SetPredecessorId (thisScriptId);
			break;
		}
		seq.Advance ();
	}
}

void History::Db::XSubstituteScript (ScriptHeader const & hdr, CommandList const & cmdList, AckBox & ackBox)
{
	Assert (hdr.GetUnitType () == Unit::Set);
	Assert (hdr.ScriptId () != gidInvalid);
	Assert (_setChanges.XFindNode (hdr.ScriptId ()) != 0 &&
			_setChanges.XFindNode (hdr.ScriptId ())->IsCandidateForExecution ());
	std::unique_ptr<History::Node> newNode = XLogIncomingScript (hdr, cmdList, false, ackBox);
	_setChanges.XSubstituteNode (std::move(newNode));
}

// Used only during full sync unpacking
void History::Db::XInsertExecutedMembershipScript (ScriptHeader const & hdr, CommandList const & cmdList)
{
	GidList emptyAckList;
	Assert (hdr.GetUnitType () == Unit::Member);
	std::unique_ptr<History::Node> newNode = XLogScript (hdr, cmdList, emptyAckList);
	newNode->SetForceExec (false);
	newNode->SetExecuted (true);
	// Precondition: script connects with our history
	GlobalId scriptPredecessorId = hdr.GetLineage ().GetLastScriptId ();
	GlobalId incomingScriptId = hdr.ScriptId ();
	Assert (incomingScriptId != gidInvalid);
	Assert (scriptPredecessorId == gidInvalid || _membershipChanges.XFindNode (scriptPredecessorId) != 0);
	Assert (_membershipChanges.XFindNode (incomingScriptId) == 0);
	Assert (incomingScriptId == newNode->GetScriptId ());
	_membershipChanges.XInsertNewNode (std::move(newNode));
}

// Returns true it we have identical script already recorded in the history
bool History::Db::XIsDuplicateScript (History::Node const * node, CommandList const & scriptCmdList) const
{
	// We have already received this script
	CommandList historyCmdList;
	XRetrieveCmdList (node, historyCmdList);
	bool isEqual = historyCmdList.IsEqual (scriptCmdList);
	if (!isEqual)
	{
		// Two different scripts in this project use the same script id.
		// There is one case when this is OK.  Joining project enlistment receives
		// in the full sync the script id of administrator's last confirmed change
		// script. This id is then assigned to the initial file inventory script
		// contained in the full sync script. Obviously the administrator's last
		// confirmed change script is a different script then the initial file inventory
		// script.
		isEqual = node->IsInventory ();
	}
	return isEqual;
}

std::unique_ptr<History::Node> History::Db::XLogCheckinScript (ScriptHeader const & hdr, 
										   CommandList const & cmdList)
{
	GidList ackList;
	if (hdr.IsMembershipChange ())
	{
		if (hdr.GetModifiedUnitId () == _projectDb.XGetMyId ())
		{
			// My membership update
			// Awaiting acknowledgement from all project members (voting and observers)
			// Note: receivers are voting members
			// Note: receiver doesn't know other receivers
			_projectDb.XGetAllMemberList (ackList);
		}
		else
		{
			// Someone else membership update
			MemberState myState = _projectDb.XGetMemberState (_projectDb.XGetMyId ());
			Assert (_projectDb.XIsProjectMember (hdr.GetModifiedUnitId ()));
			Assert (myState.IsAdmin () ||
					_projectDb.XGetMemberState (hdr.GetModifiedUnitId ()).IsAdmin () ||
					_projectDb.XGetMemberState (hdr.GetModifiedUnitId ()).IsObserver ());
			if (myState.IsDistributor ())
			{
				MemberState changedMemberState = _projectDb.XGetMemberState (hdr.GetModifiedUnitId ());
				if (changedMemberState.IsReceiver ())
				{
					// I'am a distributor and changed member is receiver -- don't await
					// membership update acknowledgment from other receivers
					GidSet filterOut;
					filterOut.insert (_projectDb.XGetMyId ());
					_projectDb.XGetReceivers (filterOut);	// Includes the changed receiver
					Assert (filterOut.find (hdr.GetModifiedUnitId ()) != filterOut.end ());
					if (!hdr.IsDefectOrRemove ())
					{
						// Don't filter out the changed receiver, because he is the only
						// receiver who has to acknowledge this membership update
						filterOut.erase (hdr.GetModifiedUnitId ());
					}
					_projectDb.XGetMulticastList (filterOut, ackList);
				}
				else
				{
					// Some other distributor membership update
					// Awaiting acknowledgement from all project members (voting and observers)
					// Note: receivers are voting members
					_projectDb.XGetAllMemberList (ackList);
				}
			}
			else
			{
				// I'm not a distributor
				// Awaiting acknowledgement from all project members (voting and observers)
				_projectDb.XGetAllMemberList (ackList);
			}
		}
	}
	else
	{
		// Awaiting acknowledgement from all voting members
		// Note: receivers are voting members
		Assert (!_projectDb.XGetMemberState (_projectDb.XGetMyId ()).IsReceiver ());
		_projectDb.XGetVotingList (ackList);
	}
	return XLogScript (hdr, cmdList, ackList);
}

std::unique_ptr<History::Node> History::Db::XLogMyRemoval (ScriptHeader const & hdr, 
									   CommandList const & cmdList)
{
	Assert (hdr.GetModifiedUnitId () == _projectDb.XGetMyId ());
	GidList ackList;
	GlobalIdPack pack (hdr.ScriptId ());
	UserId senderId = pack.GetUserId ();
	ackList.push_back (senderId);
	// Log my removal with non-empty acknowledgment list, so
	// it doesn't look as confirmed, because before my removal
	// script there can be other unconfirmed scripts.
	return XLogScript (hdr, cmdList, ackList);
}

std::unique_ptr<History::Node> History::Db::XLogMemberRemoval (ScriptHeader const & hdr,
										   CommandList const & cmdList,
										   AckBox & ackBox)
{
	Assert (hdr.GetModifiedUnitId () != _projectDb.XGetMyId ());
	// Some project member defects or is removed from the project.
	// Wait for the acknowledgments from all other project members
	// (voting or observers) except the defecting user and myself.
	GidSet filterOut;
	filterOut.insert (hdr.GetModifiedUnitId ());
	filterOut.insert (_projectDb.XGetMyId ());
	MemberState myState = _projectDb.XGetMemberState (_projectDb.XGetMyId ());
	if (myState.IsDistributor ())
	{
		// I'am a distributor -- check if receiver is removed
		if (_projectDb.XIsProjectMember (hdr.GetModifiedUnitId ()))
		{
			// The defecting member is recorded in my database
			MemberState removedUserState = _projectDb.XGetMemberState (hdr.GetModifiedUnitId ());
			if (removedUserState.IsReceiver ())
			{
				// Receiver is removed from the project -- don't await
				// removal acknowledgment from other receivers
				_projectDb.XGetReceivers (filterOut);
			}
		}
		else
		{
			// We don't know the removed user -- play safe and don't
			// expect acknowledgments from receivers
			_projectDb.XGetReceivers (filterOut);
		}
	}
	GidList ackList;
	_projectDb.XGetMulticastList (filterOut, ackList);
	Unit::ScriptId acknowledgedScriptId (hdr.ScriptId (), hdr.GetUnitType ());
	// Send acknowledgement script only to those from whom you are expecting acknowledgment
	for (GidList::const_iterator iter = ackList.begin (); iter != ackList.end (); ++iter)
		ackBox.RememberAck (*iter, acknowledgedScriptId);
	return XLogScript (hdr, cmdList, ackList);
}

std::unique_ptr<History::Node> History::Db::XLogIncomingScript (ScriptHeader const & hdr,
											CommandList const & cmdList,
											bool isConfirmed,
											AckBox & ackBox)
{
	Assert (_projectDb.XGetMyId () != gidInvalid);
	if (hdr.IsMembershipChange ())
	{
		// Check if commands and header data match
		CommandList::Sequencer seq (cmdList);
		if (hdr.IsAddMember ())
		{
			if (seq.GetMemberCmd ().GetType () != typeNewMember)
				throw Win::Exception ("Corrupted membership update script: illegal add member command");
		}
		else if (hdr.IsEditMember ())
		{
			if (seq.GetMemberCmd ().GetType () != typeEditMember)
				throw Win::Exception ("Corrupted membership update script: illegal edit member command");
		}
		else
		{
			if (seq.GetMemberCmd ().GetType () != typeDeleteMember)
				throw Win::Exception ("Corrupted membership update script: illegal defect command");
		}
	}

	if (hdr.IsDefectOrRemove ())
	{
		CommandList::Sequencer seq (cmdList);
		if (hdr.GetModifiedUnitId () == _projectDb.XGetMyId ())
			return XLogMyRemoval (hdr, cmdList);
		else
			return XLogMemberRemoval (hdr, cmdList, ackBox);
	}
	else
	{
		GlobalIdPack pack (hdr.ScriptId ());
		UserId senderId = pack.GetUserId ();
		GidList ackList;
		if (!isConfirmed)
		{
			// Awaiting acknowledgement from the script sender
			ackList.push_back (senderId);
		}
		MemberState myState = _projectDb.XGetMemberState (_projectDb.XGetMyId ());
		// Voting members acknowledge all script kinds.
		// Observers acknowledge only membership update scripts.
		if (myState.IsVotingMember () || hdr.IsMembershipChange ())
		{
			Unit::ScriptId acknowledgedScriptId (hdr.ScriptId (), hdr.GetUnitType ());
			// Acknowledge only to script sender.
			ackBox.RememberAck (senderId, acknowledgedScriptId);
		}
		return XLogScript (hdr, cmdList, ackList);
	}
}

std::unique_ptr<History::Node> History::Db::XLogScript (ScriptHeader const & hdr,
									CommandList const & cmdList,
									GidList const & ackList)
{
	std::unique_ptr<HdrNote> hdrNote (new HdrNote (hdr));
	std::unique_ptr<History::Node> newNode (new History::Node (_hdrValidEnd.XGet (), hdr, ackList));
	newNode->SetMilestone (cmdList.size () == 0);
	_hdrValidEnd.XSet (_hdrLog.Append (_hdrValidEnd.XGet (), *hdrNote));
	// Add script command list to the history node
	if (hdr.IsSetChange ())
	{
		for (CommandList::Sequencer seq (cmdList); !seq.AtEnd (); seq.Advance ())
		{
			FileCmd const & fileCmd = seq.GetFileCmd ();
			GlobalId fileId = fileCmd.GetGlobalId ();
			Assert (fileId != gidInvalid);
			std::unique_ptr<History::Node::Cmd> cmd (new History::Node::Cmd (fileId, _cmdValidEnd.XGet ()));
			_cmdValidEnd.XSet (_cmdLog.Append (_cmdValidEnd.XGet (), fileCmd));
			newNode->AddCmd (std::move(cmd));
		}
	}
	else
	{
		Assert (hdr.IsMembershipChange ());
		for (CommandList::Sequencer seq (cmdList); !seq.AtEnd (); seq.Advance ())
		{
			MemberCmd const & memberCmd = seq.GetMemberCmd ();
			UserId userId = memberCmd.GetUserId ();
			if (!hdr.IsVersion40EmergencyAdminElection () && userId != hdr.GetModifiedUnitId ())
			{
				std::string info ("Recorded project member id: ");
				info += ToHexString (hdr.GetModifiedUnitId ());
				info += "; changed project member id: ";
				info += ToHexString (userId);
				throw Win::Exception ("Corrupted membership update script: project member id mismatch", info.c_str ());
			}
			Assert (userId != gidInvalid);
			std::unique_ptr<History::Node::Cmd> cmd (new History::Node::Cmd (userId, _cmdValidEnd.XGet ()));
			_cmdValidEnd.XSet (_cmdLog.Append (_cmdValidEnd.XGet (), memberCmd));
			newNode->AddCmd (std::move(cmd));
		}
		newNode->SetDefect (hdr.IsDefectOrRemove ());
	}
	return newNode;
}

void History::Db::XRemoveMissingInventoryMarker ()
{
	Assume (!_initialFileInventoryIsExecuted, "History::XRemoveMissingInventoryMarker");			// History not initialized yet
	if (_setChanges.xsize () == 2)
	{
		// Only project creation marker and missing inventory marker were recorded in the set history
		History::Node const * missingInventory = _setChanges.XGetNode (1);
		Assume (missingInventory->IsMissing (), ::ToHexString (missingInventory->GetFlags ()).c_str ());
		_setChanges.XDeleteNodeByIdx (1);
	}
}

void History::Db::XAddMissingInventoryMarker (GlobalId inventoryId)
{
	Assert (!_initialFileInventoryIsExecuted);			// History not initialized yet
	if (_setChanges.xsize () == 1)
	{
		// Only project creation marker recorded in the set history
		History::Node const * projectCreationMarker = _setChanges.XGetNode (0);
		Assert (projectCreationMarker->IsMilestone () &&
				projectCreationMarker->IsExecuted () &&
				projectCreationMarker->IsAckListEmpty ());
		std::unique_ptr<History::Node> node (new MissingInventory (inventoryId,
														projectCreationMarker->GetScriptId ()));
		_setChanges.push_back (std::move(node));
	}
}

void History::Db::XPushBackScript (ScriptHeader const & hdr, CommandList const & cmdList)
{
	Assert (hdr.IsSetChange ());
	GidList ackList;
	// All scripts except initial file inventory await acknowledgement
	// from the script author
	if (_nextScriptId.XGet () != gidInvalid)
	{
		GlobalIdPack pack (hdr.ScriptId ());
		UserId senderId = pack.GetUserId ();
		ackList.push_back (senderId);
	}
	std::unique_ptr<History::Node> node = XLogScript (hdr, cmdList, ackList);
	if (_nextScriptId.XGet () == gidInvalid)
	{
		Assert (_setChanges.xsize () <= 2);
		// First set change script is our initial file inventory
		node->SetInventory (true);
		// First set change script is our next to be unpacked script
		_nextScriptId.XSet (node->GetScriptId ());
   		// If auto full sync is on mark initial file inventory for forced execution
		node->SetForceExec (_projectDb.XIsAutoFullSynch ());
		// Correct project creation marker script id and set it
		// as initial file inventory predecessor
		History::SortedTree::XFwdUnitSequencer seq (_setChanges, gidInvalid);
		Assert (!seq.AtEnd ());
		History::Node * projectCreationMarker = seq.GetNodeEdit ();
		Assert (projectCreationMarker->IsMilestone () && projectCreationMarker->IsExecuted () && projectCreationMarker->IsAckListEmpty ());
		GlobalIdPack pack (_projectDb.XGetMyId (), 0);
		projectCreationMarker->SetScriptId (pack);
		node->SetPredecessorId (pack);
		_setChanges.XSetFirstInterestingScriptId (pack);
		seq.Advance ();
		if (!seq.AtEnd ())
		{
			// We have seen full sync script chunk first and the missing full sync placeholder
			// was inserted into the history.  Now replace the missing script placeholder with
			// the real thing.
			Assert (seq.GetNode ()->IsMissing () && seq.GetNode ()->GetScriptId () == node->GetScriptId ());
			node->SetForceAccept (true);
			_setChanges.XSubstituteNode (std::move(node));
			return;
		}
	}
	else
	{
		// All other set changes are forced
		node->SetForceExec (true);
	}
	node->SetForceAccept (true);
	_setChanges.push_back (std::move(node));
}

void History::Db::XArchive (GlobalId scriptId)
{
	History::SortedTree::XFullSequencer seq (_setChanges, scriptId);
	Assert (!seq.AtEnd ());
	History::Node * node = seq.GetNodeEdit ();
	Assert (node->IsConfirmedScript ());
	node->SetArchive (true);
	_isArchive = true;
	Notify (changeEdit, scriptId);
}

void History::Db::XUnArchive ()
{
	Assert (_isArchive);
    // History iterator walks history notes in reverse chronological order
	for (History::SortedTree::XFullSequencer seq (_setChanges); !seq.AtEnd (); seq.Advance ())
    {
        History::Node const * node = seq.GetNode ();
        if (node->IsArchive ())
        {
			if (_isArchive)
			{
				_isArchive = false;
				// Unarchive first archived node
				History::Node * unarchivedNode = seq.GetNodeEdit ();
				unarchivedNode->SetArchive (false);
				Notify (changeAdd, unarchivedNode->GetScriptId ());
			}
			else
			{
				_isArchive = true;
				break;
			}
        }
    }
}

void History::Db::XPrepareForBranch (MemberDescription const & branchCreator,
							std::string const & branchProjectName,
							GlobalId branchVersionId,
							Project::Options const & options)
{
	// Remove all current membership history
	for (History::SortedTree::XWriteSequencer seq (_membershipChanges); !seq.AtEnd (); seq.Advance ())
		seq.MarkDeleted ();

	AckBox fakeAckBox;
	// Add branch administrator to the membership tree.
	// Branch creator becomes the branch project administrator preserving its user id
	// and uses description provided in the project branch dialog.
	StateAdmin admin;
	if (options.IsDistribution ())
	{
		admin.SetDistributor (true);
		admin.SetNoBranching (options.IsNoBranching ());
	}
	MemberInfo branchAdmin (_projectDb.XGetMyId (), admin, branchCreator);
	MemberNameTag tag (branchAdmin.Name (), _projectDb.XGetMyId ());
	MembershipUpdateComment adminComment (tag, "becomes the project branch administrator");
	ScriptHeader hdr (ScriptKindAddMember (),
					  _projectDb.XGetMyId (),
					  branchProjectName);
	hdr.SetScriptId (_projectDb.XMakeScriptId ());
	hdr.AddComment (adminComment);
	CommandList cmdList;
	std::unique_ptr<ScriptCmd> cmd (new NewMemberCmd (branchAdmin));
	cmdList.push_back (std::move(cmd));
	XGetLineages (hdr, UnitLineage::Empty);	// Don't add side lineages
	XAddCheckinScript (hdr, cmdList, fakeAckBox);
	// Update project database
	_projectDb.XUpdate (branchAdmin);

	GlobalId fisId = _setChanges.XGetFirstInterestingScriptId ();
	if (branchVersionId != gidInvalid)
	{
		// Branch at historical version -- remove from history all versions stored after selected version
		History::SortedTree::XFwdUnitSequencer seq (_setChanges, branchVersionId);
		Assert (!seq.AtEnd ());
		seq.Advance ();	// Skip branch version, which stays in the branch history
		for ( ; !seq.AtEnd (); seq.Advance ())
		{
			History::Node const * node = seq.GetNode ();
			if (node->GetScriptId () == fisId)
			{
				// Removing first interesting script -- reset marker
				Assert (node->IsAckListEmpty ());
				_setChanges.XSetFirstInterestingScriptId (gidMarker);
				// branch point is earlier than FIS, no need to mark deep fork
				fisId = gidInvalid;
			}
			seq.MarkDeleted ();
		}
	}
	else
	{
		// Branch at current version -- remove all candidates for execution and missing scripts
		for (History::SortedTree::XWriteSequencer seq (_setChanges); !seq.AtEnd (); seq.Advance ())
		{
			History::Node const * node = seq.GetNode ();
			if (node->IsExecuted ())
				break;
			seq.MarkDeleted ();
		}
	}

	_nextScriptId.XSet (gidInvalid);

	// Mark branch project history.
	// We add label to the branch project history. The label script is immediately confirmed,
	// because there is only one project member -- the branch administrator. 
	// Inserting a confirmed script into the branch history
	// will confirm all older set scripts and move FIS marker.
	if (fisId != gidInvalid  &&
		(branchVersionId == gidInvalid && fisId != XMostRecentScriptId () || 
		 branchVersionId != gidInvalid && XPrecedes (fisId, branchVersionId)))
	{
		// We also mark the FIS as deep fork, so that, if in the original project
		// the branchpoint is later rejected, we will still be able to match the two branches.
		XMarkDeepFork (fisId);
	}

	ScriptHeader labelHdr (ScriptKindSetChange (), gidInvalid, branchProjectName);
	XGetLineages (labelHdr, UnitLineage::Empty);	// Don't add side lineages
	GlobalId scriptId = _projectDb.XMakeScriptId ();
	labelHdr.SetScriptId (scriptId);
	StrTime localTime (CurrentTime ());
	std::string comment ("Branch '");
	comment += branchProjectName;
	comment += "' created ";
	comment += localTime.GetString ();
	labelHdr.AddComment (comment);
	CommandList emptyCmdList;
	XAddCheckinScript (labelHdr, emptyCmdList, fakeAckBox);
}

void History::Db::XCleanupMembershipUpdates ()
{
	// Confirm all membership updates and remove missing ones
	for (History::SortedTree::XWriteSequencer seq (_membershipChanges); !seq.AtEnd (); seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		if (node->IsMissing ())
		{
			seq.MarkDeleted ();
		}
		else if (!node->IsAckListEmpty ())
		{
			History::Node * editedNode = seq.GetNodeEdit ();
			editedNode->ClearAckList ();
		}
	}
}

void History::Db::XAcceptAck (GlobalId scriptId, UserId senderId, Unit::Type unitType, AckBox & ackBox, bool broadcastMakeRef)
{
	History::SortedTree & tree = XGetTree (unitType);
	History::SortedTree::XSequencer seq (tree, scriptId);
	if (seq.AtEnd ())
		return;	// Script already confirmed -- ignore this acknowledgement

	History::Node * acknowledgedNode = seq.GetNodeEdit ();
	acknowledgedNode->AcceptAck (senderId);
	if (acknowledgedNode->IsAckListEmpty ())
	{
		// History::Node becomes confirmed
		if (unitType == Unit::Set)
		{
			tree.XConfirmAllOlderNodes (seq);
		}
		else
		{
			Assert (unitType == Unit::Member);
			tree.XConfirmOlderPredecessors (seq);
		}
	}
	if (acknowledgedNode->IsAckListEmpty () && broadcastMakeRef)
	{
		// Script becomes confirmed and we can broadcast make reference
		GlobalIdPack acknowledgedScriptId (scriptId);
		if (acknowledgedScriptId.GetUserId () == _projectDb.XGetMyId ())
		{
			// This user is the script author -- remember to broadcast make reference
			Unit::ScriptId makeRefId (scriptId, unitType);
			if (unitType == Unit::Set)
			{
				// Send make reference script to all project members
				ackBox.RememberMakeRef (makeRefId);
				// Update our history view -- script becomes confirmed
				Notify (changeEdit, scriptId);
			}
			else
			{
				Assert (unitType == Unit::Member);
				MemberState myState = _projectDb.XGetMemberState (_projectDb.XGetMyId ());
				if (myState.IsDistributor ())
				{
					// Check whose membership change I'm marking as reference version
					UserId userId = acknowledgedNode->GetUnitId (unitType);
					Assert (_projectDb.XIsProjectMember (userId));
					MemberState userState = _projectDb.XGetMemberState (userId);
					if (userState.IsReceiver ())
					{
						// Send make reference script only to full members and this receiver
						GidSet filterOut;
						_projectDb.XGetReceivers (filterOut);
						filterOut.insert (_projectDb.XGetMyId ());
						GidList fullMembers;
						_projectDb.XGetMulticastList (filterOut, fullMembers);
						for (GidList::const_iterator iter = fullMembers.begin ();
							 iter != fullMembers.end ();
							 ++iter)
						{
							ackBox.RememberMakeRef (*iter, makeRefId);
						}
						ackBox.RememberMakeRef (userId, makeRefId);
					}
					else
					{
						// Send make reference script to all project members
						ackBox.RememberMakeRef (makeRefId);
					}
				}
				else
				{
					// Send make reference script to all project members
					ackBox.RememberMakeRef (makeRefId);
				}
			}
		}
	}
}

// We don't expect acknowledgments from this user any more
// He might be dead or changed to observer
void History::Db::XRemoveFromAckList (History::SortedTree & tree, UserId userId, bool isDead, AckBox & ackBox)
{
	Unit::Type unitType = tree.GetUnitType ();
	for (History::SortedTree::XSequencer seq (tree); !seq.AtEnd (); seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		if (!node->IsAckListEmpty ())
		{
			GidList const & ackList = node->GetAckList ();
			GidList::const_iterator iter = std::find (ackList.begin (), ackList.end (), userId);
			if (iter != ackList.end ())
			{
				// The user is on this script acknowledgment list
				GlobalIdPack scriptId (node->GetScriptId ());
				if (!isDead && scriptId.GetUserId () == userId)
					continue;	// We still wait for the final acknowledgment from the script creator (he's not dead!)

				// Pretend we have received an acknowledgement from the user
				XAcceptAck (scriptId, userId, unitType, ackBox);
			}
		}
	}
}

// We don't expect acknowledgments from this user any more
// He might be dead or changed to observer
void History::Db::XRemoveFromAckList (UserId userId, bool isDead, AckBox & ackBox)
{
	Assert (userId != gidInvalid);
	XRemoveFromAckList (_setChanges, userId, isDead, ackBox);
	// Revisit: should we remove observers (!isDead) from acknowledgment lists
	// in the membership change tree? Observers acknowledge membership updates.
	XRemoveFromAckList (_membershipChanges, userId, isDead, ackBox);
}

void History::Db::XRemoveDisconnectedScript (GlobalId scriptId, Unit::Type unitType)
{
	if (_disconnectedScripts.XActualCount () != 0)
	{
		TransactableArray<MissingScriptId>::const_iterator it;
		it = std::find_if (_disconnectedScripts.xbegin (), _disconnectedScripts.xend (), 
							IsEqualMissingScriptId (scriptId));
		if (it != _disconnectedScripts.xend ())
		{
			// No longer missing from the future
			unsigned int idx = _disconnectedScripts.XtoIndex (it);
			dbg << "Removing script from disconnected list -- " << GlobalIdPack (scriptId) << std::endl;
			_disconnectedScripts.XMarkDeleted (idx);
		}
	}
}

bool History::Db::HasIncomingOrMissingScripts () const
{
	for (History::SortedTree::Sequencer seq (_setChanges); !seq.AtEnd (); seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		if (node->IsCandidateForExecution () || node->IsMissing ())
			return true;
		else if (node->IsExecuted ())
			break;
	}
	return false;
}

bool History::Db::HasToBeRejectedScripts () const
{
	for (History::SortedTree::Sequencer seq (_setChanges); !seq.AtEnd (); seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		if (node->IsToBeRejected ())
			return true;
		else if (node->IsExecuted ())
			break;
	}
	return false;
}

unsigned int History::Db::GetIncomingScriptCount () const
{
	unsigned int count = 0;
	for (History::SortedTree::Sequencer seq (_setChanges); !seq.AtEnd (); seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		if (node->IsCandidateForExecution ())
			count++;
		else if (node->IsExecuted ())
			break;
	}
	return count;

}

bool History::Db::HasMissingScripts () const
{
	if (_setChanges.HasMissingScripts ())
		return true;
	if (_membershipChanges.HasMissingScripts ())
		return true;
	if (_disconnectedScripts.Count () != 0)
		return true;
	return false;
}

bool History::Db::HasMembershipUpdatesFromFuture () const
{
	for (TransactableArray<MissingScriptId>::const_iterator iter = _disconnectedScripts.begin ();
		 iter != _disconnectedScripts.end ();
		 ++iter)
	{
		MissingScriptId const * missingScript = *iter;
		if (missingScript->Type () == Unit::Member)
			return true;
	}
	return false;
}

void History::Db::GetMissingScripts (Unit::ScriptList & scriptIdList) const
{
	_setChanges.GetMissingScripts (scriptIdList);
	_membershipChanges.GetMissingScripts (scriptIdList);
	for (TransactableArray<MissingScriptId>::const_iterator iter = _disconnectedScripts.begin ();
		iter != _disconnectedScripts.end (); ++iter)
	{
		MissingScriptId const * id = *iter;
		dbg << "Detached missing script: " << *id << std::endl;
		scriptIdList.push_back (Unit::ScriptId (id->Gid (), id->Type ()));
	}
}

void History::Db::GetDisconnectedMissingSetScripts (GidList & scripts) const
{
	for (TransactableArray<MissingScriptId>::const_iterator iter = _disconnectedScripts.begin ();
		iter != _disconnectedScripts.end (); ++iter)
	{
		MissingScriptId const * script = *iter;
		if (script->Type () == Unit::Set)
			scripts.push_back (script->Gid ());
	}
}

bool History::Db::CanDelete (GlobalId scriptId) const
{
	History::Node const * node = _setChanges.FindNode (scriptId);
	if (node == 0)
		return true;	// Script not recorded in the history can be deleted

	return node->IsMissing ();
}

bool History::Db::IsNext (GlobalId scriptId) const
{
	if (HasNextScript ())
		return scriptId == _nextScriptId.Get ();
	return false;
}

bool History::Db::IsCandidateForExecution (GlobalId scriptId) const
{
	History::Node const * node = _setChanges.FindNode (scriptId);
	if (node != 0)
	{
		return node->IsCandidateForExecution ();
	}
	return false;
}

bool History::Db::IsBranchPoint (GlobalId scriptId) const
{
	History::Node const * node = _setChanges.FindNode (scriptId);
	if (node != 0)
	{
		return node->IsBranchPoint ();
	}
	return false;
}

bool History::Db::IsExecuted (GlobalId scriptId) const
{
	History::Node const * node = _setChanges.FindNode (scriptId);
	if (node != 0)
	{
		return node->IsExecuted () || node->IsToBeRejected ();
	}
	return false;
}

bool History::Db::IsMissing (GlobalId scriptId) const
{
	History::Node const * node = _setChanges.FindNode (scriptId);
	if (node != 0)
	{
		return node->IsMissing ();
	}
	return false;
}

bool History::Db::HasMissingPredecessors (GlobalId scriptId) const
{
	for (History::SortedTree::PredecessorSequencer seq (_setChanges, scriptId);
		 !seq.AtEnd ();
		 seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		if (node->IsExecuted ())
			return false;
		else if (node->IsMissing ())
			return true;
	}
	return false;
}

bool History::Db::IsRejected (GlobalId scriptId) const
{
	History::Node const * node = _setChanges.FindNode (scriptId);
	if (node != 0)
	{
		return node->IsRejected ();
	}
	return false;
}

bool History::Db::IsAtTreeTop (GlobalId scriptId) const
{
	Assert (_setChanges.size () != 0);
	unsigned int topIdx = _setChanges.size () - 1;
	History::Node const * topNode = _setChanges.GetNode (topIdx);
	return topNode->GetScriptId () == scriptId;
}

bool History::Db::IsCurrentVersion (GlobalId scriptId) const
{
	for (History::SortedTree::FullSequencer seq (_setChanges, gidInvalid);// Start from the current version
		 !seq.AtEnd ();
		 seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		if (node->IsExecuted () || node->IsToBeRejected ())
			return node->GetScriptId () == scriptId;
	}
	return false;
}

bool History::Db::CanDeleteMissing (GlobalId scriptId, std::string & whyCannotDelete) const
{
	History::Node const * node = _setChanges.FindNode (scriptId);
	Assume (node != 0, GlobalIdPack (scriptId).ToBracketedString ().c_str ());
	if (node->HasBeenUnpacked ())
	{
		whyCannotDelete = "Missing script has already been acknowledged\n"
			   "(You must have deleted it from your inbox earlier).\n\n"
			   "The missing entry cannot be deleted.";
		return false;
	}
	else
	{
		UserId senderId = GlobalIdPack (scriptId).GetUserId ();
		MemberState state = _projectDb.GetMemberState (senderId);
		if (!state.IsDead () || (node->IsAckListEmpty () && _projectDb.MemberCount () != 1))
		{
			if (node->IsAckListEmpty ())
				whyCannotDelete = "Missing script has already been acknowledged by all project members.\n"
					   "The missing entry cannot be deleted.";
			else
				whyCannotDelete = "Missing script sender is still a voting member in this project.\n"
					   "The missing entry cannot be deleted.";
			return false;
		}
	}
	Assert (_projectDb.GetMemberState (GlobalIdPack (scriptId).GetUserId ()).IsDead () &&
		    (!node->IsAckListEmpty () || _projectDb.MemberCount () == 1));
	return true;
}

bool History::Db::HasNextScript () const
{
	return _nextScriptId.Get () != gidInvalid;
}

bool History::Db::NextIsFullSynch () const
{
	if (HasNextScript ())
	{
		History::Node const * node = _setChanges.FindNode (_nextScriptId.Get ());
		Assume (node != 0, GlobalIdPack (_nextScriptId.Get ()).ToBracketedString ().c_str ());
		Assert (node->IsCandidateForExecution ());
		return node->IsInventory ();
	}
	return false;
}

bool History::Db::NextIsMilestone () const
{
	if (HasNextScript ())
	{
		History::Node const * node = _setChanges.FindNode (_nextScriptId.Get ());
		Assume (node != 0, GlobalIdPack (_nextScriptId.Get ()).ToBracketedString ().c_str ());
		Assert (node->IsCandidateForExecution ());
		return node->IsMilestone();
	}
	return false;
}

#if !defined (NDEBUG)
void History::Db::XMarkUndoneScript (GlobalId scriptId)
{
    // Mark this script as rejected
	History::SortedTree::XSequencer seq (_setChanges, scriptId);
	Assert (!seq.AtEnd ());
    History::Node * rejectedNode = seq.GetNodeEdit ();
    Assert (rejectedNode != 0);
	rejectedNode->SetExecuted (false);
	rejectedNode->SetRejected (true);
	Assert (rejectedNode->IsRejected ());

	// Find next script to be executed next
	XUpdateNextMarker ();
}
#endif

void History::Db::XMarkUndone (GlobalId scriptId)
{
    // Mark this script as rejected
	History::SortedTree::XFullSequencer fullSeq (_setChanges, scriptId);
	Assert (!fullSeq.AtEnd ());
    History::Node * rejectedNode = fullSeq.GetNodeEdit ();
    Assert (rejectedNode != 0);
	Assert (rejectedNode->IsToBeRejected ());
	rejectedNode->SetExecuted (false);
	Assert (rejectedNode->IsRejected ());
	if (rejectedNode->IsAckListEmpty ())
	{
		History::SortedTree::XSequencer seq (_setChanges, scriptId);
		_setChanges.XConfirmAllOlderNodes (seq);
	}

	// Find next script to be executed next
	XUpdateNextMarker ();
}

void History::Db::XMarkMissing (GlobalId scriptId)
{
	// Mark this script as missing
	History::SortedTree::XFullSequencer fullSeq (_setChanges, scriptId);
	Assert (!fullSeq.AtEnd ());
	History::Node * corruptedNode = fullSeq.GetNodeEdit ();
	Assert (corruptedNode != 0);
	Assume (corruptedNode->IsCandidateForExecution (), GlobalIdPack (scriptId).ToString ().c_str ());
	corruptedNode->SetMissing (true);
	Assert (corruptedNode->IsMissing ());

	// Find next script to be executed next
	XUpdateNextMarker ();
}

void History::Db::XMarkExecuted (ScriptHeader const & hdr)
{
	Unit::Type unitType = hdr.GetUnitType ();
	GlobalId scriptId = hdr.ScriptId ();
	Assert ((unitType == Unit::Set && _nextScriptId.XGet () == scriptId) || unitType == Unit::Member);
	History::SortedTree & tree = XGetTree (unitType);

	if (scriptId == gidInvalid)
	{
		// Executed membership updates from version 4.2 are deleted from the history
		Assert (unitType == Unit::Member);
		History::SortedTree::XFwdUnitSequencer seq (tree, scriptId, hdr.GetModifiedUnitId ());
		Assume (!seq.AtEnd (), GlobalIdPack (scriptId).ToBracketedString ().c_str ());
		if (!seq.AtEnd ()) // test just in case!
			seq.MarkDeleted ();
		return;
	}

	History::SortedTree::XUnitSequencer seq (tree, hdr.GetModifiedUnitId ());
	seq.Seek (scriptId);
	Assert (!seq.AtEnd ());
	History::Node * executedNode = 0;
	do
	{
		executedNode = seq.GetNodeEdit ();
		executedNode->SetForceExec (false);
		executedNode->SetForceAccept (false);
		executedNode->SetExecuted (true);
		// In case there are duplicate scripts in history
		seq.Advance ();
		seq.Seek (scriptId);
	} while (!seq.AtEnd ());

	if (unitType == Unit::Set)
	{
		Assert (!executedNode->IsRejected () && !executedNode->IsToBeRejected ());
		executedNode->SetRejected (false); // defensively

		if (executedNode->IsInventory ())
			_initialFileInventoryIsExecuted = true;	// Initial file inventory has been just executed -- mark history as initialized
		Notify (changeEdit, scriptId);

		// Find next script to be executed next
		XUpdateNextMarker ();
	}

	if (executedNode->IsAckListEmpty ())
	{
		History::SortedTree::XSequencer seq (tree, scriptId);
		// Executed script is confirmed
		if (unitType == Unit::Set)
		{
			tree.XConfirmAllOlderNodes (seq);
		}
		else
		{
			Assert (unitType == Unit::Member);
			tree.XConfirmOlderPredecessors (seq);
		}
	}
}

void History::Db::XUpdateNextMarker ()
{
	// Starting from the FIS marker in the set changes tree, walk the nodes until
	// we find a candidate for execution or a missing script or the tree end.
	// If we stopped at the candidate for execution then this is our next script
	// otherwise we don't have the next script.
	GlobalId fis = _setChanges.XGetFirstInterestingScriptId ();
	History::SortedTree::XFwdUnitSequencer seq (_setChanges, fis);
    History::Node const * node = nullptr;
	while (!seq.AtEnd ())
	{
		node = seq.GetNode ();
		if (node->IsCandidateForExecution () || (node->IsMissing () && !node->IsRejected ()))
			break;
		seq.Advance ();
	}
    Assert(seq.AtEnd() || node != nullptr && (node->IsCandidateForExecution() || (node->IsMissing() && !node->IsRejected())));
	if (seq.AtEnd ())
	{
		_nextScriptId.XSet (gidInvalid);
	}
	else if (node->IsCandidateForExecution ())
	{
		Assert (_setChanges.XFindNode (node->GetPredecessorId ())->IsExecuted () ||
			    _setChanges.XFindNode (node->GetPredecessorId ())->IsToBeRejected ());
		_nextScriptId.XSet (node->GetScriptId ());
	}
	else
	{
		_nextScriptId.XSet (gidInvalid);
	}
}

bool History::Db::CanArchive (GlobalId scriptId, Unit::Type unitType) const
{
	History::SortedTree const & tree = GetTree (unitType);
	return tree.CanArchive (scriptId);
}

void History::Db::CopyLog (FilePath const & destPath) const
{
	_cmdLog.Copy (destPath.GetFilePath (_cmdLogName));
	_hdrLog.Copy (destPath.GetFilePath (_hdrLogName));
}

// History Transactable interface

void History::Db::BeginTransaction ()
{
	_cache.Invalidate ();
	_memberCache.Invalidate ();
	TransactableContainer::BeginTransaction ();
}

void History::Db::Serialize (Serializer& out) const
{
    _cmdValidEnd.Serialize (out);
    _hdrValidEnd.Serialize (out);
    _setChanges.Serialize (out);
	_nextScriptId.Serialize (out);
	_membershipChanges.Serialize (out);
	_disconnectedScripts.Serialize (out);
}

void History::Db::Deserialize (Deserializer& in, int version)
{
    CheckVersion (version, VersionNo ());
    _cmdValidEnd.Deserialize (in, version);
    _hdrValidEnd.Deserialize (in, version);
	_setChanges.Deserialize (in, version);
	if (version >= 36 && version < 42)
	{
		// Read unused index
		in.GetLong ();
	}
	if (version >= 41)
	{
		_nextScriptId.Deserialize (in, version);
		_membershipChanges.Deserialize (in, version);
	}
	if (version > 47)
	{
		_disconnectedScripts.Deserialize (in, version);
	}

	_isArchive = _setChanges.XHasArchiveMarkers ();

	// Have we received the full sync script?
	History::SortedTree::XFwdUnitSequencer fseq (_setChanges, gidInvalid);
	if (!fseq.AtEnd ())
	{
		History::Node const * node = fseq.GetNode ();
		Assert (node != 0);
		if (node->IsInventory ())
		{
			// Initial file inventory found
			_initialFileInventoryIsExecuted = node->IsExecuted ();
		}
		else if (node->IsExecuted ())
		{
			// Some executed node
			if (node->IsMilestone ())
			{
				// Executed milestone node -- check if from joining user
				GlobalIdPack scriptId (node->GetScriptId ());
				if (scriptId.IsFromJoiningUser ())
					return; // Joinee project creation marker -- full sync not received yet

				fseq.Advance ();
				if (!fseq.AtEnd ())
				{
					// Check project creation marker successor
					History::Node const * nextNode = fseq.GetNode ();
					_initialFileInventoryIsExecuted = nextNode->IsExecuted ();
				}
			}
		}
	}
}

// History Table interface

void History::Db::QueryUniqueIds (Restriction const & restrict, GidList & ids) const
{
	if (restrict.HasIds ())
	{
		CommnaLineSelection (restrict.GetPreSelectedIds (), ids);
	}
	else
	{
		ids.push_back (gidInvalid);	// First script in the history view represents
									// the current project version -- it has gidInvalid script id

		if (restrict.IsFilterOn ())
		{
			FilteredQuery (restrict, ids);
		}
		else
		{
			// Regular query
			// Iterate over all nodes in the reverse chronological order, stop at node with archive bit set
			for (History::SortedTree::UISequencer seq (_setChanges); !seq.AtEnd (); seq.Advance ())
			{
				History::Node const * node = seq.GetNode ();
				if (node->IsCandidateForExecution () || node->IsMissing () || node->IsForceExec ())
					continue;	// Skip incoming, missing or forced scripts
				ids.push_back (node->GetScriptId ());
			}
		}
	}
}

void History::Db::CommnaLineSelection (GidList const & preSelectedIds, GidList & selectedIds) const
{
	// Restriction has pre-selected ids -- build command line script selection.
	// Depending on the number of pre-selected ids we will
	// build the following range selection (see RangeTableBrowser):
	//	- one id - <current; id>
	//	- two ids - <id1; id2>
	//	- more ids - <id1, id2, ... idn>
	if (preSelectedIds.size () == 1)
	{
		for (History::SortedTree::UISequencer seq (_setChanges); !seq.AtEnd (); seq.Advance ())
		{
			History::Node const * node = seq.GetNode ();
			GlobalId scriptId = node->GetScriptId ();
			selectedIds.push_back (scriptId);
			if (scriptId == preSelectedIds [0])
				break;	// Stop at selected id
		}
	}
	else if (preSelectedIds.size () == 2)
	{
		bool keepAdding = false;
		for (History::SortedTree::UISequencer seq (_setChanges); !seq.AtEnd (); seq.Advance ())
		{
			History::Node const * node = seq.GetNode ();
			GlobalId scriptId = node->GetScriptId ();
			if (keepAdding)
			{
				selectedIds.push_back (scriptId);
				if (scriptId == preSelectedIds [0] || scriptId == preSelectedIds [1])
					break;	// Stop at selected id
			}
			else if (scriptId == preSelectedIds [0] || scriptId == preSelectedIds [1])
			{
				keepAdding = true;
				selectedIds.push_back (scriptId);
			}
		}
	}
	else
	{
		std::copy (preSelectedIds.begin (), preSelectedIds.end (), std::back_inserter (selectedIds));
	}
}

void History::Db::FilteredQuery (Restriction const & restrict, GidList & selectedIds) const
{
	Assert (restrict.IsFilterOn ());
	if (restrict.IsScriptCommentFilterOn ())
	{
		// Display labels and scripts that meet condition present in the filter
		for (History::SortedTree::UISequencer seq (_setChanges); !seq.AtEnd (); seq.Advance ())
		{
			History::Node const * node = seq.GetNode ();
			GlobalId scriptId = node->GetScriptId ();
			if (restrict.IsScriptVisible (scriptId))
				selectedIds.push_back (scriptId);
		}
	}
	else
	{
		Assert (restrict.IsChangedFileFilterOn ());
		// Display labels and scripts that change files present in the filter
		ScriptRestriction scriptRestriction (restrict.GetFileFilter (), _cmdLog);
		for (History::SortedTree::UISequencer seq (_setChanges);
			 !seq.AtEnd ();
			 seq.Advance ())
		{
			History::Node const * node = seq.GetNode ();
			GlobalId scriptId = node->GetScriptId ();
			if (node->IsCandidateForExecution () || node->IsMissing () || node->IsForceExec ())
				continue;	// Skip incoming, missing or forced scripts

			if (node->IsRelevantMilestone ())
			{
				selectedIds.push_back (scriptId);
			}
			else if (scriptRestriction.IsScriptRelevant (node))
			{
				// Script changes file from the filter
				selectedIds.push_back (node->GetScriptId ());
			}
		}
	}
}

bool History::Db::IsValid () const
{
	return _setChanges.size () != 0;
}

std::string History::Db::GetStringField (Column col, GlobalId gid) const
{
	if (col == colFrom)
	{
		// Retrieve sender name -- gid is global user id
		GlobalIdPack invalidPack (gidInvalid);
		if (gid != invalidPack.GetUserId ())
			return GetSenderName (gid);
	}
	else
	{
		// Retrieve script data -- gid is global script id
		if (gid == gidInvalid)
		{
			if (col == colName)
				return CurrentVersion;
		}
		else
		{
			History::Node const * node = _cache.GetNode (gid);
			if (col == colName)
			{
				return _cache.GetComment (gid);
			}
			else if (col == colVersion)
			{
				return _cache.GetFullComment (gid);
			}
			else if (col == colTimeStamp)
			{
				if (!node->IsMissing ())
					return _cache.GetTimeStampStr (gid);
			}
			else
			{
				Assert (col == colStateName);
				if (node->IsMissing ())
				{
					if (node->IsRejected ())
						return "Branch";
					else
						return "Missing";
				}
				if (node->IsCandidateForExecution ())
					return "Incoming";
				if (node->IsArchive ())
					return "Archive";
				if (node->IsRejected () || node->IsToBeRejected ())
					return "Branch";
				if (node->IsAckListEmpty ())
					return "Confirmed";
				UserId thisUserId = _projectDb.GetMyId ();
				GlobalIdPack pack (gid);
				if (thisUserId == pack.GetUserId ())
				{
					// My script
					long scriptTimeStamp = _cache.GetTimeStamp (gid);
					if (IsTimeOlderThan (scriptTimeStamp, 2 * Week))
						return "Overdue";

					return "Tentative";
				}
				else
				{
					// Some other project member script
					return "Unconfirmed";
				}
			}
		}
	}
	return std::string ();
}


GlobalId History::Db::GetIdField (Column col, GlobalId gid) const
{
    return gidInvalid;
}

std::string History::Db::GetStringField (Column col, UniqueName const & uname) const
{
	return std::string ();
}

GlobalId History::Db::GetIdField (Column col, UniqueName const & uname) const
{
    return gidInvalid;
}

unsigned long History::Db::GetNumericField (Column col, GlobalId gid) const
{
	Assert (col == colState);
	if (gid == gidInvalid)
	{
		// Current project version
		ScriptState state;
		state.SetCurrent (true);
		return state.GetValue ();
	}
	else
	{
		Assert (gid != gidInvalid);
		History::Node const * node = _cache.GetNode (gid);
		Assert (node != 0);
		ScriptState state (node->GetFlags ());
		state.SetConfirmed (node->IsAckListEmpty ());
		state.SetProjectCreationMarker (_setChanges.IsTreeRoot (gid));
		if (state.IsInventory ())
		{
			GlobalIdPack pack (gid);
			state.SetMyInventory (pack.GetUserId () == _projectDb.GetMyId ());
		}
		return state.GetValue ();
	}
}

std::string History::Db::GetCaption (Restriction const & restrict) const
{
	std::string caption;
	if (restrict.IsFilterOn ())
	{
		// List filter file names in the history caption
		// If filter contains FileA.cpp and FileB.cpp the caption
		// will contain the following lines:
		//		"FileA.cpp" "FileB.cpp"
		//		FileA.cpp
		//		FileB.cpp
		//		"All files"
		// Build first line
		FileFilter const * filter = restrict.GetFileFilter ();
		if (filter->IsScriptFilterOn ())
		{
			// First line shows comment search keyword
			std::string const & filterPattern = filter->GetFilterPattern ();
			Assert (!filterPattern.empty ());
			caption += filter->GetFilterPattern ();
		}
		else
		{
			Assert (filter->IsFileFilterOn ());
			// First line shows file filter pattern
			std::string const & filterPattern = filter->GetFilterPattern ();
			caption += filter->GetFilterPattern ();
		}
		caption += "\n*.*";
		if (filter->FileCount () != 0)
		{
			// Build other lines
			for (FileFilter::Sequencer seq (*filter); !seq.AtEnd (); seq.Advance ())
			{
				std::string const & currentFilePath = seq.GetPath ();
				if (!IsFileNameEqual (currentFilePath, filter->GetFilterPattern ()))
				{
					caption += "\n";
					caption += seq.GetPath ();
				}
			}
		}
	}
	else
	{
		caption.assign ("*.*");
	}
	return caption;
}

void History::Db::GetUnpackedScripts (GidList & ids) const
{
	if (IsFullSyncExecuted ())
	{
		for (History::SortedTree::Sequencer seq (_setChanges); !seq.AtEnd (); seq.Advance ())
		{
			History::Node const * node = seq.GetNode ();
			if (node->IsExecuted () || node->IsToBeRejected ())
				break;
			
			ids.push_back (node->GetScriptId ());
		}
	}
	else
	{
		for (History::SortedTree::Sequencer seq (_setChanges); !seq.AtEnd (); seq.Advance ())
		{
			History::Node const * node = seq.GetNode ();
			if (node->IsInventory ())
			{
				ids.push_back (node->GetScriptId ());
				break;
			}
		}
	}
}

bool History::Db::RetrieveVersionInfo (GlobalId scriptGid, VersionInfo & info) const
{
	if (scriptGid != gidInvalid)
	{
		History::Node const * node = _setChanges.FindNode (scriptGid);
		Assert (node != 0);
		if (node->GetScriptVersion () < 23)
			return false; // Too old to read
		if (node->IsMissing())
			return false;
		std::unique_ptr<HdrNote> hdr 
			= _hdrLog.Retrieve (node->GetHdrLogOffset (), node->GetScriptVersion ());
		info.SetComment (hdr->GetComment ());
		info.SetTimeStamp (hdr->GetTimeStamp ());
		info.SetVersionId (scriptGid);
	}
	else
	{
		info.SetComment ("Current project version");
		info.SetTimeStamp (CurrentTime ());
		info.SetVersionId (gidInvalid);
	}
	return true;
}

std::string History::Db::RetrieveNextScriptCaption () const
{
	if (!HasNextScript ())
		return std::string ();

	History::SortedTree::PredecessorSequencer seq (_setChanges, _nextScriptId.Get ());
	Assert (!seq.AtEnd ());
	History::Node const * nextNode = seq.GetNode ();
	Assert (nextNode != 0);
	Assert (nextNode->IsCandidateForExecution ());
	// Move to the "next script" logical predecessor
	seq.Advance ();
	if (!seq.AtEnd ())
	{
		History::Node const * logicalPredecessorNode = seq.GetNode ();
		if (logicalPredecessorNode->IsBranchPoint ())
		{
			// Next script caused script conflict - check if there are
			// to be rejected scripts in the history.
			History::SortedTree::Sequencer seq1 (_setChanges, _nextScriptId.Get ());
			Assert (!seq1.AtEnd ());
			seq1.Advance ();
			Assert (!seq1.AtEnd ());
			History::Node const * topologicalPredecessorNode = seq1.GetNode ();
			if (topologicalPredecessorNode->IsToBeRejected ())
				return " Is in conflict with script(s) in the history. See history for details.";
		}
	}
	std::unique_ptr<HdrNote> hdr = _hdrLog.Retrieve (nextNode->GetHdrLogOffset (),
												   nextNode->GetScriptVersion ());
	return hdr->GetComment ();
}

// Returns true if forced script found
bool History::Db::RetrieveForcedScript (ScriptHeader & hdr, CommandList & cmdList) const
{
	if (HasNextScript ())
	{
		History::Node const * node = _setChanges.FindNode (_nextScriptId.Get ());
		Assert (node != 0);
		Assert (!node->IsMissing ());
		if (node->IsForceExec ())
		{
			// Script marked for forced execution
			RetrieveScript (node, hdr, cmdList);
			InitScriptHeader (node, Unit::Set, cmdList, hdr);
			return true;
		}
	}

	// Set changes tree doesn't have forced scripts -- check membership changes tree
	// Return forced membership changes in chronological order
	for (History::SortedTree::ForwardSequencer seq (_membershipChanges); !seq.AtEnd (); seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		Assert (node != 0);
		if (node->IsForceExec ())
		{
			// Script marked for forced execution
			Assert (!node->IsMissing ());
			RetrieveScript (node, hdr, cmdList);
			InitScriptHeader (node, Unit::Member, cmdList, hdr);
			return true;
		}
	}
	return false;
}

// Returns true is next script has to automatically accepted
bool History::Db::RetrieveNextScript (ScriptHeader & hdr, CommandList & cmdList) const
{
	Assert (HasNextScript ());
	History::Node const * node = _setChanges.FindNode (_nextScriptId.Get ());
	Assert (node != 0);
	if (!node->IsCandidateForExecution ())
	{
		Win::ClearError ();
		throw Win::Exception ("Corrupted history: script incorrectly marked as \"Next\"",
		                      GlobalIdPack (_nextScriptId.Get ()).ToBracketedString ().c_str ());
	}

	RetrieveScript (_nextScriptId.Get (), hdr, cmdList, Unit::Set);
	return node->IsForceAccept ();
}

// Returns true if script retrieved
bool History::Db::RetrieveScript (GlobalId scriptGid, ScriptHeader & hdr, CommandList & cmdList, Unit::Type unitType) const
{
	Assert (unitType == Unit::Set || unitType == Unit::Member);
	hdr.SetProjectName (_projectDb.ProjectName ());
	if (scriptGid != gidInvalid)
	{
		History::SortedTree const & tree = GetTree (unitType);
		History::Node const * node = tree.FindNode (scriptGid);
		if (node == 0)
			return false;
		if (node->GetScriptVersion () < 23 || node->IsMissing ())
			return false; // Too old to read or missing

		RetrieveScript (node, hdr, cmdList);
		InitScriptHeader (node, unitType, cmdList, hdr);
	}
	else
	{
		// Return script describing current project version
		hdr.AddComment ("Current project version");
		hdr.AddScriptId (gidInvalid);
		hdr.AddTimeStamp (CurrentTime ());
	}
	return true;
}

// Returns true if script retrieved
bool History::Db::XRetrieveScript (GlobalId scriptGid, ScriptHeader & hdr, CommandList & cmdList, Unit::Type unitType)
{
	Assert (unitType == Unit::Set || unitType == Unit::Member);
	hdr.SetProjectName (_projectDb.XProjectName ());
	Assert (scriptGid != gidInvalid);
	History::SortedTree & tree = XGetTree (unitType);
	History::Node const * node = tree.XFindNode (scriptGid);
	Assume (node != 0, GlobalIdPack (scriptGid).ToBracketedString ().c_str ());
	if (node->IsMissing ())
		return false;
	RetrieveScript (node, hdr, cmdList);
	InitScriptHeader (node, unitType, cmdList, hdr);
	return true;
}

bool History::Db::XRetrieveCmdList (GlobalId scriptId, CommandList & cmdList, Unit::Type unitType)
{
	Assert (scriptId != gidInvalid);
	History::SortedTree & tree = XGetTree (unitType);
	History::Node const * node = tree.XFindNode (scriptId);
	Assume (node != 0, GlobalIdPack (scriptId).ToBracketedString ().c_str ());
	Assert (!node->IsMissing ());
	return XRetrieveCmdList (node, cmdList);
}

bool History::Db::RetrieveScript (GlobalId scriptGid, FileFilter const * filter, ScriptProps & props) const
{
	Assert (scriptGid != gidInvalid);
	History::Node const * node = _setChanges.FindNode (scriptGid);
	Assume (node != 0, GlobalIdPack (scriptGid).ToBracketedString ().c_str ());
	if (node->GetScriptVersion () < 23 || node->IsMissing ())
		return false; // Too old to read or missing

	std::unique_ptr<ScriptHeader> hdr (new ScriptHeader);
	std::unique_ptr<CommandList> cmdList (new CommandList);
	props.SetAckList (node->GetAckList ());
	if (filter == 0 || !filter->IsFileFilterOn ())	// If no filter specified or specified filter is empty
	{
		RetrieveScript (node, *hdr, *cmdList);
	}
	else
	{
		// Return command list containing only commands changing pre-selected files/folders
		Assert (filter != 0);
		Assert (filter->IsFileFilterOn ());
		int scriptVersion = node->GetScriptVersion ();
		for (History::Node::CmdSequencer seq (*node); !seq.AtEnd (); seq.Advance ())
		{
			if (filter->IsIncluded (seq.GetCmd ()->GetUnitId ()))
			{
				std::unique_ptr<ScriptCmd> cmd = _cmdLog.Retrieve (seq.GetCmd ()->GetLogOffset (), 
																scriptVersion);
				cmdList->push_back (std::move(cmd));
			}
		}
		RetrieveHeader (node, *hdr);
	}
	InitScriptHeader (node, Unit::Set, *cmdList, *hdr);
	props.Init (std::move(hdr), std::move(cmdList));
	return true;
}

void History::Db::RetrieveScript (History::Node const * node, ScriptHeader & hdr, CommandList & cmdList) const
{
	Assert (node != 0);
	Assert (node->GetScriptVersion () >= 23);
	Assert (!node->IsMissing ());
	RetrieveHeader (node, hdr);
	RetrieveCommandList (node, cmdList);
}

void History::Db::InitScriptHeader (History::Node const * node, Unit::Type unitType, CommandList const & cmdList, ScriptHeader & hdr) const
{
	Assert (node != 0);
	hdr.SetUnitType (unitType);
	hdr.SetChunkInfo (1, 1, 0);
	// Set script kind
	if (unitType == Unit::Set)
	{
		hdr.SetModifiedUnitId (gidInvalid);
		hdr.SetScriptKind (ScriptKindSetChange ());
	}
	else
	{
		Assert (unitType == Unit::Member);
		hdr.SetModifiedUnitId (node->GetUnitId (unitType));
		hdr.SetVersion40 (node->GetScriptId () == gidInvalid);
		CommandList::Sequencer seq (cmdList);
		MemberCmd const & memberCmd = seq.GetMemberCmd ();
		if (memberCmd.GetType () == typeNewMember)
			hdr.SetScriptKind (ScriptKindAddMember ());
		else if (memberCmd.GetType () == typeDeleteMember)
			hdr.SetScriptKind (ScriptKindDeleteMember ());
		else
			hdr.SetScriptKind (ScriptKindEditMember ());
	}
}


bool History::Db::XRetrieveCmdList (History::Node const * node, CommandList & cmdList) const
{
	Assert (node != 0);
	if (node->GetScriptVersion () < 23 || node->IsMissing ())
		return false; // Too old to read or missing

	try
	{
		int scriptVersion = node->GetScriptVersion ();
		for (History::Node::CmdSequencer seq (*node); !seq.AtEnd (); seq.Advance ())
		{
			std::unique_ptr<ScriptCmd> cmd = _cmdLog.Retrieve (seq.GetCmd ()->GetLogOffset (), 
															 scriptVersion);
			cmdList.push_back (std::move(cmd));
		}
		return true;
	}
	catch ( ... )
	{
		// Well, something went wrong while reading script from the history log.
		GlobalIdPack pack (node->GetScriptId ());
		std::string info ("Script id: ");
		info += pack.ToString ();
		Win::ClearError ();
		throw Win::Exception ("Cannot retrieve script command list from History.", info.c_str ());
	}
	return false;
}

void History::Db::XRetrieveThisUserMembershipUpdateHistory (ScriptList & scriptList)
{
	for (History::SortedTree::XFwdUnitSequencer seq (_membershipChanges, gidInvalid, _projectDb.XGetMyId ());
		 !seq.AtEnd ();
		 seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		if (!node->IsMissing ())
		{
			std::unique_ptr<ScriptHeader> hdr (new ScriptHeader ());
			std::unique_ptr<CommandList> cmdList (new CommandList ());
			RetrieveScript (node, *hdr, *cmdList);
			InitScriptHeader (node, Unit::Member, *cmdList, *hdr);
			scriptList.push_back (std::move(hdr), std::move(cmdList));
		}
		else
		{
			// This user is missing a script from his own membership change history
			GlobalIdPack pack (node->GetScriptId ());
			if (pack.GetUserId () == _projectDb.XGetMyId ())
			{
				// This is really bad -- the user is missing his own script
				std::string info ("Missing script: ");
				info += pack.ToString ();
				throw Win::Exception ("Corrupted membership change history. Please contact support@reliosft.com", info.c_str ());
			}
			else
			{
				// The missing script was send by the project administrator (can be different from
				// the current administrator, because project administrator can change over time).
				// Skip this script.
			}
		}
	}
}

void History::Db::RetrieveSetCommands (Filter & filter, Progress::Meter & meter) const
{
	// Notice: we are going back in time, so edit changes are returned
	// in reversed chronological order.
	meter.SetRange (0, _setChanges.size (), 1);
	meter.SetActivity ("Scanning history");
	for (History::SortedTree::PredecessorSequencer seq (_setChanges, filter.GetStartId ());
		 !filter.AtEnd () && !seq.AtEnd ();
		 seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		meter.StepIt ();
		if (filter.IsScriptRelevant (node->GetScriptId ()))
		{
			// Script is relevant
			for (History::Node::CmdSequencer noteSeq (*node); !noteSeq.AtEnd (); noteSeq.Advance ())
			{
				History::Node::Cmd const * cmdNote = noteSeq.GetCmd ();
				dbg << " unit ID: " << cmdNote->GetUnitId () << std::endl;
				if (filter.IsCommandRelevant (cmdNote->GetUnitId ()))
				{
					// Command is relevant
					std::unique_ptr<FileCmd> cmd = _cmdLog.RetrieveFileCmd (cmdNote->GetLogOffset (), node->GetScriptVersion ());
					if (cmd.get() == 0)
						dbg << "  retrieved null command" << std::endl;
					else
						dbg << "  Command type: " << cmd->GetType () << std::endl;

					filter.AddCommand (std::move(cmd), node->GetScriptId ());
				}
			}
		}
	}
	meter.Close ();
}

std::string History::Db::GetLastResendRecipient (GlobalId scriptId) const
{
	Assert (scriptId != gidInvalid);
	// REVISIT: use missing script meta-data when implemented
	GlobalIdPack pack (scriptId);
	MemberNameTag recipient (GetSenderName (pack.GetUserId ()), pack.GetUserId ());
	return recipient;
}

void History::Db::XRetrieveFileData (GidSet & historicalFiles,
							std::vector<FileData> & fileData)
{
	Assert (!historicalFiles.empty ());
	// Retrieve from history historical file FileData.
	// Notice: we are going back in time and stop at first script
	// changing given file -- this is the latest FileData.
	for (History::SortedTree::XFullSequencer seq (_setChanges); !historicalFiles.empty () && !seq.AtEnd (); seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		if (node == 0)
			return;	// We have reached deleted copy of the current history, which was moved
					// behind the imported history.
		Assert (node->GetScriptVersion () >= 23);
		if (node->IsCandidateForExecution () || node->IsMissing ())
			continue;	// Skip unpacked or missing scripts
		if (!node->IsMilestone ())
		{
			for (History::Node::CmdSequencer noteSeq (*node);
				 !historicalFiles.empty () && !noteSeq.AtEnd ();
				 noteSeq.Advance ())
			{
				History::Node::Cmd const * cmdNote = noteSeq.GetCmd ();
				GlobalId gid = cmdNote->GetUnitId ();
				GidSet::const_iterator histFile = historicalFiles.find (gid);
				if (histFile != historicalFiles.end ())
				{
					// Retrieve last command changing historical file
					std::unique_ptr<FileCmd> cmd = _cmdLog.RetrieveFileCmd (cmdNote->GetLogOffset (),
																		  node->GetScriptVersion ());
					FileData historicalFileData (cmd->GetFileData ());
					if (cmd->GetType () == typeDeletedFile || cmd->GetType () == typeDeleteFolder)
					{
						// If last command recorded in the history deletes file
						// make sure that historical file has state None, because
						// our transformer may place copy of FileData in the script with
						// some bits set, even when those bits are later cleared in the
						// script sender database.
						historicalFileData.SetState (FileState ());
					}
					fileData.push_back (historicalFileData);
					historicalFiles.erase (gid);
				}
			}
		}
	}
}

std::string History::Db::GetSenderName (GlobalId scriptGid) const
{
	MemberDescription const * sender = _memberCache.GetMemberDescription (scriptGid);
	return sender->GetName ();
}

History::Db::ScriptRestriction::ScriptRestriction (FileFilter const * fileFilter, CmdLog const & cmdLog)
	: _noFilter (fileFilter == 0),
	  _cmdLog (cmdLog)
{
	if (fileFilter == 0)
		return;

	for (FileFilter::Sequencer seq (*fileFilter); !seq.AtEnd (); seq.Advance ())
	{
		GlobalId gid = seq.GetGid ();
		_currentFileFilter.insert (gid);
	}
}

History::Db::ScriptRestriction::ScriptRestriction (GidSet const & initialFileSelection, CmdLog const & cmdLog)
	: _currentFileFilter (initialFileSelection),
	  _noFilter (initialFileSelection.empty ()),
	  _cmdLog (cmdLog)
{}

bool History::Db::ScriptRestriction::IsScriptRelevant (History::Node const * node)
{
	if (_noFilter)
		return true;	// No file filter - every script is relevant

	for (History::Node::CmdSequencer noteSeq (*node); !noteSeq.AtEnd (); noteSeq.Advance ())
	{
		History::Node::Cmd const * cmdNote = noteSeq.GetCmd ();
		GlobalId fileGid = cmdNote->GetUnitId ();
		if (_currentFileFilter.find (fileGid) != _currentFileFilter.end ())
		{
			// Script changes file from the filter
			return true;
		}
	}
	return false;
}

void History::Db::CreateRangeFromCurrentVersion (GlobalId stopScriptId,
										GidSet const & fileFilter,
										Range & range) const
{
	Assert (stopScriptId != gidInvalid);
	// Find first executed node
	GlobalId startId = gidInvalid;
	for (History::SortedTree::FullSequencer seq (_setChanges, gidInvalid);
		 !seq.AtEnd ();
		 seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		if (node->IsExecuted () || node->IsToBeRejected ())
		{
			startId = node->GetScriptId ();
			break;
		}
	}
	Assert (startId != gidInvalid);
	ScriptRestriction scriptRestriction (fileFilter, _cmdLog);
	for (History::SortedTree::PredecessorSequencer seq (_setChanges, startId);// Start from the first executed script
		 !seq.AtEnd ();
		 seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		if (node->IsExecuted () || node->IsToBeRejected ())
		{
			if (scriptRestriction.IsScriptRelevant (node))
				range.AddScriptId (node->GetScriptId ());
			if (node->GetScriptId () == stopScriptId)
				break;
		}
	}
}

GlobalId History::Db::CreateRangeToFirstExecuted (GlobalId startScriptId,
										 GidSet const & fileFilter,
										 Range & range) const
{
	Assert (startScriptId != gidInvalid);
	ScriptRestriction scriptRestriction (fileFilter, _cmdLog);
	for (History::SortedTree::PredecessorSequencer seq (_setChanges, startScriptId);
		 !seq.AtEnd ();
		 seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		if (node->IsExecuted ())
			return node->GetScriptId ();
		if (scriptRestriction.IsScriptRelevant (node))
			range.AddScriptId (node->GetScriptId ());
	}
	return gidInvalid;
}

void History::Db::CreateRangeFromTwoScripts (GlobalId firstScriptId,
									GlobalId secondScriptId,
									GidSet const & fileFilter,
									Range & range) const
{
	Assert (firstScriptId != gidInvalid);
	Assert (secondScriptId != gidInvalid);
	ScriptRestriction scriptRestriction (fileFilter, _cmdLog);
	History::Node const * node1 = _setChanges.FindNode (firstScriptId);
	Assert (node1 != 0);
	History::Node const * node2 = _setChanges.FindNode (secondScriptId);
	Assert (node2 != 0);
	if (node1->IsExecuted () && node2->IsExecuted ())
	{
		// Double selection in the trunk.
		// Range from the first selected script to the second selected script.
		for (History::SortedTree::PredecessorSequencer seq (_setChanges, firstScriptId);
			 !seq.AtEnd ();
			 seq.Advance ())
		{
			History::Node const * node = seq.GetNode ();
			if (scriptRestriction.IsScriptRelevant (node))
				range.AddScriptId (node->GetScriptId ());
			if (node->GetScriptId () == secondScriptId)
				break;
		}
	}
	else if (node1->IsRejected () && node2->IsRejected ())
	{
		// Double selection in the branch.
		// Range from the first selected script to the second selected script
		// if both selected scripts belong to the same branch.
		for (History::SortedTree::PredecessorSequencer seq (_setChanges, firstScriptId);
			 !seq.AtEnd ();
			 seq.Advance ())
		{
			History::Node const * node = seq.GetNode ();
			if (scriptRestriction.IsScriptRelevant (node))
			{
				range.AddScriptId (node->GetScriptId ());
				if (node->GetScriptId () == secondScriptId)
					break;	// Both selected scripts belong to the same branch.
				if (node->IsExecuted ())
				{
					Assert (node->IsBranchPoint ());
					// Selected scripts belong to different branches.
					// The range is empty.
					range.Clear ();
					break;
				}
			}
		}
	}
	// Else range is empty because selected scripts belong to
	// the trunk and some branch.
}

void History::Db::GetForkIds (GidList & forkIds, bool deepForks) const
{
	GlobalId mostRecentExecutedScriptId = MostRecentScriptId ();
	// Iterate over history in the reverse chronological order
	for (History::SortedTree::PredecessorSequencer seq (_setChanges, mostRecentExecutedScriptId);
		 !seq.AtEnd ();
		 seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		if (deepForks)
		{
			if (node->IsDeepFork ())
				forkIds.push_back (node->GetScriptId ());
		}
		else if (node->IsMilestone ())
		{
			std::unique_ptr<HdrNote> hdr = _hdrLog.Retrieve (node->GetHdrLogOffset (),
														   node->GetScriptVersion ());
			std::string const & comment = hdr->GetComment ();
			if (comment.find ("Branch") != std::string::npos &&
				comment.find ("created") != std::string::npos)
			{
				// Branch creation marker found - remember its predecessor
				forkIds.push_back (node->GetPredecessorId ());
			}
		}
	}
}

GlobalId History::Db::CheckForkIds (GidList const & otherProjectForkIds,
						   bool deepForks,
						   GidList & myYoungerForkIds) const
{
	GidSet otherProjectIdSet (otherProjectForkIds.begin (), otherProjectForkIds.end ());
	GlobalId mostRecentExecutedScriptId = MostRecentScriptId ();
	// Iterate over history in the reverse chronological order
	for (History::SortedTree::PredecessorSequencer seq (_setChanges, mostRecentExecutedScriptId);
		 !seq.AtEnd ();
		 seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		if (otherProjectIdSet.find (node->GetScriptId ()) != otherProjectIdSet.end ())
			return node->GetScriptId ();	// Return first other project id found in our history

		if (deepForks)
		{
			if (node->IsDeepFork ())
				myYoungerForkIds.push_back (node->GetScriptId ());
		}
		else if (node->IsMilestone ())
		{
			std::unique_ptr<HdrNote> hdr = _hdrLog.Retrieve (node->GetHdrLogOffset (),
														   node->GetScriptVersion ());
			std::string const & comment = hdr->GetComment ();
			if (comment.find ("Branch") != std::string::npos &&
				comment.find ("created") != std::string::npos)
			{
				// Branch creation marker found - remember its predecessor
				// as my younger fork id
				myYoungerForkIds.push_back (node->GetPredecessorId ());
			}
		}
	}
	return gidInvalid;	// No other project id found in our history
}

void History::Db::CreateLogFiles ()
{
	_cmdLog.CreateFile ();
	_hdrLog.CreateFile ();
}

void History::Db::VerifyLogs () const
{
	if (!_cmdLog.Exists ())
		throw Win::Exception ("Corrupted Database: missing system file.", _cmdLog.GetPath ());
	if (!_hdrLog.Exists ())
		throw Win::Exception ("Corrupted Database: missing system file.", _hdrLog.GetPath ());
}

void History::Db::DumpFileChanges (std::ostream & out) const
{
	out << "===Scripts recorded in the project history:" << std::endl;
	History::SortedTree::FullSequencer seq (_setChanges);
	History::Node const * node = seq.GetNode ();
	if (node->IsCandidateForExecution () || node->IsMissing ())
	{
		out << " Unpacked scripts:" << std::endl << std::endl;
		for (; !seq.AtEnd (); seq.Advance ())
		{
			node = seq.GetNode ();
			if (node->IsExecuted () || node->IsToBeRejected ())
				break;
			if (node->GetScriptId () == _nextScriptId.Get ())
				out << " NEXT: ";
			else
				out << "       ";
			DumpNode (out, node);
			out << std::endl;
		}
	}
	GlobalId fisId = _setChanges.GetFirstInterestingScriptId ();
	GlobalId referenceId = gidInvalid;
	bool fisIdSeen = false;
	bool refNotSeen = true;
	unsigned int count = 0xffffffff;	// Make sure that we include the longest possible lineage
	out << std::endl << " Most recent executed scripts:" << std::endl << std::endl;
	for ( ; !seq.AtEnd () && count != 0; seq.Advance (), --count)
	{
		History::Node const * node = seq.GetNode ();
		GlobalId scriptId = node->GetScriptId ();
		if (node->IsAckListEmpty ())
		{
			if (scriptId == fisId)
			{
				fisIdSeen = true;
				out << " *REF: ";
			}
			else if (refNotSeen)
				out << " REF:  ";
			else
				out << "       ";

		}
		else if (scriptId == fisId)
		{
			fisIdSeen = true;
			out << " FIS:  ";
		}
		else
			out << "       ";

		DumpNode (out, node);
		out << std::endl;
		if (node->IsAckListEmpty () && refNotSeen)
		{
			refNotSeen = false;
			out << "     -------------------------------------------------- End of current lineage for the file changes tree" << std::endl;
			count = 50;	// Dump last 50 executed scripts from the history
			referenceId = scriptId;
		}
		if (node->IsRejected () || node->IsToBeRejected ())
			++count;	// Rejected or to-be-rejected nodes don't count
		if (!fisIdSeen && count == 1)
			++count;	// Keep going until first interesting id seen
	}
	out << std::endl << " Next script marker: " << GlobalIdPack (_nextScriptId.Get ()) << std::endl;
	out << std::endl << " Current lineage reference id: " << GlobalIdPack (referenceId) << std::endl;
	out << std::endl << " First interesting script: " << GlobalIdPack (fisId) << std::endl;
}

void History::Db::DumpMembershipChanges (std::ostream & out) const
{
	if (_membershipChanges.GetFirstInterestingScriptId () != 0)
	{
		out << std::endl << "===Illegal FIS marker in the membership tree: ";
		out << GlobalIdPack (_membershipChanges.GetFirstInterestingScriptId ()) << std::endl;
	}

	GidList unitIds;
	_projectDb.GetHistoricalMemberList (unitIds);
	if (unitIds.empty ())
		return;

	for (GidList::const_iterator iter = unitIds.begin (); iter != unitIds.end (); ++iter)
	{
		GlobalId id = *iter;
		out << std::endl << "===Membership changes for the user: " << std::hex << id << std::endl;
		for (History::SortedTree::UnitSequencer seq (_membershipChanges, id); !seq.AtEnd (); seq.Advance ())
		{
			History::Node const * node = seq.GetNode ();
			out << '*';
			DumpNode (out, node, true, true);	// Dump script comment and cmd details
			out << std::endl;
		}
	}

	for (History::SortedTree::FullSequencer seq (_membershipChanges); !seq.AtEnd (); seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		GlobalId editedUserId = node->GetUnitId (Unit::Member);
		GidList::const_iterator iter = std::find (unitIds.begin (), unitIds.end (), editedUserId);
		if (iter == unitIds.end ())
		{
			out << std::endl << "===Membership change for the user not recorded in the project database: " << std::hex << editedUserId << std::endl;
			DumpNode (out, node, true, true);	// Dump script comment and cmd details
			out << std::endl;
		}
	}
}

void History::Db::DumpNode (std::ostream & out, History::Node const * node, bool dumpComment, bool dumpCmdDetails) const
{
	out << *node;
	if (!node->IsMissing ())
	{
		std::unique_ptr<HdrNote> hdrNote = _hdrLog.Retrieve (node->GetHdrLogOffset (), node->GetScriptVersion ());
		Lineage const & scriptLineage = hdrNote->GetLineage ();
		if (scriptLineage.Count () == 0)
			out << ", empty lineage";
		else
			out << ", lineage: " << hdrNote->GetLineage ();
		if (dumpComment)
		{
			std::string const & comment = hdrNote->GetComment ();
			out << ", " << comment;
		}
		if (dumpCmdDetails)
		{
			CommandList cmdList;
			RetrieveCommandList (node, cmdList);
			if (cmdList.size () == 0)
			{
				out << std::endl << "**command list is empty";
			}
			else
			{
				out << std::endl;
				for (CommandList::Sequencer seq (cmdList); !seq.AtEnd (); seq.Advance ())
				{
					ScriptCmd const & cmd = seq.GetCmd ();
					cmd.Dump (out);
					out << std::endl;
				}
			}
		}
	}
}

void History::Db::DumpDisconnectedScripts (std::ostream & out) const
{

	TransactableArray<MissingScriptId>::const_iterator iter = _disconnectedScripts.begin ();
	if (iter == _disconnectedScripts.end ())
		return;
	out << " Disconnected missing scripts " << std::endl;
	do
	{
		MissingScriptId const * id = *iter;
		out << "     " << *id << std::endl;
		++iter;
	} while (iter != _disconnectedScripts.end ());
}

void History::Db::DumpCmdLog (std::ostream & out) const
{
#if 0
	File::Offset curPos = 0;
	File::Size logSize = 0;
	{
		FileInfo fileInfo (_cmdLog.GetPath ());
		logSize = fileInfo.GetSize ();
	}
	out << "===Script command log - size: " << ::FormatFileSize (logSize.ToMath ()) << std::endl;
	FileDeserializer in (_cmdLog.GetPath ());
	while (curPos != logSize)
	{
		out << "*''''" << std::setw (8) << std::setfill ('0') << std::hex << curPos.Low () << ":''''" << std::endl;
		std::unique_ptr<ScriptCmd> cmd = ScriptCmd::DeserializeCmd (in, scriptVersion);
		curPos = in.GetPosition ();
		cmd->Dump (out);
		out << std::endl;
	}
#endif
}

std::string History::Db::RetrieveComment (GlobalId scriptGid) const
{
	std::string comment;
    History::Node const * node = _setChanges.FindNode (scriptGid);
	if (node != 0)
	{
		std::unique_ptr<HdrNote> hdr = _hdrLog.Retrieve (node->GetHdrLogOffset (),
														 node->GetScriptVersion ());
		comment.assign (hdr->GetComment ());
	}
	return comment;
}

void History::Db::XCorrectItemUniqueName (GlobalId itemGid, UniqueName const & correctUname)
{
	dbg << "--> History::XCorrectItemUniqueName" << std::endl;
	for (History::SortedTree::XFullSequencer seq (_setChanges); !seq.AtEnd (); seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		History::Node::Cmd const * cmdNote = node->Find (itemGid);
		if (cmdNote != 0)
		{
			dbg << "Changing unique name in script: " << GlobalIdPack (node->GetScriptId ()) << " - " << RetrieveComment (node->GetScriptId ()) << std::endl;
			std::unique_ptr<FileCmd> cmd = _cmdLog.RetrieveFileCmd (cmdNote->GetLogOffset (),
																  node->GetScriptVersion ());
			FileData const & cmdFd = cmd->GetFileData ();
			dbg << "Script command file data: " << cmdFd;
			if (!cmdFd.GetUniqueName ().IsEqual (correctUname))
			{
				cmd->SetUniqueName (correctUname);
				dbg << "Script command file data after correction: " << cmdFd;
				History::Node * editNode = seq.GetNodeEdit ();
				editNode->ChangeCmdLogOffset (itemGid, _cmdValidEnd.XGet ()); 
				_cmdValidEnd.XSet (_cmdLog.Append (_cmdValidEnd.XGet (), *cmd));
			}
			else
			{
				dbg << "File command unique name is correct." << std::endl;
			}

			if (cmd->GetType () == typeWholeFile || cmd->GetType () == typeNewFolder)
			{
				dbg << "<-- History::XCorrectItemUniqueName - new file of folder" << std::endl;
				return;
			}
		}
	}
	dbg << "<-- History::XCorrectItemUniqueName - end of history" << std::endl;
}

void History::Db::XPreConversionDump (MemoryLog & log)
{
	log << "+++Version 4.2 history nodes -- in reverse chronological order" << std::endl;
	for (History::SortedTree::XFullSequencer seq (_setChanges); !seq.AtEnd (); seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		log << std::setw (4) << std::setfill (' ') << std::left << seq.GetIdx () << ": ";
		GlobalIdPack scriptId (node->GetScriptId ());
		log << scriptId.ToBracketedString () << "; ";
		History::Node::V42State state (node->GetFlags ());
		if (state.IsChangeScript ())
		{
			if (state.IsRejected ())
				log << "file (rejected)";
			else if (state.IsMilestone ())
				log << "file (milestone)";
			else
				log << "file";
		}
		else if (state.IsMilestone ())
		{
			log << "project creation marker";
		}
		else
		{
			log << "membership";
		}
		log << "; ";
		GidList const & ackList = node->GetAckList ();
		if (ackList.empty ())
			log << "confirmed";
		else
		{
			log << "awaiting ack from:";
			for (GidList::const_iterator iter = ackList.begin (); iter != ackList.end (); ++iter)
				log << ' ' << std::hex << *iter;
		}
		log << "; ";
		std::unique_ptr<HdrNote> hdr = _hdrLog.Retrieve (node->GetHdrLogOffset (), node->GetScriptVersion ());
		Lineage const & lineage = hdr->GetLineage ();
		log << "node lineage: " << lineage << std::endl;
	}
	log << "---Version 4.2 history nodes" << std::endl;
	log.flush ();
}

void History::Db::XPostConversionDump (MemoryLog & log)
{
	log << "+++Emergency Post Conversion History Dump -- in reverse chronological order===" << std::endl;
	for (History::SortedTree::XFullSequencer seq (_setChanges); !seq.AtEnd (); seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		log << std::setw (4) << std::setfill (' ') << std::left << seq.GetIdx () << ": " << *node << std::endl;
	}
	log << "---Emergency Post Conversion History Dump===" << std::endl;
	log.flush ();
}

void History::Db::XConvert (Progress::Meter * meter, MemoryLog & log)
{
	XPreConversionDump (log);
	log << "Converting project history database" << std::endl;
	Win::ClearError ();
	if (_setChanges.xsize () == 0)
		throw Win::Exception ("Empty history");

	meter->SetActivity ("Converting project history");
	meter->SetRange (0, 3*_setChanges.xsize (), 1);
	log << "Converting node states. There are " << _setChanges.xsize () << " nodes in the set change tree." << std::endl;
	_setChanges.XConvertNodeState (meter, log);
	// Note: converting node state might have deleted nodes from the tree.
	if (_setChanges.XActualSize () == 0)
		throw Win::Exception ("Only membership updates and rejected scripts in the history");

	log << "Nodes converted. There are " << _setChanges.XActualSize () << " nodes left in the set change tree." << std::endl;

	// Find initial project inventory script -- usually stored as
	// first or second script in the history
	History::SortedTree::XFwdUnitSequencer seq (_setChanges, gidInvalid);	// Sequencer positions itself on the first script in the set change history
	Assert (!seq.AtEnd ());
	History::Node const * node = seq.GetNode ();
	Assert (node != 0);
	std::unique_ptr<HdrNote> hdrNote = _hdrLog.Retrieve (node->GetHdrLogOffset (), node->GetScriptVersion ());
	std::string const & comment = hdrNote->GetComment ();
	if (comment.find ("Project files as of") != std::string::npos ||
		comment.find ("File(s) Added During") != std::string::npos)
	{
		// Found initial file inventory
		log << "Fisrt script in the history marked as inventory." << std::endl;
		History::Node * inventoryNode = seq.GetNodeEdit ();
		inventoryNode->SetInventory (true);
	}
	else
	{
		// Look at the next script
		seq.Advance ();
		if (!seq.AtEnd ())
		{
			// There are more scripts then just the project creation marker in the history
			node = seq.GetNode ();
			Assert (node != 0);
			hdrNote = _hdrLog.Retrieve (node->GetHdrLogOffset (), node->GetScriptVersion ());
			std::string const & comment = hdrNote->GetComment ();
			if (comment.find ("Project files as of") != std::string::npos ||
				comment.find ("File(s) Added During") != std::string::npos)
			{
				// Found initial file inventory
				log << "Second script in the history marked as inventory" << std::endl;
				History::Node * inventoryNode = seq.GetNodeEdit ();
				inventoryNode->SetInventory (true);
			}
		}
		else
		{
			// Only one script in the history -- must be the project creation marker
			Assert (_setChanges.XActualSize () == 1);
			if (!node->IsRelevantMilestone () || !node->IsAckListEmpty ())
				throw Win::Exception ("Illegal project creation marker found");
		}
	}

	if (_setChanges.XActualSize () > 1)
	{
		try
		{
			XSetPredecessorIds (&log, meter);
		}
		catch (Win::Exception )
		{
			XPostConversionDump (log);
			throw;
		}
	}
	_initialFileInventoryIsExecuted = true; // Full sync has been unpacked
}

void History::Db::XCollectScriptIds (std::map<UserId, std::pair<GlobalId, GlobalId> > & userScripts,
							Progress::Meter * meter,
							MemoryLog & log)
{
	log << "Collecting oldest and most recent script ids." << std::endl;
	GidSet users;
	std::map<GlobalId, std::pair<GlobalId, GlobalId> >::iterator iter = userScripts.begin ();
	for ( ; iter != userScripts.end (); ++iter)
	{
		UserId userId = iter->first;
		users.insert (userId);
	}
	// Walk history notes in the reverse chronological order -- from the
	// current version to the full synch.
	History::SortedTree::XFullSequencer seq (_setChanges);
	while (!seq.AtEnd ())
	{
		History::Node const * node = seq.GetNode ();
		GlobalId scriptId = node->GetScriptId ();
		GlobalIdPack pack (scriptId);
		UserId userId = pack.GetUserId ();
		iter = userScripts.find (userId);
		if (iter != userScripts.end ())
		{
			if (users.find (userId) != users.end ())
			{
				// Set most recent script id for that user
				meter->StepIt ();
				iter->second.first = scriptId;
				users.erase (userId);
			}
			// Set oldest script for that user
			iter->second.second = scriptId;
		}
		seq.Advance ();
	}
	log << "Done with collecting oldest and most recent script ids." << std::endl;
}

void History::Db::XMembershipTreeCleanup ()
{
	AckBox ackBox;
	GidList members;
	_projectDb.XGetHistoricalMemberList (members);
	for (GidList::const_iterator iter = members.begin (); iter != members.end (); ++iter)
	{
		GlobalId id = *iter;
		MemberState state = _projectDb.XGetMemberState (id);
		if (state.IsDead () || state.IsObserver ())
			XRemoveFromAckList (id, state.IsDead (), ackBox);
	}

	GidSet missingScripts;
	GidSet rejectedScripts;
	GidSet recordedScripts;
	// Iterate all nodes in the reverse chronological order
	for (History::SortedTree::XWriteSequencer seq (_membershipChanges); !seq.AtEnd (); seq.Advance ())
	{
		History::Node const * node = seq.GetNode ();
		GlobalId editedUserId = node->GetUnitId (Unit::Member);
		GidList::const_iterator iter = std::find (members.begin (), members.end (), editedUserId);
		if (iter == members.end ())
		{
			// Script editing unknown project member
			seq.MarkDeleted ();
			continue;
		}
		if (node->IsMissing ())
		{
			if (editedUserId == _projectDb.XGetMyId ())
			{
				// Missing script created by this user
				seq.MarkDeleted ();
			}
			else
			{
				UserId missingScriptSenderId = GlobalIdPack (node->GetScriptId ()).GetUserId ();
				MemberState senderState = _projectDb.XGetMemberState (missingScriptSenderId);
				MemberState editedUserState = _projectDb.XGetMemberState (editedUserId);
				if (senderState.IsDead () && editedUserState.IsDead ())
				{
					// Both are dead -- remove node
					seq.MarkDeleted ();
				}
				else
				{
					missingScripts.insert (node->GetScriptId ());
				}
			}
		}
		else if (node->IsRejected ())
		{
			rejectedScripts.insert (node->GetScriptId ());
		}
		else
		{
			recordedScripts.insert (node->GetScriptId ());
		}
	}

	if (!missingScripts.empty ())
	{
		GidSet missingDuplicates;
		std::set_intersection (missingScripts.begin (), missingScripts.end (),
							   recordedScripts.begin (), recordedScripts.end (),
							   std::insert_iterator<GidSet>(missingDuplicates, missingDuplicates.begin ()));
		if (!missingDuplicates.empty ())
		{
			for (History::SortedTree::XWriteSequencer seq (_membershipChanges); !seq.AtEnd (); seq.Advance ())
			{
				History::Node const * node = seq.GetNode ();
				if (node->IsMissing ())
				{
					GidSet::const_iterator iter = missingDuplicates.find (node->GetScriptId ());
					if (iter != missingDuplicates.end ())
					{
						// Remove missing membership script duplicate
						seq.MarkDeleted ();
					}
				}
			}
		}
	}

	if (!rejectedScripts.empty ())
	{
		GidSet rejectedDuplicates;
		std::set_intersection (rejectedScripts.begin (), rejectedScripts.end (),
							   recordedScripts.begin (), recordedScripts.end (),
							   std::insert_iterator<GidSet>(rejectedDuplicates, rejectedDuplicates.begin ()));
		if (!rejectedDuplicates.empty ())
		{
			for (History::SortedTree::XWriteSequencer seq (_membershipChanges); !seq.AtEnd (); seq.Advance ())
			{
				History::Node const * node = seq.GetNode ();
				if (node->IsRejected ())
				{
					GidSet::const_iterator iter = rejectedDuplicates.find (node->GetScriptId ());
					if (iter != rejectedDuplicates.end ())
					{
						// Remove rejected membership script duplicate
						seq.MarkDeleted ();
					}
				}
			}
		}
	}
}

class History::Db::XConversionSequencer
{
public:
	XConversionSequencer (History::SortedTree & tree, Log<History::Db::HdrNote> const & hdrLog, Progress::Meter * meter)
		: _meter (meter),
		  _treeSeq (tree),
		  _hdrLog (hdrLog),
		  _predecessorId (gidInvalid),
		  _scriptId (gidInvalid),
		  _lastRejectedIdx (-1)
	{
		Init ();
		// Hidden assumption that history doesn't end in rejected scripts?
		Assert (!_treeSeq.GetNode ()->IsRejected ());
	}

	bool AtEnd () const { return _treeSeq.AtEnd (); }
	void Advance ()
	{
		// Note: History::SortedTree::XWriteSequencer skips null nodes
		_treeSeq.Advance ();
        History::Node const * node = nullptr;
		while (!_treeSeq.AtEnd ())
		{
			if (_meter != 0)
				_meter->StepIt ();
			node = _treeSeq.GetNode ();
			if (!node->IsRejected ())
				break;
			_treeSeq.Advance ();
		}
        Assert(_treeSeq.AtEnd() || node != nullptr && !node->IsRejected());
		Init ();
	}
	GlobalId GetScriptId () const { return _scriptId; }
	GlobalId GetPredecessorId () const { return _predecessorId; }
	unsigned int GetLastRejectedscriptIdx () const { return _lastRejectedIdx; }
	void MarkScriptRejected ()
	{
		History::Node * editedNode = _treeSeq.GetNodeEdit ();
		editedNode->SetRejected (true);
		_lastRejectedIdx = _treeSeq.GetIdx ();
	}

private:
	void Init ()
	{
		_predecessorId = gidInvalid;
		_scriptId = gidInvalid;
		if (!_treeSeq.AtEnd ())
		{
			History::Node const * node = _treeSeq.GetNode ();
			Assert (node != 0);
			_scriptId = node->GetScriptId ();
			std::unique_ptr<HdrNote> hdrNote = _hdrLog.Retrieve (node->GetHdrLogOffset (), node->GetScriptVersion ());
			Lineage const & scriptLineage = hdrNote->GetLineage ();
			_predecessorId = scriptLineage.GetLastScriptId ();
		}
	}

private:
	Progress::Meter *				_meter;
	History::SortedTree::XWriteSequencer	_treeSeq;
	Log<HdrNote> const &		_hdrLog;
	GlobalId					_predecessorId;
	GlobalId					_scriptId;
	unsigned int				_lastRejectedIdx;
};

void History::Db::XSetPredecessorIds (MemoryLog * log, Progress::Meter * meter)
{
	if (log) *log << "Setting tree node predecessor ids" << std::endl;
	std::multimap<GlobalId, unsigned> successorMap;
	GidList branchPoints;
	// Walk history in the reverse chronological order
	History::SortedTree::XWriteSequencer seq (_setChanges);
    History::Node * editedNode = nullptr;
	GlobalId predecessorId = gidInvalid;
	for (; !seq.AtEnd (); seq.Advance ())
	{
		if (meter != 0)
			meter->StepIt ();
		editedNode = seq.GetNodeEdit ();
		if (log) *log << "    History::Node: " << *editedNode << " has lineage: ";
		Assert (editedNode != 0);
		// Get predecessor id from the script lineage
		std::unique_ptr<HdrNote> hdrNote = _hdrLog.Retrieve (editedNode->GetHdrLogOffset (), editedNode->GetScriptVersion ());
		Lineage const & lineage = hdrNote->GetLineage ();
		if (log) *log << lineage << std::endl;
		predecessorId = lineage.GetLastScriptId ();
		if (log) *log << "    History::Node predecessor: " << GlobalIdPack (predecessorId).ToString () << std::endl;
		editedNode->SetPredecessorId (predecessorId);
		if (predecessorId == gidInvalid)
			break;
		if (successorMap.find (predecessorId) != successorMap.end ())
			branchPoints.push_back (predecessorId);
		successorMap.insert (std::make_pair (predecessorId, seq.GetIdx ()));
	}
	Assert (seq.AtEnd () || predecessorId == gidInvalid);
	if (seq.AtEnd ())
	{
		Win::ClearError ();
		throw Win::Exception ("Cannot convert project history -- missing project creation marker.");
	}

	seq.Advance ();
	if (!seq.AtEnd ())
	{
		// Must be project creation marker
		History::Node const * node = seq.GetNode ();
		Assert (node != 0);
		editedNode->SetPredecessorId (node->GetScriptId ());
		if (successorMap.find (node->GetScriptId ()) != successorMap.end ())
			branchPoints.push_back (node->GetScriptId ());
		successorMap.insert (std::make_pair (node->GetScriptId (), seq.GetIdx ()));
		seq.Advance ();
		if (!seq.AtEnd ())
		{
			Win::ClearError ();
			throw Win::Exception ("Cannot convert project history -- extra scripts before project creation marker.");
		}
	}
	if (log != 0)
	{
		*log << "Done with setting tree node predecessor ids" << std::endl;
		*log << "Removing rejected nodes" << std::endl;
	}
	typedef std::multimap<GlobalId, unsigned>::const_iterator MmIter;
	for (GidList::const_iterator it = branchPoints.begin (); it != branchPoints.end (); ++it)
	{
		GlobalId gidBranch = *it;
		if (log) *log << "=== Branch point: " << GlobalIdPack (gidBranch).ToString () << " ===" << std::endl;
		// Resolve conflict among children of this branch point
		std::pair<MmIter, MmIter> range = successorMap.equal_range (gidBranch);
		MmIter rit = range.first;
		unsigned winnerIdx = -1;
		History::Node const * winnerNode = 0;
		// Find first non-empty candidate
		do 
		{
			winnerIdx = rit->second;
			winnerNode = _setChanges.XGetNode (winnerIdx);
			++rit;
		} while ((winnerNode == 0 || winnerNode->IsRejected () || winnerNode->IsToBeRejected ()) && rit != range.second);

		if (winnerNode == 0) // all children have already been deleted
			continue;

		if (log) *log << "---> " << *winnerNode << std::endl;
		Assert (rit == range.second || rit->second != winnerIdx);

		// compare priorities with remaining child nodes
		while (rit != range.second)
		{
			History::Node const * node = _setChanges.XGetNode (rit->second);
			if (node != 0)
			{
				if (log) *log << "---> " << *node << std::endl;
				if (!node->IsRejected () && !node->IsToBeRejected ())
				{
					GlobalIdPack gidWin (winnerNode->GetScriptId ());
					GlobalIdPack gidCur (node->GetScriptId ());
					if (gidCur.GetUserId () < gidWin.GetUserId ())
					{
						winnerNode = node;
						winnerIdx = rit->second;
					}
				}
			}
			++rit;
		}
		// We have the winner, delete all losing branches
		for (rit = range.first; rit != range.second; ++rit)
		{
			if (rit->second != winnerIdx && _setChanges.XGetNode (rit->second) != 0)
				_setChanges.XDeleteBranch (0, rit->second, successorMap, log);
		}
	}

	// Remove all rejected scripts from the top of history
	// (they could have been rejected by incoming scripts still in the inbox)
	_setChanges.XRemoveTopRejected ();

	_setChanges.XInitLastArchivable (log);
}

std::ostream & operator<<(std::ostream & os, History::Db::MissingScriptId id)
{
	GlobalIdPack idPack (id.Gid ());
	GlobalId unitId = id.UnitId ();
	os << idPack << "    " << id.Type () << " unit ID: " << unitId;
	return os;
}
