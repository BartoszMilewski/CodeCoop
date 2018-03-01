#if !defined (HISTORYNODE_H)
#define HISTORYNODE_H
//------------------------------------
//  (c) Reliable Software, 2003 - 2008
//------------------------------------

#include "GlobalId.h"
#include "Serialize.h"
#include "Lineage.h"
#include "XFileOffset.h"

#include <auto_vector.h>

#include <iosfwd>

class ScriptHeader;
namespace Mailbox
{
	class Agent;
}

namespace History
{
	class Node : public Serializable
	{
	public:

		class State
		{
		public:
			State ()
				: _value (0)
			{}
			State (State const & state) 
			{ 
				_value = state._value;
			}
			State (unsigned long value)
				: _value (value)
			{}
			void operator= (State const & state) 
			{ 
				_value = state._value;
			}

			void SetRejected (bool flag) { _bits._rejected = flag; }
			void SetArchive (bool flag) { _bits._archive = flag; }
			void SetMilestone (bool flag) { _bits._milestone = flag; }
			void SetMissing (bool flag) { _bits._missing = flag; }
			void SetExecuted (bool flag) { _bits._executed = flag; }
			void SetForceExec (bool flag) { _bits._forceExec = flag; }
			void SetBranchPoint (bool flag) { _bits._branchPoint = flag; }
			void SetInventory (bool flag) { _bits._inventory = flag; }
			void SetForceAccept (bool flag) { _bits._forceAccept = flag; }
			void SetDefect (bool flag) { _bits._defect = flag; }
			void SetDontResend (bool flag) { _bits._dontResend = flag; }
			void SetDeepFork (bool flag) { _bits._deepFork = flag; }

			unsigned long GetValue () const { return _value; }

			bool IsRejected () const { return _bits._executed == 0 && _bits._rejected != 0; }
			bool IsArchive () const { return _bits._archive != 0; }
			bool IsMilestone () const { return _bits._milestone != 0; }
			bool IsMissing () const { return _bits._missing != 0; }
			bool IsExecuted () const { return _bits._executed != 0 && _bits._rejected == 0; }
			bool IsForceExec () const { return _bits._forceExec != 0; }
			bool IsToBeRejected () const { return _bits._executed != 0 && _bits._rejected != 0; }
			bool IsCandidateForExecution () const { return _bits._executed == 0 && _bits._rejected == 0 && _bits._missing == 0; }
			bool IsBranchPoint () const { return _bits._branchPoint != 0; }
			bool IsInventory () const { return _bits._inventory != 0; }
			bool IsForceAccept () const { return _bits._forceAccept != 0; }
			bool IsDefect () const { return _bits._defect != 0; }
			bool CanRequestResend () const { return _bits._dontResend == 0; }
			bool IsDeepFork () const { return _bits._deepFork != 0; }

		private:
			union
			{
				unsigned long _value;		// for quick serialization
				// NOTICE: when adding new bits, remember to update:
				//     History::ScriptState
				//     Mailbox::ScriptState
				struct
				{
					unsigned long _archive:1;		// Start of archived history portion
					unsigned long _milestone:1;		// Milestone script
					unsigned long _rejected:1;		// Rejected script
					unsigned long _missing:1;		// Missing script
					unsigned long _executed:1;		// Executed script
					unsigned long _forceExec:1;		// Execute script after unpacking
					unsigned long _branchPoint:1;	// Branch point in the history
					unsigned long _inventory:1;		// Complete set of project files
					unsigned long _forceAccept:1;	// Accept script changes after executing it
					unsigned long _defect:1;		// Project member defect script
					unsigned long _dontResend:1;	// Don't request script resend
					unsigned long _deepFork:1;		// Marks original FIS in the branch project
				} _bits;
			};
		};

		// State bits as used by version 4.2
		class V42State
		{
		public:
			V42State (unsigned long value)
				: _value (value)
			{}
			bool IsChangeScript () const { return _bits._changeScript != 0; }
			bool IsRejected () const { return _bits._rejected != 0; }
			bool IsArchive () const { return _bits._archive != 0; }
			bool IsMilestone () const { return _bits._milestone != 0; }
			bool IsProjectCreationMarker () const { return !IsChangeScript () && IsMilestone (); }

		private:
			union
			{
				unsigned long _value;		// for quick serialization
				struct
				{
					unsigned long _changeScript:1;	
					unsigned long _archive:1;		// Marks start of archived history portion
					unsigned long _milestone:1;		// Marks milestone script
					unsigned long _rejected:1;		// Rejected script
				} _bits;
			};
		};

		// Could be file command or member command
		// The former contains GlobalId, the latter UserId (as Unit Id)
		class Cmd : public Serializable
		{
		public:
			Cmd (GlobalId id, File::Offset logOffset)
				: _unitId (id),
				  _logOffset (logOffset)
			{}
			Cmd (Cmd const * cmd)
				: _unitId (cmd->GetUnitId ()),
				  _logOffset (cmd->GetLogOffset ())
			{}
			Cmd (Deserializer& in, int version)
			{
				Deserialize (in, version);
			}

			GlobalId GetUnitId () const { return _unitId; }
			File::Offset GetLogOffset () const { return _logOffset; }

			void SetUnitId (GlobalId id) { _unitId = id; }
			void SetLogOffset (File::Offset offset) { _logOffset = offset; }

			// Serializable interface

			void Serialize (Serializer& out) const;
			void Deserialize (Deserializer& in, int version);

		private:
			// Revisit: introduce class UnitId that can be initialize with either
			GlobalId		_unitId;
			SerFileOffset	_logOffset;
		};

		class CmdSequencer
		{
		public:
			CmdSequencer (Node const & node)
				: _cur (node._cmds.begin ()),
				  _end (node._cmds.end ())
			{}

			void Advance () { ++_cur; }
			bool AtEnd () const { return _cur == _end; }
			Cmd const * GetCmd () const { return *_cur; }

		private:
			auto_vector<Cmd>::const_iterator	_cur;
			auto_vector<Cmd>::const_iterator	_end;
		};

		friend class CmdSequencer;

	public:
		Node (File::Offset logOffset, ScriptHeader const & hdr, GidList const & ackList);
		Node (Node const & node);
		Node (Deserializer& in, int version)
		{
			Deserialize (in, version);
		}

		unsigned long GetFlags () const { return _state.GetValue (); }
		void ClearState () { _state = State (); }

		bool IsArchive () const { return _state.IsArchive (); }
		bool IsMilestone () const { return _state.IsMilestone (); }
		bool IsRejected () const { return _state.IsRejected (); }
		bool IsMissing () const { return _state.IsMissing (); }
		bool IsExecuted () const { return _state.IsExecuted (); }
		bool IsForceExec () const { return _state.IsForceExec (); }
		bool IsForceAccept () const { return _state.IsForceAccept (); }
		bool IsDefect () const { return _state.IsDefect (); }
		bool IsToBeRejected () const { return _state.IsToBeRejected (); }
		bool IsCandidateForExecution () const { return _state.IsCandidateForExecution (); }
		bool IsBranchPoint () const { return _state.IsBranchPoint (); }
		bool IsInventory () const { return _state.IsInventory (); }
		bool CanRequestResend () const { return _state.CanRequestResend (); }
		bool IsAckListEmpty () const { return _ackList.size () == 0; }
		bool HasBeenUnpacked () const 
		{
			// Note: This predicate only works for set changes
			// Revisit: add bit to node if script has been seen
			return _cmds.size () != 0 || IsMilestone (); 
		}
		bool IsConfirmedScript () const { return  IsExecuted () && IsAckListEmpty (); }
		bool IsRelevantScript () const { return IsExecuted () && !IsMilestone (); }
		bool IsRejectedScript () const { return IsRejected () && !IsMilestone (); }
		bool IsRelevantMilestone () const { return IsExecuted () && IsMilestone (); }
		bool IsRejectedMilestone () const { return IsRejected () && IsMilestone (); }
		bool IsDeepFork () const { return _state.IsDeepFork () != 0; }

		File::Offset GetHdrLogOffset () const { return _hdrLogOffset; }
		int GetScriptVersion () const { return _scriptVersion; }
		GlobalId GetScriptId () const { return _scriptId; }
		GlobalId GetPredecessorId () const { return _predecessorId; }
		GidList const & GetAckList () const { return _ackList; }
		GlobalId GetUnitId (Unit::Type unitType) const;
		Cmd const * Find (GlobalId fileGid) const;
		bool IsChanging (GidSet const & preSelectedGids) const;
		bool IsChanging (GlobalId unitId) const;
		bool IsHigherPriority (Node const * node) const;
		bool IsHigherPriority (UserId userId) const;

		void AddCmd (std::unique_ptr<Cmd> node);
		void ChangeCmdLogOffset (GlobalId gid, File::Offset logOffset);
		void SetScriptId (GlobalId scriptId) { _scriptId = scriptId; }
		void SetPredecessorId (GlobalId predecessorId) { _predecessorId = predecessorId; }
		void SetHdrLogOffset (File::Offset offset) { _hdrLogOffset = offset; }
		void AcceptAck (UserId userId);
		void ClearAckList () { _ackList.clear (); }
		void SetState (unsigned long state) { _state = State (state); }
		void SetArchive (bool flag) { _state.SetArchive (flag); }
		void SetMilestone (bool flag) { _state.SetMilestone (flag); }
		void SetRejected (bool flag) { _state.SetRejected (flag); }
		void SetMissing (bool flag) { _state.SetMissing (flag); }
		void SetExecuted (bool flag) { _state.SetExecuted (flag); }
		void SetForceExec (bool flag) { _state.SetForceExec (flag); }
		void SetForceAccept (bool flag) { _state.SetForceAccept (flag); }
		void SetDefect (bool flag) { _state.SetDefect (flag); }
		void SetBranchPoint (bool flag) { _state.SetBranchPoint (flag); }
		void SetInventory (bool flag) { _state.SetInventory (flag); }
		void SetDontResend (bool flag) { _state.SetDontResend (flag); }
		void SetDeepFork (bool flag) { _state.SetDeepFork (flag); }
		void SetScriptVersion (int ver) { _scriptVersion = ver; }

		// Serializable interface
		void Serialize (Serializer& out) const;
		void Deserialize (Deserializer& in, int version);

		// Conversion from version 4.2 to 4.5
		bool ConvertState ();

	protected:
		Node () {}
		Node (GlobalId scriptId, GlobalId predecessorId, GlobalId unitId, bool canRequestResend);

	private:
		SerFileOffset		_hdrLogOffset;
		int 				_scriptVersion;
		auto_vector<Cmd>	_cmds;
		GidList 			_ackList;
		State				_state;
		GlobalId			_scriptId;
		GlobalId			_predecessorId;
	};

	class MissingNode : public Node
	{
	public:
		MissingNode (GlobalId scriptId,
					 GlobalId unitId,
					 GlobalId predecessorId,
					 Mailbox::Agent & agent,
					 bool canRequestResend);
		MissingNode (GlobalId predecessorId,
					 Mailbox::Agent & agent,
					 bool canRequestResend);
	};

	class MissingInventory : public Node
	{
	public:
		MissingInventory (GlobalId inventoryId, GlobalId predecessorId);
	};
}

std::ostream & operator<<(std::ostream & os, History::Node const & node);
std::ostream & operator<<(std::ostream & os, History::Node::State const & state);

#endif
