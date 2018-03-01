//-----------------------------------
// (c) Reliable Software, 2004 - 2008
//-----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "Sidetrack.h"
#include "ProjectDb.h"
#include "PathFind.h"
#include "ScriptCommands.h"
#include "ScriptList.h"
#include "ScriptHeader.h"
#include "ScriptCommandList.h"
#include "ScriptName.h"
#include "Chunker.h"
#include "TransportHeader.h"
#include "ScriptCmd.h"
#include "Registry.h"
#include "ScriptBasket.h"

#include <Dbg/Out.h>
#include <StringOp.h>

Sidetrack::Sidetrack (Project::Db & projectDb, PathFinder const & pathFinder)
	: _projectDb (projectDb),
	  _pathFinder (pathFinder),
	  _isNewMissing (false)
{
    AddTransactableMember (_missing);
    AddTransactableMember (_missingChunks);
    AddTransactableMember (_resendRequests);
}

// Return true if new missing script has been detected 
bool Sidetrack::IsNewMissing_Reset ()
{
	bool isNewMissing = _isNewMissing;
	_isNewMissing = false;
	return isNewMissing;
}

// Returns the # of chunks stored for this script (including this one)
// Invariant: Every script on the _missingChunk list is absent from the _missing list
unsigned Sidetrack::XAddChunk (ScriptHeader const & inHdr, std::string const & fileName)
{
	GlobalId scriptId;
	if (inHdr.IsFullSynch ())
		scriptId = inHdr.GetLineage ().GetReferenceId ();
	else
		scriptId = inHdr.ScriptId ();
	unsigned count = _missingChunks.XCount ();
	unsigned idx;
	for (idx = 0; idx < count; ++idx)
	{
		Sidetrack::MissingChunk const * missing = _missingChunks.XGet (idx);
		if (missing != 0 && missing->ScriptId () == scriptId)
		{
			// Another chunk of the same script, sent independently of our request
			if (missing->GetMaxChunkSize () != inHdr.GetMaxChunkSize ())
			{
				// New script chunk has different size than the chunks already recorded.
				// We will ignore the new script chunk.
				return missing->GetStoredCount ();
			}
			break;
		}
	}

	if (idx == count) // never seen before
	{
		_isNewMissing = true;
		std::unique_ptr<MissingChunk> missing (new MissingChunk (inHdr, _projectDb));
		missing->StoreChunk (inHdr.GetPartNumber (), fileName, _pathFinder);
		_missingChunks.XAppend (std::move(missing));
		// make sure it's not on the list of whole missing scripts
		unsigned scriptCount = _missing.XCount ();
		for (unsigned i = 0; i < scriptCount; ++i)
		{
			Sidetrack::Missing const * missing = _missing.XGet (i);
			if (missing != 0 && missing->ScriptId () == scriptId)
			{
				_missing.XMarkDeleted (i);
				break;
			}
		}
		return 1;
	}
	
	// We know about this script
	MissingChunk * missing = _missingChunks.XGetEdit (idx);
	missing->StoreChunk (inHdr.GetPartNumber (), fileName, _pathFinder);
	unsigned countSoFar = missing->GetStoredCount ();
	if (countSoFar != missing->GetPartCount ())
		return countSoFar;
	
	Assert (countSoFar == missing->GetPartCount ());
	// This is the last chunk: reconstruct the whole script and deposit it in the inbox
	missing->Reconstruct (_pathFinder);
	missing->ResetRecipients ();
	// Leave the missing chunk entry--it will be cleaned or reused by next update
	return countSoFar;
}

// Returns false if corruption discovered
bool Sidetrack::XRememberResendRequest (ScriptHeader const & inHdr, CommandList const & cmdList)
{
	Assert (cmdList.size () == 1);
	CommandList::Sequencer seq (cmdList);
	ResendFullSynchRequestCmd const & cmd = seq.GetFsResendCmd ();
	GlobalId scriptId = cmd.GetSetScripts ().GetReferenceId ();
	unsigned count = _resendRequests.XCount ();
	unsigned idx;
	for (idx = 0; idx < count; ++idx)
	{
		Sidetrack::ResendRequest const * req = _resendRequests.XGet (idx);
		if (req != 0 && req->ScriptId () == scriptId)
			break;
	}

	if (idx == count) // never seen before
	{
		if (cmd.GetSetScripts ().Count () == 0 && cmd.GetMembershipScripts ().size () == 0)
			return false;
		GlobalIdPack packedId (scriptId);
		std::string fileName (packedId.ToString ());
		fileName += "-FullSynchResendRequest.cmd";
		char const * dstPath = _pathFinder.GetSysFilePath (fileName.c_str ());
		FileSerializer out (dstPath);
		cmd.Serialize (out);

		std::unique_ptr<ResendRequest> req (
			new ResendRequest (scriptId, inHdr.GetPartCount (), inHdr.GetMaxChunkSize (), dstPath));
		req->AddChunkNo (inHdr.GetPartNumber ());
		_resendRequests.XAppend (std::move(req));
	}
	else
	{
		// We know about this script
		ResendRequest * req = _resendRequests.XGetEdit (idx);
		req->AddChunkNo (inHdr.GetPartNumber ());
	}
	return true;
}

bool Sidetrack::HasFullSynchRequests () const
{
	return _resendRequests.Count () != 0;
}

bool Sidetrack::GetFullSynchRequest (FullSynchData & fullSynchData,
							  std::vector<unsigned> & chunkNumbers,
							  unsigned & maxChunkSize,
							  unsigned & chunkCount)
{
	Sidetrack::ResendRequestIter it = _resendRequests.begin ();
	if (it == _resendRequests.end ())
		return false;
	ResendRequest const * req = *it;
	maxChunkSize = req->GetMaxChunkSize ();
	chunkCount = req->GetPartCount ();
	std::vector<unsigned> const & numbers = req->GetChunkNumbers ();
	chunkNumbers.insert (chunkNumbers.begin (), numbers.begin (), numbers.end ());
	std::string const & path = req->GetPath ();
	FileDeserializer in (path);
	ResendFullSynchRequestCmd cmd;
	ScriptCmdType type = static_cast<ScriptCmdType> (in.GetLong ());
	if (type != typeResendFullSynchRequest)
	{
		Win::ClearError ();
		throw Win::Exception ("Corrupted Full Synch resend request command");
	}
	cmd.Deserialize (in, scriptVersion);
	fullSynchData.SwapLineages (cmd.GetSetScripts (), cmd.GetMembershipScripts ());
	return true;
}

void Sidetrack::XRemoveFullSynchChunkRequest (GlobalId scriptId)
{
	unsigned count = _resendRequests.XCount ();
	for (unsigned idx = 0; idx < count; ++idx)
	{
		Sidetrack::ResendRequest const * req = _resendRequests.XGet (idx);
		if (req != 0 && (scriptId == gidInvalid || req->ScriptId () == scriptId))
		{
			File::DeleteNoEx (req->GetPath ().c_str ());
			_resendRequests.XMarkDeleted (idx);
			break;
		}
	}
}

void Sidetrack::XRemoveMissingChunk (unsigned idx)
{
	// remove this entry for missing chunks
	MissingChunk * missEdit = _missingChunks.XGetEdit (idx);
	// delete all already received chunks
	MissingChunk::PartMap & partMap = missEdit->GetPartMap ();
	for (MissingChunk::PartMap::iterator it = partMap.begin (); it != partMap.end (); )
	{
		char const * chunkPath = _pathFinder.GetSysFilePath (it->second.c_str ());
		File::DeleteNoEx (chunkPath);
		it = partMap.erase (it);
	}
	_missingChunks.XMarkDeleted (idx);
}

void Sidetrack::GetMissingScripts (Unit::Type unitType, GidSet & gids) const
{
	for (MissingIter it = _missing.begin (); it != _missing.end (); ++it)
	{
		Missing * missing = *it;
		if (missing->UnitType () == unitType)
			gids.insert (missing->ScriptId ());
	}
	// Currently only Unit::Set script are chunked.
	for (MissingChunkIter it = _missingChunks.begin (); it != _missingChunks.end (); ++it)
	{
		gids.insert ((*it)->ScriptId ());
	}
}

Sidetrack::Missing const * Sidetrack::GetMissing (GlobalId scriptId) const
{
	Assert (IsMissing (scriptId));
	MissingIter mit = std::find_if (_missing.begin (), _missing.end (), IsEqualId (scriptId));
	if (mit != _missing.end ())
	{
		return *mit;
	}
	MissingChunkIter cit = std::find_if (_missingChunks.begin (), _missingChunks.end (), IsEqualId (scriptId));
	if (cit != _missingChunks.end ())
	{
		return *cit;
	}
	return 0;	// Never happens
}

bool Sidetrack::IsMissing (GlobalId gid) const
{
	MissingIter mit = std::find_if (_missing.begin (), _missing.end (), IsEqualId (gid));
	if (mit != _missing.end ())
	{
		return true;
	}
	MissingChunkIter cit = std::find_if (_missingChunks.begin (), _missingChunks.end (), IsEqualId (gid));
	if (cit != _missingChunks.end ())
	{
		return true;
	}
	return false;
}

bool Sidetrack::IsMissingChunked (GlobalId gid, unsigned & received, unsigned & total) const
{
	MissingChunkIter cit = std::find_if (_missingChunks.begin (), _missingChunks.end (), IsEqualId (gid));
	if (cit != _missingChunks.end ())
	{
		total = (*cit)->GetPartCount ();
		received = (*cit)->GetStoredCount ();
		return true;
	}
	return false;
}

UserId Sidetrack::SentTo (GlobalId gid) const
{
	MissingIter mit = std::find_if (_missing.begin (), _missing.end (), IsEqualId (gid));
	if (mit != _missing.end ())
		return (*mit)->SentTo ();
	MissingChunkIter cit = std::find_if (_missingChunks.begin (), _missingChunks.end (), IsEqualId (gid));
	if (cit != _missingChunks.end ())
		return (*cit)->SentTo ();
	return gidInvalid;
}

UserId Sidetrack::NextRecipientId (GlobalId gid) const
{
	MissingIter mit = std::find_if (_missing.begin (), _missing.end (), IsEqualId (gid));
	if (mit != _missing.end ())
		return (*mit)->NextRecipientId ();
	MissingChunkIter cit = std::find_if (_missingChunks.begin (), _missingChunks.end (), IsEqualId (gid));
	if (cit != _missingChunks.end ())
		return (*cit)->NextRecipientId ();
	return gidInvalid;
}

// Returns true when sidetrack needs to be updated
bool Sidetrack::NeedsUpdating (Unit::ScriptList const & historyMissingScripts) const
{
	GidSet historyMissingSet;
	Unit::ScriptList::const_iterator it = historyMissingScripts.begin ();
	for (; it != historyMissingScripts.end (); ++it)
	{
		GlobalId gid = it->Gid ();
		if (NeedsUpdate (gid)) // checks all lists
		{
			// We have to request this script re-send -- sidetrack needs to be updated
			dbg << "Sidetrack in project: " << _projectDb.ProjectName () << ", User id: " << std::hex << _projectDb.GetMyId () << std::endl;
			return true;
		}
		historyMissingSet.insert (gid);
	}

	for (MissingIter it = _missing.begin (); it != _missing.end (); ++it)
	{
		if (historyMissingSet.find ((*it)->ScriptId ()) == historyMissingSet.end ())
			return true; // History missing script set has changed -- sidetrack needs to be updated
	}
	for (MissingChunkIter it = _missingChunks.begin (); it != _missingChunks.end (); ++it)
	{
		if (historyMissingSet.find ((*it)->ScriptId ()) == historyMissingSet.end ())
			return true; // History missing script set has changed -- sidetrack needs to be updated
	}
	// If we have the full sync re-send requests then sidetrack needs updating
	return _resendRequests.Count () != 0;
}

// Return true if there is an entry with this scriptId needing immediate action on either list
bool Sidetrack::NeedsUpdate (GlobalId scriptId) const
{
	for (MissingIter it = _missing.begin (); it != _missing.end (); ++it)
	{
		if ((*it)->ScriptId () == scriptId)
		{
			PackedTimeStr str ((*it)->NextTime ());
			dbg << "Sidetrack script entry: " << GlobalIdPack (scriptId) << " Time: " << str.c_str () << std::endl;
			return (*it)->NextTime () < CurrentPackedTime ();
		}
	}
	for (MissingChunkIter it = _missingChunks.begin (); it != _missingChunks.end (); ++it)
	{
		if ((*it)->ScriptId () == scriptId)
		{
			PackedTimeStr str ((*it)->NextTime ());
			dbg << "Sidetrack chunk entry: " << GlobalIdPack (scriptId) << " Time: " << str.c_str () << std::endl;
			return (*it)->NextTime () < CurrentPackedTime ();
		}
	}		
	return true; // not found
}

// Call only if no chunk entry was found for scriptId
Sidetrack::Missing * Sidetrack::XNeedsScriptResend (GlobalId scriptId, Unit::Type unitType)
{
	unsigned i = XFindScriptEntry(scriptId);
	if (i != npos)
	{
		Assert(_missing.XGet(i) != 0);
		if (_missing.XGet(i)->NextTime () < CurrentPackedTime())
			return _missing.XGetEdit(i);
		else
			return 0;
	}

	// Not found: create new entry
	_isNewMissing = true;
	std::unique_ptr<Missing> missing (new Missing (scriptId, unitType, _projectDb));
	_missing.XAppend (std::move(missing));
	return 0;
}

unsigned Sidetrack::XFindScriptEntry(GlobalId scriptId)
{
	unsigned count = _missing.XCount ();
	for (unsigned i = 0; i != count; ++i)
	{
		Missing const * missing = _missing.XGet (i);
		if (missing != 0 && missing->ScriptId () == scriptId)
		{
			return i;
		}
	}
	return npos;
}

Sidetrack::MissingChunk * Sidetrack::XNeedsChunkResend (GlobalId scriptId, bool & isFound)
{
	isFound = false;
	unsigned i = XFindChunkEntry(scriptId);
	if (i == npos)
		return 0;
	isFound = true;
	Assert(_missingChunks.XGet(i) != 0);
	if (_missingChunks.XGet(i)->NextTime () < CurrentPackedTime ())
		return _missingChunks.XGetEdit (i);
	else
		return 0;
}

unsigned Sidetrack::XFindChunkEntry(GlobalId scriptId)
{
	unsigned count = _missingChunks.XCount ();
	for (unsigned i = 0; i != count; ++i)
	{
		MissingChunk const * missing = _missingChunks.XGet (i);
		if (missing != 0 && missing->ScriptId () == scriptId)
		{
			return i;
		}
	}
	return npos;
}

bool Sidetrack::XProcessMissingScripts (Unit::ScriptList const & missingScripts, ScriptBasket & scriptBasket)
{
	GidSet missingChunkSet;
	GidSet missingScriptSet;
	Unit::ScriptList::const_iterator it = missingScripts.begin ();
	for (; it != missingScripts.end (); ++it)
	{
		// Search missing chunk list first!
		bool isChunkFound; // will be set
		Sidetrack::MissingChunk * chunkEntry = XNeedsChunkResend (it->Gid (), isChunkFound);
		if (chunkEntry != 0)
		{
			if (chunkEntry->IsFullSynch ())
				XSendFullSynchChunkRequest (chunkEntry, scriptBasket);
			else if (!chunkEntry->IsExhausted ())
				XSendChunkRequest (chunkEntry, scriptBasket);
			chunkEntry->Update (_projectDb);
		}
		if (isChunkFound)
		{
			missingChunkSet.insert (it->Gid ());
			continue;
		}

		Assert (!isChunkFound);
		// Search missing script list (add if not present)
		Sidetrack::Missing * resendEntry = XNeedsScriptResend (it->Gid (), it->Type ());
		missingScriptSet.insert (it->Gid ());
		if (resendEntry != 0)
		{
			if (!resendEntry->IsExhausted ())
				XSendScriptRequest (resendEntry, scriptBasket);
			resendEntry->Update (_projectDb);
		}
		// Revisit: full sync chunk?
	}
	// remove entries that are not in the set
	unsigned count = _missing.XCount ();
	for (unsigned i = 0; i != count; ++i)
	{
		Missing const * missing = _missing.XGet (i);
		if (missing != 0 && missingScriptSet.find (missing->ScriptId ()) == missingScriptSet.end ())
		{
			_missing.XMarkDeleted (i);
		}
	}
	count = _missingChunks.XCount ();
	for (unsigned i = 0; i != count; ++i)
	{
		MissingChunk const * missing = _missingChunks.XGet (i);
		if (missing != 0 && missingChunkSet.find (missing->ScriptId ()) == missingChunkSet.end ())
		{
			XRemoveMissingChunk (i);
		}
	}
	// Postcondition: every script on the _missingChunk list is absent from the _missing list
	return IsNewMissing_Reset ();
}

void Sidetrack::XRequestResend(GlobalId scriptId, UserIdList const & addresseeList, ScriptBasket & scriptBasket)
{
	unsigned i = XFindChunkEntry(scriptId);
	if (i != npos)
	{
		Sidetrack::MissingChunk const * chunkEntry = _missingChunks.XGet(i);
		Assert(chunkEntry != 0);
		Assume(!chunkEntry->IsFullSynch (), "Cannot ask for resend of full sync script");
		XMakeChunkRequest (chunkEntry, addresseeList, scriptBasket);
		return;
	}

	i = XFindScriptEntry(scriptId);
	if (i != npos)
	{
		Sidetrack::Missing const * scriptEntry = _missing.XGet(i);
		Assert(scriptEntry != 0);
		XMakeScriptRequest (scriptEntry, addresseeList, scriptBasket);
	}
}

void Sidetrack::XRemoveMissingScript (GlobalId gid)
{
	unsigned count = _missing.XCount ();
	for (unsigned i = 0; i != count; ++i)
	{
		Missing const * missing = _missing.XGet (i);
		if (missing != 0 && missing->ScriptId () == gid)
		{
			_missing.XMarkDeleted (i);
			return;
		}
	}
	count = _missingChunks.XCount ();
	for (unsigned i = 0; i != count; ++i)
	{
		MissingChunk const * missing = _missingChunks.XGet (i);
		if (missing != 0 && missing->ScriptId () == gid)
		{
			XRemoveMissingChunk (i);
			return;
		}
	}
}

void Sidetrack::XSendScriptRequest (Sidetrack::Missing * resendEntry,  ScriptBasket & scriptBasket)
{
	UserId nextRecipientId = resendEntry->NextRecipientId ();
	Assert (nextRecipientId >= 0);
	if (!_projectDb.XIsProjectMember (nextRecipientId) ||
		_projectDb.XGetMemberState (nextRecipientId).IsDead ())
		return;
	UserIdList userIds;
	userIds.push_back(nextRecipientId);
	XMakeScriptRequest(resendEntry, userIds, scriptBasket);
}

void Sidetrack::XMakeScriptRequest(Sidetrack::Missing const * resendEntry, 
								   UserIdList const & userIds, 
								   ScriptBasket & scriptBasket)
{
	std::unique_ptr<ScriptHeader> hdr(new ScriptHeader (
						ScriptKindResendRequest (),
						gidInvalid,
						_projectDb.XProjectName ()));
	hdr->SetScriptId (_projectDb.XMakeScriptId ());
	std::string comment ("Requesting script re-send: ");
	comment += GlobalIdPack (resendEntry->ScriptId ()).ToString ();
	hdr->AddComment (comment);
	hdr->SetUnitType (resendEntry->UnitType ());
	dbg << "Sidetrack: Send script request: " << GlobalIdPack (resendEntry->ScriptId ()) << std::endl;
	CommandList cmdList;
	std::unique_ptr<ScriptCmd> cmd (new ResendRequestCmd (resendEntry->ScriptId ()));
	cmdList.push_back (std::move(cmd));
	scriptBasket.Put(std::move(hdr), cmdList, userIds);
}

void Sidetrack::XSendChunkRequest (Sidetrack::MissingChunk * resendEntry,  ScriptBasket & scriptBasket)
{
	UserId nextRecipientId = resendEntry->NextRecipientId ();
	Assert (nextRecipientId >= 0);
	if (!_projectDb.XIsProjectMember (nextRecipientId) ||
		_projectDb.XGetMemberState (nextRecipientId).IsDead ())
		return;
	UserIdList userIds;
	userIds.push_back(nextRecipientId);
	XMakeChunkRequest(resendEntry, userIds, scriptBasket);
}

void Sidetrack::XMakeChunkRequest(Sidetrack::MissingChunk const * resendEntry, 
								   UserIdList const & userIds, 
								   ScriptBasket & scriptBasket)
{
	dbg << "Sidetrack: Send chunk requests: " << GlobalIdPack (resendEntry->ScriptId ()) << std::endl;
	// iterate over missing chunks and send a request for each
	unsigned partCount = resendEntry->GetPartCount ();
	unsigned maxChunkSize = resendEntry->GetMaxChunkSize ();
	MissingChunk::PartMap const & partMap = resendEntry->GetPartMap ();
	for (unsigned partNo = 1; partNo <= partCount; ++partNo)
	{
		if (partMap.find (partNo) != partMap.end ())
			continue; // we already have this chunk
		std::unique_ptr<ScriptHeader> hdr(new ScriptHeader (
							ScriptKindResendRequest (),
							gidInvalid,
							_projectDb.XProjectName ()));
		hdr->SetScriptId (_projectDb.XMakeScriptId ());
		std::string comment ("Requesting script chunk re-send: ");
		comment += GlobalIdPack (resendEntry->ScriptId ()).ToString ();
		comment += " (";
		comment += ToString (partNo);
		comment += " of ";
		comment += ToString (partCount);
		comment += "); max chunk size = ";
		comment + ToString (maxChunkSize);
		hdr->AddComment (comment);
		hdr->SetUnitType (resendEntry->UnitType ());
		hdr->SetChunkInfo (partNo, partCount, maxChunkSize);
		CommandList cmdList;
		std::unique_ptr<ScriptCmd> cmd (new ResendRequestCmd (resendEntry->ScriptId ()));
		cmdList.push_back (std::move(cmd));
		scriptBasket.Put(std::move(hdr), cmdList, userIds);
	}
}

void Sidetrack::XSendFullSynchChunkRequest (Sidetrack::MissingChunk * resendEntry,  ScriptBasket & scriptBasket)
{
	dbg << "Sidetrack: Send FS chunk request: " << GlobalIdPack (resendEntry->ScriptId ()) << std::endl;
	// iterate over missing chunks and send a request for each
	unsigned partCount = resendEntry->GetPartCount ();
	unsigned maxChunkSize = resendEntry->GetMaxChunkSize ();
	MissingChunk::PartMap const & partMap = resendEntry->GetPartMap ();
	if (partMap.begin () == partMap.end ())
	{
		throw Win::InternalException ("Corrupted or invalid full sync script.\n"
			"You must defect and re-join the following project:", _projectDb.XProjectName ().c_str ());
	}
	for (unsigned partNo = 1; partNo <= partCount; ++partNo)
	{
		if (partMap.find (partNo) != partMap.end ())
			continue; // we already have this chunk
		std::unique_ptr<ScriptHeader> hdr(new ScriptHeader (
							ScriptKindFullSynchResendRequest (),
							gidInvalid,
							_projectDb.XProjectName ()));
		hdr->SetScriptId (_projectDb.XMakeScriptId ());
		std::string comment ("Requesting Full Synch chunk re-send: ");
		comment += GlobalIdPack (resendEntry->ScriptId ()).ToString ();
		comment += " (";
		comment += ToString (partNo);
		comment += " of ";
		comment += ToString (partCount);
		comment += "); max chunk size = ";
		comment += FormatFileSize (maxChunkSize);
		hdr->AddComment (comment);
		hdr->SetUnitType (resendEntry->UnitType ());
		hdr->SetChunkInfo (partNo, partCount, maxChunkSize);
		// Get script inventory from existing chunk
		Assume (partMap.begin () != partMap.end (), GlobalIdPack (resendEntry->ScriptId ()).ToBracketedString ().c_str ());
		char const * firstChunkPath = _pathFinder.GetSysFilePath (partMap.begin ()->second.c_str ());
		FileDeserializer in (firstChunkPath);

		ScriptHeader origHeader (in);
		Lineage setScripts;
		origHeader.SwapMainLineage (setScripts);
		auto_vector<UnitLineage> membershipScripts;
		origHeader.SwapSideLineages (membershipScripts);
		std::unique_ptr<ScriptCmd> cmd (
			new ResendFullSynchRequestCmd (resendEntry->ScriptId (), setScripts, membershipScripts));
		CommandList cmdList;
		cmdList.push_back (std::move(cmd));
		UserId adminId = _projectDb.XGetAdminId ();
		if (adminId == gidInvalid)
		{
			throw Win::InternalException (
				"Cannot ask for a full sync re-send\n" 
				"because there is no admin in the project:", 
				_projectDb.XProjectName ().c_str ());
		}
		scriptBasket.Put(std::move(hdr), cmdList, adminId);
	}
}

void Sidetrack::Dump (std::ostream  & out) const
{
	if (_missing.Count () != 0)
	{
		out << std::endl << "===Missing scripts:" << std::endl;
		for (MissingIter it = _missing.begin (); it != _missing.end (); ++it)
		{
			Missing const * missingScript = *it;
			out << *missingScript << std::endl;
		}
	}
	if (_missingChunks.Count () != 0)
	{
		out << std::endl << "===Missing script chunks:" << std::endl;
		for (MissingChunkIter it = _missingChunks.begin (); it != _missingChunks.end (); ++it)
		{
			MissingChunk const * missingChunk = *it;
			out << *missingChunk << std::endl;
		}
	}		
}

//
// Sidetrack::MissingChunk
//
Sidetrack::MissingChunk::MissingChunk (ScriptHeader const & inHdr, Project::Db & projectDb)
	: Missing (inHdr.ScriptId (), inHdr.GetUnitType (), projectDb),
	_partCount (inHdr.GetPartCount ()),
	_maxChunkSize (inHdr.GetMaxChunkSize ())
{
	if (inHdr.IsFullSynch ())
	{
		_flags.set (bitFullSynch, true);
		_scriptId = inHdr.GetLineage ().GetReferenceId ();
		_unitType = Unit::Set;
	}
	// delay resend requests for chunks
	// add 1 hour delay for every 5 chunks
	// with the maximum delay of 6 hours (for 30 and more chunks)
	unsigned int hoursDelayed = 6;
	if (_partCount < 30)
	{
		hoursDelayed = _partCount / 5;
	}
	_nextTime += PackedTimeInterval (0, hoursDelayed * 60);
}

void Sidetrack::MissingChunk::Serialize (Serializer& out) const
{
	Missing::Serialize (out);
	out.PutLong (_partCount);
	out.PutLong (_maxChunkSize);
	out.PutLong (_received.size ());
	for (PartMap::const_iterator it = _received.begin (); it != _received.end (); ++it)
	{
		out.PutLong (it->first);
		out.PutLong (it->second.size ());
		out.PutBytes (it->second.data (), it->second.size ());
	}
	out.PutLong (_flags.to_ulong ());
}

void Sidetrack::MissingChunk::Deserialize (Deserializer& in, int version)
{
	Missing::Deserialize (in, version);
	_partCount = in.GetLong ();
	_maxChunkSize = in.GetLong ();
	unsigned count = in.GetLong ();
	for (unsigned i = 0; i < count; ++i)
	{
		unsigned partNo = in.GetLong ();
		unsigned size = in.GetLong ();
		std::string fileName;
		fileName.resize (size);
		in.GetBytes (&fileName [0], size);
		_received [partNo] = fileName;
	}
	unsigned long long value = in.GetLong ();
	_flags = value;
}

void Sidetrack::MissingChunk::Update (Project::Db & projectDb)
{
	if (IsFullSynch ())
	{
		_nextTime.Now ();
		PackedTimeInterval day (1);
		_nextTime += day;
	}
	else
		Sidetrack::Missing::Update (projectDb);
}

void Sidetrack::MissingChunk::StoreChunk (unsigned partNumber, std::string const & fileName, PathFinder const & pathFinder)
{
	char const * srcPath = pathFinder.InBoxDir ().GetFilePath (fileName);
	char const * dstPath = pathFinder.GetSysFilePath (fileName.c_str ());
	File::Copy (srcPath, dstPath);
	PartMap::const_iterator it = _received.find (partNumber);
	if (it != _received.end ())
	{
		// we've already had it
		if (!IsFileNameEqual (fileName, it->second))
		{
			// delete previous one, if names different
			dstPath = pathFinder.GetSysFilePath (it->second.c_str ());
			File::DeleteNoEx (dstPath);
		}
	}
	_received [partNumber] = fileName;
}

class SafeChunks
{
public:
	typedef std::vector<std::string>::const_iterator Iterator;
	SafeChunks (PathFinder const & pathFinder, Sidetrack::MissingChunk::PartMap & chunks)
	{
		for (unsigned i = 1; i <= chunks.size (); ++i)
		{
			char const * chunkPath = pathFinder.GetSysFilePath (chunks [i].c_str ());
			_paths.push_back (chunkPath);
		}
		chunks.clear ();
	}
	~SafeChunks ()
	{
		for (std::vector<std::string>::iterator it = _paths.begin (); it != _paths.end (); ++it)
			File::DeleteNoEx (*it);
	}
	Iterator begin () const { return _paths.begin (); }
	Iterator end () const { return _paths.end (); }
private:
	std::vector<std::string> _paths;
};

void Sidetrack::MissingChunk::Reconstruct (PathFinder const & pathFinder)
{
	SafeChunks chunks (pathFinder, _received);
	// Reconstruct headers
	Assert (chunks.begin () != chunks.end ());
	std::string const & firstChunkPath = *chunks.begin ();
	FileDeserializer in (firstChunkPath);
	TransportHeader transHdr (in);
	ScriptHeader hdr (in);
	in.Close ();
	hdr.SetChunkInfo (1, 1, _maxChunkSize);
	// Assemble new script file in the database directory
	ScriptFileName fileName (ScriptId (), "local user", hdr.GetProjectName ());
	std::string scriptName (fileName.Get ());
	std::string srcPath (pathFinder.GetSysFilePath (scriptName.c_str ()));
	FileSerializer out (srcPath);
	transHdr.Save (out);
	hdr.Save (out);
	for (SafeChunks::Iterator it = chunks.begin (); it != chunks.end (); ++it)
	{
		FileDeserializer in (*it);
		ScriptChunk chunk (in);
		out.PutBytes (chunk.GetCargo (), chunk.GetSize ());
	}
	out.Close ();
	char const * dstPath = pathFinder.InBoxDir ().GetFilePath (scriptName);
	File::Copy (srcPath.c_str (), dstPath);
	File::DeleteNoEx (srcPath.c_str ());
	// chunks are deleted automatically
}

//
// Sidetrack::Missing
//
Sidetrack::Missing::Missing (GlobalId scriptId, Unit::Type unitType, Project::Db & projectDb)
	: _scriptId (scriptId), _unitType (unitType)
{
	_nextTime.Now ();
	Registry::UserDispatcherPrefs dispatcherPrefs;
	unsigned long delayMinutes;
	dispatcherPrefs.GetResendDelay (delayMinutes);
	PackedTimeInterval delay (0, delayMinutes);
	_nextTime += delay;

	// first resend request goes to the script creator
	// if this is not my script and we know script author
	GlobalIdPack pack (scriptId);
	UserId scriptAuthorId = pack.GetUserId ();
	if (scriptAuthorId != projectDb.XGetMyId () && projectDb.XIsProjectMember (scriptAuthorId))
		_recipients.push_back (scriptAuthorId);
	else
		SelectRecipient (projectDb);
}

void Sidetrack::Missing::Update (Project::Db & projectDb)
{
	_nextTime.Now ();
	Registry::UserDispatcherPrefs dispatcherPrefs;
	unsigned long repeatMinutes;
	dispatcherPrefs.GetRepeatInterval (repeatMinutes);
	PackedTimeInterval repeatInterval (0, repeatMinutes);
	_nextTime += repeatInterval;
	SelectRecipient (projectDb);
}

void Sidetrack::Missing::SelectRecipient (Project::Db & projectDb)
{
	if (IsFullSynch ())
		return; // full sync resend request always goes to administrator

	if (_recipients.empty () || _recipients.back () == gidInvalid)
	{
		dbg << "Restart asking for " << GlobalIdPack (_scriptId) << std::endl;
		_recipients.clear ();
	}
	GidList memberIds;
	projectDb.XGetResendCandidates (memberIds);
	UserId myId = projectDb.XGetMyId ();
	GidList::const_iterator it;
	dbg << "Missing::XUpdate: Sorted list of potential recipients" << std::endl;
	for (it = memberIds.begin (); it != memberIds.end (); ++it)
	{
		dbg << *it << std::endl;
		if (*it == myId)
		{
			dbg << "    my own id" << std::endl;
			continue;
		}
		GidList::const_iterator itr;
		for (itr = _recipients.begin (); itr != _recipients.end (); ++itr)
		{
			if (*it == *itr)
			{
				dbg << "    already sent" << std::endl;
				break; // has been sent to that one
			}
		}
		if (itr == _recipients.end ())
			break;
	}
	if (it != memberIds.end ())
	{
		dbg << "Next to ask: " << *it << std::endl;
		_recipients.push_back (*it);
	}
	else
	{
		dbg << "No more candidates to ask for resend\n" << std::endl;
		_recipients.push_back (gidInvalid); // gone through all users
	}
}
	
UserId Sidetrack::Missing::SentTo () const
{
	Assert (!_recipients.empty ());
	return _recipients.size () == 1 ? _recipients [0] : _recipients [_recipients.size () - 2];
}

void Sidetrack::Missing::ResetRecipients ()
{
	// Leave only the first one (the original sender)
	while (_recipients.size () > 1)
		_recipients.pop_back ();
}

void Sidetrack::Missing::Serialize (Serializer& out) const
{
	out.PutLong (_scriptId);
	out.PutLong (_unitType);
	_nextTime.Serialize (out);
	_recipients.Serialize (out);
}

void Sidetrack::Missing::Deserialize (Deserializer& in, int version)
{
	_scriptId = in.GetLong ();
	_unitType = static_cast<Unit::Type> (in.GetLong ());
	_nextTime.Deserialize (in, version);
	_recipients.Deserialize (in, version);
}

// Sidetrack::ResendRequest

void Sidetrack::ResendRequest::Serialize (Serializer& out) const
{
	out.PutLong (_scriptId);
	out.PutLong (_flags.to_ulong ());
	_path.Serialize (out);
	out.PutLong (_partCount);
	out.PutLong (_maxChunkSize);
	_chunkNumbers.Serialize (out);
}

void Sidetrack::ResendRequest::Deserialize (Deserializer& in, int version)
{
	_scriptId = in.GetLong ();
	unsigned long long value = in.GetLong ();
	_flags = value;
	_path.Deserialize (in, version);
	_partCount = in.GetLong ();
	_maxChunkSize = in.GetLong ();
	_chunkNumbers.Deserialize (in, version);
}

void Sidetrack::ResendRequest::AddChunkNo (unsigned chunkNo)
{
	if (std::find (_chunkNumbers.begin (), _chunkNumbers.end (), chunkNo) == _chunkNumbers.end ())
		_chunkNumbers.push_back (chunkNo);
}

std::ostream & operator<<(std::ostream & os, Sidetrack::Missing const & missingScript)
{
	if (missingScript.IsFullSynch ())
	{
		os << " " << GlobalIdPack (missingScript.ScriptId ()).ToSquaredString () << "; Full Sync Script; "; 
		os  << "next re-send request goes on " 
			<< PackedTimeStr (missingScript.NextTime ()).c_str () 
			<< " to project administrator.";
	}
	else
	{
		os << " " << GlobalIdPack (missingScript.ScriptId ()).ToSquaredString () << "; unit id: " 
			<< missingScript.UnitType ();
		if (missingScript.IsExhausted ())
			os << "; re-send list exhausted";
		else
			os  << "; next re-send request goes on " 
			<< PackedTimeStr (missingScript.NextTime ()).c_str () 
			<< "; to user " << std::hex << missingScript.NextRecipientId ();
		os << std::endl;
		std::vector<UserId> const & recipients = missingScript.GetRequestRecipients ();
		if (recipients.size () > 1)
		{
			os << "    Re-send request have been sent to the following project members: ";
			for (unsigned i = 0; i < recipients.size () - 1; ++i)
				os << std::hex << recipients [i] << ' ';
			os << std::endl;
		}
	}
	return os;
}

std::ostream & operator<<(std::ostream & os, Sidetrack::MissingChunk const & missingChunk)
{
	os << reinterpret_cast<Sidetrack::Missing const &>(missingChunk);
	os << std::endl;
	os << " Part count: " << std::dec << missingChunk.GetPartCount () << "; max chunk size: " << std::dec << missingChunk.GetMaxChunkSize () << std::endl;
	Sidetrack::MissingChunk::PartMap const & parts = missingChunk.GetPartMap ();
	if (parts.size () != 0)
	{
		os << "Received parts:" << std::endl;
		for (Sidetrack::MissingChunk::PartMap::const_iterator iter = parts.begin (); iter != parts.end (); ++iter)
		{
			std::pair<unsigned, std::string> const & part = *iter;
			os << "   " << std::dec << part.first << ": " << part.second << std::endl;
		}
	}
	return os;
}
