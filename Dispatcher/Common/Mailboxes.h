#if !defined (MAILBOXSPEC_H)
#define MAILBOXSPEC_H
// ----------------------------------
// (c) Reliable Software, 1999 - 2002
// ----------------------------------
#include "Mailbox.h"
#include "Table.h"
#include "ScriptInfo.h"
#include <auto_vector.h>

// Specialisations of Mailbox class

class Addressee;
class Address;
class Recipient;
class ScriptInfo;
class ScriptStatus;
class AddressDatabase;
class DispatcherCmd;
class DispatcherCmdExec;
class ScriptQuarantine;
class Configuration;
class Transport;
namespace UnknownRecipient { class Data; }

// =============================================
class PublicInbox : public Mailbox, public Table
{
public:
	struct AddresseeType
	{
		enum Bits
		{
			Local,
			Remote,
			Unknown
		};
	};
public:
    bool ProcessScripts (MailTruck & truck, WorkQueue & workQueue);

	// Table interface
    void QueryUniqueIds (std::vector<int>& uids, Restriction const * restrict = 0);
    std::string	GetStringField (Column col, int uid) const;
    int	GetNumericField (Column col, int uid) const;
	std::string QueryCaption (Restriction const & r) const { return _viewCaption; }

    void QueryUniqueNames (std::vector<std::string>& unames, Restriction const * restrict = 0);
    std::string	GetStringField (Column col, std::string const & uname) const;
    int	GetNumericField (Column col, std::string const & uname) const;

	// Public Inbox must have an id for identification in Mailboxes view.
	int GetId () const { return 0; }

protected:
	PublicInbox (FilePath const & path,
				 AddressDatabase & addressDb,
				 ScriptQuarantine & scriptQuarantine,
				 Configuration & config,
				 DispatcherCmdExecutor & cmdExecutor)
		: Mailbox (path, cmdExecutor),
		  _addressDb (addressDb),
		  _scriptQuarantine (scriptQuarantine),
		  _config (config)

	{}

	bool ReadTransportHeaders (std::vector<std::string> & fileNameList, ScriptVector & scriptList);
	virtual bool VerifyScriptSource (ScriptInfo const & script) { return true; }

	void ProcessAttachments (
		ScriptVector & scriptTicketList, 
		MailTruck & truck);
	bool ExecAttachments (ScriptTicket const & script);
	void ForwardDispatcherScript (
		std::unique_ptr<ScriptTicket> script, 
		Address const & sender, 
		MailTruck & truck);
	void ShipToSatellites (ScriptTicket & script, MailTruck & truck);
	void ShipToHubs (ScriptTicket & script, MailTruck & truck);

	Recipient * FindRecipient (ScriptInfo const & script, ScriptInfo::RecipientInfo const & addressee) const;

    virtual void DispatchToLocal (ScriptTicket & script, int addrIdx, MailTruck & truck) = 0;
	virtual void DispatchToRemote (ScriptTicket & script, int addrIdx, MailTruck & truck) = 0;
	virtual AddresseeType::Bits ConfirmRemoteWithKnownHub (ScriptTicket & script, int addrIdx, bool isJoinRequest)
	{
		// known hubId: must be local cluster
		return AddresseeType::Local;
	}
	virtual AddresseeType::Bits ConfirmRemoteWithUnknownHub (ScriptTicket & script, int addrIdx, bool isJoinRequest)
	{ 
		// unknown hubId: must be remote
		return AddresseeType::Remote; 
	}

protected:
	AddressDatabase & _addressDb;
	ScriptQuarantine  & _scriptQuarantine;
	Configuration	& _config;

	static std::string _rowName;
	static std::string _viewCaption;
};

class StandalonePublicInbox : public PublicInbox
{
public:
    StandalonePublicInbox (FilePath const & path,
						   AddressDatabase & addressDb,
						   ScriptQuarantine & scriptQuarantine,
						   Configuration & config,
						   DispatcherCmdExecutor & cmdExecutor)
		: PublicInbox (path, addressDb, scriptQuarantine, config, cmdExecutor)
	{}
protected:
	bool VerifyScriptSource (ScriptInfo const & script) { return false; }
	void DispatchToLocal (ScriptTicket & script, int addrIdx, MailTruck & truck)
	{
		Assert (!"Should never be called");
	}
	void DispatchToRemote (ScriptTicket & script, int addrIdx, MailTruck & truck)
	{
		Assert (!"Should never be called");
	}
	AddresseeType::Bits ConfirmRemoteWithKnownHub (ScriptTicket & script, int addrIdx, bool isJoinRequest);
	AddresseeType::Bits ConfirmRemoteWithUnknownHub (ScriptTicket & script, int addrIdx, bool isJoinRequest);

	AddresseeType::Bits AskUserKnownHubId (ScriptTicket & script, int addrIdx, bool isJoin);
	AddresseeType::Bits AskUserUnknownHubId (ScriptTicket & script, int addrIdx, bool isJoin);
private:
	void ExecUnknownRecipDirective (UnknownRecipient::Data const & directive, 
									ScriptTicket & script, 
									int addrIdx, 
									bool isJoinRequest);

	bool IsIncomingJoinWithTransport (ScriptTicket const & script, Transport & hubRoute);
};

class HubPublicInbox : public PublicInbox
{
public:
    HubPublicInbox (FilePath const & path,
					AddressDatabase & addressDb,
					ScriptQuarantine & scriptQuarantine,
					Configuration & config,
					DispatcherCmdExecutor & cmdExecutor)
		: PublicInbox (path, addressDb, scriptQuarantine, config, cmdExecutor)
	{}
protected:
	PublicInbox::AddresseeType::Bits ExecUnknownRecipDirective (
									UnknownRecipient::Data const & directive, 
									ScriptTicket & script, 
									int addrIdx, 
									bool isJoinRequest,
									bool knownHubId);
    bool VerifyScriptSource (ScriptInfo const & script);
    void DispatchToLocal (ScriptTicket & script, int addrIdx, MailTruck & truck);
    void DispatchToRemote (ScriptTicket & script, int addrIdx, MailTruck & truck);
	AddresseeType::Bits ConfirmRemoteWithKnownHub (ScriptTicket & script, int addrIdx, bool isJoinRequest);
	AddresseeType::Bits ConfirmRemoteWithUnknownHub (ScriptTicket & script, int addrIdx, bool isJoinRequest);
};

class SatPublicInbox : public PublicInbox
{
public:
    SatPublicInbox (FilePath const & path,
					AddressDatabase & addressDb,
					ScriptQuarantine & scriptQuarantine,
					Configuration & config,
					DispatcherCmdExecutor & cmdExecutor)
		: PublicInbox (path, addressDb, scriptQuarantine, config, cmdExecutor)
	{}
protected:
    void DispatchToLocal (ScriptTicket & script, int addrIdx, MailTruck & truck);
    void DispatchToRemote (ScriptTicket & script, int addrIdx, MailTruck & truck);
};

class ProxyHubPublicInbox : public PublicInbox
{
public:
    ProxyHubPublicInbox (FilePath const & path,
						 AddressDatabase & addressDb,
						 ScriptQuarantine & scriptQuarantine,
					Configuration & config,
					DispatcherCmdExecutor & cmdExecutor)
		: PublicInbox (path, addressDb, scriptQuarantine, config, cmdExecutor)
	{}
protected:
    void DispatchToLocal (ScriptTicket & script, int addrIdx, MailTruck & truck);
    void DispatchToRemote (ScriptTicket & script, int addrIdx, MailTruck & truck);
};

#endif
