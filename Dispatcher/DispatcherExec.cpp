//------------------------------------
//  (c) Reliable Software, 1999 - 2006
//------------------------------------
#include "precompiled.h"
#include "DispatcherExec.h"
#include "DispatcherCmd.h"
#include "Addressee.h"
#include "AddressDb.h"
#include "ScriptInfo.h"
#include "Config.h"
#include "Registry.h"
#include "ConfigExpt.h"
#include "Prompter.h"
#include "AppInfo.h"
#include "Validators.h"
#include "EmailMan.h"
#include "EmailConfigData.h"
#include "RegFunc.h"
#include "RejectedInvitationDlg.h"
#include "AlertMan.h"
#include <File/Path.h>
#include <File/MemFile.h>

std::unique_ptr<DispatcherCmdExec> DispatcherCmdExec::CreateCmdExec (DispatcherCmd const & cmd)
{
	switch (cmd.GetType ())
	{
	case typeAddressChange:
		return std::unique_ptr<DispatcherCmdExec> (new AddressChangeExec (cmd));
		break;
	case typeAddMember:
		return std::unique_ptr<DispatcherCmdExec> (new AddMemberExec (cmd));
		break;
	case typeReplaceTransport:
		return std::unique_ptr<DispatcherCmdExec> (new ReplaceTransportExec (cmd));
		break;
	case typeReplaceRemoteTransport:
		return std::unique_ptr<DispatcherCmdExec> (new ReplaceRemoteTransportExec (cmd));
        break;
	case typeChangeHubId:
		return std::unique_ptr<DispatcherCmdExec> (new ChangeHubIdExec (cmd));
        break;
	case typeAddSatelliteRecipients:
		return std::unique_ptr<DispatcherCmdExec> (new AddSatelliteRecipientsExec (cmd));
        break;
	case typeForceDispatch:
		return std::unique_ptr<DispatcherCmdExec> (new ForceDispatchExec (cmd));
		break;
	case typeChunkSize:
		return std::unique_ptr<DispatcherCmdExec> (new ChunkSizeExec (cmd));
		break;
	case typeDistributorLicense:
		return std::unique_ptr<DispatcherCmdExec> (new DistributorLicenseExec (cmd));
		break;
	case typeInvitation:
		return std::unique_ptr<DispatcherCmdExec> (new InvitationExec (cmd));
		break;
	default:
		Assert (!"Unknown dispatcher command type");
		return std::unique_ptr<DispatcherCmdExec> ();
	};
}

InvitationExec::InvitationExec (DispatcherCmd const & command)
	: DispatcherCmdExec (command)
{
	_invitation = dynamic_cast<InvitationCmd const *> (&_command);
}

void InvitationExec::GetParams (ScriptTicket const & script, std::string const & thisHubId)
{
	_params.Add ("isToBeForwarded", script.ToBeFwd () ? "yes" : "no");
}

DispatcherCmdExec::CmdResult InvitationExec::Do (
					  AddressDatabase & addressDb, 
					  DispatcherCmdExecutor & cmdExecutor,
					  Configuration & config)
{
	// Are we in the target cluster?
	Invitee const & invitee = _invitation->GetInvitee ();
	if (!IsNocaseEqual (config.GetHubId (), invitee.GetHubId ()))
		return Continue;

	// Has this script (or its another chunk) been already processed?
	Recipient const * invitedRecip = addressDb.Find (invitee);
	if (invitedRecip != 0)
		return Continue;

	std::string const & targetCompName = invitee.GetComputerName ();
	Topology const & topology = config.GetTopology ();
	FilePath const & publicInbox = config.GetPublicInboxPath ();

	if (topology.HasHub ())
	{
		// satellite, off-site satellite, off-site hub
		std::string thisCompName = config.GetSatComputerName ();
		Assert (!thisCompName.empty ()); // topology that has a hub must have its computer name defined
		if (IsNocaseEqual (thisCompName, targetCompName))
			return ExecuteOnTargetComputer (cmdExecutor, addressDb, publicInbox);

		if (_params.GetValue ("isToBeForwarded") == "yes")
		{
			// the script was produced by this computer
			// we are just an intermediate computer
			return Continue;
		}

		if (topology.IsTemporaryHub ())
		{
			// our hub is the real addressee of this invitation
			// we are just an intermediate computer
			return Continue;
		}

		if (InformUser ("Your computer name is different than the computer name "
						"\nspecified in the invitation."))
		{
			RejectInvitation (addressDb, publicInbox);
			return Delete;
		}
		else
		{
			return Ignore;
		}
	}
	else
	{
		Assert (topology.IsHubOrPeer ());
		if (targetCompName.empty ())
			return ExecuteOnTargetComputer (cmdExecutor, addressDb, publicInbox);

		if (topology.IsPeer ())
		{
			if (InformUser ("The invitation is addressed to a satellite."
						    "\nYou have no satellites, because you are configured"
						    "\nas an e-mail peer."))
			{
				RejectInvitation (addressDb, publicInbox);
				return Delete;
			}
			else
			{
				return Ignore;
			}
		}
		else
		{
			Assert (topology.IsHub ());
			if (IsNocaseEqual (targetCompName, Registry::GetComputerName ()))
			{
				if (InformUser ("The invitation is addressed to your machine as a satellite."
							  "\nYour machine is not a satellite!"))
				{
					RejectInvitation (addressDb, publicInbox);
					return Delete;
				}
				else
				{
					return Ignore;
				}
			}
			return ExecuteOnIntermediateHub (cmdExecutor, addressDb, publicInbox);
		}
	}
}

DispatcherCmdExec::CmdResult InvitationExec::ExecuteOnTargetComputer (
		DispatcherCmdExecutor & cmdExecutor,
		AddressDatabase & addressDb,
		FilePath const & publicInbox)
{
	Tri::State state = cmdExecutor.InviteToProject (
							_invitation->GetAdminName (),
							_invitation->GetAdminEmailAddress (), 
							_invitation->GetInvitee ());
	if (state == Tri::No)
		RejectInvitation (addressDb, publicInbox);

	return TriState2CmdResult (state);
}

DispatcherCmdExec::CmdResult InvitationExec::ExecuteOnIntermediateHub (
		DispatcherCmdExecutor & cmdExecutor,
		AddressDatabase & addressDb,
		FilePath const & publicInbox)
{
	Invitee const & invitee = _invitation->GetInvitee ();

	Tri::State state = cmdExecutor.AddAskInvitedClusterRecipient (invitee);
	if (state == Tri::No)
		RejectInvitation (addressDb, publicInbox, false); // is not target

	return TriState2CmdResult (state);
}

void InvitationExec::RejectInvitation (
		AddressDatabase & addressDb, 
		FilePath const & publicInbox, 
		bool isTarget)
{
	// Notice 1: after rejection we want to have addressing entry of the invitee
	// in the address database as 'removed' so that successive chunks are
	// eaten quietly by Dispatcher.
	Invitee const & invitee = _invitation->GetInvitee ();
	if (isTarget)
	{
		addressDb.AddTempLocalRecipient (invitee, true); // removed
	}
	else
	{
		addressDb.AddRemovedClusterRecipient (invitee);
	}
	std::vector<unsigned char> const & defectScript = _invitation->GetDefectScript ();
	MemFileNew defectFile (publicInbox.GetFilePath (_invitation->GetDefectFilename ()), defectScript.size ());
	std::copy (defectScript.begin (), defectScript.end (), defectFile.GetBuf ());
}

bool InvitationExec::InformUser (char const * explanation) const
{
	Invitee const & invitee = _invitation->GetInvitee ();
	RejectedInvitationCtrl ctrl (
			invitee.GetProjectName ().c_str (),
			invitee.GetUserName ().c_str (),
			invitee.GetComputerName ().c_str (),
			explanation);

	return ThePrompter.GetData (ctrl, "Rejected Invitation");
}

DispatcherCmdExec::CmdResult InvitationExec::TriState2CmdResult (Tri::State state) const
{
	switch (state)
	{
	case Tri::Yes:
		return Continue;
	case Tri::No:
		return Delete;
	case Tri::Maybe:
		return Ignore;
	default:
		Assert (!"Incorrect trinary state");
		return Continue;
	};
}

void AddressChangeExec::GetParams (ScriptTicket const & script, std::string const & thisHubId)
{
	_params.Add ("project", script.GetProjectName ());
}

DispatcherCmdExec::CmdResult AddressChangeExec::Do (
						AddressDatabase & addressDb, 
						DispatcherCmdExecutor & cmdExecutor,
						Configuration & config)
{
	AddressChangeCmd const & addrChange = dynamic_cast<AddressChangeCmd const &> (_command);

	if (!addrChange.NewHubId ().empty () &&
		config.GetTopology ().IsHub ()   &&
		IsNocaseEqual (config.GetHubId (), addrChange.OldHubId ())) // He belongs to this hub
	{
		// special behavior in case of Full Synch on hub:
		// check if temporary recipient's address with random userId exists
		// if not, warn user that the recipient might have already defected
		UserIdPack userId (addrChange.OldUserId ());
		if (userId.IsRandom ())
		{
			Recipient * recip = addressDb.Find (Address (
														addrChange.OldHubId (),
														_params.GetValue ("project"), 
														addrChange.OldUserId ()));
			if (0 == recip || recip->IsRemoved ())
			{
				// oops, recipient defected before receiving Full Synch;
				// or we are processing this command for the second time
				if (addressDb.Find (Address (addrChange.NewHubId (),
											_params.GetValue ("project"), 
											addrChange.NewUserId ())))
				{
					// New address registered, so we have already processed this command.
					// Script still sits in mailbox for some reason.
					// (This may happen if exception is thrown after processing addendums, but
					// before/during delivering script.)
					return Continue;
				}
				else
				{
					// we mustn't register new address !
					std::string msg;
					msg += "It looks like the user ";
					msg += addrChange.OldHubId ();
					msg += ",\nthe recipient of the Full Sync script";
					msg += "\nhas already defected from the project ";
					msg += _params.GetValue ("project");
					msg += ".\n\nDo you want to delete this script (recommended)?";
					if (ThePrompter.Prompt (msg, "Full Synch cannot be delivered."))
					{
						return Delete;
					}
					else
					{
						return Ignore;
					}
				}
			}
		}
	}

	if (addrChange.NewHubId ().empty ())
	{
		Assert (addrChange.NewUserId ().empty ());
		Address toBeRemoved (
					addrChange.OldHubId (), 
					_params.GetValue ("project"),
					addrChange.OldUserId ());
		addressDb.RemoveAddress (toBeRemoved);
		return Continue;
	}

	if (IsMySatSwitchingToPeer (addrChange, config, addressDb))
	{
		Address toBeRemoved (
					addrChange.OldHubId (), 
					_params.GetValue ("project"),
					addrChange.OldUserId ());
		addressDb.RemoveSatelliteAddress (toBeRemoved);

		if (!addressDb.IsRemoteHub (addrChange.NewHubId ()))
		{
			addressDb.AddInterClusterTransport (addrChange.NewHubId (), Transport (addrChange.NewHubId ()));
			std::string info = "One of your satellites has changed its configuration to E-mail Peer."
								"\nIt is now using ";
			info += addrChange.NewHubId ();
			info += " as its e-mail address.";
			TheAlertMan.PostInfoAlert (info);
		}
		return Continue;
	}

	if (addressDb.UpdateAddress (
				_params.GetValue ("project"), 
				addrChange.OldHubId (), addrChange.OldUserId (),
				addrChange.NewHubId (), addrChange.NewUserId ()))
	{
		return Continue;
	}
	else
		return Ignore;
}

bool AddressChangeExec::IsMySatSwitchingToPeer (
		AddressChangeCmd const & addrChange, 
		Configuration const & config,
		AddressDatabase const & addressDb) const
{
	if (config.GetTopology ().IsHub () &&
		IsNocaseEqual (addrChange.OldHubId (), config.GetHubId ())  &&
		!IsNocaseEqual (addrChange.NewHubId (), config.GetHubId ()))
	{
		Address satAddress (
			addrChange.OldHubId (), 
			_params.GetValue ("project"),
			addrChange.OldUserId ());

		if (addressDb.FindClusterRecipient (satAddress) != 0)
		{
			return true;
		}
	}
	return false;
}

void AddMemberExec::GetParams (ScriptTicket const & script, std::string const & thisHubId)
{
	_params.Add ("project", script.GetProjectName ());
	_params.Add ("isSenderFromThisCluster", script.IsFromThisCluster (thisHubId) ? "yes" : "no");
}

DispatcherCmdExec::CmdResult AddMemberExec::Do (
						AddressDatabase & addressDb, 
						DispatcherCmdExecutor & cmdExecutor,
						Configuration & config)
{
	if (!config.GetTopology ().IsHubOrPeer ())
		return Continue;

	if (_params.GetValue ("isSenderFromThisCluster") == "no")
		return Continue;

	AddMemberCmd const & cmd = dynamic_cast<AddMemberCmd const &> (_command);
	if (addressDb.FindLocal (Address (
								cmd.HubId (), 
								_params.GetValue ("project"), 
								cmd.GetUserId ())))
		return Continue; // the sender is on this hub: nothing to do

	if (!addressDb.UpdateTransport (Address (cmd.HubId (), 
											_params.GetValue ("project"), 
											cmd.GetUserId ()),
											cmd.TransportMethod ()))
	{
		// don't know the guy and can't add him either
		return Ignore;
	}

	addressDb.KillClusterRecipient (Address (cmd.HubId (), 
 											_params.GetValue ("project"), 
											std::string ("*")));
	if (config.GetTopology ().IsPeer ())
	{
		// if it has cluster users it cannot be a simple peer (must be hub)
		config.BeginEdit ();
		ConfigData & newCfg = config.XGetData ();
		newCfg.MakeHubWithEmail ();
		throw ConfigExpt ();
	};

	return Continue;
}

void AddSatelliteRecipientsExec::GetParams (ScriptTicket const & script, std::string const & thisHubId)
{
	_params.Add ("isSenderFromThisCluster", script.IsFromThisCluster (thisHubId) ? "yes" : "no");
}

DispatcherCmdExec::CmdResult AddSatelliteRecipientsExec::Do (
	AddressDatabase & addressDb, 
	DispatcherCmdExecutor & cmdExecutor,
	Configuration & config)
{
	if (!config.GetTopology ().IsHubOrPeer ())
		return Continue;

	if (_params.GetValue ("isSenderFromThisCluster") == "no")
		return Continue;

	AddSatelliteRecipientsCmd const & cmd = dynamic_cast<AddSatelliteRecipientsCmd const &> (_command);
	if ((cmd.beginActive () == cmd.endActive ()) &&
		(cmd.beginRemoved () == cmd.endRemoved ()))
	{
		return Continue; // nothing to do
	}

	for (AddSatelliteRecipientsCmd::const_iterator it = cmd.beginActive ();
		 it != cmd.endActive ();
		 ++it)
	{
		Address newAddr (*it);
		addressDb.AddClusterRecipient (newAddr, cmd.TransportMethod ());

		// historical cleanup: remove a special entry with userid equal to "*"
		Address specialAddr (newAddr);
		specialAddr.SetUserId ("*");
		addressDb.KillClusterRecipient (specialAddr);
	}
	
	for (AddSatelliteRecipientsCmd::const_iterator it = cmd.beginRemoved (); 
		 it != cmd.endRemoved ();	
		 ++it)
	{
		Address newAddr (*it);
		addressDb.AddRemovedClusterRecipient (newAddr);
	}

	if (config.GetTopology ().IsPeer ())
	{
		// if it has cluster users it cannot be a simple peer (must be hub)
		config.BeginEdit ();
		ConfigData & newCfg = config.XGetData ();
		newCfg.MakeHubWithEmail ();
		throw ConfigExpt ();
	};

	return Continue;
}

// Dispatcher to Dispatcher scripts

DispatcherCmdExec::CmdResult ReplaceTransportExec::Do (
						AddressDatabase & addressDb, 
						DispatcherCmdExecutor & cmdExecutor,
						Configuration & config)
{
 	ReplaceTransportCmd const & cmd = dynamic_cast<ReplaceTransportCmd const &> (_command);
	if (config.GetTopology ().IsHub ())
	{
		addressDb.ReplaceTransport (cmd.OldTransport (), cmd.NewTransport ());
		cmdExecutor.PostForceDispatch ();
	}
	else
	{
		config.BeginEdit ();
		ConfigData & newCfg = config.XGetData ();
		newCfg.SetActiveTransportToHub (cmd.NewTransport ());
		throw ConfigExpt ();
	}
	return Continue;
}

DispatcherCmdExec::CmdResult ForceDispatchExec::Do (
						AddressDatabase & addressDb, 
						DispatcherCmdExecutor & cmdExecutor,
						Configuration & config)
{
	cmdExecutor.PostForceDispatch ();
	return Continue;
}

DispatcherCmdExec::CmdResult ChunkSizeExec::Do (
						AddressDatabase & addressDb, 
						DispatcherCmdExecutor & cmdExecutor,
						Configuration & config)
{
 	ChunkSizeCmd const & cmd = dynamic_cast<ChunkSizeCmd const &> (_command);
	unsigned long chunkSize = cmd.GetChunkSize ();
	Assume (ChunkSizeValidator (chunkSize).IsInValidRange (), 
		"Invalid new max chunk size received in a script");

	TheEmail.ChangeMaxChunkSize (chunkSize);
	return Continue;
}

// Sent along a join request
DispatcherCmdExec::CmdResult ReplaceRemoteTransportExec::Do (
						AddressDatabase & addressDb, 
						DispatcherCmdExecutor & cmdExecutor,
						Configuration & config)
{
 	ReplaceRemoteTransportCmd const & cmd = dynamic_cast<ReplaceRemoteTransportCmd const &> (_command);
	// Add Hub ID -> Transport translation entry for the source hub in the target cluster
	// So that return scripts can be dispatched correctly
	if (!IsNocaseEqual (config.GetHubId (), cmd.HubId ()))	// We are not the cluster of origin
	{
		addressDb.AddInterClusterTransport (cmd.HubId (), cmd.GetTransport ());
	}
	return Continue;
}

// Sent from hub to satellites when hub id/transport edited
DispatcherCmdExec::CmdResult ChangeHubIdExec::Do (
						AddressDatabase & addressDb, 
						DispatcherCmdExecutor & cmdExecutor,
						Configuration & config)
{
 	ChangeHubIdCmd const & cmd = dynamic_cast<ChangeHubIdCmd const &> (_command);
	// If we are a satellite of the hub in question, change our hub ID/Transport entry
	if (config.GetTopology ().HasHub ()
		&& IsNocaseEqual (config.GetHubId (), cmd.OldHubId ()))
	{
		config.BeginEdit ();
		ConfigData & newCfg = config.XGetData ();
		newCfg.SetHubId (cmd.NewHubId ());
		newCfg.SetInterClusterTransportToMe (cmd.NewTransport ());
		throw ConfigExpt ();
	}
	return Continue;
}

DispatcherCmdExec::CmdResult DistributorLicenseExec::Do (AddressDatabase & addressDb,
						  DispatcherCmdExecutor & cmdExecutor,
						  Configuration & config)
{
 	DistributorLicenseCmd const & cmd = dynamic_cast<DistributorLicenseCmd const &> (_command);
	cmdExecutor.AddDispatcherLicense (cmd.Licensee (), cmd.StartNum (), cmd.Count ());
	return Continue;
}
