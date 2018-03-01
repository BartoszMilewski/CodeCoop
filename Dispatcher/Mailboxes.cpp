// -----------------------------------
// (c) Reliable Software, 1999 -- 2006
// Specializations of Mailbox class
// -----------------------------------
#include "precompiled.h"
#undef DBG_LOGGING
#define DBG_LOGGING true
#include "Mailboxes.h"
#include "AddressDb.h"
#include "Recipient.h"
#include "Addressee.h"
#include "UserIdPack.h"
#include "MailTruck.h"
#include "ScriptInfo.h"
#include "ScriptQuarantine.h"
#include "Config.h"
#include "ConfigData.h"
#include "WorkQueue.h"
#include "DispatcherScript.h"
#include "DispatcherExec.h"

#include "Dialogs.h"
#include "FwdFolderDlg.h"
#include "OutputSink.h"
#include "Prompter.h"
#include "DispatcherParams.h"
#include "resource.h"

#include <LightString.h> // class Msg

bool InvitationIsLess (ScriptTicket const * s1, ScriptTicket const * s2)
{
	return !s1->IsInvitation () && s2->IsInvitation ();
}

// ===============================
// ProcessScripts returns false when there still are some 
// outgoing scripts left in the mailbox
bool PublicInbox::ProcessScripts (MailTruck & truck, 
								  WorkQueue & workQueue)
{
	Assert (_status == stOk);

	// List the directory
	NocaseSet deletedScripts;
	UpdateFromDisk (deletedScripts);
	_scriptQuarantine.Delete (deletedScripts);
	// Prune the list
	std::vector<std::string> fileNameList;
	workQueue.GetNewAndMarkOldScripts(fileNameList);
	if (fileNameList.empty())
		return true;

	// Until the end of this method, this thread owns the files on disk,
	// and the ScriptList

	ScriptVector scriptList;
	bool accessDenied = ReadTransportHeaders (fileNameList, scriptList);
	std::sort (scriptList.begin (), scriptList.end (), InvitationIsLess);

	dbg << "PublicInbox::ProcessScripts" << std::endl;

	// May null scripts in scriptList
	ProcessAttachments(scriptList, truck);

	while (scriptList.size () != 0)
	{
		std::unique_ptr<ScriptTicket> originalScript (scriptList.pop_back ());
		if (originalScript.get() == 0)
			continue;

		dbg << " o " << originalScript->GetName() << std::endl;

		if (_scriptQuarantine.FindModuloChunkNumber (originalScript->GetName ()))
		{
			dbg << "  Script found in Quarantine: " << originalScript->GetName() << std::endl;
			continue;
		}
		if (!VerifyScriptSource (originalScript->GetInfo()))
			continue;

		// -- Transfer ownership to truck, originalScript is invalidated, --
		ScriptTicket & scriptTicket = truck.PutScript (std::move(originalScript));
		ScriptInfo const & scriptInfo = scriptTicket.GetInfo();

		// For each addresse, find recipient address and either copy locally or attach 
		// a request to ScriptTicket
		for (int addrIdx = 0; addrIdx < scriptInfo.GetAddresseeCount (); addrIdx++)
		{
			ScriptInfo::RecipientInfo const & addressee = scriptInfo.GetAddressee (addrIdx);
			dbg << "    > " << addressee.GetUserId() << " @ " << addressee.GetHubId() << std::endl;
			Recipient * recip = FindRecipient (scriptInfo, addressee);
			// In what follows, we either stamp recipient or add transport request to ScriptTicket
			if (recip) 
			{
				// AcceptScript checks script->IsStamped(addrIdx) 
				if (!recip->AcceptScript (scriptTicket, truck, addrIdx))
				{
					// Revisit: this code is too specific for this level
					// Script was not accepted.
					// This can happen if the recipient is in local cluster,
					// we have him registered as cluster recipient, 
					// but we don't know his forwarding path.
					// This in turn may happen when a new project is created on a satellite.
					// A satellite's Co-op sends a special "New Project Announcement" script 
					// to inform the hub of a new cluster recipient. The script contains 
					// recipient address, but need not always contain his forwarding path.
					// Also, an already registered satellite member's address may be deleted 
					// from the database as an effect of a user action.
					// Display dialog about unknown forward path
					UserIdPack idPack (recip->GetUserId ());
					FwdFolderData dlgData (recip->GetHubId ().c_str (), 
										   idPack.GetUserIdString ().c_str (),
										   recip->GetProjectName ().c_str (),
										   scriptInfo.GetComment ().c_str ());
					FwdFolderCtrl ctrl (dlgData);
					if (ThePrompter.GetData (ctrl, "Unknown forwarding path to a satellite"))
					{
						_addressDb.UpdateTransport (*recip, dlgData.GetTransport ());
						recip->AcceptScript (scriptTicket, truck, addrIdx);
					}
					else
					{
						_scriptQuarantine.Insert (scriptInfo.GetName (), "Unknown forwarding path to a satellite");
						truck.RemoveScript(scriptTicket);
					}
				}
			}
			else if (!scriptTicket.IsStamped(addrIdx))
			{
				// If the recipient not found in the address database, he seems to be remote.
				// However, before we assume that he is remote, hub and standalone public inboxes
				// perform a few checks. Just in case.
				ScriptInfo::RecipientInfo const & addressee = scriptInfo.GetAddressee (addrIdx);
				UserIdPack idPack (scriptInfo.GetSenderUserId ());
				bool const isJoinRequest = idPack.IsRandom ();

				AddresseeType::Bits addrType = AddresseeType::Unknown;

				if (IsNocaseEqual (_config.GetHubId (), addressee.GetHubId ()))
				{
					addrType = ConfirmRemoteWithKnownHub (scriptTicket, addrIdx, isJoinRequest);
				}
				else
				{
					addrType = ConfirmRemoteWithUnknownHub (scriptTicket, addrIdx, isJoinRequest);
				}

				if (addrType == AddresseeType::Remote)
				{
					DispatchToRemote (scriptTicket, addrIdx, truck);
				}
				else if (addrType == AddresseeType::Local)
				{
					DispatchToLocal (scriptTicket, addrIdx, truck);
				}
				else
				{
					Assert (addrType == AddresseeType::Unknown);
					// Delete/Ignore dialog will be displayed
					// at end of processing run, if the recipient was not stamped
					scriptTicket.MarkAddressingError (addrIdx);
				}
			}
		}
	}
	// come back and process mailbox again
	// until there are no new valid scripts
	return false;
}

// Returns true if couldn't read some scripts due to access violations
bool PublicInbox::ReadTransportHeaders (std::vector<std::string> & fileNameList, ScriptVector & scriptList)
{
	bool accessDenied = false;
	std::vector<std::string> badScripts;

	{
		for (std::vector<std::string>::const_iterator it = fileNameList.begin(); it != fileNameList.end(); ++it)
		{
			if (_scriptQuarantine.Find (*it))
				continue;
			ScriptStatus status = _scriptFiles.GetStatus(*it);
			std::unique_ptr<ScriptInfo> scriptInfo;
			try
			{
				scriptInfo.reset (new ScriptInfo (GetPath(), *it));
				if (scriptInfo->HasHeader ())
				{
					if (scriptInfo->IsHeaderValid ())
					{
						std::unique_ptr<ScriptTicket> script(new ScriptTicket(*scriptInfo));
						scriptList.push_back (std::move(script));
					}
					else
					{
						badScripts.push_back (scriptInfo->GetName ());
						status.SetIsCorrupted ();
					}
				}
				else
				{
					badScripts.push_back (scriptInfo->GetName ());
					status.SetNoHeader ();
				}
			}
			catch (...)
			{
				Win::ClearError ();
				status.SetAccessDenied ();
				accessDenied = true;
			}
			_scriptFiles.SetStatus(*it, status);
		}
	}

	for (std::vector<std::string>::iterator it = badScripts.begin ();
		 it != badScripts.end ();
		 ++it)
	{
		_scriptQuarantine.Insert (*it, "Script is corrupted");
		Msg msg;
		msg << "has corrupted information about its recipients (or unknown version number).";

		// Revisit: add Delete option in this dialog?
		BadScriptData data ("Corrupted script", it->c_str (), 
							_path.GetDir (), msg.c_str ()); 
		BadScriptCtrl ctrl (&data);
		ThePrompter.GetData (ctrl, "Corrupted script received");
	}
	return accessDenied;
}


Recipient * PublicInbox::FindRecipient (
				ScriptInfo const & script,
				ScriptInfo::RecipientInfo const & addressee) const
{
	Recipient * recip = 0;
	if (addressee.HasWildcardUserId ())
	{
		recip = _addressDb.FindWildcard (
								addressee.GetHubId (), 
								script.GetProjectName (), 
								script.GetSenderHubId (),
								script.GetSenderUserId ());
	}
	else
	{
		recip = _addressDb.Find (Address (addressee.GetHubId (), 
								 script.GetProjectName (), 
								 addressee.GetUserId ()));
	}
	return recip;
}

// ============================
// Returns true, if further 
// processing required
bool HubPublicInbox::VerifyScriptSource (ScriptInfo const & script)
{
	Address const & sender (script.GetSender ());
	if (sender.GetProjectName ().empty () || sender.GetHubId ().empty ())
		return false;

	// register unknown cluster sender
	if (script.IsFromThisCluster (_config.GetHubId ()) &&
		!script.IsDefect ())
	{
		if (!_addressDb.Find (sender))
		{
			// New cluster recipient forwards a script through the hub for the first time
			// ask him for a return path to his machine
			// Revisit: propose some default forwarding path to the user
			UserIdPack idPack (sender.GetUserId ());
			FwdFolderData dlgData (sender.GetHubId ().c_str (), 
								   idPack.GetUserIdString ().c_str (),
								   sender.GetProjectName ().c_str (),
								   script.GetComment ().c_str ());
			FwdFolderCtrl ctrl (dlgData, true);	// Join
			if (ThePrompter.GetData (ctrl, "New satellite sender missing forwarding path"))
			{
				_addressDb.AddClusterRecipient (sender, dlgData.GetTransport ());
				_addressDb.KillClusterRecipient (Address (	sender.GetHubId (), 
															sender.GetProjectName (), 
															std::string ("*")));
			}
			else
			{
				_scriptQuarantine.Insert (script.GetName (), "New satellite sender missing forwarding path");
				return false;
			}
		}
	}
	return true;
}

void HubPublicInbox::DispatchToRemote (ScriptTicket & script, int addrIdx, MailTruck & truck)
{
	if (script.ToBeFwd ())
	{
		std::string const & hubId = script.GetAddressee (addrIdx).GetHubId();
		Transport transport;
		_addressDb.GetAskRemoteTransport (hubId, transport);
		if (transport.IsUnknown ())
		{
			// Pretty rare case. 
			// We don't know the transport and a user ignored our prompt for it.
			_scriptQuarantine.Insert (script.GetName (), "Unknown transport to a remote hub");
		}
		else
			script.AddRequest(addrIdx, transport);
	}
	else
		script.MarkPassedOver (addrIdx);
}

// called with known hubId or in non-email config
void HubPublicInbox::DispatchToLocal (ScriptTicket & script, int addrIdx, MailTruck & truck)
{
	// the address must just have been added to DB
	ScriptInfo::RecipientInfo const & addressee = script.GetAddressee (addrIdx);
	Assert (!script.IsStamped(addrIdx));
	Recipient * recip = FindRecipient (script.GetInfo(), addressee);
	Assert (recip != 0);
	recip->AcceptScript (script, truck, addrIdx);
}

// ==================================
void SatPublicInbox::DispatchToRemote (ScriptTicket & script, int addrIdx, MailTruck & truck)
{
	if (script.ToBeFwd ())
	{
		// for further forwarding
		script.AddRequest(addrIdx, _config.GetActiveTransportToHub ());
		_addressDb.VerifyAddRemoteHub (script.GetAddressee (addrIdx).GetHubId());
	}
	else
		script.MarkPassedOver (addrIdx);
}

// called only with known hubId
void SatPublicInbox::DispatchToLocal (ScriptTicket & script, int addrIdx, MailTruck & truck)
{
	if (script.ToBeFwd ())
	{
		// for distribution to local cluster
		script.AddRequest(addrIdx, _config.GetActiveTransportToHub ());
	}
	else
		script.MarkPassedOver (addrIdx);
}

void ProxyHubPublicInbox::DispatchToRemote (ScriptTicket & script, int addrIdx, MailTruck & truck)
{
	if (script.ToBeFwd ())
	{
		std::string const & hubId = script.GetAddressee (addrIdx).GetHubId();
		Transport transport;
		_addressDb.GetAskRemoteTransport (hubId, transport);
		if (transport.IsUnknown ())
		{
			// Pretty rare case. 
			// We don't know the transport and a user ignored our prompt for it.
			_scriptQuarantine.Insert (script.GetName (), "Unknown transport to a remote hub");
		}
		else
			script.AddRequest(addrIdx, transport);
	}
	else
		script.MarkPassedOver (addrIdx);
}

// called only with known hubId
void ProxyHubPublicInbox::DispatchToLocal (ScriptTicket & script, int addrIdx, MailTruck & truck)
{
	script.AddRequest(addrIdx, _config.GetActiveTransportToHub ());
}

// Return true when script is a Join Request and either has
// - forward flag reset and nonempty forwarding path -- machine is to be satellite,
//   we must know a hub path,
// - forward flag set -- machine is to be a hub
bool StandalonePublicInbox::IsIncomingJoinWithTransport (
		ScriptTicket const & script,
		Transport & hubTransport)
{
	if (script.GetAddresseeCount () != 1)
		return false;
	
	ScriptInfo::RecipientInfo const & addressee = script.GetAddressee (0);
	if (!script.IsStamped(0) && addressee.HasWildcardUserId ())
	{
		if (addressee.IsDispatcher ())
			return false;

		if (_addressDb.FindLocal (script.GetSender ()))
		{
			return false;
		}

		if (script.ToBeFwd ())
			return true;

		if (script.HasDispatcherAddendum ())
		{
			// retrieve hub transport required for reconfiguration
			// don't care for executing attachments -- they will be processed after reconfig
			FileDeserializer in (script.GetPath ());
			DispatcherScript addendums (in);
			for (DispatcherScript::CommandIter command = addendums.begin (); 
				 command != addendums.end ();
				 ++command)
			{
				if ((*command)->GetType () == typeAddMember)
				{
					AddMemberCmd const * cmd = 
						dynamic_cast<AddMemberCmd const *>(*command);
					
					hubTransport = cmd->TransportMethod (); 
					return !hubTransport.IsUnknown ();
				}
			}
		}
	}
	return false;
}

// =============================
// Table interface
void PublicInbox::QueryUniqueIds (
			std::vector<int>& uids,
			Restriction const * restrict)
{
	Assert (0 == restrict);
	_restriction = 0;
	uids.push_back (GetId ()); 
}

std::string	PublicInbox::GetStringField (Column col, int uid) const
{
	Assert (0 == _restriction);
	if (colProjName == col)
		return _rowName;
	else if (colPath == col)
		return GetDir ();

	Assert (!"Invalid string column");
	return std::string ();
}
 
int	PublicInbox::GetNumericField (Column col, int uid) const
{
	Assert (0 == _restriction);
	switch (col)
	{
	case colScriptCount:
		return GetScriptCount ();
	case colStatus:
		return GetStatus ();
	};

	Assert (!"Invalid numeric column");
	return 0;
}

void PublicInbox::QueryUniqueNames (
			std::vector<std::string>& unames,
			Restriction const * restrict)
{
	Assert (restrict == 0);
	GetScriptFiles (unames);
}

std::string	PublicInbox::GetStringField (Column col, std::string const & uname) const
{
	Assert (0 == _restriction);
	Assert (Table::colComment == col);
	return GetScriptComment (uname);
}

int	PublicInbox::GetNumericField (Column col, std::string const & uname) const
{
	Assert (0 == _restriction);
	Assert (Table::colStatus == col);
	return GetScriptStatus (uname).GetValue ();
}

std::string PublicInbox::_viewCaption = "Public Inbox";
std::string PublicInbox::_rowName = "Public";

