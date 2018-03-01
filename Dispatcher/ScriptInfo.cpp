//----------------------------------
// (c) Reliable Software 1998 - 2008
// ---------------------------------

#include "precompiled.h"
#include "ScriptInfo.h"
#include "ScriptStatus.h"
#include "TransportHeader.h"
#include "DispatcherScript.h"
#include "AlertMan.h"

#include <Dbg/Assert.h>
#include <Ex/WinEx.h>
#include <StringOp.h>

ScriptInfo::ScriptInfo (FilePath const & mailboxPath, std::string name)
	: _fileName (name),
	  _size (0, 0),
	  _fullPath(mailboxPath.GetFilePath (name.c_str ()))
{
    FileDeserializer in (_fullPath);
	_size = in.GetSize ();
    std::unique_ptr<TransportHeader> header;
    try
    {
        header.reset (new TransportHeader (in));
		ScriptSubHeader subHdr (in);
		_comment = subHdr.GetComment ();
		_partNumber = subHdr.GetPartNumber ();
		_partCount = subHdr.GetPartCount ();
		_maxChunkSize = subHdr.GetMaxChunkSize ();
    }
	catch (Win::InternalException e)
	{
		TheAlertMan.PostInfoAlert (e);
		_flags.set (HeaderPresent, true);
		_flags.set (ValidHeader, false);
		return;
	}
    catch (...)
    {
		Win::ClearError ();
        _flags.set (HeaderPresent, true);
        _flags.set (ValidHeader, false);
        return;
    }

    _flags.set (HeaderPresent, !header->IsEmpty ());
    _flags.set (ValidHeader, header->IsValid ());
    if (!HasHeader () || !IsHeaderValid ())
        return;

    _flags.set (ToBeForwarded, header->ToBeForwarded ());
	_flags.set (DefectScript, header->IsDefectScript ());
	_flags.set (ControlScript, header->IsControlScript ());
    _flags.set (DispatcherAddendum, header->IsDispatcherAddendum ());
	_flags.set (BccRecipients, header->UseBccRecipients ());
	_flags.set (Invitation, header->IsInvitation ());
	_scriptId = header->GetScriptId ();
    _senderAddress.Set (header->GetSenderAddress ());

	AddresseeList const & recipients = header->GetRecipients ();
	for (AddresseeList::const_iterator it = recipients.begin (); it != recipients.end (); ++it)
	{
		_recipients.push_back(RecipientInfo(it->GetHubId(), it->GetStringUserId(), it->ReceivedScript()));
	}
}

bool ScriptInfo::operator== (ScriptInfo const & script) const
{
	return IsFileNameEqual (_fileName, script._fileName);
}

bool ScriptInfo::IsFromThisCluster (std::string const & thisHubId) const
{
	return ToBeFwd () || 
		   IsNocaseEqual (_senderAddress.GetHubId (), thisHubId);
}

ScriptTicket::ScriptTicket(ScriptInfo const & info)
	: _info(info),
	  _emailReqCount (0),
	  _fwdReqCount (0)
{
	// revisit: don't copy
	unsigned recipientCount = info._recipients.size ();
    _recipientFlags.resize (recipientCount);
    std::fill (_recipientFlags.begin (), _recipientFlags.end (), Unknown);

    _stampedRecipients.reserve (recipientCount);
	for (std::vector<ScriptInfo::RecipientInfo>::const_iterator it = info._recipients.begin();
		it != info._recipients.end(); ++it)
	{
		_stampedRecipients.push_back(it->IsDelivered());
	}
	Assert (RequestsTally ());
}

void ScriptTicket::MarkAddressingError (int idx) 
{ 
	_recipientFlags [idx] = Error; 
}

void ScriptTicket::MarkPassedOver (int idx)
{
	_recipientFlags [idx] = PassedOver;
}

void ScriptTicket::StampRecipient (int idx)
{
	if (File::Exists (GetPath ()))
	{
		File::MakeReadWrite (GetPath ());
		MemMappedHeader memHeader (GetPath ());
		memHeader.StampDelivery (idx);
	}
}

void ScriptTicket::StampDelivery (int idx)
{
    _flags.set (DeliveredToAnyone, true);
    _stampedRecipients [idx] = true;
	StampRecipient (idx);
}

void ScriptTicket::StampFinalDelivery (int idx)
{
    _flags.set (DeliveredToAnyone, true);
	if (AllRequestsDeliveredTo (idx))
	{
		_stampedRecipients [idx] = true;
		StampRecipient (idx);
	}
}

bool ScriptTicket::AllRequestsDeliveredTo (int idx)
{
	// For all requests to a given recipient
	std::pair<RequestIter, RequestIter> range = _requests.equal_range (idx);
	for (RequestIter it = range.first; it != range.second; ++it)
		if (!it->second.IsDelivered ())
			return false; // Not all requests successful!
	return true;
}


bool ScriptTicket::operator== (ScriptTicket const & script) const
{
	return _info.GetName() == script.GetInfo().GetName();
}

bool ScriptTicket::CanDelete () const
{
	// client can delete scripts 
	// 1) with no recipients (e.g. Membership Updates containing only dispatcher addendum(s))
	//	  but with one exception: we must not delete breakdown defects
	// 2) delivered to all recipients (non empty recipient list!)
	// 3) delivered to at last one recipient on this machine and with no unknown recipients
	if (GetAddresseeCount () == 0)
	{
		if (!IsDefect ())
			return true;		// 1)
	}
	else if (DeliveredToAll ())
	{
		return true;			// 2)
	}
	else if (DeliveredToAnyoneOnThisMachine ())
	{
		if (!HasUnknownNotStampedRecipients ())
			return true;	// 3)
	}
	return false;
}

bool ScriptTicket::DeliveredToAll () const
{
	Assert (GetAddresseeCount () > 0);
    for (unsigned int i = 0; i < _stampedRecipients.size (); i++)
    {
        if (!_stampedRecipients [i] && _recipientFlags [i] != PassedOver)
			return false;
    }
    return true;
}

bool ScriptTicket::HasUnknownNotStampedRecipients () const
{
	Assert (GetAddresseeCount () > 0);
    for (unsigned int i = 0; i < _stampedRecipients.size (); i++)
    {
        if (!_stampedRecipients [i] && _recipientFlags [i] != PassedOver)
        {
            return true;
        }
    }
    return false;
}

bool ScriptTicket::IsPassedOver (int idx) const
{
	Assert (0 <= idx && idx < (int)_recipientFlags.size ());
	return _recipientFlags [idx] == PassedOver;
}

bool ScriptTicket::IsAddressingError (int idx) const
{
	Assert (0 <= idx && idx < (int)_recipientFlags.size ());
	return _recipientFlags [idx] == Error;
}

bool ScriptTicket::RemovesMember (std::string const & hubId, std::string const & userId)
{
	std::unique_ptr<DispatcherScript> addendums (
		new DispatcherScript (FileDeserializer (GetPath ())));

	for (DispatcherScript::CommandIter cmdIter = addendums->begin (); 
		cmdIter != addendums->end (); ++cmdIter)
	{
		DispatcherCmd const * cmd = *cmdIter;
		AddressChangeCmd const * addressChange 
			= dynamic_cast<AddressChangeCmd const *> (cmd);
		if (addressChange != 0)
		{
			if (addressChange->NewHubId ().empty () 
				&& addressChange->NewUserId ().empty ()
				&& IsNocaseEqual (addressChange->OldHubId (), hubId)
				&& IsNocaseEqual (addressChange->OldUserId (), userId))
			{
				return true;
			}
		}
	}
	return false;
}

// May be called multiple times with the same recipIdx when broadcasting to all satellites
void ScriptTicket::AddRequest (int recipIdx, Transport const & transport)
{
	Transport::Method method = transport.GetMethod();
	Assume (method == Transport::Email || method == Transport::Network, "Unknown Script Transport");

	_requests.insert (std::make_pair (recipIdx, DispatchRequest (this, recipIdx, transport)));
	if (transport.IsEmail ())
		++_emailReqCount;
	else
		++_fwdReqCount;
	Assert (RequestsTally ());
}

std::string const & ScriptTicket::GetEmail(ScriptTicket::RequestIter ri) const
{
	DispatchRequest const & req = ri->second;
	if (req.IsFwdEmail())
	{
		return req.GetTransport().GetRoute ();
	}
	else
	{
		int idx = ri->first;
		return GetAddressee(idx).GetHubId();
	}
}

void ScriptTicket::MarkEmailReqDone (std::vector<std::string> const & addresses, bool isSuccess)
{
	for (RequestIter rit = _requests.begin(); rit != _requests.end(); ++rit)
	{
		int idx = rit->first;
		if (!rit->second.IsForward())
		{
			std::string const & email = GetEmail(rit);
			if (std::find(addresses.begin(), addresses.end(), email) != addresses.end())
			{
				rit->second.MarkTried ();
				if (isSuccess)
					rit->second.MarkDelivered();
				Assert (_emailReqCount != 0);
				--_emailReqCount;
			}
		}
	}
	Assert (RequestsTally ());
}

void ScriptTicket::MarkRequestDone (Transport const & transport, bool isSuccess)
{
	for (RequestIter rit = _requests.begin (); rit != _requests.end (); ++rit)
	{
		DispatchRequest & request = rit->second;
		if (request.GetTransport () == transport)
		{
			request.MarkTried ();
			if (isSuccess)
				request.MarkDelivered();
			if (request.IsForward ())
			{
				Assert (_fwdReqCount != 0);
				--_fwdReqCount;
			}
			else
			{
				Assert (_emailReqCount != 0);
				--_emailReqCount;
			}
		}
	}
	Assert (RequestsTally ());
}

bool ScriptTicket::AreRequestsPending () const
{
	for (ConstRequestIter it = _requests.begin (); it != _requests.end (); ++it)
	{
		if (!it->second.IsDelivered())
			return true;
	}
	return false;
}

bool ScriptTicket::AreForwardRequestsPending () const
{
	for (ConstRequestIter it = _requests.begin (); it != _requests.end (); ++it)
	{
		DispatchRequest const & req = it->second;
		if (req.IsForward() && !req.IsDelivered())
		{
			return true;
		}
	}
	return false;
}

void ScriptTicket::RefreshRequests ()
{
	for (RequestIter it = _requests.begin (); it != _requests.end (); ++it)
	{
		DispatchRequest & req = it->second;
		if (!req.IsDelivered())
		{
			if (req.WasTried ())
			{
				req.MarkTried (false);
				if (it->second.IsForward ())
					++_fwdReqCount;
				else
					++_emailReqCount;
			}
		}
	}
	Assert (RequestsTally ());
}

bool ScriptTicket::RequestsTally () const
{
	int count = 0;
	for (std::multimap<int, DispatchRequest>::const_iterator it = _requests.begin ();
		it != _requests.end (); ++it)
	{
		if (!it->second.WasTried ())
			++count;
	}
	return count == _fwdReqCount + _emailReqCount; 
}

