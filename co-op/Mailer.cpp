//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "Mailer.h"
#include "GlobalId.h"
#include "TransportHeader.h"
#include "ScriptTrailer.h"
#include "ScriptHeader.h"
#include "ScriptCommandList.h"
#include "ScriptList.h"
#include "DispatcherScript.h"
#include "MemberInfo.h"
#include "ProjectDb.h"
#include "Catalog.h"
#include "ScriptIo.h"
#include "ScriptName.h"
#include "UserIdPack.h"
#include "Addressee.h"
#include "Serialize.h"

#include <File/File.h>
#include <File/Path.h>
#include <File/SafePaths.h>

void ScriptMailer::SaveSingleRecipientScripts (ScriptBuilder & builder,
											   TransportHeader & txHdr,
											   AddresseeList const & recipients)
{
	for (AddresseeList::ConstIterator iter = recipients.begin (); iter != recipients.end (); ++iter)
	{
		Addressee const & recipient = *iter;
		AddresseeList singleRecipient;
		singleRecipient.push_back (recipient);
		txHdr.AddRecipients (singleRecipient);
		UserIdPack pack (recipient.GetStringUserId ());
		std::string tag ("user id-");
		tag += pack.GetUserIdString ();
		builder.Save (tag);
	}
}

// Returns true when script has to be send separately to each project member
bool ScriptMailer::XBuildTxHdr (TransportHeader & txHdr, DispatcherScript const * dispatcherScript)
{
	bool senderIsDistributor = _projectDb.XAddSender (txHdr);
	bool useBccRecipients = senderIsDistributor && _projectDb.XUseBccRecipients ();
	txHdr.SetBccRecipients (useBccRecipients);
	if (dispatcherScript != 0)
	{
		txHdr.SetDispatcherAddendum (dispatcherScript->CmdCount () > 0);
		txHdr.SetInvitation (dispatcherScript->IsInvitation ());
	}
	return senderIsDistributor && !useBccRecipients;
}

// Returns true when script has to be send separately to each project member
bool ScriptMailer::BuildTxHdr (TransportHeader & txHdr, DispatcherScript const * dispatcherScript)
{
	bool senderIsDistributor = _projectDb.AddSender (txHdr);
	bool useBccRecipients = senderIsDistributor && _projectDb.UseBccRecipients ();
	txHdr.SetBccRecipients (useBccRecipients);
	txHdr.SetDispatcherAddendum (dispatcherScript != 0 && dispatcherScript->CmdCount () > 0);
	return senderIsDistributor && !useBccRecipients;
}

// This method is to be obsoleted
void ScriptMailer::XBroadcast (ScriptHeader & hdr,
							   CommandList const & cmdList, 
							   CheckOut::List const * checkOutNotifications)
{
	// Always called during transaction
	TransportHeader txHdr (hdr.ScriptId (), hdr.IsPureControl (), hdr.IsDefectOrRemove ());
	bool singleRecipientScript = XBuildTxHdr (txHdr, 0);

	AddresseeList recipients;
	_projectDb.XRetrieveBroadcastRecipients (recipients);
	Assert (recipients.size () != 0);

	ScriptBuilder builder (&_catalog, &hdr);
	builder.AddTransportHeader (&txHdr);
	builder.AddCommandList (&cmdList);
	builder.AddCheckoutNotification (checkOutNotifications);

	if (singleRecipientScript)
	{
		SaveSingleRecipientScripts (builder, txHdr, recipients);
	}
	else
	{
		// Save just one script for all recipients
		txHdr.AddRecipients (recipients);
		builder.Save ("all");
	}
}

void ScriptMailer::Broadcast (ScriptHeader & hdr,
							  CommandList const & cmdList, 
							  CheckOut::List const * checkOutNotifications)
{
	TransportHeader txHdr (hdr.ScriptId (), hdr.IsPureControl (), hdr.IsDefectOrRemove ());
	bool singleRecipientScript = BuildTxHdr (txHdr, 0);

	AddresseeList recipients;
	_projectDb.RetrieveBroadcastRecipients (recipients);
	Assert (recipients.size () != 0);

	ScriptBuilder builder (&_catalog, &hdr);
	builder.AddTransportHeader (&txHdr);
	builder.AddCommandList (&cmdList);
	builder.AddCheckoutNotification (checkOutNotifications);

	dbg << "Broadcast: \n" << recipients;
	if (singleRecipientScript)
	{
		SaveSingleRecipientScripts (builder, txHdr, recipients);
	}
	else
	{
		// Save just one script for all recipients
		txHdr.AddRecipients (recipients);
		builder.Save ("all");
	}
}

std::string ScriptMailer::BroadcastForcedDefect (ScriptHeader & hdr,
												 CommandList const & cmdList,
												 FilePath & scriptPath,
												 int projectId)
{
	TransportHeader txHdr (hdr.ScriptId (), hdr.IsPureControl (), hdr.IsDefectOrRemove ());
	std::string userId (_catalog.GetUserId (projectId));
	Assert (hdr.GetProjectName () == _catalog.GetProjectName (projectId));
	std::string hubId (_catalog.GetHubId ());
	txHdr.AddSender (Address (hubId.c_str (), hdr.GetProjectName ().c_str (), userId.c_str ()));
	ScriptBuilder builder (&_catalog, &hdr);
	builder.AddTransportHeader (&txHdr);
	builder.AddCommandList (&cmdList);
	// very special case
	ScriptFileName fileName (hdr.ScriptId (),
							 "all",
							 hdr.GetProjectName ());
	std::string name = fileName.Get ();
	builder.Save (scriptPath, name);
	return scriptPath.GetFilePath (name);
}

void ScriptMailer::Multicast (ScriptHeader & hdr,
							  CommandList const & cmdList,
							  AddresseeList const & recipients,
							  CheckOut::List const * notification,
							  DispatcherScript const * dispatcherScript)
{
	// Always called outside transaction
	Assert (_projectDb.GetMyId () != gidInvalid);
	Assume (recipients.size () != 0, "ScriptMailer::Multicast called with empty recipient list");
	TransportHeader txHdr (hdr.ScriptId (), hdr.IsPureControl (), hdr.IsDefectOrRemove ());
	bool singleRecipientScript = BuildTxHdr (txHdr, dispatcherScript);

	ScriptBuilder builder (&_catalog, &hdr);
	builder.AddTransportHeader (&txHdr);
	builder.AddCommandList (&cmdList);
	builder.AddDispatcherScript (dispatcherScript);
	builder.AddCheckoutNotification (notification);

	dbg << "Multicast:\n" << recipients;
	if (singleRecipientScript || recipients.size () == 1)
	{
		SaveSingleRecipientScripts (builder, txHdr, recipients);
	}
	else
	{
		txHdr.AddRecipients (recipients);
		builder.Save ("some members");
	}
}

void ScriptMailer::XMulticast (ScriptHeader & hdr,
							   CommandList const & cmdList,
							   GidSet const & filterOut,
							   CheckOut::List const * notification,
							   DispatcherScript const * dispatcherScript)
{
	Assert (_projectDb.XGetMyId () != gidInvalid);
	TransportHeader txHdr (hdr.ScriptId (), hdr.IsPureControl (), hdr.IsDefectOrRemove ());
	bool singleRecipientScript = XBuildTxHdr (txHdr, dispatcherScript);

	AddresseeList recipients;
	_projectDb.XRetrieveMulticastRecipients (filterOut, recipients);
	if (recipients.size () == 0)
		return;

	ScriptBuilder builder (&_catalog, &hdr);
	builder.AddTransportHeader (&txHdr);
	builder.AddCommandList (&cmdList);
	builder.AddDispatcherScript (dispatcherScript);
	builder.AddCheckoutNotification (notification);

	if (singleRecipientScript || recipients.size () == 1)
	{
		SaveSingleRecipientScripts (builder, txHdr, recipients);
	}
	else
	{
		txHdr.AddRecipients (recipients);
		builder.Save ("some members");
	}
}

void ScriptMailer::XFutureMulticast (
			ScriptHeader & hdr,
			CommandList const & cmdList,
			GidSet const & filterOut,
			DispatcherScript const & dispatcherScript,
			UserId senderId,
			std::string & scriptFilename,
			std::vector<unsigned char> & script)
{
	TransportHeader txHdr (hdr.ScriptId (), hdr.IsPureControl (), hdr.IsDefectOrRemove ());
	txHdr.SetBccRecipients (false);
	txHdr.SetDispatcherAddendum (true);
	std::unique_ptr<MemberDescription> sender = _projectDb.XRetrieveMemberDescription (senderId);
	Address senderAddress (sender->GetHubId (), _projectDb.XProjectName (), sender->GetUserId ());
	txHdr.AddSender (senderAddress);

	AddresseeList recipients;
	_projectDb.XRetrieveMulticastRecipients (filterOut, recipients);
	Assert (recipients.size () > 0);
	txHdr.AddRecipients (recipients);

	ScriptBuilder builder (&_catalog, &hdr);
	builder.AddTransportHeader (&txHdr);
	builder.AddCommandList (&cmdList);
	builder.AddDispatcherScript (&dispatcherScript);

	CountingSerializer countingSerializer;
	builder.Save (countingSerializer);
	script.resize (static_cast<unsigned>(countingSerializer.GetSize()));
	MemorySerializer memSerializer (&script [0], script.size ());
	builder.Save (memSerializer);

	std::string tag;
	if (recipients.size () == 1)
	{
		UserIdPack pack (recipients [0].GetStringUserId ());
		tag = "user id-";
		tag += pack.GetUserIdString ();
	}
	else
		tag = "some members";

	ScriptFileName fileName (hdr.ScriptId (), tag, hdr.GetProjectName ());
	scriptFilename = fileName.Get ();
}

void ScriptMailer::XUnicast (ScriptHeader & hdr,
							 CommandList const & cmdList,
							 MemberDescription const & recipient,
							 CheckOut::List const * notification,
							 DispatcherScript const * dispatcherScript)
{
	// Note: ScriptHeader contains chunk information (for re-send requests)
	Assert (_projectDb.XGetMyId () != gidInvalid);
	TransportHeader txHdr (hdr.ScriptId (), hdr.IsPureControl (), hdr.IsDefectOrRemove ());
	XBuildTxHdr (txHdr, dispatcherScript);

	Addressee addressee (recipient.GetHubId (), recipient.GetUserId ());
	AddresseeList recipients;
	recipients.push_back (addressee);
	txHdr.AddRecipients (recipients);

	ScriptBuilder builder (&_catalog, &hdr);
	builder.AddTransportHeader (&txHdr);
	builder.AddCommandList (&cmdList);
	builder.AddDispatcherScript (dispatcherScript);
	builder.AddCheckoutNotification (notification);

	MemberNameTag tag (recipient.GetName (), recipient.GetUserId ());
	dbg << "Unicast \"" << hdr.GetComment () << "\" to " << tag << std::endl;
	builder.Save (tag);
}

void ScriptMailer::XUnicast (std::vector<unsigned> const & chunkList,
							 ScriptHeader & hdr,
							 ScriptList const & scriptList,
							 MemberDescription const & recipient,
							 CheckOut::List const * notification)
{
	// Note: ScriptHeader contains chunk information (for re-send requests)
	Assert (_projectDb.XGetMyId () != gidInvalid);
	TransportHeader txHdr (hdr.ScriptId (), hdr.IsPureControl (), hdr.IsDefectOrRemove ());
	
	XBuildTxHdr (txHdr, 0);	// No Dispatcher script

	Addressee addressee (recipient.GetHubId (), recipient.GetUserId ());
	AddresseeList recipients;
	recipients.push_back (addressee);
	txHdr.AddRecipients (recipients);

	ScriptBuilder builder (&_catalog, &hdr);
	builder.AddTransportHeader (&txHdr);
	builder.AddScriptList (&scriptList);
	builder.AddCheckoutNotification (notification);

	MemberNameTag tag (recipient.GetName (), recipient.GetUserId ());
	builder.Save (chunkList, tag);
}

void ScriptMailer::XUnicast (ScriptHeader & hdr,
							 ScriptList const & scriptList,
							 MemberDescription const & recipient,
							 CheckOut::List const * notification,
							 ScriptTrailer const * trailer,
							 DispatcherScript const * dispatcherScript)
{
	// Always called during transaction
	Assert (_projectDb.GetMyId () != gidInvalid);
	TransportHeader txHdr (hdr.ScriptId (), hdr.IsPureControl (), hdr.IsDefectOrRemove ());
	XBuildTxHdr (txHdr, dispatcherScript);

	Addressee addressee (recipient.GetHubId (), recipient.GetUserId ());
	AddresseeList recipients;
	recipients.push_back (addressee);
	txHdr.AddRecipients (recipients);

	ScriptBuilder builder (&_catalog, &hdr);
	builder.AddTransportHeader (&txHdr);
	builder.AddScriptList (&scriptList);
	builder.AddScriptTrailer (trailer);
	builder.AddDispatcherScript (dispatcherScript);
	builder.AddCheckoutNotification (notification);

	MemberNameTag tag (recipient.GetName (), recipient.GetUserId ());
	builder.Save (tag);
}

void ScriptMailer::UnicastJoin (ScriptHeader & hdr,
								CommandList const & cmdList,
								MemberDescription const & sender,
								MemberDescription const & admin,
								DispatcherScript const * dispatcherScript)
{
	// Always called outside transaction
	TransportHeader txHdr (hdr.ScriptId (), hdr.IsPureControl (), hdr.IsDefectOrRemove ());
	txHdr.SetDispatcherAddendum (dispatcherScript != 0 && dispatcherScript->CmdCount () > 0);
	txHdr.AddSender (Address (sender.GetHubId (), hdr.GetProjectName (), sender.GetUserId ()));
	Addressee addressee (admin.GetHubId (), admin.GetUserId ());
	AddresseeList recipients;
	recipients.push_back (addressee);
	txHdr.AddRecipients (recipients);

	ScriptBuilder builder (&_catalog, &hdr);
	builder.AddTransportHeader (&txHdr);
	builder.AddCommandList (&cmdList);
	builder.AddDispatcherScript (dispatcherScript);
	builder.Save (admin.GetName ());
}


void ScriptMailer::ForwardJoinRequest (std::string const & scriptPath)
{
	Assert (_projectDb.GetAdminId () != gidInvalid);
	{
		// Deserializer (in) and serializer (out) scope
		FileDeserializer in (scriptPath);
		File::Size scriptFileSize = in.GetSize ();
		Assert (!scriptFileSize.IsLarge ());
		TransportHeader inTxHdr;
		DispatcherScript addendum;
		unsigned txHdrsize = static_cast<unsigned>(inTxHdr.Read (in));
		Assert (inTxHdr.IsValid ());
		unsigned addendumSize = static_cast<unsigned>(addendum.Read (in));
		Assert (addendumSize != 0);
		// Get rid of commands relating to sender's cluster
		if (!IsNocaseEqual (inTxHdr.GetSenderAddress ().GetHubId (), _catalog.GetHubId ()))
		{
			DispatcherScript::CommandIter it = addendum.begin ();
			while (it != addendum.end ())
			{
				DispatcherCmd * cmd = *it;
				if (cmd->IsForSendersCluster ())
					it = addendum.erase (it);
				else
					++it;
			}
		}
		// Prepare transport header for the forwarded script
		TransportHeader forwardTxHdr (inTxHdr.GetScriptId (), inTxHdr.IsControlScript (), inTxHdr.IsDefectScript ());
		forwardTxHdr.SetDispatcherAddendum (true);
		std::unique_ptr<MemberDescription> sender = _projectDb.RetrieveMemberDescription (_projectDb.GetMyId ());
		std::unique_ptr<MemberDescription> admin = _projectDb.RetrieveMemberDescription (_projectDb.GetAdminId ());
		forwardTxHdr.AddSender (Address (sender->GetHubId (), _projectDb.ProjectName (), sender->GetUserId ()));
		Addressee addressee (admin->GetHubId (), admin->GetUserId ());
		AddresseeList recipients;
		recipients.push_back (addressee);
		forwardTxHdr.AddRecipients (recipients);

		// Create forwarded script file name
		ScriptFileName scriptName (inTxHdr.GetScriptId (), "project administrator", _projectDb.ProjectName ());
		std::string forwardScriptName (scriptName.Get ());
		FilePath publicInbox (_catalog.GetPublicInboxDir ());

		// Save forwarded script in the public inbox preseving original join request format
		FileSerializer out (publicInbox.GetFilePath (forwardScriptName.c_str ()));
		forwardTxHdr.Save (out);

		unsigned long joinRequestSize = scriptFileSize.Low () - (txHdrsize + addendumSize);
		Assert (joinRequestSize != 0);
		std::vector<unsigned char> buf;
		buf.resize (joinRequestSize);
		File::Offset seekOffset (txHdrsize, 0);
		in.SetPosition (seekOffset);
		in.GetBytes (&buf [0], joinRequestSize);
		out.PutBytes (&buf [0], joinRequestSize);

		addendum.Save (out);
	}

	// All files are closed now -- delete incoming join request
	File::DeleteNoEx (scriptPath.c_str ());
}

std::string ScriptMailer::XSaveInTmpFolder (ScriptHeader & hdr,
											ScriptList const & scriptList,
											MemberDescription const & recipient,
											ScriptTrailer const & trailer,
											DispatcherScript const & dispatcherScript)
{
	// Always called during transaction
	Assert (_projectDb.GetMyId () != gidInvalid);
	TransportHeader txHdr (hdr.ScriptId (), hdr.IsPureControl (), hdr.IsDefectOrRemove ());
	XBuildTxHdr (txHdr, &dispatcherScript);
	// Turn off the script's forwarding flag, so the recipient's dispatcher
	// doesn't complain about "new satellite user" 
	txHdr.SetForward (false);

	Addressee addressee (recipient.GetHubId (), recipient.GetUserId ());
	AddresseeList recipients;
	recipients.push_back (addressee);
	txHdr.AddRecipients (recipients);

	ScriptBuilder builder (&_catalog, &hdr);
	builder.AddTransportHeader (&txHdr);
	builder.AddScriptList (&scriptList);
	builder.AddScriptTrailer (&trailer);
	builder.AddDispatcherScript (&dispatcherScript);

	MemberNameTag tag (recipient.GetName (), recipient.GetUserId ());
	ScriptFileName fileName (hdr.ScriptId (), tag, hdr.GetProjectName ());
	SafeTmpFile tmpFilePath (fileName.Get ());
	FileSerializer out (tmpFilePath.GetFilePath ());
	builder.Save (out);
	tmpFilePath.Commit ();
	return fileName.Get ();
}
