//------------------------------------
//  (c) Reliable Software, 1997 - 2007
//------------------------------------

#include "precompiled.h"
#include "ScriptCmd.h"
#include "ScriptCommands.h"
#include "DumpWin.h"
#include "PathFind.h"

#include <Ex/WinEx.h>

#include <StringOp.h>

std::unique_ptr<ScriptCmd> ScriptCmd::DeserializeCmd (Deserializer& in, int version)
{
	ScriptCmdType type = static_cast<ScriptCmdType> (in.GetLong ());
	std::unique_ptr<ScriptCmd> command;
	switch (type)
	{
	case typeWholeFile:
		command.reset (new WholeFileCmd);
		break;
	case typeTextDiffFile:
		command.reset (new TextDiffCmd);
		break;
	case typeBinDiffFile:
		command.reset (new BinDiffCmd);
		break;
	case typeDeletedFile:
		command.reset (new DeleteCmd);
		break;
	case typeNewFolder:
		command.reset (new NewFolderCmd);
		break;
	case typeDeleteFolder:
		command.reset (new DeleteFolderCmd);
		break;
	case typeUserCmd:
		command.reset (new MembershipPlaceholderCmd);
		break;
	case typeAck:
		command.reset (new AckCmd);
		break;
	case typeMakeReference:
		command.reset (new MakeReferenceCmd);
		break;
	case typeResendRequest:
		command.reset (new ResendRequestCmd);
		break;
	case typeResendFullSynchRequest:
		command.reset (new ResendFullSynchRequestCmd);
		break;
	case typeNewMember:
		command.reset (new NewMemberCmd);
		break;
	case typeDeleteMember:
		command.reset (new DeleteMemberCmd);
		break;
	case typeEditMember:
		command.reset (new EditMemberCmd);
		break;
	case typeJoinRequest:
		command.reset (new JoinRequestCmd);
		break;
	case typeVerificationRequest:
		command.reset (new VerificationRequestCmd);
		break;
	default:
		throw Win::Exception ("Unknown script command.\n"
			"Script is either corrupt or from a newer version.");
	}
	command->Deserialize (in, version);
	return command;
}

std::unique_ptr<ScriptCmd> ScriptCmd::DeserializeV40CtrlCmd (Deserializer& in, int version)
{
	// Translate version 4.2 control command to the current control command
	Version40::ControlType type = static_cast<Version40::ControlType> (in.GetLong ());
	std::unique_ptr<ScriptCmd> command;
	switch (type)
	{
	case Version40::typeAck:
		command.reset (new AckCmd);
		break;
	case Version40::typeAckUserInfo:
		command.reset (new AckCmd (true));	// Version 4.0 membership update acknowledgement
		break;
	case Version40::typeMakeReference:
		command.reset (new MakeReferenceCmd);
		break;
	case Version40::typeDefect:
		break;
	case Version40::typeJoinRequest:
		command.reset (new JoinRequestCmd);
		break;
	case Version40::typeNewUser:
		command.reset (new NewMemberCmd);
		break;
	case Version40::typeMembershipUpdate:
		command.reset (new EditMemberCmd);
		break;
	case Version40::typeNack:
	default:
		throw Win::Exception ("Corrupt script (v4.0): Unknown command type.");
	}
	command->Deserialize (in, version);
	return command;
}

std::ostream & operator<<(std::ostream & os, ScriptCmdType type)
{
	switch (type)
	{
	case typeWholeFile:
		os << "new file";
		break;
	case typeTextDiffFile:
		os << "edit textual file";
		break;
	case typeDeletedFile:
		os << "delete file";
		break;
	case typeNewFolder:
		os << "new folder";
		break;
	case typeDeleteFolder:
		os << "delete folder";
		break;
	case typeUserCmd:	// Obsolete in version 4.5
		os << "placeholder command";
		break;
	case typeBinDiffFile:
		os << "edit binary file";
		break;
	// These are new command types introduced in version 4.5
	case typeAck:
		os << "acknowledgement";
		break;
	case typeMakeReference:
		os << "make reference";
		break;
	case typeResendRequest:
		os << "resend request";
		break;
	case typeNewMember:
		os << "new member";
		break;
	case typeDeleteMember:
		os << "delete member";
		break;
	case typeEditMember:
		os << "edit member";
		break;
	case typeJoinRequest:
		os << "join request";
		break;
	case typeResendFullSynchRequest:
		os << "full sync resend request";
		break;
	case typeVerificationRequest:
		os << "verification request";
		break;
	default:
		os << "unknown command type";
		break;
	}
	return os;
}
