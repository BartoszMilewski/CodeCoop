//------------------------------------
//  (c) Reliable Software, 1999 - 2006
//------------------------------------

#include "precompiled.h"
#include "DispatcherCmd.h"

#include <Ex/WinEx.h>
#include <numeric>

std::string InvitationCmd::REQUIRES_ACTION = "Invitation: project path needed";

std::unique_ptr<DispatcherCmd> DispatcherCmd::DeserializeCmd (Deserializer & in, int version)
{
    DispatcherCmdType type = static_cast<DispatcherCmdType>(in.GetLong ());
	std::unique_ptr<DispatcherCmd> cmd;
    switch (type)
    {
    case typeAddressChange:
        cmd.reset (new AddressChangeCmd);
        break;
	case typeAddMember:
		cmd.reset (new AddMemberCmd);
		break;
	case typeReplaceTransport:
		cmd.reset (new ReplaceTransportCmd);
		break;
	case typeReplaceRemoteTransport:
        cmd.reset (new ReplaceRemoteTransportCmd);
        break;
	case typeChangeHubId:
        cmd.reset (new ChangeHubIdCmd);
        break;
	case typeAddSatelliteRecipients:
        cmd.reset (new AddSatelliteRecipientsCmd);
        break;
	case typeForceDispatch:
		cmd.reset (new ForceDispatchCmd);
		break;
	case typeChunkSize:
		cmd.reset (new ChunkSizeCmd);
		break;
	case typeDistributorLicense:
		cmd.reset (new DistributorLicenseCmd);
		break;
	case typeInvitation:
		cmd.reset (new InvitationCmd);
		break;
    default:
		throw Win::InternalException ("A script contains an unrecognized Dispatcher command. "
									 "The sender is probably using a newer version of Code Co-op.");
    }
    cmd->Deserialize (in, version);
    return cmd;
}

void InvitationCmd::Serialize (Serializer & out) const
{
	out.PutLong (GetType ());
	_adminName.Serialize (out);
	_adminEmail.Serialize (out);
	_invitee.Serialize (out);
	_defectFilename.Serialize (out);
	_defectScript.Serialize (out);
}

void InvitationCmd::Deserialize (Deserializer & in, int version)
{
	// Command type has been read by DispatcherCmd::DeserializeCmd
	_adminName.Deserialize (in, version);
	_adminEmail.Deserialize (in, version);
	_invitee.Deserialize (in, version);
	_defectFilename.Deserialize (in, version);
	_defectScript.Deserialize (in, version);
}

void AddressChangeCmd::Serialize (Serializer & out) const
{
	out.PutLong (GetType ());
	_oldHubId.Serialize (out);
	_oldUserId.Serialize (out);
	_newHubId.Serialize (out);
	_newUserId.Serialize (out);
}

void AddressChangeCmd::Deserialize (Deserializer & in, int version)
{
    // Command type has been read by DispatcherCmd::DeserializeCmd
	_oldHubId.Deserialize (in, version);
	_oldUserId.Deserialize (in, version);
	_newHubId.Deserialize (in, version);
	_newUserId.Deserialize (in, version);
}

void AddMemberCmd::Serialize (Serializer & out) const
{
	out.PutLong (GetType ());
	_hubId.Serialize (out);
	_userId.Serialize (out);
	_transport.Serialize (out);
}

void AddMemberCmd::Deserialize (Deserializer & in, int version)
{
    // Command type has been read by DispatcherCmd::DeserializeCmd
	_hubId.Deserialize (in, version);
	_userId.Deserialize (in, version);
	if (version <= 35)
	{
		SerString fwdPath;
 		fwdPath.Deserialize (in, version);
		_transport.Init (fwdPath.c_str ());
	}
	else
	{
		_transport.Deserialize (in, version);
	}
}

void AddSatelliteRecipientsCmd::Serialize (Serializer & out) const
{
	out.PutLong (GetType ());
	_transport.Serialize (out);
	_activeAddresses.Serialize (out);
	_removedAddresses.Serialize (out);
}

void AddSatelliteRecipientsCmd::Deserialize (Deserializer & in, int version)
{
	// Command type has been read by DispatcherCmd::DeserializeCmd
	_transport.Deserialize (in, version);
	_activeAddresses.Deserialize (in, version);
	_removedAddresses.Deserialize (in, version);
}

void ReplaceTransportCmd::Serialize (Serializer & out) const
{
	out.PutLong (GetType ());
	_old.Serialize (out);
	_new.Serialize (out);
}

void ReplaceTransportCmd::Deserialize (Deserializer & in, int version)
{
    // Command type has been read by DispatcherCmd::DeserializeCmd
	_old.Deserialize (in, version);
	_new.Deserialize (in, version);
}

void ForceDispatchCmd::Serialize (Serializer & out) const
{
	out.PutLong (GetType ());
}

void ForceDispatchCmd::Deserialize (Deserializer & in, int version)
{
    // Command type has been read by DispatcherCmd::DeserializeCmd
}

void ChunkSizeCmd::Serialize (Serializer & out) const
{
	out.PutLong (GetType ());
	out.PutLong (_chunkSize);
}

void ChunkSizeCmd::Deserialize (Deserializer & in, int version)
{
    // Command type has been read by DispatcherCmd::DeserializeCmd
	_chunkSize = in.GetLong ();
}

void ReplaceRemoteTransportCmd::Serialize (Serializer & out) const
{
	out.PutLong (GetType ());
	_hubId.Serialize (out);
	_transport.Serialize (out);
}

void ReplaceRemoteTransportCmd::Deserialize (Deserializer & in, int version)
{
    // Command type has been read by DispatcherCmd::DeserializeCmd
	_hubId.Deserialize (in, version);
	_transport.Deserialize (in, version);
}

void ChangeHubIdCmd::Serialize (Serializer & out) const
{
	out.PutLong (GetType ());
	_oldHubId.Serialize (out);
	_newHubId.Serialize (out);
	_newTransport.Serialize (out);
}

void ChangeHubIdCmd::Deserialize (Deserializer & in, int version)
{
    // Command type has been read by DispatcherCmd::DeserializeCmd
	_oldHubId.Deserialize (in, version);
	_newHubId.Deserialize (in, version);
	_newTransport.Deserialize (in, version);
}

void DistributorLicenseCmd::Serialize (Serializer & out) const
{
	out.PutLong (GetType ());
	_licensee.Serialize (out);
	unsigned long sum = 0;
	sum = std::accumulate (_licensee.begin (), _licensee.end (), sum);
	long A = sum + _startNum;
	long B = A - _startNum;
	long C = B + _count;
	long D = A + 2*B + 3*C;
	out.PutLong (A);
	out.PutLong (B);
	out.PutLong (C);
	out.PutLong (D);
}

void DistributorLicenseCmd::Deserialize (Deserializer & in, int version)
{
	_licensee.Deserialize (in, version);
	unsigned long sum = 0;
	sum = std::accumulate (_licensee.begin (), _licensee.end (), sum);
	long A = in.GetLong ();
	long B = in.GetLong ();
	long C = in.GetLong ();
	long D = in.GetLong ();
	if (A + 2*B + 3*C != D)
		throw Win::Exception ("Invalid Distribution License");

	_count = C - B;
	_startNum = A - B;
	if (sum + _startNum != A)
		throw Win::Exception ("Invalid Distribution License");
}
