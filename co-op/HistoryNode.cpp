//------------------------------------
//  (c) Reliable Software, 2003 - 2008
//------------------------------------

#include "precompiled.h"
#include "HistoryNode.h"
#include "ScriptHeader.h"
#include "MailboxHelper.h"

using namespace History;

void Node::Cmd::Serialize (Serializer& out) const
{
    out.PutLong (_unitId);
    _logOffset.Serialize (out);
}

void Node::Cmd::Deserialize (Deserializer& in, int version)
{
    _unitId = in.GetLong ();
    _logOffset.Deserialize (in, version);;
}

Node::Node (File::Offset logOffset,
			ScriptHeader const & hdr,
			GidList const & ackList)
    : _hdrLogOffset (logOffset),
      _scriptVersion (scriptVersion),
	  _ackList (ackList),
      _scriptId (hdr.ScriptId ()),
	  _predecessorId (hdr.GetLineage ().GetLastScriptId ())
{
	_state.SetForceExec (hdr.IsMembershipChange ());
}


Node::Node (GlobalId scriptId, GlobalId predecessorId, GlobalId unitId, bool canRequestResend) 
    : _scriptVersion (scriptVersion),
      _scriptId (scriptId),
	  _predecessorId (predecessorId)
{
	Assert (scriptId != gidInvalid);
	Assert (scriptId != predecessorId);
	_state.SetMissing (true);
	_state.SetDontResend (!canRequestResend);
	if (canRequestResend)
	{
		// Awaiting make reference script from the sender
		GlobalIdPack pack (_scriptId);
		_ackList.push_back (pack.GetUserId ());
	}
	if (unitId != gidInvalid)
	{
		// Unit notes have only one command
		// No command log offset yet, but unit id is known
		std::unique_ptr<Node::Cmd> cmd (new Node::Cmd (unitId, File::Offset::Invalid));
		AddCmd (std::move(cmd));
	}
}

Node::Node (Node const & node)
    : _hdrLogOffset (node.GetHdrLogOffset ()),
      _scriptVersion (node.GetScriptVersion ()),
	  _ackList (node._ackList),
	  _state (node.GetFlags ()),
      _scriptId (node.GetScriptId ()),
	  _predecessorId (node.GetPredecessorId ())
{
    // Copy commands
    for (Node::CmdSequencer seq (node); !seq.AtEnd (); seq.Advance ())
    {
        std::unique_ptr<Cmd> node (new Cmd (seq.GetCmd ()));
        _cmds.push_back (std::move(node));
    }
}

MissingNode::MissingNode (GlobalId scriptId, 
						  GlobalId predecessorId, 
						  GlobalId unitId, 
						  Mailbox::Agent & agent,
						  bool canRequestResend)
	: Node (scriptId, predecessorId, unitId, canRequestResend)
{
	Unit::Type unitType = (unitId == gidInvalid) ? Unit::Set : Unit::Member;
	agent.RememberMissingScriptPlaceholder (scriptId, unitType);
}

MissingNode::MissingNode (GlobalId predecessorId,
						  Mailbox::Agent & agent,
						  bool canRequestResend)
	: Node (agent.GetKnownMissingId (),
			predecessorId,
			agent.GetKnownMissingUnitId (),
			canRequestResend)
{
	Unit::Type unitType = (agent.GetKnownMissingUnitId () == gidInvalid) ? Unit::Set : Unit::Member;
	agent.RememberMissingScriptPlaceholder (agent.GetKnownMissingId (), unitType);
}

MissingInventory::MissingInventory (GlobalId inventoryId, GlobalId predecessorId)
	: Node (inventoryId, predecessorId, gidInvalid, true)	// can request resend
{
	SetInventory (true);
}

bool Node::IsHigherPriority (Node const * node) const
{
	GlobalIdPack otherId (node->GetScriptId ());
	return IsHigherPriority (otherId.GetUserId ());
}

bool Node::IsHigherPriority (UserId otherUserId) const
{
	GlobalIdPack thisId (GetScriptId ());
	return thisId.GetUserId () < otherUserId;
}

void Node::AddCmd (std::unique_ptr<Cmd> node)
{
    _cmds.push_back (std::move(node));
}

class IsEqualId : public std::unary_function<Node::Cmd const *, bool>
{
public:
	IsEqualId (GlobalId id)
		: _id (id)
	{}

	bool operator () (Node::Cmd const * cmd) const
	{
		return cmd->GetUnitId () == _id;
	}
private:
	GlobalId	_id;
};

void Node::ChangeCmdLogOffset (GlobalId gid, File::Offset logOffset)
{
	auto_vector<Node::Cmd>::iterator iter =
		std::find_if (_cmds.begin (), _cmds.end (), IsEqualId (gid));
	Assert (iter != _cmds.end ());
	Cmd * cmdNote = *iter;
	cmdNote->SetLogOffset (logOffset);
}

GlobalId Node::GetUnitId (Unit::Type unitType) const
{
	if (unitType == Unit::Set)
		return gidInvalid;

	Assert (_cmds.size () == 1);
	return _cmds [0]->GetUnitId ();
}

Node::Cmd const * Node::Find (GlobalId id) const
{
	auto_vector<Node::Cmd>::const_iterator iter =
		std::find_if (_cmds.begin (), _cmds.end (), IsEqualId (id));
	if (iter != _cmds.end ())
		return *iter;
	else
		return 0;
}

bool Node::IsChanging (GidSet const & preSelectedGids) const
{
    for (unsigned int i = 0; i < _cmds.size(); i++)
    {
		GlobalId gid = _cmds [i]->GetUnitId ();
        if (preSelectedGids.find (gid) != preSelectedGids.end ())
        {
            return true;
        }
    }
    return false;
}

bool Node::IsChanging (UserId unitId) const
{
	if (unitId == gidInvalid)
		return true;	// Unit id equal gidInvalid is used by the set change notes
						// and every node in the set change vector changes the set
	// Unit changes have only one command -- check if it changes given unit
	Assert (_cmds.size () == 1);
	Assert (_cmds [0] != 0);
	return _cmds [0]->GetUnitId () == unitId;
}

void Node::AcceptAck (UserId userId)
{
	GidList::iterator pos = std::find (_ackList.begin (), _ackList.end (), userId);
	if (pos != _ackList.end ())
		_ackList.erase (pos);
}

void Node::Serialize (Serializer& out) const
{
    _hdrLogOffset.Serialize (out);
    out.PutLong (_scriptVersion);
    out.PutLong (_cmds.size ());
	unsigned int i;
    for (i = 0; i < _cmds.size (); i++)
        _cmds [i]->Serialize (out);
    out.PutLong (_ackList.size ());
    for (i = 0; i < _ackList.size (); i++)
        out.PutLong (_ackList [i]);
    out.PutLong (_state.GetValue ());
	out.PutLong (_scriptId);
	out.PutLong (_predecessorId);
}

void Node::Deserialize (Deserializer& in, int version)
{
    _hdrLogOffset.Deserialize (in, version);
    _scriptVersion = in.GetLong ();

    unsigned int count = in.GetLong ();
	unsigned int i = 0;
    for (i = 0; i < count; ++i)
    {
        std::unique_ptr<Cmd> cmd (new Cmd (in, version));
        _cmds.push_back (std::move(cmd));
    }

    count = in.GetLong ();
    for (i = 0; i < count; i++)
        _ackList.push_back (in.GetLong ());
	if (version < 27)
	{
		bool changeScript = in.GetLong () != 0;
		_scriptId = gidInvalid;
	}
	else
	{
		_state = in.GetLong ();
		_scriptId = in.GetLong ();
	}
	if (version >= 37 && version < 41)
	{
		// Eat unused script kind
		in.GetLong ();
	}
	if (version < 43)
	{
		_predecessorId = gidInvalid;
	}
	else
	{
		_predecessorId = in.GetLong ();
	}
}

bool Node::ConvertState ()
{
	V42State oldState (_state.GetValue ());
	if (!oldState.IsChangeScript () && !oldState.IsProjectCreationMarker ())
		return false;	// Placeholder node -- remove during conversion from version 4.2 to 4.5

	// Remove old state bits
	ClearState ();
	SetArchive (oldState.IsArchive ());
	SetMilestone (oldState.IsMilestone ());
	SetRejected (oldState.IsRejected ()); // rejected node will be removed later
	SetExecuted (!oldState.IsRejected ());
	return true;
}

std::ostream & operator<<(std::ostream & os, History::Node const & node)
{
	GlobalIdPack scriptId (node.GetScriptId ());
	GlobalIdPack predecessorId (node.GetPredecessorId ());
	os << " " << scriptId.ToSquaredString () << " (" << predecessorId.ToString () << "), ";
	History::Node::State state (node.GetFlags ());
	os << state;
	GidList const & ackList = node.GetAckList ();
	if (ackList.empty ())
		os << ", confirmed";
	else
	{
		os << ", awaiting ack from: ";
		for (GidList::const_iterator iter = ackList.begin (); iter != ackList.end (); ++iter)
			os << std::hex << *iter << " ";
	}
	return os;
}

std::ostream & operator<<(std::ostream & os, History::Node::State const & state)
{
	if (state.IsCandidateForExecution ())
	{
		if (state.IsBranchPoint ())
		{
			if (state.IsMilestone ())
			{
				if (state.IsForceExec ())
					os << "candidate for execution (branch point; milestone; forced)";
				else
					os << "candidate for execution (branch point; milestone)";
			}
			else if (state.IsForceExec ())
				os << "candidate for execution (branch point; forced)";
			else
				os << "candidate for execution (branch point)";
		}
		else if (state.IsMilestone ())
		{
			if (state.IsForceExec ())
				os << "candidate for execution (milestone; forced)";
			else
				os << "candidate for execution (milestone)";
		}
		else
		{
			if (state.IsForceExec ())
			{
				if (state.IsInventory ())
					os << "candidate for execution (forced, inventory)";
				else
					os << "candidate for execution (forced)";
			}
			else
			{
				if (state.IsInventory ())
					os << "candidate for execution (inventory)";
				else
					os << "candidate for execution";
			}
		}
	}
	else if (state.IsMissing ())
	{
		if (state.IsBranchPoint ())
		{
			if (state.IsRejected ())
			{
				if (state.CanRequestResend ())
					os << "missing (branch point; rejected)";
				else
					os << "missing (branch point; rejected; don't request resend)";
			}
			else if (state.CanRequestResend ())
				os << "missing (branch point)";
			else
				os << "missing (branch point; don't request resend)";
		}
		else if (state.IsRejected ())
		{
			if (state.CanRequestResend ())
				os << "missing (rejected)";
			else
				os << "missing (rejected; don't request resend)";
		}
		else if (state.CanRequestResend ())
			os << "missing";
		else
			os << "missing (don't request resend)";
	}
	else if (state.IsExecuted ())
	{
		if (state.IsBranchPoint ())
		{
			if (state.IsMilestone ())
				os << "executed (branch point; milestone)";
			else
				os << "executed (branch point)";
		}
		else if (state.IsMilestone ())
		{
			os << "executed (milestone)";
		}
		else
		{
			if (state.IsInventory ())
				os << "executed (inventory)";
			else
				os << "executed";
		}
	}
	else if (state.IsRejected ())
	{
		if (state.IsBranchPoint ())
		{
			if (state.IsMilestone ())
				os << "rejected (branch point; milestone)";
			else if (state.IsForceExec ())
				os << "rejected (branch point; forced)";
			else
				os << "rejected (branch point)";
		}
		else if (state.IsMilestone ())
		{
			os << "rejected (milestone)";
		}
		else if (state.IsForceExec ())
		{
			os << "rejected (forced)";
		}
		else
		{
			os << "rejected";
		}
	}
	else if (state.IsToBeRejected ())
	{
		if (state.IsBranchPoint ())
		{
			if (state.IsMilestone ())
				os << "to be rejected (branch point; milestone)";
			else
				os << "to be rejected (branch point)";
		}
		else if (state.IsMilestone ())
		{
			os << "to be rejected (milestone)";
		}
		else
		{
			os << "to be rejected";
		}
	}
	else
	{
		os << "unknown (0x" << std::hex << state.GetValue () << ")";
	}
	return os;
}
