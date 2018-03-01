//------------------------------------
//  (c) Reliable Software, 1997 - 2007
//------------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "CmdExec.h"
#include "DispatcherScript.h"
#include "DispatcherCmd.h"
#include "Mailer.h"
#include "DataBase.h"
#include "History.h"
#include "Catalog.h"
#include "Registry.h"
#include "Addressee.h"
#include "PathFind.h"
#include "Agents.h"
#include "Transformer.h"
#include "OutputSink.h"
#include "Diff.h"
#include "BinDiffer.h"
#include "HistoryScriptState.h"
#include "FileList.h"

#include <Ex/Winex.h>

void Undo::DiffBuffer::SaveTo (std::string const & path) const
{
	// First, delete the version from before "undo"
	File::Delete (path);
	// Overwrite file with reconstructed version
	MemFileNew file (path, _size);
	// Copy undo buffer to file
	memcpy (file.GetBuf (), _recBuf.get (), _size);
}

CheckSum Undo::DiffBuffer::GetChecksum () const
{
	CheckSum cs (_recBuf.get (), File::Size (_size, 0));
	return cs;
}

void Undo::NewBuffer::SaveTo (std::string const & path) const
{
	// Saving undo new means delete file from disk.
	File::Delete (path);
}

//
// File command execs
//

std::unique_ptr<Undo::Buffer> CmdFileExec::Undo (std::string const & refFilePath) const
{
	Assert (!"CmdFileExec::Undo - should never be called!");
	return std::unique_ptr<Undo::Buffer>();
}

void CmdFileExec::VerifyParent (DataBase const & dataBase, Transformer const & trans) const throw (Win::Exception)
{
	bool parentIsInProject;
	UniqueName const & uname = trans.GetUniqueName ();
	FileData const * parentFd = dataBase.XFindByGid (uname.GetParentId ());
	if (parentFd != 0)
	{
		FileState parentState = parentFd->GetState ();
	    parentIsInProject =  parentState.IsPresentIn (Area::Project) || parentState.IsPresentIn (Area::Synch);
	}
	else
	{
		parentIsInProject = false;
	}

	if (!parentIsInProject)
	{
		std::string info ("Cannot add new ");
		info += trans.IsFolder () ? "folder" : "file";
		info += " to the project when its parent folder is not in the project.";
		Project::XPath projectPath (dataBase);
		throw Win::Exception (info.c_str (), projectPath.XMakePath (uname));
	}
}

void WholeFileExec::Do (DataBase & dataBase, 
						PathFinder & pathFinder, 
						TransactionFileList & fileList,
						bool inPlace)
{
	dbg << "WholeFileExec::Do" << std::endl;
	GlobalId gid = _command.GetGlobalId ();
	unsigned long size = _command.GetNewFileSize ().Low ();
	Area::Location targetArea = inPlace ? Area::Reference : Area::Synch;
	char const * path = pathFinder.XGetFullPath (gid, targetArea);

	if (!File::DeleteNoEx (path))
		throw Win::Exception ("Could not delete file from work area.", path);

	_command.SaveTo (path);
	CheckSum csFile (path);

	// Checksum comparison
	if (_command.GetNewCheckSum () != csFile)
		throw ScriptCorruptException();

	Transformer trans (dataBase, gid);

	// Check if parent folder is in the project
	if (!inPlace)
		VerifyParent (dataBase, trans);

	trans.UnpackNew (targetArea);
}

std::unique_ptr<Undo::Buffer> WholeFileExec::Undo (std::string const & refFilePath) const
{
	std::unique_ptr<Undo::Buffer> buf (new Undo::NewBuffer ());
	return buf;
}

void DeleteFileExec::Do (DataBase & dataBase, 
						 PathFinder & pathFinder, 
						 TransactionFileList & fileList,
						 bool inPlace)
{
	dbg << "DeleteFileExec::Do" << std::endl;
	GlobalId gid = _command.GetGlobalId ();
	Transformer trans (dataBase, gid);
	FileState state = _command.GetFileData ().GetState ();
	Area::Location targetArea = inPlace ? Area::Reference : Area::Synch;
	trans.UnpackDelete (pathFinder, fileList, targetArea, state.IsCoDelete ());
}

std::unique_ptr<Undo::Buffer> DeleteFileExec::Undo (std::string const & refFilePath) const
{
	_command.SaveTo (refFilePath);
	CheckSum checkSum (refFilePath);
	std::unique_ptr<Undo::Buffer> buf (new Undo::DeleteBuffer (checkSum));
	return buf;
}

void DiffFileExec::Do (DataBase & dataBase,
					   PathFinder & pathFinder,
					   TransactionFileList & fileList,
					   bool inPlace)
{
	dbg << "DiffFileExec::Do" << std::endl;
	GlobalId gid = _command.GetGlobalId ();
	Transformer trans (dataBase, gid);
	if (_command.IsContentsChanged ())
	{
		char const * refPath = pathFinder.XGetFullPath (gid, Area::Reference);
		MemFileReadOnly refFile (refPath);
		CheckSum csFile (refFile);
		if (_command.GetOldCheckSum () != csFile)
		{
			throw Win::Exception ("Cannot execute script editing command because of checksum mismatch.\n\n"
								"You probably modified your copy of the project file without checking it out.\n"
								"Reconstruct the original file by executing Project>Repair.",
								pathFinder.XGetFullPath (gid, Area::Project));
		}

		char const * synchPath = pathFinder.XGetFullPath (gid, Area::Synch);
		if (!File::DeleteNoEx (synchPath))
			throw Win::Exception ("Could not delete file from synch area", synchPath);
		MemFileNew fileSynch (synchPath, _command.GetNewFileSize ());
		// Transform reference file into synch version of the file
		Reconstruct (fileSynch.GetBuf (), _command.GetNewFileSize ().LowSmall (), 
					refFile.GetBuf (), refFile.GetSize ().LowSmall ());

		// Checksum comparison
		CheckSum csFileNew (fileSynch);
		if (_command.GetNewCheckSum () != csFileNew)
		{
			throw ScriptCorruptException();
		}
	}

	Area::Location targetArea;
	if (inPlace)
	{
		targetArea = Area::Reference;
		if (_command.IsContentsChanged ())
		{
			std::string refPath (pathFinder.XGetFullPath (gid, Area::Reference));
			// Move Synch to Reference
			if (!File::DeleteNoEx (refPath.c_str ()))
				throw Win::Exception ("Could not delete file from reference area", refPath.c_str ());
			char const * synchPath = pathFinder.XGetFullPath (gid, Area::Synch);
			File::Move (synchPath, refPath.c_str ());
			trans.MakeInArea (targetArea);
		}
	}
	else
	{
		targetArea = Area::Synch;
		if (_command.IsContentsChanged ())
		{
			// File already created in the Synch Area
			trans.MakeInArea (targetArea);
			// Propagate alias from Refernce Area if any
			trans.CopyAlias (Area::Reference, Area::Synch);
		}
		else
		{
			// Just rename -- copy file from Reference Area to Synch Area
			// Copying files copies aliases if any
			trans.CopyReference2Synch (pathFinder, fileList);
		}
	}

	// Rename if necessary
	FileData const & scriptFileData = _command.GetFileData ();
	if (scriptFileData.IsRenamedIn (Area::Original))
	{
		UniqueName const & newUname = scriptFileData.GetUniqueName ();
		if (!inPlace)
		{
			// If executing command from the incoming script verify target folder
			GlobalId parentGid = newUname.GetParentId ();
			FileData const * newParent = dataBase.XGetFileDataByGid (parentGid);
			FileState parentState = newParent->GetState ();
			if (parentState.IsToBeDeleted () &&	!parentState.WillBeRestored ())
			{
				// We cannot accept the move, because locally the new parent is being
				// removed from the project
				std::string info ("This script cannot be unpacked now, "
					"because it moves a file to the following folder:\n\n");
				Project::XPath projPath (dataBase);
				info += projPath.XMakePath (parentGid);
				GlobalIdPack pack (parentGid);
				info += " ";
				info += pack.ToBracketedString ();
				info += "\n\nwhich has been locally removed from the project.\n";
				info += "Uncheck-out the folder and then unpack the script.";
				TheOutput.Display (info.c_str ());
				throw Win::Exception ();
			}
		}
		trans.RenameIn (targetArea, newUname);
	}
	if (scriptFileData.IsTypeChangeIn (Area::Original))
	{
		trans.ChangeTypeIn (targetArea, scriptFileData.GetType ());
	}
}

void TextDiffFileExec::Reconstruct (char * newBuf, int newSize, const char * refBuf, int refSize) const
{
	LineBuf refLines (refBuf, _command.GetOldFileSize ().Low ()); 
	LineArray const & newLines = _command._newLines;
	LineArray const & oldLines = _command._oldLines;
	int count = _command._clusters.size ();
	int countDelLines = 0;
	int countAddLines = 0;
	int cur = 0;
	for (int i = 0; i < count; i++)
	{
		Cluster const & cluster = _command._clusters [i];
		int oldL = cluster.OldLineNo ();
		int newL = cluster.NewLineNo ();
		int cluLen = cluster.Len ();
		
		if (oldL == -1)
		{
			// add New lines
			for (int j = 0; j < cluLen; j++)
			{
				char const * lineBuf = newLines.GetLine (countAddLines);
				int len = newLines.GetLineLen (countAddLines);
				Assume (newLines.LineNumber (countAddLines) == newL + j, "Text Diff: New Lines");
				countAddLines++;
				if (len < 0 || len > newSize - cur)
					throw ScriptCorruptException ();
				memcpy (newBuf + cur, lineBuf, len);
				cur += len;
			}
		}
		else if (newL == -1)
		{
			// compare deleted lines (redundancy check)
			for (int j = 0; j < cluLen; j++)
			{
				char const * lineBuf = oldLines.GetLine (countDelLines);
				int len = oldLines.GetLineLen (countDelLines);
				Assume (oldLines.LineNumber (countDelLines) == oldL + j, "Text Diff: Old Lines");
				countDelLines++;
				Line const * refLine = refLines.GetLine (oldL + j);
				int refLineLen = static_cast<int> (refLine->Len ());
				if (len < 0 || len != refLineLen)
					throw ScriptCorruptException ();
				if (memcmp (lineBuf, refLine->Buf (), len) != 0)
					throw ScriptCorruptException ();
			}
		}
		else
		{
			// move lines
			for (int j = 0; j < cluLen; j++)
			{
				Line const * line = refLines.GetLine (oldL + j);
				int len = static_cast<int> (line->Len ());
				if (len < 0 || len > newSize - cur)
					throw ScriptCorruptException ();
				memcpy (newBuf + cur, line->Buf (), len);
				cur += line->Len ();
			}
		}
	}
}

std::unique_ptr<Undo::Buffer> TextDiffFileExec::Undo (std::string const & refFilePath) const
{
	MemFileAlways refFile (refFilePath);
	LineBuf refLines (refFile.GetBuf (), _command.GetNewFileSize ().Low ());
	std::unique_ptr<Undo::DiffBuffer> buf (new Undo::DiffBuffer (_command.GetOldFileSize ()));
	char * recBuf = buf->Get ();
	LineArray const & oldDiffLines = _command._oldLines;
	LineArray const & newDiffLines = _command._newLines;
	ClusterSort sortClusters (_command._clusters);
	sortClusters.SortByOldLineNo ();

	int count = _command._clusters.size ();
	int cur = 0;
	for (int i = 0; i < count; i++)
	{
		Cluster const * cluster = sortClusters [i];
		int oldL = cluster->OldLineNo ();
		int newL = cluster->NewLineNo ();
		int len = cluster->Len ();
		
		if (oldL == -1)
		{
			// these are added lines
			// we just skip them
			for (int j = 0; j < len; j++)
			{
				Line const * line = refLines.GetLine (newL + j);
				int idx = newDiffLines.FindLine (newL + j);
				if (idx == -1)
					throw Win::Exception ("Corrupt Script", _command.GetName ());
				char const * lineBuf = newDiffLines.GetLine (idx);
				int len = newDiffLines.GetLineLen (idx);
				if (len != line->Len () || memcmp (lineBuf, line->Buf (), len) != 0)
					throw Win::Exception ("Error reversing a script command: Added lines don't match",
										_command.GetName ());
			}
		}
		else if (newL == -1)
		{
			// deleted lines
			// we have to restore them
			for (int j = 0; j < len; j++)
			{
				int idx = oldDiffLines.FindLine (oldL + j);
				if (idx == -1)
					throw Win::Exception ("Corrupt Script", _command.GetName ());

				char const * lineBuf = oldDiffLines.GetLine (idx);
				int len = oldDiffLines.GetLineLen (idx);
				memcpy (&recBuf [cur], lineBuf, len);
				cur += len;
			}
		}
		else
		{
			// move lines
			// we have to move them back
			for (int j = 0; j < len; j++)
			{
				Line const * line = refLines.GetLine (newL + j);
				memcpy (&recBuf [cur], line->Buf (), line->Len ());
				cur += line->Len ();
			}
		}
	}
	return std::move(buf);
}

void BinDiffFileExec::Reconstruct (char * newBuf, int newLen, 
								   const char * refBuf, int refLen) const
{
	std::vector<MatchingBlock> blocks;	
	int count = _command._clusters.size ();	
	for (int i = 0; i < count; i++)
	{
		Cluster const & cluster = _command._clusters [i];
		int oldOffset = cluster.OldLineNo ();
		int newOffset = cluster.NewLineNo ();
		int len = cluster.Len ();
		if (oldOffset != -1 && newOffset != -1)
		{
			if (len < 0 || len > newLen - newOffset || len > refLen - oldOffset)
				throw ScriptCorruptException ();
			memcpy (newBuf + newOffset, refBuf + oldOffset, len);
			blocks.push_back (MatchingBlock (oldOffset, newOffset, len));
		}
	}
	// Find gaps between copied blocks
    GapsFinder gapFinder (_command.GetOldFileSize (), _command.GetNewFileSize (), blocks);
    if (_command._newLines.GetCount () != 0)
	{
		// All added blocks are combined into one "line"
		char const * srcBuffer = _command._newLines.GetLine (0);
		int srcLen = static_cast<int> (_command._newLines.GetLineLen (0));
		if (srcLen < 0)
			throw ScriptCorruptException ();
		// Fill the gaps
		std::vector<MatchingBlock> gaps;
		gapFinder.GetAddBlocks (gaps);
		int srcOffset = 0;
		std::vector <MatchingBlock>::iterator it;		
		for (it = gaps.begin (); it != gaps.end (); ++it)
		{
			int tgtOffset = it->DestinationOffset ();
			int len = it->Len ();
			if (len < 0 || len > newLen - tgtOffset || len > srcLen - srcOffset)
				throw ScriptCorruptException ();
			memcpy (newBuf + tgtOffset, srcBuffer + srcOffset, len);
			srcOffset += len;
		}
	}

    if (_command._oldLines.GetCount () != 0)
	{
		// All deleted blocks are combined into one "line"
		const char * bufDelete = _command._oldLines.GetLine (0);
		int bufDeleteLen = static_cast<int> (_command._oldLines.GetLineLen (0));
		if (bufDeleteLen < 0)
			throw ScriptCorruptException ();
		// Sanity check: compare deleted blocks with original file
		// comparing gaps with data in bufDelete
		std::vector<MatchingBlock> deletedBlocks;
		gapFinder.GetDeleteBlocks (deletedBlocks);
		int deleteBuffOffset = 0;
		std::vector <MatchingBlock>::iterator it;
		for (it = deletedBlocks.begin (); it != deletedBlocks.end (); ++it)
		{
			int oldOffset = it->SourceOffset ();
			int len  = it->Len ();
			if (len < 0 || len > bufDeleteLen - deleteBuffOffset || len > refLen - oldOffset)
				throw ScriptCorruptException ();
			if (memcmp (bufDelete + deleteBuffOffset, refBuf + oldOffset, len) != 0)
					throw ScriptCorruptException ();
			deleteBuffOffset  += len;
		}
	}
}

std::unique_ptr<Undo::Buffer> BinDiffFileExec::Undo (std::string const & refFilePath) const
{
	MemFileAlways refFile (refFilePath);
	const char * refBuf = refFile.GetBuf ();
	std::unique_ptr<Undo::DiffBuffer> buf (new Undo::DiffBuffer (_command.GetOldFileSize ()));
	char * recBuf = buf->Get ();
	std::vector<Cluster> const & clusters = _command._clusters;
	std::vector<MatchingBlock> blocks;
	std::vector<Cluster>::const_iterator itClu;
	for (itClu = clusters.begin (); itClu != clusters.end (); ++itClu)
	{
		int oldOffset = itClu->OldLineNo ();
		int newOffset = itClu->NewLineNo ();
		int len = itClu->Len ();		
		if (oldOffset != -1 && newOffset != -1)
		{
			// moved blocks
			// we have to move them back			
			memcpy (recBuf + oldOffset, refBuf + newOffset, len);
			blocks.push_back (MatchingBlock (oldOffset, newOffset, len));
		}
	}

	GapsFinder gapFinder (_command.GetOldFileSize (), _command.GetNewFileSize (), blocks);
	// Sanity check: comparing gaps with data in bufAdd
	if (_command._newLines.GetCount () != 0)
	{
		// All added blocks are combined into one "line"
		char const * blockAdd = _command._newLines.GetLine (0);
		std::vector<MatchingBlock> blocksAdd;
		gapFinder.GetAddBlocks (blocksAdd);
		int addBuffOffset = 0;
		std::vector <MatchingBlock>::iterator it;		
		for (it = blocksAdd.begin (); it != blocksAdd.end (); ++it)
		{
			int newOffset = it->DestinationOffset ();
			int len = it->Len ();
			if (memcmp (blockAdd + addBuffOffset, refBuf + newOffset, len) != 0)
				throw Win::Exception ("Error reversing a script command: Added blocks don't match",
									_command.GetName ());
			addBuffOffset += len;
		}
	}

	if (_command._oldLines.GetCount () != 0)
	{
		// All deleted blocks are combined into one "line"
		char const * blockDel = _command._oldLines.GetLine (0);
		// filling gaps with data from bufDelete	
		std::vector<MatchingBlock> blocksDelete;
		gapFinder.GetDeleteBlocks (blocksDelete);
		int deleteBuffOffset = 0;
		std::vector <MatchingBlock>::iterator it;
		for (it = blocksDelete.begin (); it != blocksDelete.end (); ++it)
		{
			int oldOffset = it->SourceOffset ();
			int len  = it->Len ();
			memcpy (recBuf + oldOffset, blockDel + deleteBuffOffset, len);
			deleteBuffOffset += len;
		}
	}
	return std::move(buf);
}

void NewFolderExec::Do (DataBase & dataBase, 
						PathFinder & pathFinder, 
						TransactionFileList & fileList,
						bool inPlace)
{
	dbg << "NewFolderExec::Do" << std::endl;
	Transformer trans (dataBase, _command.GetGlobalId ());

	// Check if parent folder is in the project
	if (!inPlace)
		VerifyParent (dataBase, trans);

	Area::Location targetArea = inPlace ? Area::Reference : Area::Synch;
	trans.UnpackNew (targetArea);
}

void DeleteFolderExec::Do (DataBase & dataBase, 
						   PathFinder & pathFinder, 
						   TransactionFileList & fileList,
						   bool inPlace)
{
	dbg << "DeleteFolderExec::Do" << std::endl;
	GlobalId gid = _command.GetGlobalId ();
	Transformer trans (dataBase, gid);
	FileState state = _command.GetFileData ().GetState ();
	Area::Location area = inPlace ? Area::Reference : Area::Synch;
	trans.UnpackDelete (pathFinder, fileList, area, state.IsCoDelete ());
}

//
// Control command execs
//

void CmdAckExec::Do (Unit::Type unitType, AckBox & ackBox, bool isSenderReceiver)
{
	GlobalId scriptId = _command.GetScriptId ();
	_history.XAcceptAck (scriptId, _sender, unitType, ackBox);
	if (unitType == Unit::Set)
		_history.Notify (changeEdit, scriptId);
}

void CmdMakeReferenceExec::Do (Unit::Type unitType, AckBox & ackBox, bool isSenderReceiver)
{
	GlobalId scriptId = _command.GetScriptId ();
	_history.XAcceptAck (scriptId, _sender, unitType, ackBox, false);	// Don't broadcast make ref
	if (unitType == Unit::Set)
		_history.Notify (changeEdit, scriptId);
}

void CmdResendRequestExec::Do (Unit::Type unitType, AckBox & ackBox, bool isSenderReceiver)
{
	GlobalId requestedScriptId = _command.GetScriptId ();
	dbg << "Resending " << GlobalIdPack (requestedScriptId) << std::endl;
	History::ScriptState scriptState;
	if (_history.XIsRecorded (requestedScriptId, scriptState, unitType))
	{
		if (!scriptState.IsInventory ())
			_history.XRetrieveScript (requestedScriptId, _hdr, _cmdList, unitType);
	}
	else
	{
		dbg << "Not found in history" << std::endl;
	}
}

void CmdVerificationRequestExec::Do (Unit::Type unitType, AckBox & ackBox, bool isSenderReceiver)
{
	_hdr.SetScriptKind (ScriptKindVerificationPackage ());
	_hdr.SetUnitType (Unit::Ignore);
	_hdr.AddTimeStamp (CurrentTime ());
	_hdr.AddComment ("Project verification package");
	// include receiver lineages only when sender is not a receiver
	_history.XGetLineages (_hdr, UnitLineage::Maximal, !isSenderReceiver); 
	GlobalId senderReferenceId = _command.GetScriptId ();
	Lineage lineage;
	_history.XGetLineageStartingWith (lineage, senderReferenceId);
	if (lineage.Count () > 1)
	{
		// Add set lineage only if it is longer then just the sender reference version
		std::unique_ptr<UnitLineage> sideLineage (new UnitLineage (lineage, Unit::Set, gidInvalid));
		_hdr.AddSideLineage (std::move(sideLineage));
	}
	VerificationRequestCmd const & verificationRequest =
		dynamic_cast<VerificationRequestCmd const &>(_command);
	if (!isSenderReceiver)
		_history.XRetrieveDefectedTrees (verificationRequest.GetKnownDeadMembers (), _scriptList);
}

//
// Member command execs
//

void CmdNewMemberExec::Do (ThisUserAgent & agent)
{
	// New user has joined the project
	MemberInfo const & newUserInfo = _command.GetMemberInfo ();
	_projectDb.XAddMember (newUserInfo);
}

void CmdDeleteMemberExec::Do (ThisUserAgent & agent)
{
	// Project member has defected from the project
	MemberInfo defectInfo (_command.GetMemberInfo ());
	defectInfo.SetSuicide (true);
	UserId userId = defectInfo.Id ();
	if (_projectDb.XIsProjectMember (userId))
	{
		MemberState userState = _projectDb.XGetMemberState (userId);
		if (userState.IsDead ())
			return;	// We already know about this member removal from the project -- ignore update

		std::unique_ptr<MemberInfo> currentInfo = _projectDb.XRetrieveMemberInfo (userId);
		currentInfo->SetState (defectInfo.State ());
		_projectDb.XUpdate (*currentInfo);
	}
	else
	{
		// We didn't know that project member and now we have received
		// his/her defect script -- add member data so we have trace of
		// his/her existence.
		_projectDb.XAddMember (defectInfo);
	}
}

void CmdEditMemberExec::Do (ThisUserAgent & agent)
{
	MemberInfo const & oldInfo = _command.GetOldMemberInfo ();
	MemberInfo const & newInfo = _command.GetNewMemberInfo ();
	if (oldInfo.Id () != newInfo.Id ())
	{
		// Version 4.2 edit member command
		ExecuteV40Cmd (agent);
		return;
	}

	GlobalId changedUserId = newInfo.Id ();
	Assert (changedUserId != gidInvalid);
	if (_projectDb.XIsProjectMember (changedUserId))
	{
		ChangeUserData (oldInfo, newInfo, agent);
	}
	else
	{
		// We have received membership update for the member we don't know.
		_projectDb.XAddMember (newInfo);
	}
}

void CmdEditMemberExec::ExecuteV40Cmd (ThisUserAgent & agent)
{
	MemberInfo const & oldInfo = _command.GetOldMemberInfo ();
	MemberInfo const & newInfo = _command.GetNewMemberInfo ();
	Assert (oldInfo.Id () != newInfo.Id ());

	if (oldInfo.Id () != gidInvalid)
	{
		// Emeregency administrator election
		if (_projectDb.XIsProjectMember (oldInfo.Id ()))
		{
			std::unique_ptr<MemberInfo> currentOldInfo = _projectDb.XRetrieveMemberInfo (oldInfo.Id ());
			ChangeUserData (*currentOldInfo, oldInfo, agent);
		}
		else
		{
			// We have received membership update for the member we don't know.
			_projectDb.XAddMember (oldInfo);
		}
	}

	// Regular membership update
	if (_projectDb.XIsProjectMember (newInfo.Id ()))
	{
		std::unique_ptr<MemberInfo> currentnewInfo = _projectDb.XRetrieveMemberInfo (newInfo.Id ());
		ChangeUserData (*currentnewInfo, newInfo, agent);
	}
	else
	{
		// We have received membership update for the member we don't know.
		_projectDb.XAddMember (newInfo);
	}
}

void CmdEditMemberExec::ChangeUserData (MemberInfo const & oldInfo, MemberInfo const & newInfo, ThisUserAgent & agent)
{
	// Editable fields in the member info:
	//    1. State
	//    2. Name
	//    3. Hub's Email Address
	//    4. Comment (phone)
	//    5. License
	Assert (oldInfo.Id () == newInfo.Id ());
	UserId changedUserId = newInfo.Id ();
	Assert (IsValidUid (changedUserId));
	Assert (changedUserId != gidInvalid);
	Assert (_projectDb.XIsProjectMember (changedUserId));
	Assert ((oldInfo.State ().IsVerified () == newInfo.State ().IsVerified ()) &&
			(oldInfo.State ().IsReceiver () == newInfo.State ().IsReceiver ()) &&
			(oldInfo.State ().IsDistributor () == newInfo.State ().IsDistributor ()) &&
			(oldInfo.State ().NoBranching () == newInfo.State ().NoBranching ()));
	std::unique_ptr<MemberInfo> currentInfo = _projectDb.XRetrieveMemberInfo (changedUserId);
	MemberState curState = currentInfo->State ();
	// Don't allow state change when verified project member have defected.
	bool stateChange = curState.IsDead () && curState.IsVerified () ? false : !newInfo.State ().IsEqual (oldInfo.State ());
	bool descriptionChange = !newInfo.Description ().IsEqual (oldInfo.Description ());
	bool changesDetected = descriptionChange || stateChange;
	if (!changesDetected)
		return;

	// Update only changed fields
	if (stateChange)
		currentInfo->SetState (newInfo.State ());

	if (descriptionChange)
	{
		if (!IsNocaseEqual (newInfo.Name (), oldInfo.Name ()))
			currentInfo->SetName (newInfo.Name ());

		if (!IsNocaseEqual (newInfo.HubId (), oldInfo.HubId ()))
		{
			currentInfo->SetHubId (newInfo.HubId ());
			if (changedUserId == _projectDb.XGetMyId ())
				agent.HubIdChange (newInfo.HubId ());
		}

		if (!IsNocaseEqual (newInfo.Comment (), oldInfo.Comment ()))
			currentInfo->SetComment (newInfo.Comment ());

		if (!IsNocaseEqual (newInfo.License (), oldInfo.License ()))
			currentInfo->SetLicense (newInfo.License ());

		agent.DescriptionChange (changedUserId == _projectDb.XGetMyId ());
	}

	Assert (changesDetected);
	_projectDb.XUpdate (*currentInfo);
}
