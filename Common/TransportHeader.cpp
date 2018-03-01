//----------------------------------
// (c) Reliable Software 1998 - 2006
// ---------------------------------

#include "precompiled.h"
#include "TransportHeader.h"
#include "OutputSink.h"

#include <iomanip>
#include <sstream>

const unsigned char patternDelivered   = 0xc5;
const unsigned char patternUndelivered = 0x2e;
const unsigned char patternForwardOn   = 0xd3;
const unsigned char patternForwardOff  = 0x4b;

TransportHeader::TransportHeader (Deserializer & in)
	: _isValid (false), 
	  _isEmpty (true)
{
    Read (in);
}

void TransportHeader::Serialize (Serializer& out) const
{
	out.PutLong (_scriptId);
    out.PutByte (_toBeForwarded ? patternForwardOn : patternForwardOff);
	_flags.Serialize (out);
    out.PutLong (_recipientCount);
    std::for_each (_recipients.begin (), _recipients.end (), SerializeFlags (out));
	_sender.Serialize (out);
    _recipients.Serialize (out);
}

void TransportHeader::Deserialize (Deserializer& in, int version)
{
    _isEmpty = false;
    CheckVersion (version, VersionNo ());

	if (version > 31)
		_scriptId = in.GetLong ();

	unsigned char b = in.GetByte ();
	if (b == patternForwardOn)
		_toBeForwarded = true;
	else if (b == patternForwardOff)
		_toBeForwarded = false;
	else
		throw Win::Exception ("Script header is corrupted.");

	if (version < 30)
		_flags.SetDefect (in.GetBool ());
	else
		_flags.Deserialize (in, version);

    _recipientCount = in.GetLong ();
	std::vector<bool> deliveryFlags;
	deliveryFlags.reserve (_recipientCount);
	for (int i = 0; i < _recipientCount; ++i)
	{
		unsigned char b = in.GetByte ();
		if (b == patternDelivered)
			deliveryFlags.push_back (true);
		else if (b == patternUndelivered)
			deliveryFlags.push_back (false);
		else
			throw Win::Exception ("Script header is corrupt");
	}
	if (version <= 36)
	{
		SerString projectName;
		projectName.Deserialize (in, version);
		SerString userId;
		userId.Deserialize (in, version);
		SerString hubId;
		hubId.Deserialize (in, version);
		_sender.Set (hubId, projectName, userId);
	}
	else
	{
		_sender.Deserialize (in, version);
	}

    _recipients.Deserialize (in, version);
    for (int j = 0; j < _recipientCount; j++)
    {
        _recipients [j].SetDeliveryFlag (deliveryFlags [j]);
    }
	_isValid = true;
}

void TransportHeader::AddRecipients (AddresseeList const & recipients)
{
	_recipientCount = recipients.size ();
	_recipients.clear ();
	std::copy (recipients.begin (), recipients.end (), std::back_inserter (_recipients));
}

void TransportHeader::SerializeFlags::operator () (Addressee const & addressee)
{
    _out.PutByte (addressee.ReceivedScript () ? patternDelivered : patternUndelivered);
}

void MemMappedHeader::StampDelivery (int addresseeIdx)
{
    char* buf = GetBuf ();
    long version = *(long*)(buf + Serializer::SizeOfLong ());
    Assert (version >= 26);
    // Calculate the position of the given recipient's delivery flag:
    // Find start index of delivery flags zone:
    // 1. skip transport header's section header: 
    //    SectionId + VersionNo + section header size = 3 * sizeOf (long)
    int deliveryFlagsPos = 3 * Serializer::SizeOfLong ();
    // 2. skip transport header's beginning: 
	//    script id
	if (version > 31)
		deliveryFlagsPos += Serializer::SizeOfLong ();
    //    forward flag
    if (version < 28)
        deliveryFlagsPos += Serializer::SizeOfLong ();
    else
        deliveryFlagsPos += Serializer::SizeOfByte ();
    // + ack flag + recipient counter = 2 * sizeOf (long)
    deliveryFlagsPos += 2 * Serializer::SizeOfLong ();
    
    // stamp delivery
    buf [deliveryFlagsPos + addresseeIdx] = patternDelivered;
    Flush ();
}

void MemMappedHeader::StampForwardFlag (bool on)
{
    char* buf = GetBuf ();
    long version = *(long*)(buf + Serializer::SizeOfLong ());
    Assert (version >= 26);
    // 1. Skip transport header's section header: 
    //    SectionId + VersionNo + section header size = 3 * sizeOf (long)
	int fwdFlagPos = 3 * Serializer::SizeOfLong ();
	// 2. script id
	if (version > 31)
		fwdFlagPos += Serializer::SizeOfLong ();

    if (version < 28)
    {
        long* fwdFlagLong = (long*) (buf + fwdFlagPos);
        *fwdFlagLong = (on ? inFileTrue : inFileFalse);
    }
    else
    {
        buf [fwdFlagPos] = (on ? patternForwardOn : patternForwardOff);
    }
    Flush ();
}

void ScriptSubHeader::Serialize (Serializer& out) const 
{
	out.PutLong (_partNumber);
	out.PutLong (_partCount);
	out.PutLong (_maxChunkSize);
	_comment.Serialize (out);
}

void ScriptSubHeader::Deserialize (Deserializer& in, int version)
{
	if (version < 32)
		return;	
	
	if (version > 44)
	{
		_partNumber = in.GetLong ();
		_partCount = in.GetLong ();
		_maxChunkSize = in.GetLong ();
	}
	else
	{
		_partNumber = 1;
		_partCount = 1;
		_maxChunkSize = 0;
	}
	_comment.Deserialize (in, version);
}

std::ostream & operator<<(std::ostream & os, TransportHeader const & txHdr)
{
	os << "Project name: " << txHdr.GetProjectName () << std::endl;
	os << "Script sender: ";
	Address const & sender = txHdr.GetSenderAddress ();
	os << sender.GetHubId () << " (" << sender.GetUserId () << ')' << std::endl;
	os << "Transport flags: ";
	if (txHdr.ToBeForwarded ())
		os << "to be forwarded, ";
	if (txHdr.IsDispatcherAddendum ())
		os << "Dispatcher addendum, ";
	if (txHdr.IsControlScript ())
		os << "control script, ";
	if (txHdr.IsDefectScript ())
		os << "defect script";
	os << std::endl;
	os << "Script recipients:" << std::endl;
	AddresseeList const & recipients = txHdr.GetRecipients ();
	for (AddresseeList::ConstIterator iter = recipients.begin (); iter != recipients.end (); ++iter)
	{
		os << "   " << *iter << std::endl;
	}
	return os;
}

