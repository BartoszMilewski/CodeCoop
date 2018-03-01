//------------------------------------
//  (c) Reliable Software, 2002
//------------------------------------
#include "precompiled.h"
#include "Mailboxes.h"
#include "DispatcherScript.h"
#include "DispatcherExec.h"
#include "AddressDb.h"
#include "Addressee.h"
#include "Config.h"
#include "MailTruck.h"
#include "ScriptInfo.h"
#include "ScriptQuarantine.h"
#include "AlertMan.h"
#include <Dbg/Out.h>

// Called for every script
// keep order of processing attachments same as order of processing scripts
// currently order of processing scripts is from the last, to the first on scriptList
// scriptInfoList may acquire null entries
void PublicInbox::ProcessAttachments (ScriptVector & scriptList, 
									  MailTruck & truck)
{
	for (int i = scriptList.size () - 1; i >= 0; --i)
	{
		ScriptTicket & scriptTicket = *scriptList[i];
		if (!scriptTicket.HasDispatcherAddendum ())
			continue;

		if (_scriptQuarantine.FindModuloChunkNumber (scriptTicket.GetName ()))
			continue;

		// is this a dispatcher script?
		Address const & sender = scriptTicket.GetSender ();
		if (sender.IsDispatcher ())
		{
			std::unique_ptr<ScriptTicket> scriptHolder = scriptList.extract(i);
			ForwardDispatcherScript (std::move(scriptHolder), sender, truck);
		}
		else 
		{
			if (ExecAttachments (scriptTicket))
				scriptList.assign_direct(i, 0);
		}
	}
}

// Returns true if no further action is required for the script
bool PublicInbox::ExecAttachments (ScriptTicket const & script)
{
	std::unique_ptr<DispatcherScript> addendums;
	try
	{
		addendums.reset (new DispatcherScript (FileDeserializer (script.GetPath ())));
	}
	catch (Win::InternalException e)
	{
		TheAlertMan.PostInfoAlert (e);
		return false; // continue with the script
	}

	bool done = false;
	for (DispatcherScript::CommandIter command = addendums->begin (); 
		command != addendums->end (); ++command)
	{
		std::unique_ptr<DispatcherCmdExec> cmdExec (DispatcherCmdExec::CreateCmdExec (**command));
		cmdExec->GetParams (script, _config.GetHubId ());
		DispatcherCmdExec::CmdResult result = DispatcherCmdExec::Continue;
		if (cmdExec.get () != 0)
		{
			result = cmdExec->Do (_addressDb, _cmdExecutor, _config);
		}
		switch (result)
		{
		case DispatcherCmdExec::Delete:
			DeleteScript (script.GetName ());
			done = true;
			break;
		case DispatcherCmdExec::Ignore:
			_scriptQuarantine.Insert (script.GetName (), (*command)->GetErrorString ());
			break;
		default:
			Assert (DispatcherCmdExec::Continue == result);
		};
	}
	return done;
}

void PublicInbox::ForwardDispatcherScript (std::unique_ptr<ScriptTicket> scriptTicket, 
										   Address const & sender, 
										   MailTruck & truck)		
{
	Topology topology = _config.GetTopology ();
	Assert (scriptTicket->GetAddresseeCount () > 0);
	ScriptInfo const & scriptInfo = scriptTicket->GetInfo();
	ScriptInfo::RecipientInfo const & addressee = scriptInfo.GetAddressee(0);
	// revisit: is it possible that in hub->hub some recipients are stamped and others not?
	ScriptTicket & truckScript = truck.PutScript (std::move(scriptTicket));

	if (truckScript.IsStamped(0))
		return;

	bool done = false;

	if (sender.IsHubDispatcher ())
	{
		// hub -> sat
		if (addressee.IsSatDispatcher ())
		{
			Assert (truckScript.GetAddresseeCount () == 1);
			if (topology.HasSat ()) // we are the sender
			{
				ShipToSatellites (truckScript, truck);
				done = true;
			}
		}
		else if (addressee.IsHubDispatcher ()) // hub -> hub
		{
			if (truckScript.ToBeFwd ()) // we are the sender
			{
				ShipToHubs (truckScript, truck);
				done = true;
			}
		}
		// wildcard recipient, proces here and remove!
	}
	else if (sender.IsSatDispatcher ())
	{
		// sat -> hub
		Assert (truckScript.GetAddresseeCount () == 1);
		if (addressee.IsHubDispatcher ())
		{
			if (topology.HasHub ()) // we are the sender
			{
				truckScript.AddRequest(0, _config.GetActiveTransportToHub ());
				done = true;
			}
		}
		else
			throw Win::Exception ("Script sent by a satellite, but not addressed to a hub");
	}
	if (!done) // we are not the sender
	{
		ExecAttachments(truckScript);
		truckScript.StampDelivery (0);
	}
}

void PublicInbox::ShipToSatellites (ScriptTicket & script, MailTruck & truck)
{
	Assert (script.GetAddresseeCount () == 1);
	Assert (script.GetAddressee (0).IsDispatcher ());
	std::set<Transport> transports;
	_addressDb.GetActiveSatelliteTransports (transports);
	if (transports.empty ())
	{
		script.StampDelivery (0);
		return;
	}
	dbg << "Ship to satellites" << std::endl;
	for (std::set<Transport>::iterator it = transports.begin ();
		it != transports.end ();
		++it)
	{
		dbg << "    " << *it << std::endl;
		script.AddRequest(0, *it);
	}
}

void PublicInbox::ShipToHubs (ScriptTicket & script, MailTruck & truck)
{
	for (int i = 0; i < script.GetAddresseeCount (); ++i)
	{
		ScriptInfo::RecipientInfo const & addressee = script.GetAddressee (i);
		Assert (addressee.IsDispatcher ());
		Transport transport;
		_addressDb.GetAskRemoteTransport (addressee.GetHubId (), transport);
		if (transport.IsUnknown ())
		{
			// Pretty rare case. 
			// We don't know the transport and a user ignored our prompt for it.
			_scriptQuarantine.Insert (script.GetName (), "Unknown transport to a remote hub");
		}
		else
			script.AddRequest(i, transport);
	}
}

