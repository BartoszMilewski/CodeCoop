#if !defined MEMBERINFO_H
#define MEMBERINFO_H
//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "Serialize.h"
#include "MemberDescription.h"
#include "GlobalId.h"

#include <Dbg/Assert.h>
#include <TriState.h>

namespace Project
{
	class Db;
}
class Catalog;

namespace XML { class Node; }

class MemberCache
{
public:
	MemberCache (Project::Db const & dataBase)
		: _dataBase (dataBase),
		  _lastUserId (gidInvalid)
	{}
	MemberDescription const * GetMemberDescription (UserId userId);
	void Invalidate () { _lastUserId = gidInvalid; }

private:
	Project::Db const & _dataBase;
	UserId				_lastUserId;
	std::unique_ptr<MemberDescription> _memberDescription;
};

class MemberState
{
public:
    MemberState ()
		: _value (0)
	{
		_bits._Ver = 1;
	}
    MemberState (MemberState const & state)
		: _value (state.GetValue ())
	{}
    explicit MemberState (unsigned long value)
        : _value (value)
    {}
    void operator= (MemberState const & state) 
    { 
        _value = state._value;
    }
	bool operator < (MemberState const & state) const;

    unsigned long GetValue () const { return _value; } 
    char const * GetDisplayName () const; 
    void Reset () 
    { 
        _value = 0;
		_bits._Ver = 1;
    }

	void MakeVotingMember ()
	{
		_bits._V = 1;
		_bits._A = 1;
		_bits._Adm = 0;
	}
	void MakeObserver ()
	{
		_bits._V = 0;
		_bits._A = 1;
		_bits._Adm = 0;
	}
	void MakeDead (bool suicide = false)
	{
		_bits._V = 0;
		_bits._A = 0;
		_bits._Adm = 0;
		_bits._S = suicide ? 1 : 0;
	}
	void MakeAdmin ()
	{
		_bits._V = 1;
		_bits._A = 1;
		_bits._Adm = 1;
	}
	void MakeReceiver (bool noBranching)
	{
		_bits._V = 1;
		_bits._A = 1;
		_bits._Adm = 0;
		_bits._Recv = 1;
		_bits._Distr = 0;
		_bits._NoBra = noBranching;
	}
	void SetSuicide (bool flag) { _bits._S = flag ? 1 : 0; }
	void SetVerified (bool flag) { _bits._Ver = flag ? 1 : 0; }
	void SetDistributor (bool flag) { _bits._Distr = flag? 1: 0; }
	void SetReceiver (bool flag) { _bits._Recv = flag? 1: 0; }
	void SetObserver (bool flag) { _bits._V = flag? 0: 1; }
	void SetNoBranching (bool flag) { _bits._NoBra = flag? 1: 0; }
	void SetCheckoutNotification (bool flag) { _bits._Out = flag ? 1 : 0; }

	bool IsEqual (MemberState state) const { return _value == state._value; }
	bool IsEqualModuloCoNotify (MemberState state) const 
	{
		MemberState myState(*this);
		myState.SetCheckoutNotification(false);
		state.SetCheckoutNotification(false);
		return myState._value == state._value; 
	}

    bool IsVoting () const { return _bits._V != 0; }
	bool IsActive () const { return _bits._A != 0; }
	bool HasAdminPriv () const { return _bits._Adm != 0; }
	bool IsSuicide () const { return _bits._S != 0; }
	bool IsVerified () const	 { return _bits._Ver != 0; }
	bool IsDistributor () const { return _bits._Distr != 0; }
	bool IsReceiver () const { return _bits._Recv != 0; }
	bool NoBranching () const { return _bits._NoBra != 0; }
	bool IsCheckoutNotification () const { return _bits._Out != 0; }

	bool IsVotingMember () const { return IsActive () && IsVoting (); }
	bool IsObserver () const     { return IsActive () && !IsVoting (); }
	bool IsDead () const		 { return !IsActive (); }
	bool IsAdmin () const		 { return IsVotingMember () && HasAdminPriv (); }
	bool HasDefected () const	 { return IsDead () && IsSuicide (); }


private:
    union
    {
        unsigned long _value;       // For quick serialization
        struct
        {
            unsigned long _V:1;     // 1 - voting member -- sends, accepts and responds to synchronization scripts
									// 0 - observer -- only accepts synchronization scripts
			unsigned long _A:1;		// 1 - active -- active project member
									// 0 - defected or removed -- no longer a project member
			unsigned long _Adm:1;	// Administrator
			unsigned long _S:1;		// 0 - removed from the project
									// 1 - suicide (voluntary defect)
			unsigned long _Ver:1;	// 1 - version >= 4.5 member
									// 0 - version <  4.5 member
			unsigned long _Distr:1;	// distributor
			unsigned long _Recv:1;	// receiver
			unsigned long _NoBra:1;	// no branching allowed
			unsigned long _Out:1;	// 1 - member sends checkout notifications
									// 0 - member doesn't send checkout notifications
        } _bits;
    };
};

std::ostream& operator<<(std::ostream & os, MemberState const & state);

class StateVotingMember : public MemberState
{
public:
	StateVotingMember ()
	{
		MakeVotingMember ();
	}
	StateVotingMember (MemberState const & state)
		: MemberState (state)	// Copies verification, distributor, receiver and no branching bits
	{
		MakeVotingMember ();
	}
};

class StateAdmin : public MemberState
{
public:
	StateAdmin ()
	{
		MakeAdmin ();
	}
	StateAdmin (MemberState const & state)
		: MemberState (state)	// Copies verification, distributor, receiver and no branching bits
	{
		MakeAdmin ();
	}
};

class StateObserver : public MemberState
{
public:
	StateObserver ()
	{
		MakeObserver ();
	}
	StateObserver (MemberState const & state)
		: MemberState (state)	// Copies verification, distributor, receiver and no branching bits
	{
		MakeObserver ();
	}
};

class StateDead : public MemberState
{
public:
	StateDead (MemberState const & state, bool suicide = false)
		: MemberState (state)	// Copies verification, distributor, receiver and no branching bits
	{
		MakeDead (suicide);
	}
};

class MemberStateInfo
{
public:
	MemberStateInfo (UserId id, MemberState state)
		: _id (id), _state (state)
	{}
	bool operator< (MemberStateInfo const & info) const;
	UserId Id () const { return _id; }
private:
	UserId	_id;
	MemberState	_state;
};

// Basic project member information

class Member : public Serializable
{
public:
    Member ()
        : _id (gidInvalid),
		  _preHistoricScript (gidInvalid),
		  _mostRecentScript (gidInvalid)
    {}
    Member (Member const & member)
        : _id (member._id),
          _state (member._state),
		  _preHistoricScript (member._preHistoricScript),
		  _mostRecentScript (member._mostRecentScript)
    {}
    Member (Deserializer& in, int version)
    {
        Deserialize (in, version);
    }

	UserId Id () const { return _id; }
	MemberState State () const { return _state; }

	bool IsAdmin () const { return _state.IsAdmin (); }
	bool IsVoting () const { return _state.IsVoting (); }
	bool IsReceiver () const { return _state.IsReceiver (); }
	bool IsDistributor () const { return _state.IsDistributor (); }
	bool NoBranching () const { return _state.NoBranching (); }
	bool IsDead () const { return _state.IsDead (); }
	bool HasDefected () const { return _state.HasDefected (); }
    char const * GetStateDisplayName () const { return _state.GetDisplayName (); }
	Tri::State IsPrehistoricScript (GlobalId scriptId) const;
	Tri::State IsFutureScript (GlobalId scriptId) const;
	bool IsIdentical (Member const & member) const;

	GlobalId GetPreHistoricScript () const { return _preHistoricScript; }
	GlobalId GetMostRecentScript () const { return _mostRecentScript; }

	void SetPreHistoricScript (GlobalId scriptId) { _preHistoricScript = scriptId; }
	void SetMostRecentScript (GlobalId scriptId);
	void ResetScriptMarkers ();
	void SetState (MemberState newState) { _state = newState; }
	void MakeVoting () { _state.MakeVotingMember ();	}
	void MakeObserver () { _state.MakeObserver (); }
	void MakeAdmin () { _state.MakeAdmin (); }
	void MakeDistributor (bool noBranching)
	{
		_state.SetDistributor (true);
		_state.SetNoBranching (noBranching);
	}
	void Defect (bool suicide) { _state.MakeDead (suicide); }
	void SetSuicide (bool flag) { _state.SetSuicide (flag); }

	// Serializable interface

    void Serialize (Serializer & out) const;
    void Deserialize (Deserializer & in, int version);

protected:
    Member (UserId id, MemberState state)
        : _id (id),
          _state (state),
		  _preHistoricScript (gidInvalid),
		  _mostRecentScript (gidInvalid)
    {}

protected:
	UserId		_id;
    MemberState	_state;
	GlobalId	_preHistoricScript;
	GlobalId	_mostRecentScript;
};

// Full project member information

class MemberInfo : public Member
{
public:
    MemberInfo (UserId id, MemberState state, MemberDescription const & description)
        : Member (id, state),
          _description (description)
    {}
    MemberInfo (UserId id, MemberState state, MemberDescription && description)
        : Member (id, state),
          _description (description)
    {}
    MemberInfo (Member const & member, MemberDescription const & description)
        : Member (member),
          _description (description)
    {}
    MemberInfo (MemberInfo const & mi)
        : Member (mi),
          _description (mi.Description ())
    {}
    MemberInfo (MemberInfo && mi)
        : Member (mi),
          _description (std::move(mi._description))
    {}
    MemberInfo (Deserializer& in, int version)
    {
        Deserialize (in, version);
    }
	MemberInfo (std::string const & srcPath, File::Offset offset, int version);
	MemberInfo ()
	{}

	void Pad () { _description.Pad (); }

	void SetName (std::string const & name) { _description.SetName (name); }
	void SetHubId (std::string const & hubId) { _description.SetHubId (hubId); }
	void SetComment (std::string const & comment) { _description.SetComment (comment); }
	void SetLicense (std::string const & license) { _description.SetLicense (license); }

	MemberDescription const & Description () const { return _description; }
    std::string const & Name ()  const { return _description.GetName (); }
    std::string const & HubId () const { return _description.GetHubId (); }
    std::string const & Comment () const { return _description.GetComment (); }
    std::string const & License () const { return _description.GetLicense (); }
	std::string const & GetUserId () const { return _description.GetUserId (); }

	bool IsIdentical (MemberInfo const & info) const;

	void SetUserId (UserId id);

	// Serializable interface

    void Serialize (Serializer& out) const;
    void Deserialize (Deserializer& in, int version);

	void Dump (XML::Node * parent) const;
private:
    MemberDescription _description;
};

std::ostream& operator<<(std::ostream& os, MemberInfo const & info);

// Project Administrator election algorithm

template<class memberIter>
memberIter ElectAdmin (memberIter begin, memberIter end)
{
	GlobalId minVotingUserId = -1;
	// Find first voting member
	memberIter iter = begin;
	memberIter admin = begin;
	for (; iter != end; ++iter)
	{
		if (iter->IsVoting () && !iter->IsReceiver ())
		{
			minVotingUserId = iter->Id ();
			admin = iter;
			++iter;
			break;
		}
	}
	// Find the administrator
	while (iter != end)
	{
		if ((iter->Id () < minVotingUserId) && iter->IsVoting () && !iter->IsReceiver ())
		{
			admin = iter;
			minVotingUserId = iter->Id ();
		}
		++iter; 
	}
	if (minVotingUserId == -1)
		return end;
	else
		return admin;
}

#endif
