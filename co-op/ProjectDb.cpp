//------------------------------------
//  (c) Reliable Software, 1997 - 2007
//------------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "ProjectDb.h"
#include "Global.h"
#include "SysPath.h"
#include "License.h"
#include "Addressee.h"
#include "FeedbackMan.h"
#include "History.h"
#include "MemoryLog.h"
#include "TransportHeader.h"
#include "ProjectOptions.h"

#include <numeric>

//
// Project data stored in the database
//

namespace Project
{
	class Collector
	{
	public:
		virtual ~Collector () {}

		virtual void Store (MemberNote const * member) = 0;
	};

	class MemberSelector
	{
	public:
		MemberSelector (Collector & collector)
			: _collector (collector)
		{}
		virtual void operator() (MemberNote const * member) = 0;

		void Store (MemberNote const * member) { _collector.Store (member); }

	private:
		Collector &	_collector;
	};
}

using namespace Project;

void MemberNote::Serialize (Serializer& out) const
{
	Member::Serialize (out);
	_offset.Serialize (out);
}

void MemberNote::Deserialize (Deserializer& in, int version)
{
	Member::Deserialize (in, version);
	_offset.Deserialize (in, version);
}

char Db::_userLogName [] = "UserLog.bin";

void Db::VerifyLogs () const
{
	if (!_userLog.Exists ())
		throw Win::Exception ("Corrupted Database: missing system file.", _userLog.GetPath ());
}

void Db::CopyLog (FilePath const & destPath) const
{
	_userLog.Copy (destPath.GetFilePath (_userLogName));
}

void Db::InitPaths (SysPathFinder& pathFinder)
{
	_userLog.Init (pathFinder.GetSysFilePath (_userLogName));
}

void Db::XDefect ()
{
	_myId.XSet (_myId.GetDefaultValue ());
}

GlobalId Db::XMakeGlobalId (bool isForProjectRoot)
{
	UserId userId = _myId.XGet ();
	if (userId == _myId.GetDefaultValue ())
	{
		if (!isForProjectRoot)
		{
			Win::ClearError ();
			throw Win::Exception ("This operation cannot be performed before a full sync script is received.");
		}
		else
		{
			userId = UserId (0);	// Project root id is always equal 0-0
		}
	}
	if (_counter.XGet () == OrdinalInvalid)
	{
		Win::ClearError ();
		throw Win::Exception ("New file/folder global id exceeds global id limit (1048574). Please, contact support@relisoft.com");
	}
	GlobalIdPack gid (userId, _counter.XGet ());
	Assert ((userId != UserId (0) || gid.GetOrdinal () != 0) || isForProjectRoot);
	_counter.XSet (_counter.XGet () + 1);
	return gid;
}

GlobalId Db::XMakeScriptId ()
{
	if (_scriptCounter.XGet () == OrdinalInvalid)
	{
		Win::ClearError ();
		throw Win::Exception ("New script id exceeds script id limit (1048574). Please, contact support@relisoft.com");
	}
	GlobalIdPack gid (_myId.XGet (), _scriptCounter.XGet ());
	_scriptCounter.XSet (_scriptCounter.XGet () + 1);
	// Update our most recent script id
	XUpdateSender (gid);
	return gid;
}

GlobalId Db::GetNextScriptId () const
{
	if (_myId.Get () == _myId.GetDefaultValue () || _scriptCounter.Get () == OrdinalInvalid)
		return gidInvalid;
	GlobalIdPack gid (_myId.Get (), _scriptCounter.Get ());
	return gid;
}

// Functional
class IsLessId : public std::binary_function<MemberNote, MemberNote, bool>
{
public:
	bool operator() (MemberNote const * member1, MemberNote const * member2) const
	{
		return member1->Id () < member2->Id ();
	}
};

class IsAdmin : public std::unary_function<MemberNote, bool>
{
public:
	bool operator() (MemberNote const * member) const
	{
		return member->IsAdmin ();
	}
};

class AddMember: public std::binary_function<unsigned, MemberNote const *, unsigned>
{
public:
	unsigned operator () (unsigned val, MemberNote const * member)
	{
		if (!member->IsDead ())
			return val + 1;
		else 
			return val;
	}
};

unsigned Db::MemberCount () const
{
	return std::accumulate (_members.begin (), _members.end (), 0, AddMember ());
}

UserId Db::XMakeUserId ()
{
	if (_myId.XGet () == _myId.GetDefaultValue ())
	{
		Win::ClearError ();
		throw Win::Exception ("This operation cannot be performed before a full sync script is received.");
	}
	// Find the highest user id in use
	MemberIter userWithMaxId = std::max_element (_members.xbegin (), _members.xend (), IsLessId ());
	Assert (userWithMaxId != _members.xend ());
	UserId newUserId = (*userWithMaxId)->Id () + 1;
	if (!IsValidUid (newUserId))
	{
		Win::ClearError ();
		throw Win::Exception ("New project member id exceeds the user id limit (4094). Please, contact support@relisoft.com");
	}
	return newUserId;
}

UserId Db::XGetAdminId () const
{
	MemberIter admin = std::find_if (_members.xbegin (), _members.xend (), IsAdmin ());
	if (admin != _members.xend ())
		return (*admin)->Id ();
	else
		return gidInvalid;
}

UserId Db::GetAdminId () const
{
	MemberIter admin = std::find_if (_members.begin (), _members.end (), IsAdmin ());
	if (admin != _members.end ())
		return (*admin)->Id ();
	else
		return gidInvalid;
}

GlobalId Db::RandomId () const
{
	return ::RandomId ();
}

GlobalId Db::RandomId (UserId userId) const
{
	return ::RandomId (userId);
}

void Db::XUpdateScriptCounter (GlobalId gid)
{
	// Script counter always contains next free script number
	GlobalIdPack pack (gid);
	if (pack.GetOrdinal () >= _scriptCounter.XGet ())
	{
		Assert (pack.GetOrdinal () + 1 < OrdinalInvalid);
		_scriptCounter.XSet (pack.GetOrdinal () + 1);
	}
}

void Db::XUpdateFileCounter (GlobalId gid)
{
	// File counter always contains next free file number
	GlobalIdPack pack (gid);
	if (pack.GetOrdinal () >= _counter.XGet ())
	{
		Assert (pack.GetOrdinal () + 1 < OrdinalInvalid);
		_counter.XSet (pack.GetOrdinal () + 1);
	}
}

void Db::XSetCopyright (std::string const & copyright)
{
	_copyright.XSet (copyright);
}

void Db::XSetProjectName (std::string const & projectName)
{
	_projectName.XSet (projectName);
}

bool Db::AddSender (TransportHeader & txHdr) const
{
	UserId myUserId = GetMyId ();
	Assert (myUserId != gidInvalid);
	std::unique_ptr<MemberDescription> sender = RetrieveMemberDescription (myUserId);
	Address senderAddress (sender->GetHubId (),
						   ProjectName (),
						   sender->GetUserId ());
	txHdr.AddSender (senderAddress);
	MemberState state = GetMemberState (myUserId);
	return state.IsDistributor ();
}

bool Db::XAddSender (TransportHeader & txHdr) const
{
	UserId myUserId = XGetMyId ();
	Assert (myUserId != gidInvalid);
	std::unique_ptr<MemberDescription> sender = XRetrieveMemberDescription (myUserId);
	Address senderAddress (sender->GetHubId (),
						   XProjectName (),
						   sender->GetUserId ());
	txHdr.AddSender (senderAddress);
	MemberState state = XGetMemberState (myUserId);
	return state.IsDistributor ();
}

class AllMemberSelector : public MemberSelector
{
public:
	AllMemberSelector (Collector & collector)
		: MemberSelector (collector)
	{}
	void operator () (MemberNote const * member)
	{
		MemberState const state = member->State ();
		if (!state.IsDead ())
			Store (member);
	}
};

class HistoricalMemberSelector : public MemberSelector
{
public:
	HistoricalMemberSelector (Collector & collector, GidSet const & filterOut = GidSet ())
		: MemberSelector (collector), _filterOut (filterOut)
	{}
	void operator () (MemberNote const * member)
	{
		if (!_filterOut.empty ())
		{
			if (_filterOut.find (member->Id ()) != _filterOut.end ())
				return;	// Filtered out -- don't store
		}
		Store (member);
	}
private:
	GidSet const & _filterOut;
};

class DeadMemberSelector : public MemberSelector
{
public:
	DeadMemberSelector (Collector & collector)
		: MemberSelector (collector)
	{}
	void operator () (MemberNote const * member)
	{
		MemberState const state = member->State ();
		if (state.IsDead ())
			Store (member);
	}
};

class ReceiverSelector : public MemberSelector
{
public:
	ReceiverSelector (Collector & collector)
		: MemberSelector (collector)
	{}
	void operator () (MemberNote const * member)
	{
		if (member->IsReceiver ())
			Store (member);
	}
};

class BroadcastMemberSelector : public MemberSelector
{
public:
	BroadcastMemberSelector (UserId myUserId, Collector & collector)
		: MemberSelector (collector),
		  _myUserId (myUserId)
	{}
	void operator () (MemberNote const * member)
	{
		MemberState const state = member->State ();
		UserId memberId = member->Id ();
		if (!state.IsDead () && memberId != _myUserId)
		{
			// If not defected and not this user
			Store (member);
		}
	}
private:
	UserId		_myUserId;
};

class MulticastMemberSelector : public MemberSelector
{
public:
	MulticastMemberSelector (GidSet const & filterOut, Collector & collector)
		: MemberSelector (collector),
		  _filterOut (filterOut)
	{}
	void operator () (MemberNote const * member)
	{
		MemberState const state = member->State ();
		UserId memberId = member->Id ();
		if (!state.IsDead () && _filterOut.find (memberId) == _filterOut.end ())
		{
			// If not defected and not in the filterOut list
			Store (member);
		}
	}
private:
	GidSet const & _filterOut;
};

class AckMemberSelector : public MemberSelector
{
public:
	AckMemberSelector (UserId myUserId, UserId skipUserId, Collector & collector)
		: MemberSelector (collector),
		  _myUserId (myUserId),
		  _skipUserId (skipUserId)
	{}
	void operator () (MemberNote const * member)
	{
		MemberState const state = member->State ();
		UserId memberId = member->Id ();
		if (state.IsVotingMember () && memberId != _myUserId && memberId != _skipUserId)
		{
			// Is voting member and not this user
			Store (member);
		}
	}
private:
	UserId	_myUserId;
	UserId	_skipUserId;
};

class ResendCandidateSelector : public MemberSelector
{
public:
	ResendCandidateSelector (UserId myUserId, Collector & collector)
		: MemberSelector (collector),
		  _myUserId (myUserId)
	{}
	void operator () (MemberNote const * member)
	{
		MemberState const state = member->State ();
		UserId memberId = member->Id ();
		if (!state.IsDead () && !state.IsReceiver () && memberId != _myUserId)
		{
			// Not dead and not receiver and not me - can be a resend request candidate
			Store (member);
		}
	}
private:
	UserId	_myUserId;
};

class GidCollector : public Collector
{
public:
	GidCollector (GidList & gidList)
		: _gidList (gidList)
	{}
	void Store (MemberNote const * member)
	{
		_gidList.push_back (member->Id ());
	}
private:
	GidList &	_gidList;
};

class GidSetCollector : public Collector
{
public:
	GidSetCollector (GidSet & gidSet)
		: _gidSet (gidSet)
	{}
	void Store (MemberNote const * member)
	{
		_gidSet.insert (member->Id ());
	}
private:
	GidSet &	_gidSet;
};

void Db::XGetResendCandidates (GidList & memberIds) const
{
	GidCollector collectGids (memberIds);
	ResendCandidateSelector selectResendCandidates (_myId.XGet (), collectGids);
	XGetMemberList (selectResendCandidates);
}

void Db::XGetMemberList (Project::MemberSelector & selectMembers) const
{
	// Revisit: Compiler bug? This line produces no code!!!
	//std::for_each (_members.xbegin (), _members.xend (), selectMembers);
	TransactableArray<MemberNote>::const_iterator it = _members.xbegin ();
	for (; it != _members.xend (); ++it)
	{
		MemberNote const * note = *it;
		selectMembers (note);
	}
}

void Db::GetMemberList (Project::MemberSelector & selectMembers) const
{
	// Revisit: Compiler bug? This line produces no code!!!
	//std::for_each (_members.begin (), _members.end (), selectMembers);
	TransactableArray<MemberNote>::const_iterator it = _members.begin ();
	for (; it != _members.end (); ++it)
	{
		MemberNote const * note = *it;
		selectMembers (note);
	}
}

void Db::XGetAllMemberList (GidList & memberList) const
{
	GidCollector collectGids (memberList);
	BroadcastMemberSelector selectMembers (_myId.XGet (), collectGids);
	XGetMemberList (selectMembers); 
}

void Db::GetAllMemberList (GidList & memberList) const
{
	GidCollector collectGids (memberList);
	BroadcastMemberSelector selectMembers (_myId.Get (), collectGids);
	GetMemberList (selectMembers);
}

void Db::XGetMulticastList (GidSet const & filterOut, GidList & memberList) const
{
	GidCollector collectGids (memberList);
	MulticastMemberSelector selectMulticastMembers (filterOut, collectGids);
	XGetMemberList (selectMulticastMembers);
}

void Db::GetHistoricalMemberList (GidList & memberList, GidSet const & skipMembers) const
{
	GidCollector collectGids (memberList);
	HistoricalMemberSelector selectMembers (collectGids, skipMembers);
	GetMemberList (selectMembers);
}

void Db::GetDeadMemberList (GidList & memberList) const
{
	GidCollector collectGids (memberList);
	DeadMemberSelector selectMembers (collectGids);
	GetMemberList (selectMembers);
}

void Db::XGetDeadMemberList (GidList & memberList) const
{
	GidCollector collectGids (memberList);
	DeadMemberSelector selectMembers (collectGids);
	XGetMemberList (selectMembers);
}

void Db::GetReceivers (GidSet & receivers) const
{
	GidSetCollector collectGids (receivers);
	ReceiverSelector selectMembers (collectGids);
	GetMemberList (selectMembers);
}

void Db::XGetReceivers (GidSet & receivers) const
{
	GidSetCollector collectGids (receivers);
	ReceiverSelector selectMembers (collectGids);
	XGetMemberList (selectMembers);
}

void Db::XGetHistoricalMemberList (GidList & memberList) const
{
	GidCollector collectGids (memberList);
	HistoricalMemberSelector selectMembers (collectGids);
	XGetMemberList (selectMembers);
}

void Db::XGetVotingList (GidList & memberList, GlobalId skipUserId) const
{
	GidCollector collectGids (memberList);
	AckMemberSelector selectMembers (_myId.XGet (), skipUserId, collectGids);
	XGetMemberList (selectMembers);
}

bool Db::XScriptNeeded (bool isMemberChange) const
{
	MemberState thisMemberState = XGetMemberState (_myId.XGet ());
	if (thisMemberState.IsVotingMember () || isMemberChange)
	{
		GidList broadcast;
		XGetAllMemberList (broadcast);
		return broadcast.size () != 0;
	}

	return false;
}

// Functional
class IsVotingMember : public std::unary_function<MemberNote, bool>
{
public:
	bool operator() (MemberNote const * member) const
	{
		return member->State ().IsVotingMember ();
	}
};

// Functional
class IsEqualId : public std::unary_function<MemberNote, bool>
{
public:
	explicit IsEqualId (UserId id) : _id (id) { Assert (IsValidUid (id)); }
	bool operator() (MemberNote const * member) const
	{
		Assert (IsValidUid (member->Id ()));
		return member->Id () == _id;
	}
private:
	UserId	_id;
};

bool Db::IsProjectMember (UserId userId) const
{
	dbg << "IsProjectMember: " << std::hex << userId << std::endl;
	MemberIter iter = std::find_if (_members.begin (), _members.end (), IsEqualId (userId));
	return iter != _members.end ();
}

bool Db::XIsProjectMember (UserId userId) const
{
	dbg << "XIsProjectMember: " << std::hex << userId << std::endl;
	MemberIter iter = std::find_if (_members.xbegin (), _members.xend (), IsEqualId (userId));
	return iter != _members.xend ();
}

Tri::State Db::XIsPrehistoric (GlobalId scriptId) const
{
	if (_myId.XGet () == gidInvalid)
		return Tri::Maybe;

	GlobalIdPack pack (scriptId);
	dbg << "XIsPrehistoric: " << std::hex << pack.GetUserId () << std::endl;
	MemberIter iter = std::find_if (_members.xbegin (), _members.xend (), IsEqualId (pack.GetUserId ()));
	if (iter != _members.xend ())
	{
		MemberNote const * member = *iter;
		return member->IsPrehistoricScript (scriptId);
	}
	return Tri::Maybe;
}

Tri::State Db::XIsFromFuture (GlobalId scriptId) const
{
	if (_myId.XGet () == gidInvalid)
		return Tri::Maybe;

	GlobalIdPack pack (scriptId);
	dbg << "XIsFromFuture: " << std::hex << pack.GetUserId () << std::endl;
	MemberIter iter = std::find_if (_members.xbegin (), _members.xend (), IsEqualId (pack.GetUserId ()));
	if (iter != _members.xend ())
	{
		MemberNote const * member = *iter;
		return member->IsFutureScript (scriptId);
	}
	return Tri::Maybe;
}

GlobalId Db::GetMostRecentScriptId (UserId userId) const
{
	dbg << "GetMostRecentScriptId: " << std::hex << userId << std::endl;
	MemberIter iter = std::find_if (_members.begin (), _members.end (), IsEqualId (userId));
	Assume (iter != _members.end (), "Unknown member ID");
	MemberNote const * member = *iter;
	return member->GetMostRecentScript ();
}

GlobalId Db::GetPreHistoricScriptId (UserId userId) const
{
	dbg << "GetPreHistoricScriptId: " << std::hex << userId << std::endl;
	MemberIter iter = std::find_if (_members.begin (), _members.end (), IsEqualId (userId));
	Assume (iter != _members.end (), "Unknown member ID");
	MemberNote const * member = *iter;
	return member->GetPreHistoricScript ();
}

void Db::XUpdateSender (GlobalId scriptId)
{
	Assert (scriptId != gidInvalid);
	GlobalIdPack pack (scriptId);
	if (!pack.IsFromJoiningUser ())
	{
		UserId userId = pack.GetUserId ();
		dbg << "XUpdateSender: " << std::hex << userId << std::endl;
		MemberIter iter = std::find_if (_members.xbegin (), _members.xend (), IsEqualId (userId));
		if (iter != _members.xend ())
		{
			MemberNote * member = _members.XGetEdit (_members.XtoIndex (iter));
			member->SetMostRecentScript (scriptId);
		}
	}
}

bool Db::IsProjectAdmin () const
{
	UserId myId = GetMyId ();
	return myId != gidInvalid && myId == GetAdminId ();
}

bool Db::XIsProjectAdmin () const
{
	UserId myId = XGetMyId ();
	return myId != gidInvalid && myId == XGetAdminId ();
}

bool Db::XIsReceiver (UserId userId) const
{
	MemberState state = XGetMemberState (userId);
	return state.IsReceiver ();
}

MemberState Db::XGetMemberState (UserId userId) const
{
	dbg << "XGetMemberState: " << std::hex << userId << std::endl;
	MemberIter iter = std::find_if (_members.xbegin (), _members.xend (), IsEqualId (userId));
	Assume (iter != _members.xend (), "Unknown user ID");
	return (*iter)->State ();
}

MemberState const Db::GetMemberState (UserId userId) const
{
	dbg << "GetMemberState: " << std::hex << userId << std::endl;
	MemberIter iter = std::find_if (_members.begin (), _members.end (), IsEqualId (userId));
	Assume (iter != _members.end (), "Unknown user ID");
	return (*iter)->State ();
}

std::unique_ptr<MemberInfo> Db::RetrieveMemberInfo (UserId userId) const
{
	dbg << "RetrieveMemberInfo: " << std::hex << userId << std::endl;
	MemberIter iter = std::find_if (_members.begin (), _members.end (), IsEqualId (userId));
	Assume (iter != _members.end (), "Unknown user ID");
	MemberNote const * memberNote = *iter;
	std::unique_ptr<MemberDescription> description 
		= _userLog.Retrieve (memberNote->GetOffset (), VersionNo ());
	std::unique_ptr<MemberInfo> info (new MemberInfo (*memberNote, *description));
	return info;
}

std::unique_ptr<MemberInfo> Db::XRetrieveMemberInfo (UserId userId) const
{
	dbg << "XRetrieveMemberInfo: " << std::hex << userId << std::endl;
	MemberIter iter = std::find_if (_members.xbegin (), _members.xend (), IsEqualId (userId));
	Assume (iter != _members.xend (), "Unknown user ID");
	MemberNote const * memberNote = *iter;
	std::unique_ptr<MemberDescription> description 
		= _userLog.Retrieve (memberNote->GetOffset (), VersionNo ());
	std::unique_ptr<MemberInfo> info (new MemberInfo (*memberNote, *description));
	return info;
}

std::unique_ptr<MemberDescription> Db::RetrieveMemberDescription (UserId userId) const
{
	Assert (userId != gidInvalid);
	if (GetMyId () == gidInvalid)
	{
		// This user awaits full sync.  The only project member that
		// can send him a script is project administrator.
		std::unique_ptr<MemberDescription> admin (new MemberDescription (AdminName, 
																	   std::string (), 
																	   std::string (), 
																	   std::string (), 
																	   std::string ()));
		return admin;
	}
	dbg << "RetrieveMemberDescription: " << std::hex << userId << std::endl;
	MemberIter iter = std::find_if (_members.begin (), _members.end (), IsEqualId (userId));
	if (iter == _members.end ())
	{
		// Unknown project member, either defected and removed from user log or
		// not seen yet, because of missing new user announcement.
		std::unique_ptr<MemberDescription> defectedMember (new MemberDescription (UnknownName, 
																				std::string (), 
																				std::string (), 
																				std::string (), 
																				std::string ()));
		return defectedMember;
	}
	return _userLog.Retrieve ((*iter)->GetOffset (), VersionNo ());
}

std::unique_ptr<MemberDescription> Db::XRetrieveMemberDescription (UserId userId) const
{
	Assert (userId != gidInvalid);
	dbg << "XRetrieveMemberDescription: " << std::hex << userId << std::endl;
	MemberIter iter = std::find_if (_members.xbegin (), _members.xend (), IsEqualId (userId));
	if (iter == _members.xend ())
	{
		std::string info ("User id = ");
		info += ToHexString (userId);
		throw Win::Exception ("Corrupted database: missing project member description.", info.c_str ());
	}
	return _userLog.Retrieve ((*iter)->GetOffset (), VersionNo ());
}

void Db::XPrepareForBranch (std::string const & branchProjectName, Project::Options const & options)
{
	XSetProjectName (branchProjectName);
	// Defect all current project members except this user
	for (MemberIter iter = _members.xbegin (); iter != _members.xend (); ++iter)
	{
		MemberNote * note = *iter;
		if (note->Id () != XGetMyId () && !note->State ().IsDead ())
		{
			note->Defect (false);	// Removed by administrator
		}
	}
	// Set branch project options
	XInitProjectProperties (options);
}

void Db::XInitProjectProperties (Project::Options const & options)
{
	_properties.XSet (AutoSynch, options.IsAutoSynch ());
	_properties.XSet (AutoJoin, options.IsAutoJoin ());
	_properties.XSet (AutoFullSynch, options.IsAutoFullSynch ());
	_properties.XSet (KeepCheckedOut, options.IsKeepCheckedOut ());
	_properties.XSet (AllBcc, options.UseBccRecipients ());
}

// Returns true if member added to or updated in the database
bool Db::XAddMember (MemberInfo const & newMemberInfo)
{
	UserId newMemberId = newMemberInfo.Id ();
	dbg << "XAddMember: " << std::hex << newMemberId << std::endl;
	MemberIter iter = std::find_if (_members.xbegin (), _members.xend (), IsEqualId (newMemberId));
	if (iter != _members.xend ())
	{
		std::unique_ptr<MemberInfo> currentInfo = XRetrieveMemberInfo (newMemberId);
		if (currentInfo->IsIdentical (newMemberInfo))
			return false;	// Duplicate project member description

		XUpdate (newMemberInfo, true); // overwrite
		return true;
	}
	else if (_myId.XGet () != gidInvalid)
	{
		// Added member is not recorded yet and I'm not awaiting the full sync
		MemberState myState = XGetMemberState (_myId.XGet ());
		if (myState.IsReceiver ())
		{
			// I'm a receiver
			if (newMemberInfo.State ().IsReceiver ())
			{
				Assert (!"I'm receiver and I'm trying to add another receiver!");
				return false; // I'm receiver and I'm trying to add another receiver -- ignore add member
			}
		}
	}

	if (newMemberInfo.State ().IsAdmin ())
		XClearAdmin ();

	std::unique_ptr<MemberNote> newUser (new MemberNote (newMemberInfo, _lastUserOffset.XGet ()));
	_lastUserOffset.XSet (_userLog.Append (_lastUserOffset.XGet (), newMemberInfo.Description ()));
	_members.XAppend (std::move(newUser));

	if (_myId.XGet () == gidInvalid)
	{
		// First user added to the database is always
		// this project enlistment owner.
		Assert (_members.XCount () == 1);
		_myId.XSet (newMemberId);
	}
	return true;
}

void Db::XReplaceDescription (UserId userId, MemberDescription const & newDescription)
{
	dbg << "XReplaceDescription: " << std::hex << userId << std::endl;
	MemberIter iter = std::find_if (_members.xbegin (), _members.xend (), IsEqualId (userId));
	if (iter != _members.xend ())
	{
		MemberNote * member = _members.XGetEdit (_members.XtoIndex (iter));
		member->SetOffset (_lastUserOffset.XGet ());
		_lastUserOffset.XSet (_userLog.Append (_lastUserOffset.XGet (), newDescription));
	}
}

void Db::XUpdate (MemberInfo const & update, bool overwrite)
{
	if (update.State ().IsAdmin ())
		XClearAdmin ();

	dbg << "XUpdate: " << std::hex << update.Id () << std::endl;
	MemberIter iter = std::find_if (_members.xbegin (), _members.xend (), IsEqualId (update.Id ()));
	if (iter != _members.xend ())
	{
		MemberNote * member = _members.XGetEdit (_members.XtoIndex (iter));
		if (!member->State ().IsDead ())
			member->SetState (update.State ());
		if (overwrite)
		{
			member->SetPreHistoricScript (update.GetPreHistoricScript ());
			member->SetMostRecentScript (update.GetMostRecentScript ());
		}
		std::unique_ptr<MemberDescription> currentDescription = _userLog.Retrieve (member->GetOffset (), VersionNo ());
		if (!currentDescription->IsEqual (update.Description ()))
		{
			// Update member description
			member->SetOffset (_lastUserOffset.XGet ());
			_lastUserOffset.XSet (_userLog.Append (_lastUserOffset.XGet (), update.Description ()));
		}
	}
	else
	{
		// Can happen when we receive re-send New user announcement
		// which is send out as membership update
		XAddMember (update);
	}
}

void Db::XClearAdmin ()
{
	for (unsigned int i = 0; i < _members.XCount (); ++i)
	{
		MemberNote const * note = _members.XGet (i);
		if (note != 0)
		{
			MemberState state = note->State ();
			if (state.IsAdmin () && !state.IsVerified ())
			{
				// Clear un-verified member administrative privilege
				StateVotingMember newState (state);
				MemberNote * editNote = _members.XGetEdit (i);
				editNote->SetState (newState);
			}
		}
	}
}

void Db::XChangeState (UserId userId, MemberState newState)
{
	dbg << "XChangeState: " << std::hex << userId << std::endl;
	MemberIter iter = std::find_if (_members.xbegin (), _members.xend (), IsEqualId (userId));
	if (iter != _members.xend ())
	{
		MemberNote * member = _members.XGetEdit (_members.XtoIndex (iter));
		member->SetState (newState);
	}
}

void Db::XMemberDefect (UserId userId)
{
	dbg << "XMemberDefect: " << std::hex << userId << std::endl;
	MemberIter iter = std::find_if (_members.xbegin (), _members.xend (), IsEqualId (userId));
	if (iter != _members.xend ())
	{
		MemberNote * member = _members.XGetEdit (_members.XtoIndex (iter));
		member->Defect (true);	// Voluntary defect
	}
}

class InfoCollector : public Project::Collector
{
public:
	InfoCollector (std::vector<MemberInfo> & memberList, Log<MemberDescription> const & log, int logVersion)
		: _infoList (memberList),
		  _log (log),
		  _version (logVersion)
	{}
	void Store (MemberNote const * member)
	{
		std::unique_ptr<MemberDescription> description 
			= _log.Retrieve (member->GetOffset (), _version);
        _infoList.emplace_back(MemberInfo(*member, *description));
	}
private:
	std::vector<MemberInfo> 	 &	_infoList;
	Log<MemberDescription> const &	_log;
	int 							_version;
};

std::vector<MemberInfo> Db::RetrieveVotingMemberList () const
{
    std::vector<MemberInfo>  memberList;
	InfoCollector collectMemberInfo (memberList, _userLog, VersionNo ());
	AckMemberSelector selectVotingMembers (_myId.Get (),	// Exclude me
										   gidInvalid,		// and don't exclude anyone else
										   collectMemberInfo);
	GetMemberList (selectVotingMembers);
    return memberList;
}

std::vector<MemberInfo> Db::RetrieveMemberList () const
{
    std::vector<MemberInfo> memberList;
	InfoCollector collectMemberInfo (memberList, _userLog, VersionNo ());
	AllMemberSelector selectAllMembers (collectMemberInfo);
	GetMemberList (selectAllMembers);
    return memberList;
}

std::vector<MemberInfo> Db::XRetrieveMemberList () const
{
    std::vector<MemberInfo> memberList;
	InfoCollector collectMemberInfo (memberList, _userLog, VersionNo ());
	AllMemberSelector selectAllMembers (collectMemberInfo);
	XGetMemberList (selectAllMembers);
    return memberList;
}

std::vector<MemberInfo> Db::RetrieveHistoricalMemberList () const
{
    std::vector<MemberInfo> memberList;
	InfoCollector collectMemberInfo (memberList, _userLog, VersionNo ());
	HistoricalMemberSelector selectMembers (collectMemberInfo);
	GetMemberList (selectMembers);
    return memberList;
}

std::vector<MemberInfo> Db::RetrieveBroadcastList () const
{
    std::vector<MemberInfo> memberList;
	InfoCollector collectMemberInfo (memberList, _userLog, VersionNo ());
	BroadcastMemberSelector selectBroadcastMembers (_myId.Get (), collectMemberInfo);
	GetMemberList (selectBroadcastMembers);
    return memberList;
}

class AddresseeCollector : public Collector
{
public:
	AddresseeCollector (AddresseeList & addresseeList, Log<MemberDescription> const & log, int logVersion)
		: _addresseeList (addresseeList),
		  _log (log),
		  _version (logVersion)
	{}
	void Store (MemberNote const * member)
	{
		std::unique_ptr<MemberDescription> description 
			= _log.Retrieve (member->GetOffset (), _version);
		Addressee addressee (description->GetHubId (), description->GetUserId ());
		_addresseeList.push_back (addressee);
	}
private:
	AddresseeList & 				_addresseeList;
	Log<MemberDescription> const &	_log;
	int 							_version;
};

void Db::XRetrieveBroadcastRecipients (AddresseeList & addresseeList) const
{
	AddresseeCollector collectAddressees (addresseeList, _userLog, VersionNo ());
	BroadcastMemberSelector selectBroadcastMembers (_myId.XGet (), collectAddressees);
	XGetMemberList (selectBroadcastMembers);
}

void Db::RetrieveBroadcastRecipients (AddresseeList & addresseeList) const
{
	AddresseeCollector collectAddressees (addresseeList, _userLog, VersionNo ());
	BroadcastMemberSelector selectBroadcastMembers (_myId.Get (), collectAddressees);
	GetMemberList (selectBroadcastMembers);
}

void Db::XRetrieveMulticastRecipients (GidSet const & filterOut,
									   AddresseeList & addresseeList) const
{
	AddresseeCollector collectAddressees (addresseeList, _userLog, VersionNo ());
	MulticastMemberSelector selectMulticastMembers (filterOut, collectAddressees);
	XGetMemberList (selectMulticastMembers);
}

// Conversion from version 4.2 to 4.5
void Db::XSetOldestAndMostRecentScriptIds (History::Db & history, Progress::Meter * meter, MemoryLog & log)
{
	log << "Setting pre-historic and most recent script ids." << std::endl;
	meter->SetActivity ("Converting project membership database");
	meter->SetRange (0, 3 * _members.XCount () + 2, 1);
	meter->StepIt ();
	// For every project member, set both the oldest and the most recent script ids to gidInvalid
	std::map<UserId, std::pair<GlobalId, GlobalId> > userScripts;
	for (unsigned int i = 0; i < _members.XCount (); i++)
	{
		MemberNote const * member = _members.XGet (i);
		GlobalId userId = member->Id ();
		userScripts [userId] = std::make_pair (gidInvalid, gidInvalid);
	}
	history.XCollectScriptIds (userScripts, meter, log);
	// Remember collected script ids
	for (unsigned int i = 0; i < _members.XCount (); i++)
	{
		MemberNote * member = _members.XGetEdit (i);
		UserId userId = member->Id ();
		std::pair <GlobalId, GlobalId> scripts = userScripts [userId];
		member->SetMostRecentScript (scripts.first);
		GlobalId oldestScriptId = scripts.second;
		GlobalId preHistoricId; 
		if (oldestScriptId == gidInvalid)
		{
			// No scripts from that user found
			Assert (scripts.first == gidInvalid);
			preHistoricId = gidInvalid;
		}
		else
		{
			int oldestScriptOrdinal = GlobalIdPack (oldestScriptId).GetOrdinal ();
			if (oldestScriptOrdinal == 0)
			{
				// If we have script with ordinal 0 from this user, then we have all his scripts, 
				// Therefore there are no prehistoric scripts
				preHistoricId = gidInvalid;	
			}
			else
				preHistoricId = GlobalIdPack (userId, oldestScriptOrdinal - 1);
		}
		member->SetPreHistoricScript (preHistoricId);
		log << "Member id: " << std::hex << member->Id () << " pre-historic script id = ";
		GlobalIdPack pack1 (member->GetPreHistoricScript ());
		log << pack1.ToBracketedString () << "; most recent script id: ";
		GlobalIdPack pack2 (member->GetMostRecentScript ());
		log << pack2.ToBracketedString () << std::endl;
		meter->StepIt ();
	}
	log << "Done with setting pre-historic and most recent script ids." << std::endl;
}

void Db::Clear () throw ()
{
	TransactableContainer::Clear ();
	_userLog.Clear ();
}

void Db::Serialize (Serializer& out) const
{
	_projectName.Serialize (out);
	_copyright.Serialize (out);
	_myId.Serialize (out);
	_counter.Serialize (out);
	_scriptCounter.Serialize (out);
	_lastUserOffset.Serialize (out);
	_members.Serialize (out);
	_properties.Serialize (out);
}

void Db::Deserialize (Deserializer& in, int version)
{
	CheckVersion (version, VersionNo ());
	_projectName.Deserialize (in, version);
	_copyright.Deserialize (in, version);
	_myId.Deserialize (in, version);
	_counter.Deserialize (in, version);
	_scriptCounter.Deserialize (in, version);
	_lastUserOffset.Deserialize (in, version);
	_members.Deserialize (in, version);
	_properties.Deserialize (in, version);
}
