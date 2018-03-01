#if !defined (DISPATCHEREXEC_H)
#define DISPATCHEREXEC_H
//------------------------------------
//  DispatcherExec.h
//  (c) Reliable Software, 1999-2006
//------------------------------------
class DispatcherCmd;
class AddressDatabase;
class Configuration;
class DispatcherCmdExecutor;
class InvitationCmd;
class ScriptTicket;

#include <TriState.h>
#include <StringOp.h>

// Every Dispatcher script command has a corresponding exec -- an object
// that executes the command.

class FilePath;

class DispatcherCmdExec
{
public:
	enum CmdResult
	{
		Delete,
		Ignore,
		Continue
	};
public:
    DispatcherCmdExec (DispatcherCmd const & command)
        : _command (command)
    {}
    virtual ~DispatcherCmdExec () {}
	virtual void GetParams (ScriptTicket const & script, std::string const & thisHubId) {}
	virtual CmdResult Do (AddressDatabase & addressDb, 
						  DispatcherCmdExecutor & cmdExecutor,
						  Configuration & config) = 0;

	static std::unique_ptr<DispatcherCmdExec> CreateCmdExec (DispatcherCmd const & cmd);

protected:
    DispatcherCmd const & _command;
	NamedValues			  _params;
};

class InvitationExec : public DispatcherCmdExec
{
public:
	InvitationExec (DispatcherCmd const & command);
	void GetParams (ScriptTicket const & script, std::string const & thisHubId);
	CmdResult Do (AddressDatabase & addressDb, 
				  DispatcherCmdExecutor & cmdExecutor,
				  Configuration & config);

private:
	DispatcherCmdExec::CmdResult ExecuteOnIntermediateHub (
			DispatcherCmdExecutor & cmdExecutor, 
			AddressDatabase & addressDb,
			FilePath const & publicInbox);
	DispatcherCmdExec::CmdResult ExecuteOnTargetComputer (
			DispatcherCmdExecutor & cmdExecutor, 
			AddressDatabase & addressDb,
			FilePath const & publicInbox);
	void RejectInvitation (
			AddressDatabase & addressDb, 
			FilePath const & publicInbox, 
			bool isTarget = true);
	bool InformUser (char const * explanation) const;
	CmdResult TriState2CmdResult (Tri::State state) const;
private:
	InvitationCmd   const *	_invitation;
};

// Updates or removes address from satellite recipient list
class AddressChangeCmd;

class AddressChangeExec : public DispatcherCmdExec
{
public:
    AddressChangeExec (DispatcherCmd const & command)
        : DispatcherCmdExec (command)
    {}
	void GetParams (ScriptTicket const & script, std::string const & thisHubId);
	CmdResult Do (AddressDatabase & addressDb, 
					DispatcherCmdExecutor & cmdExecutor,
					Configuration & config);
private:
	bool IsMySatSwitchingToPeer (AddressChangeCmd const & addrChange, 
								Configuration const & config,
								AddressDatabase const & addressDb) const;
};

// Add new member to satellite recipient list (sender's hub only)
class AddMemberExec : public DispatcherCmdExec
{
public:
    AddMemberExec (DispatcherCmd const & command)
        : DispatcherCmdExec (command)
    {}
	void GetParams (ScriptTicket const & script, std::string const & thisHubId);
	CmdResult Do (AddressDatabase & addressDb, 
					DispatcherCmdExecutor & cmdExecutor,
					Configuration & config);
};

// Add satellite recipients to hub's satellite recipient list (sender's hub only)
class AddSatelliteRecipientsExec : public DispatcherCmdExec
{
public:
	AddSatelliteRecipientsExec (DispatcherCmd const & command)
		: DispatcherCmdExec (command)
	{}
	void GetParams (ScriptTicket const & script, std::string const & thisHubId);
	CmdResult Do (AddressDatabase & addressDb, 
		DispatcherCmdExecutor & cmdExecutor,
		Configuration & config);
};

// On a hub, replace transport to all satellite recipients who are using it
// On a satellite, replace transport to hub
class ReplaceTransportExec : public DispatcherCmdExec
{
public:
    ReplaceTransportExec (DispatcherCmd const & command)
        : DispatcherCmdExec (command)
    {}
	CmdResult Do (AddressDatabase & addressDb, 
					DispatcherCmdExecutor & cmdExecutor,
					Configuration & config);
};

// Replace transport for this hub Id in all remote hub tables
class ReplaceRemoteTransportExec : public DispatcherCmdExec
{
public:
    ReplaceRemoteTransportExec (DispatcherCmd const & command)
        : DispatcherCmdExec (command)
    {}
	CmdResult Do (AddressDatabase & addressDb, 
					DispatcherCmdExecutor & cmdExecutor,
					Configuration & config);
};

// Replace this hub id in everybody's remote hub list 
class ChangeHubIdExec : public DispatcherCmdExec
{
public:
    ChangeHubIdExec (DispatcherCmd const & command)
        : DispatcherCmdExec (command)
    {}
	CmdResult Do (AddressDatabase & addressDb, 
					DispatcherCmdExecutor & cmdExecutor,
					Configuration & config);
};

class ForceDispatchExec : public DispatcherCmdExec
{
public:
    ForceDispatchExec (DispatcherCmd const & command)
        : DispatcherCmdExec (command)
    {}
	CmdResult Do (AddressDatabase & addressDb, 
					DispatcherCmdExecutor & cmdExecutor,
					Configuration & config);
};

class ChunkSizeExec : public DispatcherCmdExec
{
public:
    ChunkSizeExec (DispatcherCmd const & command)
        : DispatcherCmdExec (command)
    {}
	CmdResult Do (AddressDatabase & addressDb, 
					DispatcherCmdExecutor & cmdExecutor,
					Configuration & config);
};

class DistributorLicenseExec : public DispatcherCmdExec
{
public:
	DistributorLicenseExec (DispatcherCmd const & command)
        : DispatcherCmdExec (command)
    {}
	CmdResult Do (AddressDatabase & addressDb, 
					DispatcherCmdExecutor & cmdExecutor,
					Configuration & config);
};

#endif
