#if !defined (SCRIPTPROPS_H)
#define SCRIPTPROPS_H
//----------------------------------
// (c) Reliable Software 2002 - 2007
// ---------------------------------

#include "GlobalId.h"
#include "FileDisplayTable.h"
#include "Lineage.h"

#include <Win/Win.h>

class ScriptHeader;
class CommandList;
namespace Project
{
	class Db;
}
class PathFinder;
class MemberInfo;
class MemberDescription;
class Sidetrack;

class ScriptProps
{
public:
	ScriptProps (Project::Db const & projectDb, PathFinder const & pathFinder);
	ScriptProps (Project::Db const & projectDb, PathFinder const & pathFinder, std::string const & path);
	ScriptProps (Project::Db const & projectDb, Sidetrack const & sidetrack, GlobalId scriptId);

	void SetAckList (GidList const & ackList) { _ackList = ackList; }
	void Init (std::unique_ptr<ScriptHeader> hdr, std::unique_ptr<CommandList> cmdList);

	char const * GetCaption () const;
	std::string const & GetCheckinComment () const;
	GlobalId GetScriptId () const;
	Unit::Type GetScriptType () const { return _requestedUnitType; }
	long GetScriptTimeStamp () const;
	FileDisplayTable const & GetScriptFileTable () const { return *_fileTable; }
	std::unique_ptr<MemberInfo> RetrieveSenderInfo () const;
	std::unique_ptr<MemberInfo> RetrieveMemberInfo (UserId userId) const;
	MemberInfo const & GetCurrentMemberInfo () const { return *_currentInfo; }
	MemberInfo const & GetUpdatedMemberInfo () const { return *_updateInfo; }
	std::vector<std::pair<GlobalId, bool> > const & GetAcks () const { return _acks; }
	GlobalId GetRequestedScriptId () const { return _requestedScriptId; }
	GidList const & GetKnownDeadMembers () const { return _knownDeadMembers; }
	std::string GetProjectName () const;
	unsigned GetPartCount () const;
	unsigned GetPartNumber () const;
	unsigned GetMaxChunkSize () const;
	unsigned GetReceivedPartCount () const { return _receivedPartCount; }
	bool IsResendListExchausted () const { return _nextRecipientId == gidInvalid; }
	PackedTime GetNextResendTime () const { return _nextRequestTime; }
	UserId NextRecipientId () const { return _nextRecipientId; }
	

	bool IsFromHistory () const { return _isFromHistory; }
	bool IsOverdue () const { return _isOverdue; }
	bool IsMissing () const { return _hdr.get () == 0; }
	bool IsSetChange () const;
	bool IsMembershipUpdate () const;
	bool IsAddMember () const;
	bool IsDefectOrRemove () const;
	bool IsCtrl () const;
	bool IsJoinRequest () const;
	bool IsScriptResendRequest () const;
	bool IsProjectVerificationRequest () const { return _isProjectVerificationRequest; }
	bool IsFullSynchResendRequest () const;
	bool IsFullSynch () const;
	bool IsPackage () const;
	bool IsAck () const;
	bool IsAwaitingFinalAck () const;

public:
	class LineageSequencer
	{
	public:
		LineageSequencer (ScriptProps const & props);

		void Advance ();
		bool AtEnd () const { return _sequencer.AtEnd (); }

		char const * GetScriptId () const { return _id.c_str (); }
		char const * GetSender () const { return _sender.c_str (); }

	private:
		void Init ();

	private:
		Lineage const &				_lineage;
		Lineage::ReverseSequencer	_sequencer;
		Project::Db const &			_projectDb;
		std::string					_id;
		std::string					_sender;
	};

	friend class LineageSequencer;

private:
	struct UserInfo
	{
		UserInfo () {}
		UserInfo (UserId userId, MemberDescription const & member);
		UserId GetUserId() const;
	
		std::string	_name;
		std::string	_hubId;
		std::string	_id;
	};

	void AddUserInfo (Project::Db const & projectDb, UserId userId);

public:
	class MemberSequencer
	{
	public:
		MemberSequencer (ScriptProps const & props)
			: _cur (props._users.begin ()),
			  _end (props._users.end ())
		{}

		void Advance () { ++_cur; }
		bool AtEnd () const { return _cur == _end; }

		char const * GetName () const { return _cur->_name.c_str (); }
		char const * GetHubId () const { return _cur->_hubId.c_str (); }
		char const * GetStrId () const { return _cur->_id.c_str (); }
		UserId GetUserId() const { return _cur->GetUserId(); }

	private:
		std::vector<UserInfo>::const_iterator	_cur;
		std::vector<UserInfo>::const_iterator	_end;
	};

	friend class MemberSequencer;

private:
	Project::Db const &				_projectDb;
	std::unique_ptr<ScriptHeader>		_hdr;
	GidList							_ackList;
	std::vector<UserInfo>			_users;
	std::unique_ptr<FileDisplayTable>	_fileTable;
	std::unique_ptr<MemberInfo>		_currentInfo;
	std::unique_ptr<MemberInfo>		_updateInfo;
	std::vector<std::pair<GlobalId, bool> >	_acks;
	GidList							_knownDeadMembers;
	GlobalId						_requestedScriptId;
	Unit::Type						_requestedUnitType;
	UserId							_nextRecipientId;
	PackedTime						_nextRequestTime;
	unsigned						_partCount;
	unsigned						_maxChunkSize;
	unsigned						_receivedPartCount;
	bool							_isFromHistory;
	bool							_isOverdue;
	bool							_isMissingFullSynch;
	bool							_isProjectVerificationRequest;
};

#endif
