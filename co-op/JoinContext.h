#if !defined (JOINCONTEXT_H)
#define JOINCONTEXT_H
//----------------------------------
// (c) Reliable Software 2006 - 2007
//----------------------------------

#include "MemberInfo.h"
#include "DistributorLicense.h"

namespace Mailbox { class Db; }
class ActivityLog;

class JoinContext
{
public:
	JoinContext (Catalog & catalog, 
				 ActivityLog & log, 
				 MemberState adminState,
				 bool isInvitation = false)
		:_distributorPool (catalog),
		 _log (log),
		 _delete (false),
		 _adminState (adminState),
		 _isInvitation (isInvitation),
		 _manualInvitationDispatch (false)
	{
		Assert (_adminState.IsAdmin ());
	}
	~JoinContext ();
	bool Init (Mailbox::Db & mailBox);
	bool WillAccept (std::string const & projName);
	void SetComputerName (std::string const & computerName)
	{
		Assert (IsInvitation ());
		_computerName = computerName;
	}
	void SetManualInvitationDispatch (bool flag)
	{
		Assert (IsInvitation ());
		_manualInvitationDispatch = flag;
	}
	std::string const & GetComputerName () const { return _computerName; }
	MemberInfo const & GetJoineeInfo () const { return _joineeInfo; }
	MemberDescription const & GetJoineeDescription () const { return _joineeInfo.Description (); }
	std::string const & GetInvitationFileName () const { return _invitationFileName; }
	void SetJoineeState (MemberState state) { _joineeInfo.SetState (state); }
	void SetJoineeInfo (MemberInfo const & info) { _joineeInfo = info; }

	bool IsInvitation () const { return _isInvitation; }
	bool IsManualInvitationDispatch () const { return _manualInvitationDispatch; }
	void SetInvitationFileName (std::string const & fileName) { _invitationFileName = fileName; }
	GlobalId GetScriptId () const { return _scriptId; }
	void DeleteScript (bool yesno) { _delete = yesno; }
	bool ShouldDeleteScript () const { return _delete; }

private:
	DistributorLicensePool	_distributorPool;
	ActivityLog &			_log;
	MemberState				_adminState;
	MemberInfo				_joineeInfo;
	bool const				_isInvitation;
	// join specific
	bool					_delete;
	GlobalId				_scriptId;
	// invitation specific
	std::string				_computerName;
	bool					_manualInvitationDispatch;
	std::string				_invitationFileName;
};

#endif
