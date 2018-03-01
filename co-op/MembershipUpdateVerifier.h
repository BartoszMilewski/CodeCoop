#if !defined (MEMBERSHIPUPDATEVERIFIER_H)
#define MEMBERSHIPUPDATEVERIFIER_H
//---------------------------------------------
// (c) Reliable Software 2005
//---------------------------------------------

class MemberInfo;
class ScriptHeader;
class CommandList;
class Permissions;
namespace Project
{
	class Db;
}
namespace History
{
	class Db;
}

class MembershipUpdateVerifier
{
public:
	MembershipUpdateVerifier (ScriptHeader const & hdr, CommandList const & cmdList, bool isQuickVisit);

	void CheckUpdate (Project::Db const & projectDb, History::Db const & history, Permissions const & userPermissions);

	bool CanExecuteMembershipUpdate () const { return _canExecute; }
	bool HasResponseUpdate () const { return _response.get () != 0; }
	bool ConfirmConversion () const { return _confirmConversion; }

	MemberInfo const & GetResponse () const { return *_response; }

private:
	void CheckAddMember (Project::Db const & projectDb, History::Db const & history);
	void CheckEditMember (Project::Db const & projectDb, Permissions const & userPermissions);
	void AdminChecks (Project::Db const & projectDb);
	void ThisUserChecks (Project::Db const & projectDb, Permissions const & userPermissions);
	void CheckLicensedMember (Project::Db const & projectDb);
	void RejectUpdate ();

private:
	bool						_isQuickVisit;
	bool						_isAddMember;
	bool						_isEditMember;
	bool						_isFromVersion40;
	bool						_canExecute;
	bool						_confirmConversion;
	MemberInfo const *			_incomingUpdate;
	std::unique_ptr<MemberInfo>	_response;
};

#endif
