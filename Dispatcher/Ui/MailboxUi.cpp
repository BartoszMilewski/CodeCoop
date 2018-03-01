// ----------------------------------
// (c) Reliable Software 2002 -- 2006
// ----------------------------------

#include "precompiled.h"
#include "Mailboxes.h"
#include "AddressDb.h"
#include "Addressee.h"
#include "ScriptInfo.h"
#include "ScriptQuarantine.h"
#include "Config.h"
#include "UnknownRecipWiz.h"
#include "ConfigExpt.h"
#include "AppInfo.h"
#include "DispatcherParams.h"
#include "Prompter.h"

PublicInbox::AddresseeType::Bits StandalonePublicInbox::ConfirmRemoteWithKnownHub (
	ScriptTicket & script, 
	int addrIdx, 
	bool isJoinRequest)
{
	// if script is sent from one of local enlistments ask about config change
	if (script.ToBeFwd () &&
		_addressDb.FindLocal (script.GetSender ()))
	{
		// wizard must throw ConfigExpt or it is canceled by a user;
		AddresseeType::Bits result = AskUserKnownHubId (script, addrIdx, isJoinRequest);
		// user has canceled the wizard or wanted to ignore the recipient
		Assert (result == AddresseeType::Unknown);
		_scriptQuarantine.Insert (script.GetName (), "Reconfiguration required");
	}
	else
	{
		// unrecognized script, don't even ask what to do
		// Delete/Ignore dialog will be displayed at end of processing run
	}
	return AddresseeType::Unknown;
}

PublicInbox::AddresseeType::Bits StandalonePublicInbox::AskUserKnownHubId (
	ScriptTicket & script, 
	int addrIdx, 
	bool isJoinRequest)
{
	// We are here if there is an outgoing script to a LAN recipient in Public Inbox
	// Ask the user whether this machine should serve as
	//	  - a hub (ask for fwd path)
	//	  - a sat (ask for hub path)
	// If the script is a Join Request, also ask the user whether he has made a mistake
	// (e.g. the recipient's project name/hubId address is misspelled)
	// In this case stamp delivery and defect from project.
	
	Assert (_addressDb.FindLocal (script.GetSender ()) != 0);
	std::set<Transport> proposedTransports; // empty
	NocaseSet projectList;
	_addressDb.GetActiveProjectList (projectList);
	ScriptInfo::RecipientInfo const & addressee = script.GetAddressee (addrIdx);

	UnknownRecipient::Data unknownRecipData (
						Address (
							addressee.GetHubId (), 
							script.GetProjectName (),
							addressee.GetDisplayUserId ()),
						proposedTransports,
						projectList,
						script.GetComment (),
						true,
						isJoinRequest,
						false,
						true,
						_config.GetOriginalData ());
	
	UnknownRecipient::Standalone2LanHandlerSet hndlrSet (unknownRecipData, isJoinRequest);
	if (ThePrompter.GetWizardData (hndlrSet))
	{
		ExecUnknownRecipDirective (unknownRecipData, script, addrIdx, isJoinRequest);
	}
	return AddresseeType::Unknown;
}

PublicInbox::AddresseeType::Bits StandalonePublicInbox::ConfirmRemoteWithUnknownHub(
	ScriptTicket & script, 
	int addrIdx,
	bool isJoinRequest)
{
	// if script is sent from one of local enlistments ask about config change
	if (script.ToBeFwd () && _addressDb.FindLocal (script.GetSender()))
	{
		// wizard must throw ConfigExpt or it is cancelled by a user
		AddresseeType::Bits result = AskUserUnknownHubId (script, addrIdx, isJoinRequest);
		// user has cancelled the wizard or wanted to ignore the recipient
		Assert (result == AddresseeType::Unknown);
		_scriptQuarantine.Insert (script.GetName (), "Reconfiguration required");
	}
	else
	{
		// unrecognized script, don't even ask what to do
		// Delete/Ignore dialog will be displayed at end of processing run
	}
	return AddresseeType::Unknown;
}

PublicInbox::AddresseeType::Bits StandalonePublicInbox::AskUserUnknownHubId (
	ScriptTicket & script, 
	int addrIdx,
	bool isJoinRequest)
{
	// We are here if there is an outgoing script to unrecognized recipient with
	// unknown e-mail address in Public Inbox.
	// Ask the user whether to start collaboration:
	//       - in LAN group (hub or a sat, fwd/hub path)
	//		 - through email only (reconfigure to hub with e-mail)
	// If the script is a Join Request, also ask the user whether he has made a mistake
	// (e.g. the recipient's project name/hubId address is misspelled)
	// In this case stamp delivery and defect from project.

	// Revisit: if user says: i am a part of a LAN group AND i want to be hub
	// we still don't know anything about recipient.
	// He may be on satellite or may be remote.  Now, we wrongly assume that he is remote.
	// We need to ask about it.

	Assert (_addressDb.FindLocal (script.GetSender ()) != 0);
	
	std::set<Transport> proposedTransports;
	NocaseSet projectList;
	_addressDb.GetActiveProjectList (projectList);
	ScriptInfo::RecipientInfo const & addressee = script.GetAddressee (addrIdx);

	UnknownRecipient::Data unknownRecipData (
						Address (
							addressee.GetHubId (), 
							script.GetProjectName (),
							addressee.GetDisplayUserId ()),
						proposedTransports,
						projectList,
						script.GetComment (),
						false,
						isJoinRequest,
						false,
						true,
						_config.GetOriginalData ());
	
	UnknownRecipient::Standalone2LanOrEmailHandlerSet hndlrSet (unknownRecipData, isJoinRequest);
	if (ThePrompter.GetWizardData (hndlrSet))
	{
		ExecUnknownRecipDirective (unknownRecipData, script, addrIdx, isJoinRequest);
	}
	return AddresseeType::Unknown;
}

//-----------------
// Hub Public Inbox
//-----------------

PublicInbox::AddresseeType::Bits HubPublicInbox::ConfirmRemoteWithKnownHub (
	ScriptTicket & script, 
	int addrIdx, 
	bool isJoinRequest)
{
	// Note: if IsFwd () then this script was created in our cluster
	// else it came to us from outside

	// It is addressed to our hub, but there is not project/user ID locally
	// Could happen if
	// - the addressee defected
	// - our cluster database was corrupted
	// propose only : LAN user, not  member of project
	std::set<Transport> proposedTransports;
	_addressDb.GetActiveSatelliteTransports (proposedTransports);

	NocaseSet projectList;
	_addressDb.GetActiveProjectList (projectList);
	bool isIncoming = !script.ToBeFwd ();
	ScriptInfo::RecipientInfo const & addressee = script.GetAddressee (addrIdx);

	UnknownRecipient::Data unknownRecipData (
		Address (
		addressee.GetHubId (), 
		script.GetProjectName (),
		addressee.GetDisplayUserId ()),
		proposedTransports,
		projectList,
		script.GetComment (),
		true,
		isJoinRequest,
		isIncoming,
		_addressDb.FindLocal (script.GetSender ()) != 0,
		_config.GetOriginalData ());

	// Ask the user whether
	// (1) this recipient is not in this project (defected, etc.) (stamp delivery)
	// (2) the project name is misspelled in Join Request (stamp delivery)
	// (3) this recipient is an unknown cluster recipient (ask for fwd path)

	UnknownRecipient::DefectedMisspelledOrClusterHandlerSet hndlrSet (unknownRecipData);
	if (ThePrompter.GetWizardData (hndlrSet, "Script dispatching problem."))
	{
		return ExecUnknownRecipDirective (unknownRecipData, script, addrIdx, isJoinRequest, true);
	}
	else
	{
		_scriptQuarantine.Insert (script.GetName (), "Unknown script recipient");
		return AddresseeType::Unknown;
	}
}

PublicInbox::AddresseeType::Bits HubPublicInbox::ConfirmRemoteWithUnknownHub (
	ScriptTicket & script, 
	int addrIdx, 
	bool isJoinRequest)
{
	if (script.ToBeFwd ())
	{
		// originated in our cluster
		if (_config.GetTopology ().UsesEmail ())
			return AddresseeType::Remote;
		else
		{
			std::set<Transport> proposedTransports;
			NocaseSet projectList;
			_addressDb.GetActiveProjectList (projectList);
			ScriptInfo::RecipientInfo const & addressee = script.GetAddressee (addrIdx);

			UnknownRecipient::Data unknownRecipData (
								Address (
									addressee.GetHubId (), 
									script.GetProjectName (),
									addressee.GetDisplayUserId ()),
								proposedTransports,
								projectList,
								script.GetComment (),
								false,
								isJoinRequest,
								!script.ToBeFwd (),
								_addressDb.FindLocal (script.GetSender ()) != 0,
								_config.GetOriginalData ());

			// We are here if:
			// - an outgoing script is to be sent by e-mail, 
			//   a recipient's e-mail (hub ID) is locally unknown, 
			//   but the Dispatcher is configured not to use e-mail
			UnknownRecipient::SwitchToEmailHandlerSet hndlrSet (unknownRecipData);
			if (ThePrompter.GetWizardData (hndlrSet, "Script dispatching problem."))
			{
				return ExecUnknownRecipDirective (unknownRecipData, 
												script, 
												addrIdx, 
												isJoinRequest, 
												false);
			}
			else
			{
				_scriptQuarantine.Insert (script.GetName (), "Unknown script recipient");
				return AddresseeType::Unknown;
			}
		}
	}
	else
		return AddresseeType::Remote;
}

void StandalonePublicInbox::ExecUnknownRecipDirective (
								UnknownRecipient::Data const & directive, 
								ScriptTicket & script, 
								int addrIdx, 
								bool isJoinRequest)
{
	if (directive.CanIgnoreRecipient ())
	{
		script.StampDelivery (addrIdx);
		if (isJoinRequest)
		{
			// Revisit: defect from project
		}
		return;
	}
	
	_config.BeginEdit ();
	ConfigData & newCfg = _config.XGetData ();
	if (directive.IsCluster ())
	{
		if (directive.WantToBeSatellite ())
		{
			newCfg.MakeSatellite (directive.GetTransport ());
		}
		else
		{
			ScriptInfo::RecipientInfo const & addressee = script.GetAddressee (addrIdx);
			// Revisit: do not add wildcard recipient to address database any more
			_addressDb.AddClusterRecipient (Address (addressee.GetHubId (), 
													script.GetProjectName (), 
													addressee.GetUserId ()), 
											directive.GetTransport ());
			newCfg.MakeHubNoEmail ();
		}
	}
	else
	{
		Assert (directive.IsRemote ());
		newCfg.MakeHubWithEmail ();
	}

	std::string hubId = directive.GetHubId ();
	if (!hubId.empty ())
	{
		newCfg.SetHubId (hubId);
		newCfg.SetInterClusterTransportToMe (directive.GetInterClusterTransportToMe ());
	}
	throw ConfigExpt ();
}

PublicInbox::AddresseeType::Bits 	HubPublicInbox::ExecUnknownRecipDirective (
									UnknownRecipient::Data const & directive, 
									ScriptTicket & script, 
									int addrIdx, 
									bool isJoinRequest,
									bool isKnownHubId)
{
	if (directive.CanIgnoreRecipient ())
	{
		script.StampDelivery (addrIdx);
	}
	else if (directive.IsCluster ())
	{
		ScriptInfo::RecipientInfo const & addressee = script.GetAddressee (addrIdx);
		_addressDb.AddClusterRecipient (Address (addressee.GetHubId (), 
												script.GetProjectName (), 
												addressee.GetUserId ()), 
										directive.GetTransport ());
		return AddresseeType::Local;
	}
	else
	{
		if (isKnownHubId)
		{
			// we are told to email anyway
			Assert (directive.IsRemote ());
			if (_config.GetTopology ().UsesEmail ())
			{
				return AddresseeType::Remote;
			}
			else // no email--we can't email it!
			{
				// Revisit: Problem. The script will not be e-mailed.
				// After config change the Dispatcher will ask what to do for the next time,
				// because e-mail is locally known. (This time the script will be e-mailed)

				_config.BeginEdit ();
				ConfigData & newCfg = _config.XGetData ();
				newCfg.SetUseEmail (true);
				throw ConfigExpt ();
			}
		}
		else // we don't know this hub, let's start using email
		{
			Assert (directive.WantToStartUsingEmail ());
			_config.BeginEdit ();
			ConfigData & newCfg = _config.XGetData ();
			newCfg.MakeHubWithEmail ();
			throw ConfigExpt ();
		}
	}
	return AddresseeType::Unknown;
}
