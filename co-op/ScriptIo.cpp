//------------------------------------
//  (c) Reliable Software, 2003 - 2007
//------------------------------------

#include "precompiled.h"
#undef DBG_LOGGING
#define DBG_LOGGING false

#include "ScriptIo.h"
#include "TransportHeader.h"
#include "ScriptHeader.h"
#include "ScriptTrailer.h"
#include "ScriptCommandList.h"
#include "ScriptList.h"
#include "DispatcherScript.h"
#include "Catalog.h"
#include "ScriptName.h"
#include "CheckoutNotifications.h"

#include <File/Path.h>
#include <Ex/Winex.h>

// Script Builder

void ScriptBuilder::Save (std::string const & toString)
{
	dbg << "ScriptBuilder::Save" << std::endl;
	FilePath publicInbox (_catalog->GetPublicInboxDir ());
	Assert (_scriptHdr != 0);
	ScriptFileName fileName (_scriptHdr->ScriptId (),
							 toString,
							 _scriptHdr->GetProjectName ());
	if (DontSplit ())
	{
		dbg << "	DontSplit!" << std::endl;
		Assert (_scriptHdr->IsResendRequest ());
		Save (publicInbox, fileName.Get ());
		return;
	}

	unsigned count = GetChunkCount ();
	dbg << "	Number of chunks: " << count << std::endl;
	if (count > 1)
	{
		for (ChunkSeq seq (*this); !seq.AtEnd (); seq.Advance ())
		{
			SaveChunk (publicInbox, 
						fileName.Get (seq.ChunkNumber (), count), 
						seq);
		}
	}
	else
	{
		Save (publicInbox, fileName.Get ());
	}
}

void ScriptBuilder::Save (std::vector<unsigned> const & chunkList, std::string const & toString)
{
	FilePath publicInbox (_catalog->GetPublicInboxDir ());
	Assert (_scriptHdr != 0);
	ScriptFileName fileName (_scriptHdr->ScriptId (),
							 toString,
							 _scriptHdr->GetProjectName ());
	ChunkSeq seq (*this);
	for (std::vector<unsigned>::const_iterator it = chunkList.begin (); it != chunkList.end (); ++it)
	{
		seq.Seek (*it);
		SaveChunk (publicInbox, 
					fileName.Get (*it, GetChunkCount ()), 
					seq);
	}
}

void ScriptBuilder::SaveChunk (FilePath & toPath, 
							   std::string const & fileName, 
							   ChunkSeq const & chunks)
{
	dbg << "ScriptBuilder::SaveChunk " << chunks.ChunkNumber () << std::endl;
	char const * path = toPath.GetFilePath (fileName);
	FileSerializer out (path);
	if (_txHdr != 0)
		_txHdr->Save (out);

	_scriptHdr->SetChunkInfo (chunks.ChunkNumber (), chunks.ChunkCount (), chunks.MaxChunkSize ());
	_scriptHdr->Save (out);

	ScriptChunk chunk (chunks.GetChunk (), chunks.GetChunkSize ());
	chunk.Save (out);

	if (_scriptTrailer != 0)
		_scriptTrailer->Save (out);
	if (_dispatcherScript != 0)
		_dispatcherScript->Save (out);
	if (_checkOutNotification != 0)
		_checkOutNotification->Save (out);
}

void ScriptBuilder::Save (FilePath & toPath, std::string const & fileName) const
{
	char const * path = toPath.GetFilePath (fileName);
	FileSerializer out (path);
	Save (out);
}

void ScriptBuilder::Save (Serializer & out) const
{
	Assert (_scriptHdr != 0 && (_cmdList != 0 || _scriptList != 0));
	if (_txHdr != 0)
		_txHdr->Save (out);
	_scriptHdr->Save (out);
	if (_cmdList != 0)
	{
		Assert (_scriptList == 0);
		_cmdList->Save (out);
	}
	else
	{
		Assert (_scriptList != 0);
		_scriptList->Save (out);
	}
	if (_scriptTrailer != 0)
		_scriptTrailer->Save (out);
	if (_dispatcherScript != 0)
		_dispatcherScript->Save (out);
	if (_checkOutNotification != 0)
		_checkOutNotification->Save (out);
}

//
// Script reader
//

class HeaderReader : public ScriptHeader
{
public:
	HeaderReader (Deserializer & in)
	{
		_readOk = Read (in) != 0;
	}

	bool IsReadOk () const { return _readOk; }

private:
	bool	_readOk;
};

class CmdListReader : public CommandList
{
public:
	CmdListReader (Deserializer & in)
	{
		_readOk = (Read (in) != 0);
	}

	bool IsReadOk () const { return _readOk; }

private:
	bool	_readOk;
};

std::unique_ptr<TransportHeader> ScriptReader::RetrieveTransportHeader (File::Offset offset) const
{
	Assert (!_path.empty ());
    FileDeserializer in (_path);
	in.SetPosition (offset);
	return RetrieveTransportHeader (in);
}

std::unique_ptr<ScriptHeader> ScriptReader::RetrieveScriptHeader (File::Offset offset) const
{
	Assert (!_path.empty ());
    FileDeserializer in (_path);
	in.SetPosition (offset);
	return RetrieveScriptHeader (in);
}

std::unique_ptr<ScriptTrailer> ScriptReader::RetrieveScriptTrailer (File::Offset offset) const
{
	Assert (!_path.empty ());
    FileDeserializer in (_path);
	in.SetPosition (offset);
	return RetrieveScriptTrailer (in);
}

std::unique_ptr<CommandList> ScriptReader::RetrieveCommandList (File::Offset offset) const
{
	Assert (!_path.empty ());
    FileDeserializer in (_path);
	in.SetPosition (offset);
	return RetrieveCommandList (in);
}

std::unique_ptr<DispatcherScript> ScriptReader::RetrieveDispatcherScript (File::Offset offset) const
{
	Assert (!_path.empty ());
    FileDeserializer in (_path);
	in.SetPosition (offset);
	return RetrieveDispatcherScript (in);
}

std::unique_ptr<TransportHeader> ScriptReader::RetrieveTransportHeader (FileDeserializer & in) const
{
	return std::unique_ptr<TransportHeader> (new TransportHeader (in));
}

std::unique_ptr<ScriptHeader> ScriptReader::RetrieveScriptHeader (FileDeserializer & in) const
{
	std::unique_ptr<HeaderReader> hdr (new HeaderReader (in));
	if (hdr->IsReadOk ())
	{
		// Version 4.5 or higher script format -- script header saved in a separate section
		return std::move(hdr);
	}
	else
	{
		// Version 4.1 or lower script format -- script header and command list saved in one section
		Version40Reader reader (in);	// Read only header
		return reader.GetHeader ();
	}
}

std::unique_ptr<ScriptTrailer> ScriptReader::RetrieveScriptTrailer (FileDeserializer & in) const
{
	return std::unique_ptr<ScriptTrailer> (new ScriptTrailer (in));
}

std::unique_ptr<CommandList> ScriptReader::RetrieveCommandList (FileDeserializer & in) const
{
	std::unique_ptr<CmdListReader> cmdList (new CmdListReader (in));
	if (cmdList->IsReadOk ())
	{
		// Version 4.5 or higher script format -- command list saved in a separate section
		return std::move(cmdList);
	}
	else
	{
		// Version 4.1 or lower script format -- script header and command list saved in one section
		Version40Reader reader (in);
		return reader.GetCmdList ();
	}
}

std::unique_ptr<DispatcherScript> ScriptReader::RetrieveDispatcherScript (FileDeserializer & in) const
{
	return std::unique_ptr<DispatcherScript> (new DispatcherScript (in));
}

std::unique_ptr<CheckOut::List> ScriptReader::RetrieveCheckoutNotification (FileDeserializer & in) const
{
	return std::unique_ptr<CheckOut::List> (new CheckOut::List (in));
}

void ScriptReader::Version40Reader::Deserialize (Deserializer & in, int version)
{
	_hdr.reset (new ScriptHeader);
	_hdr->Deserialize (in, version);
	_cmdList.reset (new CommandList);
	if (_hdr->IsControl ())
	{
		// Read just one control command and add it to the list
		std::unique_ptr<ScriptCmd> cmd = ScriptCmd::DeserializeV40CtrlCmd (in, version);
		// Provide more detailed script kind expected by the version 4.5
		ScriptCmdType type = cmd->GetType ();
		switch (type)
		{
		case typeAck:
			{
				_hdr->SetScriptKind (ScriptKindAck ());
				AckCmd const * ackCmd = dynamic_cast<AckCmd const *>(cmd.get ());
				if (ackCmd->IsV40MembershipUpdateAck ())
				{
					_hdr->SetUnitType (Unit::Member);
					// Set unit id to the sender of acknowledged scripts.
					GlobalIdPack pack (ackCmd->GetScriptId ());
					_hdr->SetModifiedUnitId (pack.GetUserId ());
				}
				else
				{
					_hdr->SetUnitType (Unit::Set);
					_hdr->SetModifiedUnitId (gidInvalid);
				}
			}
			break;
		case typeMakeReference:
			_hdr->SetScriptKind (ScriptKindAck ());
			_hdr->SetUnitType (Unit::Set);
			break;
		case typeNewMember:
			{
				_hdr->SetScriptKind (ScriptKindAddMember ());
				_hdr->SetUnitType (Unit::Member);
				NewMemberCmd const * memberCmd = dynamic_cast<NewMemberCmd const *>(cmd.get ());
				_hdr->SetModifiedUnitId (memberCmd->GetMemberInfo ().Id ());
			}
			break;
		case typeDeleteMember:
			{
				_hdr->SetScriptKind (ScriptKindDeleteMember ());
				_hdr->SetUnitType (Unit::Member);
				DeleteMemberCmd const * memberCmd = dynamic_cast<DeleteMemberCmd const *>(cmd.get ());
				_hdr->SetModifiedUnitId (memberCmd->GetMemberInfo ().Id ());
			}
			break;
		case typeEditMember:
			{
				// Check for version 4.2 commnads -- defect or emergency admin election
				EditMemberCmd const * memberCmd = dynamic_cast<EditMemberCmd const *>(cmd.get ());
				MemberInfo const & newInfo = memberCmd->GetNewMemberInfo ();
				if (memberCmd->IsVersion40Defect ())
				{
					Assert (_hdr->IsFromVersion40 ());
					// Version 4.2 defect command
					std::unique_ptr<ScriptCmd> tmp (new DeleteMemberCmd (newInfo));
					_hdr->SetScriptKind (ScriptKindDeleteMember ());
					_hdr->SetUnitType (Unit::Member);
					_hdr->SetModifiedUnitId (newInfo.Id ());
					cmd = std::move(tmp);
					break;
				}
				if (memberCmd->IsVersion40EmergencyAdminElection ())
					_hdr->SetVersion40EmergencyAdminElection (true);
				_hdr->SetScriptKind (ScriptKindEditMember ());
				_hdr->SetUnitType (Unit::Member);
				_hdr->SetModifiedUnitId (newInfo.Id ());
			}
			break;
		case typeJoinRequest:
			_hdr->SetScriptKind (ScriptKindJoinRequest ());
			break;
		case typeResendRequest:
		case typeResendFullSynchRequest:
		default:
			Win::ClearError ();
			throw Win::Exception ("Cannot read version 4.2 script -- illegal control command type.");
			break;
		}
		_cmdList->push_back (std::move(cmd));
	}
	else
	{
		_cmdList->Deserialize (in, version);
	}
}
