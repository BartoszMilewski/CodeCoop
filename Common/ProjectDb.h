#if !defined (PROJECTDB_H)
#define PROJECTDB_H
//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "Transact.h"
#include "GlobalId.h"
#include "MemberInfo.h"
#include "XString.h"
#include "XArray.h"
#include "XBitSet.h"
#include "XLong.h"
#include "XFileOffset.h"
#include "Params.h"

#include <TriState.h>

class AddresseeList;
class FilePath;
namespace Progress { class Meter; }
class MemoryLog;
class TransportHeader;
namespace History
{
	class Db;
}

namespace Project
{
	class Options;
	class Collector;
	class MemberSelector;

	class MemberNote : public Member
	{
	public:
		MemberNote (MemberInfo const & member, File::Offset offset)
			: Member (member),
			 _offset (offset)
		{}
		MemberNote (Deserializer& in, int version)
		{
			Deserialize (in, version);
		}

		File::Offset GetOffset () const { return _offset; }

		void SetOffset (File::Offset offset)  { _offset = offset; }

		// Serializable interface

		void Serialize (Serializer& out) const;
		void Deserialize (Deserializer& in, int version);

	private:
		SerFileOffset	_offset;
	};

	class Db : public TransactableContainer
	{
		friend class DataBase;
		friend class HelpAboutData;

	public:
		Db ()
		{
			AddTransactableMember (_copyright);
			AddTransactableMember (_projectName);
			AddTransactableMember (_myId);
			AddTransactableMember (_counter);
			AddTransactableMember (_scriptCounter);
			AddTransactableMember (_lastUserOffset);
			AddTransactableMember (_members);
			AddTransactableMember (_properties);
		}

		GlobalId    XMakeGlobalId (bool isForProjectRoot = false);
		GlobalId    XMakeScriptId ();
		GlobalId	GetNextScriptId () const;
		UserId      XMakeUserId ();
		UserId      GetAdminId () const;
		UserId      XGetAdminId () const;
		UserId      GetMyId () const { return UserId (_myId.Get ()); }
		UserId      XGetMyId () const { return UserId (_myId.XGet ()); }
		GlobalId    RandomId () const;
		GlobalId    RandomId (UserId userId) const;
		GlobalId	GetMostRecentScriptId (UserId userId) const;
		GlobalId	GetPreHistoricScriptId (UserId userId) const;
		void		XUpdateScriptCounter (GlobalId gid);
		void		XUpdateFileCounter (GlobalId gid);
		bool		AddSender (TransportHeader & txHdr) const;
		bool		XAddSender (TransportHeader & txHdr) const;

		void		VerifyLogs () const;
		void		CopyLog (FilePath const & destPath) const;
		void        InitPaths (SysPathFinder& pathFinder);
		void		XDefect ();

		std::string const & GetCopyright () const { return _copyright; }
		std::string const & ProjectName () const { return _projectName; }
		std::string const & XProjectName () const { return _projectName.XGet (); }
		unsigned	MemberCount () const;
		bool		IsProjectMember (UserId userId) const;
		bool		XIsProjectMember (UserId userId) const;
		bool		IsProjectAdmin () const;
		bool		XIsProjectAdmin () const;
		bool		XIsReceiver (UserId userId) const;
		bool        XScriptNeeded (bool isMemberChange) const;
		Tri::State	XIsPrehistoric (GlobalId scriptId) const;
		Tri::State	XIsFromFuture (GlobalId scriptId) const;
		void		XUpdateSender (GlobalId scriptId);
		void        XGetAllMemberList (GidList & memberList) const;
		void        GetAllMemberList (GidList & memberList) const;
		void		XGetMulticastList (GidSet const & filterOut, GidList & memberList) const;
		void		GetHistoricalMemberList (GidList & memberList, GidSet const & skipMembers = GidSet ()) const;
		void		XGetHistoricalMemberList (GidList & memberList) const;
		void		GetDeadMemberList (GidList & memberList) const;
		void		XGetDeadMemberList (GidList & memberList) const;
		void		GetReceivers (GidSet & receivers) const;
		void		XGetReceivers (GidSet & receivers) const;
		void        XGetVotingList (GidList & memberList, UserId skipUserId = gidInvalid) const;
		void        XSetCopyright (std::string const & copyright);
		void        XSetProjectName (std::string const & projectName);

		bool		XAddMember (MemberInfo const & member);
		void		XReplaceDescription (UserId userId, MemberDescription const & newDescription);
		void        XUpdate (MemberInfo const & update, bool overwrite = false);
		void		XChangeState (UserId userId, MemberState newState);
		void        XMemberDefect (UserId userId);
		MemberState XGetMemberState (UserId userId) const;
		MemberState const GetMemberState (UserId userId) const;
		std::unique_ptr<MemberDescription> RetrieveMemberDescription (UserId userId) const;
		std::unique_ptr<MemberDescription> XRetrieveMemberDescription (UserId userId) const;
		std::vector<MemberInfo> RetrieveVotingMemberList () const;
		std::vector<MemberInfo> RetrieveMemberList () const;
		std::vector<MemberInfo> XRetrieveMemberList () const;
		void		XGetResendCandidates (GidList & memberIds) const;
		std::vector<MemberInfo> RetrieveHistoricalMemberList () const;
		std::vector<MemberInfo> RetrieveBroadcastList () const;
		void		XRetrieveBroadcastRecipients (AddresseeList & recipients) const;
		void		RetrieveBroadcastRecipients (AddresseeList & recipients) const;
		void		XRetrieveMulticastRecipients (GidSet const & filterOut,
												  AddresseeList & recipients) const;
		std::unique_ptr<MemberInfo> RetrieveMemberInfo (UserId userId) const;
		std::unique_ptr<MemberInfo> XRetrieveMemberInfo (UserId userId) const;
		void		XPrepareForBranch (std::string const & branchProjectName, Project::Options const & options);

		// Project options
		bool		IsAutoSynch () const { return _properties.Test (AutoSynch); }
		bool		IsAutoJoin () const { return _properties.Test (AutoJoin); }
		bool		IsKeepCheckedOut () const { return _properties.Test (KeepCheckedOut); }
		bool		IsAutoFullSynch () const { return _properties.Test (AutoFullSynch); }
		bool		XIsAutoFullSynch () const { return _properties.XTest (AutoFullSynch); }
		bool		UseBccRecipients () const { return _properties.Test (AllBcc); }
		bool		XUseBccRecipients () const { return _properties.XTest (AllBcc); }
		void		XSetAutoSynch (bool flag) { _properties.XSet (AutoSynch, flag); }
		void		XSetAutoJoin (bool flag) { _properties.XSet (AutoJoin, flag); }
		void		XSetKeepCheckedOut (bool flag) { _properties.XSet (KeepCheckedOut, flag); }
		void		XSetAutoFullSynch (bool flag) { _properties.XSet (AutoFullSynch, flag); }
		void		XSetBccRecipients (bool flag) { _properties.XSet (AllBcc, flag); }
		void		XInitProjectProperties (Project::Options const & options);

		// Conversion from version 4.2 to 4.5
		void		XSetOldestAndMostRecentScriptIds (History::Db & history, Progress::Meter * meter, MemoryLog & log);

		// Transactable interface
		void		Clear () throw ();

		void        Serialize (Serializer& out) const;
		void        Deserialize (Deserializer& in, int version);
		bool        IsSection () const { return true; }
		int         SectionId () const { return 'USER'; }
		int         VersionNo () const { return modelVersion; }

	private:
		enum
		{
			// Project options bits -- no more then 31 bits
			KeepCheckedOut = 0,
			AutoSynch = 1,
			AutoJoin = 2,
			AutoFullSynch = 3,
			AllBcc = 4
		};

		typedef TransactableArray<MemberNote>::const_iterator MemberIter;

		void XGetMemberList (Project::MemberSelector & selectMember) const;
		void GetMemberList (Project::MemberSelector & selectMember) const;
		void XClearAdmin ();

	private:
		static char _userLogName [];

		// Revisit: These are not longs but UserIds
		XLongWithDefault<gidInvalid>	_myId;
		XString							_projectName;
		XString							_copyright;
		XLong							_counter;
		XLong							_scriptCounter;
		Log<MemberDescription>			_userLog;
		XFileOffset						_lastUserOffset;
		TransactableArray<MemberNote>	_members;
		XBitSet							_properties;
	};
}

#endif
