//------------------------------------
//  (c) Reliable Software, 2003 - 2007
//------------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "ScriptCommands.h"
#include "DumpWin.h"
#include "PathFind.h"
#include "CmdExec.h"
#include "Diff.h"
#include "CluSum.h"
#include "BinDiffer.h"
#include "LineBuf.h"
#include "LineCounter.h"
#include "ProjectDb.h"

#include <Ex/WinEx.h>
#include <StringOp.h>

//
// FileCmd
//

std::unique_ptr<FileCmd> FileCmd::DeserializeCmd (Deserializer& in, int version)
{
	ScriptCmdType type = static_cast<ScriptCmdType> (in.GetLong ());
	std::unique_ptr<FileCmd> command;
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
	default:
		throw Win::Exception ("Corrupt script: Unknown file command type.");
	}
	command->Deserialize (in, version);
	return command;
}

bool FileCmd::Verify () const
{
	return _fileData.VerifyFileType ();
}

void FileCmd::Dump (DumpWindow & dumpWin, PathFinder const & pathFinder) const
{
	char const * projectPath = pathFinder.GetRootRelativePath (_fileData.GetGlobalId ());
	if (projectPath == 0)
		projectPath = _fileData.GetName ().c_str ();

	FileType type = _fileData.GetType ();
	std::string info (type.IsFolder () ? "Folder" : "File");
	info += " \"";
	info += projectPath;
	info += "\"";
	dumpWin.PutLine (info.c_str (), DumpWindow::styH1);
	GlobalIdPack gid (GetGlobalId ());
	std::string gidInfo ("Global id: ");
	gidInfo += gid.ToString ();
	dumpWin.PutLine (gidInfo.c_str ());
	GlobalIdPack gidParent (_fileData.GetUniqueName ().GetParentId ());
	std::string parentGid ("Parent id: ");
	parentGid += gidParent.ToString ();
	dumpWin.PutLine (parentGid.c_str ());
	std::string typeInfo ("Type: ");
	typeInfo += type.GetName ();
	dumpWin.PutLine (typeInfo.c_str ());
	std::string checksumInfo ("Checksum: old = 0x");
	checksumInfo += ToHexString (GetOldCheckSum ().GetSum ());
	checksumInfo += "; new = 0x";
	checksumInfo += ToHexString (_newCheckSum.GetSum ());
	dumpWin.PutLine (checksumInfo.c_str ());
	std::string crcInfo ("CRC: old = 0x");
	crcInfo += ToHexString (GetOldCheckSum ().GetCrc ());
	crcInfo += "; new = 0x";
	crcInfo += ToHexString (_newCheckSum.GetCrc ());
	dumpWin.PutLine (crcInfo.c_str ());
	dumpWin.PutLine ("Changes:", DumpWindow::styH2);
}

bool FileCmd::IsEqual (ScriptCmd const & cmd) const
{
	// File command are considered equal if they change
	// the same file and edit changes lead to the same
	// new file checksums.
	FileCmd const & fileCmd = dynamic_cast<FileCmd const &>(cmd);
	if (GetGlobalId () != fileCmd.GetGlobalId ())
		return false;	// Changes different files
	if (GetNewCheckSum () != fileCmd.GetNewCheckSum ())
		return false;	// New checksums don't match
	return true;
}

void FileCmd::Serialize (Serializer& out) const
{
	out.PutLong (GetType ());
	_fileData.Serialize (out);
	out.PutLong (_newCheckSum.GetCrc ());
	out.PutLong (_newCheckSum.GetSum ());
}

void FileCmd::Deserialize (Deserializer& in, int version)
{
	if (version < 12)
	{
		GlobalId gid = in.GetLong ();
		GlobalId parentId = GlobalId (-2); // revisit: what is -2?
		if (version < 11)
		{
			// No parent id in the script command
		}
		else
		{
			parentId = in.GetLong ();
		}
		SerString name;
		name.Deserialize (in, version);
		_fileData.SetName (parentId, name);
		_fileData.SetGid (gid);
		TextFile type;
		_fileData.SetType (type);
	}
	else
		_fileData.Deserialize (in, version);

	_fileData.Verify ();

	unsigned long newCrc = in.GetLong ();
	unsigned long newSum = in.GetLong ();
	_newCheckSum.Init (newSum, newCrc);
	if (version < 35)
	{
		// Patch the checksum in older versions of FileData
		// Old checksum was stored in place of new crc
		unsigned long oldSum = _newCheckSum.GetCrc ();
		if (version < 16)
		{
			_newCheckSum.SetWild ();
		}
		_fileData.SetCheckSum (CheckSum (oldSum, CheckSum::crcWildCard));
		_newCheckSum.Init (newSum, CheckSum::crcWildCard);
	}
}

// LazyFileBuf
// Can hold the contents of a file or a path to the file on disk

void FileCmd::LazyFileBuf::SetSource (std::string const & filePath,
									  File::Size fileSize,
									  File::Offset fileOffset,
									  bool deleteFile)
{
	if (fileSize.IsLarge ())
		throw Win::InternalException ("File size exceeds 4GB", filePath.c_str ());

	_fileSize = fileSize;
	_fileOffset = fileOffset;
	_deleteFile = deleteFile;
	_filePath = filePath;
}

void FileCmd::LazyFileBuf::CopyIn (char const * buf, File::Size len)
{
	if (len.IsLarge ())
		throw Win::InternalException ("File size exceeds 4GB");
	_fileSize = len;
	_buf = new char [_fileSize.Low ()];
	memcpy (_buf, buf, _fileSize.Low ());
}

void FileCmd::LazyFileBuf::Write (Serializer& out) const
{
	if (_fileSize.IsLarge ())
		throw Win::InternalException ("File size exceeds 4GB", _filePath.c_str ());

	unsigned long len = _fileSize.Low ();
	if (_buf != 0)
	{
		out.PutBytes (_buf, len);
	}
	else
	{
		Assert (!_filePath.empty ());

		if (_fileOffset.IsZero ())
		{
			FileInfo fileInfo (_filePath);
			if (fileInfo.GetSize () != _fileSize)
				throw Win::InternalException ("File command: file size mismach.", _filePath.c_str ());
		}

		FileIo in (_filePath, File::ReadOnlyMode ());
		in.SetPosition (_fileOffset);

		unsigned long const TenMegaBytes = 0x1000000;
		unsigned long chunkSize = len < TenMegaBytes ? len : TenMegaBytes;
		std::vector<unsigned char> ioBuf (chunkSize);
		while (len != 0)
		{
			in.Read (&ioBuf [0], chunkSize);
			out.PutBytes (&ioBuf [0], chunkSize);
			len -= chunkSize;
			chunkSize = len < TenMegaBytes ? len : TenMegaBytes;
		}
	}
}

void FileCmd::LazyFileBuf::Serialize (Serializer & out) const
{
	unsigned long len = _fileSize.Low ();
	out.PutLong (len);
	Write (out);
}

void FileCmd::LazyFileBuf::Deserialize (Deserializer & in, int version)
{
	// Lazy buffer deserialization
	unsigned long bufSize = in.GetLong ();
	File::Size fileSize (bufSize, 0);
	File::Offset fileOffset = in.GetPosition ();
	SetSource (in.GetPath (), fileSize, fileOffset);
	fileOffset += fileSize;
	in.SetPosition (fileOffset);
}

void FileCmd::LazyFileBuf::Dump (DumpWindow & dumpWin) const
{
	if (_buf != 0)
	{
		std::string contents (_buf, _fileSize.Low ());
		dumpWin.PutLine (contents, DumpWindow::styCodeFirst);
	}
	else
		dumpWin.PutLine (_filePath, DumpWindow::styNormal);
}

//
// WholeFileCmd
//

void WholeFileCmd::Serialize (Serializer& out) const
{
	FileCmd::Serialize (out);
#if !defined(NDEBUG)
	long long pos = 0;
	if (out.IsCounting())
	{
		CountingSerializer const * counter = dynamic_cast<CountingSerializer const *>(&out);
		pos = counter->GetSize();
		dbg << "  ~  " << GetName() << " offset: " << pos << ", size: " << GetNewFileSize().Low() << std::endl;
	}
#endif

	_buf.Serialize (out);

#if !defined(NDEBUG)
	if (out.IsCounting())
	{
		CountingSerializer const * counter = dynamic_cast<CountingSerializer const *>(&out);
		dbg << "  ~  serialized size: " << counter->GetSize() - pos << std::endl;
	}
#endif
}

void WholeFileCmd::Deserialize (Deserializer& in, int version)
{
	// The command type has been read by ScriptCmd::DeserializeCmd
	FileCmd::Deserialize (in, version);
	long long pos = in.GetPosition().ToMath();
	dbg << "  ~  "  << GetName() << " offset: " << pos << " size: " << GetOldFileSize().Low() << std::endl;
	if (version < 35)
	{
		_fileData.SetCheckSum (CheckSum ()); // patch old inconsistency 
		if (version < 10)
			in.GetLong ();	//Eat redundant length
	}
	_buf.Deserialize (in, version);
	dbg << "  ~  deserialized size: " << in.GetPosition().ToMath() - pos << std::endl;
}

void WholeFileCmd::SaveTo (std::string const & path) const
{
	FileSerializer out (path);
	_buf.Write (out);
}

void WholeFileCmd::Dump (DumpWindow & dumpWin, PathFinder const & pathFinder) const
{
	FileCmd::Dump (dumpWin, pathFinder);
	dumpWin.PutLine ("Newly created");
	dumpWin.PutLine ("File contents:", DumpWindow::styH2);
	if (GetFileType ().IsTextual ())
	{
		_buf.Dump (dumpWin);
	}
	else
	{
		dumpWin.PutLine ("Binary file -- contents not displayed.");
	}
}

std::unique_ptr<CmdFileExec> WholeFileCmd::CreateExec () const
{
	return std::unique_ptr<CmdFileExec> (new WholeFileExec (*this));
}

//
// DeleteCmd
//

void DeleteCmd::Serialize (Serializer& out) const
{
	FileCmd::Serialize (out);
	_buf.Serialize (out);
}

void DeleteCmd::Deserialize (Deserializer& in, int version)
{
	// The command type has been read by ScriptCmd::DeserializeCmd
	FileCmd::Deserialize (in, version);
	if (version < 10)
		in.GetLong ();	//Eat redundant length
	_buf.Deserialize (in, version);
}

void DeleteCmd::SaveTo (std::string const & path) const
{
	FileSerializer out (path);
	_buf.Write (out);
}

void DeleteCmd::Dump (DumpWindow & dumpWin, PathFinder const & pathFinder) const
{
	FileCmd::Dump (dumpWin, pathFinder);
	if (_fileData.GetState ().IsCoDelete ())
		dumpWin.PutLine ("Deleted");
	else
		dumpWin.PutLine ("Removed");
	dumpWin.PutLine ("File contents:", DumpWindow::styH2);
	if (GetFileType ().IsTextual ())
	{
		_buf.Dump (dumpWin);
	}
	else
	{
		dumpWin.PutLine ("Binary file -- contents not displayed.");
	}
}

SynchKind DeleteCmd::GetSynchKind () const
{
	FileState state = _fileData.GetState ();
	return state.IsCoDelete () ? synchDelete : synchRemove;
}

std::unique_ptr<CmdFileExec> DeleteCmd::CreateExec () const
{
	return std::unique_ptr<CmdFileExec> (new DeleteFileExec (*this));
}

//
// Diff commands
//

bool LineArray::IsEqual (LineArray const & lineArray) const
{
	if (_offsets.size () != lineArray._offsets.size ())
		return false;
	if (_offsets != lineArray._offsets)
		return false;
	if (_lineNo.size () != lineArray._lineNo.size ())
		return false;
	if (_lineNo != lineArray._lineNo)
		return false;
	// Revisit: _bufLen is ALWAYS equal zero and the buffer length is stored in _end
	if (_bufLen != lineArray._bufLen)
		return false;
	if (_end != lineArray._end)
		return false;
	if (memcmp (_buf, lineArray._buf, _end) != 0)
		return false;
	return true;
}

void DiffCmd::AddCluster (Cluster const & cluster)
{
	// by value
	_clusters.push_back (cluster);
}

void DiffCmd::AddOldLine (int oldNum, SimpleLine const & line)
{
	int len = line.Len ();
	_oldLines.Add (oldNum, len, line.Buf ());
}

void DiffCmd::AddNewLine (int newNum, SimpleLine const & line)
{
	int len = line.Len ();
	_newLines.Add (newNum, len, line.Buf ());
}

class PartialDiff: public DifferSource
{
public:
	PartialDiff (std::vector<Cluster> & clus, LineArray const & oldLines, LineArray const & newLines)
		: _oldLines (oldLines), _newLines (newLines), _line (0, 0)
	{
		for (std::vector<Cluster>::iterator it = clus.begin (); it != clus.end (); ++it)
		{
			Cluster & clu = *it;
			_clus.push_back (&clu);
		}
	}
	SimpleLine const * GetNewLine (int i) const;
	SimpleLine const * GetOldLine (int i) const;
	CluSeq GetClusterSeq () const;
	EditStyle::Source GetChangeSource () const { return EditStyle::chngUser; }

private:
	std::vector<Cluster *>	_clus;
	LineArray const &		_oldLines;
	LineArray const &		_newLines;
	mutable SimpleLine		_line;
};

CluSeq PartialDiff::GetClusterSeq () const
{
	return CluSeq (_clus.begin (), _clus.end ());
}

SimpleLine const * PartialDiff::GetNewLine (int i) const
{
	int idx = _newLines.FindLine (i);
	if (idx == -1)
		_line.SetLen (-1); // skip line
	else
	{
		_line.Init (_newLines.GetLine (idx), _newLines.GetLineLen (idx));
	}
	return &_line;
}

SimpleLine const * PartialDiff::GetOldLine (int i) const
{
	int idx = _oldLines.FindLine (i);
	if (idx == -1)
		_line.SetLen (-1); // skip line
	else
		_line.Init (_oldLines.GetLine (idx), _oldLines.GetLineLen (idx));

	return &_line;
}

SynchKind DiffCmd::GetSynchKind () const
{
	if (_fileData.IsRenamedIn (Area::Original))
	{
		UniqueName const & name = _fileData.GetUniqueName ();
		UniqueName const & alias = _fileData.GetUnameIn (Area::Original);
		return name.GetParentId () == alias.GetParentId () ? synchRename : synchMove;
	}
	else
	{
		return synchEdit;
	}
}

bool DiffCmd::IsEqual (ScriptCmd const & cmd) const
{
	if (!FileCmd::IsEqual (cmd))
		return false;

	DiffCmd const & diffCmd = reinterpret_cast<DiffCmd const &>(cmd);
	if (_oldFileSize != diffCmd.GetOldFileSize ())
		return false;
	if (_newFileSize != diffCmd.GetNewFileSize ())
		return false;
	if (_clusters.size () != diffCmd._clusters.size ())
		return false;
	if (!_newLines.IsEqual (diffCmd._newLines))
		return false;
	if (!_oldLines.IsEqual (diffCmd._oldLines))
		return false;
	return true;
}

void DiffCmd::DisplayChanges (DumpWindow & dumpWin, PathFinder const & pathFinder) const
{
	FileData const & fileData = GetFileData ();
	UniqueName const & uname = fileData.GetUniqueName ();
	if (fileData.IsRenamedIn (Area::Original))
	{
		UniqueName const & oldOriginal = fileData.GetUnameIn (Area::Original);
		if (oldOriginal.GetParentId () == uname.GetParentId ())
		{
			std::string info ("Renamed from '");
			info += oldOriginal.GetName ();
			info += "' to '";
			info += uname.GetName ();
			info += '\'';
			dumpWin.PutLine (info.c_str ());
		}
		else
		{
			std::string info ("Moved from: ");
			info += pathFinder.GetRootRelativePath (oldOriginal);
			info += " to: ";
			info += pathFinder.GetRootRelativePath (uname);
			dumpWin.PutLine (info.c_str ());
		}
	}
	if (fileData.IsTypeChangeIn (Area::Original))
	{
		FileType const & oldType = fileData.GetTypeIn (Area::Original);
		FileType const & newType = fileData.GetType ();
		std::string info ("Type changed from: ");
		info += oldType.GetName ();
		info += " to ";
		info += newType.GetName ();
		dumpWin.PutLine (info.c_str ());
	}
}

void DiffCmd::Serialize (Serializer& out) const
{
	FileCmd::Serialize (out);
	out.PutLong (_oldFileSize.Low ());
	out.PutLong (_newFileSize.Low ());
	int count = _clusters.size ();
	out.PutLong (count);
	for (int i = 0; i < count; i++)
	{
		out.PutLong (_clusters [i].OldLineNo ());
		out.PutLong (_clusters [i].NewLineNo ());
		out.PutLong (_clusters [i].Len ());
	}
	_newLines.Serialize (out);
	_oldLines.Serialize (out);
}

void DiffCmd::Deserialize (Deserializer& in, int version)
{
	// The command type has been read by ScriptCmd::DeserializeCmd
	FileCmd::Deserialize (in, version);

	_oldFileSize = File::Size (in.GetLong (), 0);
	_newFileSize = File::Size (in.GetLong (), 0);
	int count = in.GetLong ();
	// Resource Management! Make it auto_array
	// Not relevant any longer? Kalle Dalheimer, 000229
	_clusters.clear();
	for (int i = 0; i < count; i++)
	{
		int oldL = in.GetLong ();
		int newL = in.GetLong ();
		int len  = in.GetLong ();
		_clusters.push_back( Cluster (oldL, newL, len) );
	}
	_newLines.Deserialize (in, version);
	_oldLines.Deserialize (in, version);
}

std::unique_ptr<CmdFileExec> TextDiffCmd::CreateExec () const
{
	return std::unique_ptr<CmdFileExec> (new TextDiffFileExec (*this));
}

void TextDiffCmd::Dump (DumpWindow & dumpWin, PathFinder const & pathFinder) const
{
	FileCmd::Dump (dumpWin, pathFinder);
	if (GetFileType ().IsTextual ())
	{
		if (_clusters.size () == 0)
		{
			DisplayChanges (dumpWin, pathFinder);
		}
		else
		{
			dumpWin.PutLine ("Edited");
			DisplayChanges (dumpWin, pathFinder);
			dumpWin.PutLine ("File edits:", DumpWindow::styH2);
			dumpWin.PutLine (" Old     New", DumpWindow::styCodeFirst);
			dumpWin.PutLine (" line    line   Contents or number of unchanged lines", DumpWindow::styCodeFirst);
			dumpWin.PutLine (" no.     no.", DumpWindow::styCodeFirst);
			dumpWin.PutLine ("-----------------------------------------------------\n", DumpWindow::styCodeFirst);

			//Revisit: Why PartialDiff doesn't take std::vector<Cluster> const & ?
			PartialDiff diff (const_cast<std::vector<Cluster> &>(_clusters), _oldLines, _newLines);
			ClusterSum cluSum (diff);
			LineCounterDiff counter;
			for (; !cluSum.AtEnd (); cluSum.Advance ())
			{
				cluSum.DumpCluster (dumpWin, EditStyle::chngUser, counter);
			}
		}
	}
	else
	{
		dumpWin.PutLine ("Edited");
		DisplayChanges (dumpWin, pathFinder);
		dumpWin.PutLine ("File edits:", DumpWindow::styH2);
		dumpWin.PutLine ("Binary file -- contents changes not displayed");
	}
}

std::unique_ptr<CmdFileExec> BinDiffCmd::CreateExec () const
{
	return std::unique_ptr<CmdFileExec> (new BinDiffFileExec (*this));
}

void BinDiffCmd::Dump (DumpWindow & dumpWin, PathFinder const & pathFinder) const
{
	FileCmd::Dump (dumpWin, pathFinder);
	dumpWin.PutLine ("Edited");
	DisplayChanges (dumpWin, pathFinder);

	dumpWin.PutLine ("File edits:", DumpWindow::styH2);

	if (GetFileType ().IsTextual ())
	{
		dumpWin.PutLine ("Binary diff of ASCII file -- no line number information available");
		std::vector<MatchingBlock> blocks;
		int count = _clusters.size ();	
		for (int i = 0; i < count; i++)
		{
			Cluster const & cluster = _clusters [i];
			int oldOffset = cluster.OldLineNo ();
			int newOffset = cluster.NewLineNo ();
			int len = cluster.Len ();
			if (oldOffset != -1 && newOffset != -1)	
				blocks.push_back(MatchingBlock (oldOffset, newOffset, len));
		}
		GapsFinder gapsFinder (_oldFileSize, _newFileSize, blocks);
		const char * bufAdd = 0;
		if (_newLines.GetCount () != 0)
			 bufAdd = _newLines.GetLine (0);

		const char * bufDelete = 0;		
		if (_oldLines.GetCount () != 0)
			 bufDelete = _oldLines.GetLine (0);

		std::vector<MatchingBlock> blocksAdd;
		gapsFinder.GetAddBlocks (blocksAdd);
		int addBuffOffset = 0;
		std::vector<MatchingBlock> blocksDelete;
		gapsFinder.GetDeleteBlocks (blocksDelete);
		int deleteBuffOffset = 0;
		EditStyle::Action action;	
		std::vector <MatchingBlock>::iterator itAdd = blocksAdd.begin ();
		std::vector <MatchingBlock>::iterator itDel = blocksDelete.begin ();

		while (itAdd != blocksAdd.end () || itDel != blocksDelete.end ())
		{
			const char * currentBuf = 0;
			int lenBlock;
			int offset;
			
			if (itAdd == blocksAdd.end ()
				||(itDel != blocksDelete.end () && itDel->SourceOffset () <= itAdd->DestinationOffset ()) )
			{// ran out of add blocks OR next delete block precedes the next add block
				lenBlock = itDel->Len ();
				currentBuf = bufDelete + deleteBuffOffset;
				deleteBuffOffset += lenBlock;
				offset = itDel->SourceOffset ();
				action = EditStyle::actDelete;
				++itDel;
			}
			else 
			{// there is an add block AND there is no delete block before it
				offset = itAdd->DestinationOffset ();
				lenBlock = itAdd->Len ();
				currentBuf = bufAdd + addBuffOffset;
				addBuffOffset += lenBlock ;
				action = EditStyle::actInsert;
				++itAdd;
			}
			EditStyle edit (EditStyle::chngUser, action);
			LineBuf lines (currentBuf, lenBlock);
			int count = lines.Count ();
			for (int k = 0; k < count; ++k)
			{
				const Line * line = lines.GetLine (k);
				dumpWin.PutLine (line->Buf (), edit);
			}
		}	
	}
	else
	{
		dumpWin.PutLine ("Binary file -- contents changes not displayed");
	}
}

void LineArray::Add (int lineNo, int len, char const * buf)
{
	if (_end + len + 1 > _bufLen)
		Grow (_end + len + 1);
	memcpy (_buf + _end, buf, len);
	_offsets.push_back (_end);
	_end += len;
	_buf [_end] = '\0';
	_end++;
	_lineNo.push_back (lineNo);
};

int LineArray::FindLine (int lineNo) const
{
  unsigned int i = 0;
	for (i = 0; i < _lineNo.size (); i++)
		if (_lineNo [i] == lineNo)
			break;
	if (_lineNo.size () == i)
		i = -1;
	return i;
}

void LineArray::Grow (int newSize)
{
	int newLen = _bufLen * 2;
	if (newLen < newSize)
		newLen = newSize;
	char * newBuf = new char [newLen];
	memcpy (newBuf, _buf, _end);
	_bufLen = newLen;
	delete []_buf;
	_buf = newBuf;
}

void LineArray::Serialize (Serializer& out) const
{
	int count = _offsets.size ();
	out.PutLong (count);
	int i = 0;
	for (i = 0; i < count; i++)
		out.PutLong (_offsets [i]);
	for (i = 0; i < count; i++)
		out.PutLong (_lineNo [i]);
	out.PutLong (_end);
	out.PutBytes (_buf, _end);
}

void LineArray::Deserialize (Deserializer& in, int version)
{
	int count = in.GetLong ();
	if (version <= 8)
	{
		for (int i = 0; i < count; i++)
		{
			int off = in.GetLong ();
			// leave space for cr/lf
			_offsets.push_back (off + 2 * i);
		}
	}
	else
	{
		for (int i = 0; i < count; i++)
			_offsets.push_back (in.GetLong ());
	}
	for (int i = 0; i < count; i++)
		_lineNo.push_back (in.GetLong ());
	// _end points beyond the end of strings
	if (version < 10)
		in.GetLong ();	//Eat redundant length
	_end = in.GetLong ();
	_buf = new char [_end];
	in.GetBytes (_buf, _end);

	if (version <= 8)
	{
		// Transfer ownership
		auto_array<char> oldBuf (_buf);
		// add space for cr/lf for every line
		_buf = new char [_end + 2 * count];
		for (int i = 0; i < count; i++)
		{
			strcpy (_buf + _offsets [i], oldBuf.get () + _offsets [i] - 2 * i);
			strcat (_buf + _offsets [i], "\r\n");
		}
	}
}

//
// New folder command
//

void NewFolderCmd::Dump (DumpWindow & dumpWin, PathFinder const & pathFinder) const
{
	FileCmd::Dump (dumpWin, pathFinder);
	dumpWin.PutLine ("Newly created");
}

std::unique_ptr<CmdFileExec> NewFolderCmd::CreateExec () const
{
	return std::unique_ptr<CmdFileExec> (new NewFolderExec (*this));
}

//
// Delete folder command
//

void DeleteFolderCmd::Dump (DumpWindow & dumpWin, PathFinder const & pathFinder) const
{
	FileCmd::Dump (dumpWin, pathFinder);
	dumpWin.PutLine ("Removed");
}

std::unique_ptr<CmdFileExec> DeleteFolderCmd::CreateExec () const
{
	return std::unique_ptr<CmdFileExec> (new DeleteFolderExec (*this));
}

//
// Membership Placeholder Command -- present only in histories imported from version 4.2
//

void MembershipPlaceholderCmd::Deserialize (Deserializer& in, int version)
{
	FileCmd::Deserialize (in, version);
	_userInfo.Deserialize (in, version);
}

std::unique_ptr<CmdFileExec> MembershipPlaceholderCmd::CreateExec () const
{
	return std::unique_ptr<CmdFileExec>();
}

//
// Control commands
//

void CtrlCmd::Serialize (Serializer & out) const
{
	out.PutLong (GetType ());
	out.PutLong (_scriptId);
}

void CtrlCmd::Deserialize (Deserializer& in, int version)
{
	// The command type has been read by ScriptCmd::DeserializeCmd
	_scriptId = in.GetLong ();
}

std::unique_ptr<CmdCtrlExec> AckCmd::CreateExec (History::Db & history, UserId sender) const
{
	return std::unique_ptr<CmdCtrlExec> (new CmdAckExec (*this, history, sender));
}

std::unique_ptr<CmdCtrlExec> MakeReferenceCmd::CreateExec (History::Db & history, UserId sender) const
{
	return std::unique_ptr<CmdCtrlExec> (new CmdMakeReferenceExec (*this, history, sender));
}

std::unique_ptr<CmdCtrlExec> ResendRequestCmd::CreateExec (History::Db & history, UserId sender) const
{
	return std::unique_ptr<CmdCtrlExec> (new CmdResendRequestExec (*this, history, sender));
}

std::unique_ptr<CmdCtrlExec> ResendFullSynchRequestCmd::CreateExec (History::Db & history, UserId sender) const
{
	return std::unique_ptr<CmdCtrlExec>();
}

void ResendFullSynchRequestCmd::Serialize (Serializer & out) const
{
	CtrlCmd::Serialize (out);
	_setScripts.Serialize (out);
	unsigned int count = _membershipScripts.size ();
	out.PutLong	 (count);
	for (unsigned int i = 0; i < count; ++i)
	{
		_membershipScripts [i]->Serialize (out);
	}
}

void ResendFullSynchRequestCmd::Deserialize (Deserializer& in, int version)
{
	CtrlCmd::Deserialize (in, version);
	_setScripts.Deserialize (in, version);
	unsigned int count = in.GetLong ();
	for (unsigned int i = 0; i < count; ++i)
	{
		std::unique_ptr<UnitLineage> tmp (new UnitLineage (in, version));
		_membershipScripts.push_back (std::move(tmp));
	}
}

std::unique_ptr<CmdCtrlExec> VerificationRequestCmd::CreateExec (History::Db & history, 
															   UserId sender) const
{
	return std::unique_ptr<CmdCtrlExec> (
		new CmdVerificationRequestExec (*this, history, sender));
}

void VerificationRequestCmd::Serialize (Serializer & out) const
{
	CtrlCmd::Serialize (out);
	_knownDeadMembers.Serialize (out);
}

void VerificationRequestCmd::Deserialize (Deserializer& in, int version)
{
	CtrlCmd::Deserialize (in, version);
	_knownDeadMembers.Deserialize (in, version);
}

//
// Member commands
//

void NewMemberCmd::Dump (DumpWindow & dumpWin, PathFinder const & pathFinder) const
{
	// Revisit: implementation
}

void NewMemberCmd::Dump (std::ostream & os) const
{
	os << "**New member announcement:" << std::endl;
	os << _memberInfo;
}

bool NewMemberCmd::IsEqual (ScriptCmd const & cmd) const
{
	NewMemberCmd const & memberCmd = dynamic_cast<NewMemberCmd const &>(cmd);
	MemberInfo const thisMemberInfo = GetMemberInfo ();
	return thisMemberInfo.IsIdentical (memberCmd.GetMemberInfo ());
}

void NewMemberCmd::Serialize (Serializer& out) const
{
	out.PutLong (GetType ());
	_memberInfo.Serialize (out);
}

void NewMemberCmd::Deserialize (Deserializer& in, int version)
{
	// The command type has been read by ScriptCmd::DeserializeCmd or by ScriptCmd::DeserializeV40Cmd
	if (version < 41)
	{
		// Deserialize Version40::typeNewUser
		GlobalId scriptId = in.GetLong ();
	}
	_memberInfo.Deserialize (in, version);
}

std::unique_ptr<CmdMemberExec> NewMemberCmd::CreateExec (Project::Db & projectDb) const
{
	return std::unique_ptr<CmdMemberExec> (new CmdNewMemberExec (*this, projectDb));
}

void NewMemberCmd::VerifyLicense () const
{
	MemberState newUserState = _memberInfo.State ();
	// Check license only for voting members
	if (newUserState.IsVoting ())
		_memberInfo.Description ().VerifyLicense ();
}

void DeleteMemberCmd::Dump (DumpWindow & dumpWin, PathFinder const & pathFinder) const
{
	// Revisit: implementation
}

void DeleteMemberCmd::Dump (std::ostream & os) const
{
	os << "**Member removal:" << std::endl;
	os << _memberInfo;
}

bool DeleteMemberCmd::IsEqual (ScriptCmd const & cmd) const
{
	DeleteMemberCmd const & memberCmd = dynamic_cast<DeleteMemberCmd const &>(cmd);
	MemberInfo const thisMemberInfo = GetMemberInfo ();
	return thisMemberInfo.IsIdentical (memberCmd.GetMemberInfo ());
}

void DeleteMemberCmd::Serialize (Serializer& out) const
{
	out.PutLong (GetType ());
	_memberInfo.Serialize (out);
}

void DeleteMemberCmd::Deserialize (Deserializer& in, int version)
{
	// The command type has been read by ScriptCmd::DeserializeCmd
	// Delete member command didn't exist in version 4.2
	_memberInfo.Deserialize (in, version);
}

std::unique_ptr<CmdMemberExec> DeleteMemberCmd::CreateExec (Project::Db & projectDb) const
{
	return std::unique_ptr<CmdMemberExec> (new CmdDeleteMemberExec (*this, projectDb));
}

void EditMemberCmd::Dump (DumpWindow & dumpWin, PathFinder const & pathFinder) const
{
	// Revisit: implementation
}

void EditMemberCmd::Dump (std::ostream & os) const
{
	os << "**Member data change." << std::endl;
	os << "***Old data:" << std::endl;
	os << _oldMemberInfo;
	os << std::endl << "***New data:" << std::endl;
	os << _newMemberInfo;
}

bool EditMemberCmd::IsEqual (ScriptCmd const & cmd) const
{
	EditMemberCmd const & memberCmd = dynamic_cast<EditMemberCmd const &>(cmd);
	MemberInfo const thisMemberOldInfo = GetOldMemberInfo ();
	MemberInfo const thisMemberNewInfo = GetNewMemberInfo ();
	return thisMemberOldInfo.IsIdentical (memberCmd.GetOldMemberInfo ()) &&
		   thisMemberNewInfo.IsIdentical (memberCmd.GetNewMemberInfo ());
}

void EditMemberCmd::Serialize (Serializer& out) const
{
	out.PutLong (GetType ());
	_oldMemberInfo.Serialize (out);
	_newMemberInfo.Serialize (out);
}

void EditMemberCmd::Deserialize (Deserializer& in, int version)
{
	// The command type has been read by ScriptCmd::DeserializeCmd or by ScriptCmd::DeserializeV40Cmd
	if (version < 41)
	{
		// Deserialize Version40::typeMembershipUpdate
		GlobalId scriptId = in.GetLong ();
		unsigned int count = in.GetLong ();
		if (count != 1)
		{
			if (count == 2)
			{
				// Possible emergency admin election
				_oldMemberInfo.Deserialize (in, version);
				_newMemberInfo.Deserialize (in, version);
				if (IsVersion40EmergencyAdminElection ())
					return;
			}
			// Multiple changes in one membership update are not supported
			Win::ClearError ();
			throw Win::Exception ("Unsupported membership update format; Script sender should update Code Co-op to version 4.2 or higher.");
		}
		// In version 4.2 command didn't carried old member info
		_newMemberInfo.Deserialize (in, version);
		return;
	}

	// The command type has been read by ScriptCmd::DeserializeCmd
	_oldMemberInfo.Deserialize (in, version);
	_newMemberInfo.Deserialize (in, version);
}

std::unique_ptr<CmdMemberExec> EditMemberCmd::CreateExec (Project::Db & projectDb) const
{
	return std::unique_ptr<CmdMemberExec> (new CmdEditMemberExec (*this, projectDb));
}

void EditMemberCmd::VerifyLicense () const
{
	MemberState newState = _newMemberInfo.State ();
	// Check license only for voting members
	if (newState.IsVoting ())
		_newMemberInfo.Description ().VerifyLicense ();
}

bool EditMemberCmd::IsVersion40EmergencyAdminElection () const
{
	return (_oldMemberInfo.Id () != _newMemberInfo.Id ()) && (_oldMemberInfo.Id () != gidInvalid) &&
		   (!_oldMemberInfo.State ().IsAdmin () &&  _newMemberInfo.State ().IsAdmin () ||
			 _oldMemberInfo.State ().IsAdmin () && !_newMemberInfo.State ().IsAdmin ());
}

bool EditMemberCmd::IsVersion40Defect () const
{
	return (_oldMemberInfo.Id () == gidInvalid) && _newMemberInfo.State ().IsDead ();
}

//
// Join request command
//

void JoinRequestCmd::Dump (DumpWindow & dumpWin, PathFinder const & pathFinder) const
{
	// Revisit: impelementation
}

void JoinRequestCmd::Serialize (Serializer& out) const
{
	out.PutLong (GetType ());
	_memberInfo.Serialize (out);
}

void JoinRequestCmd::Deserialize (Deserializer& in, int version)
{
	// The command type has been read by ScriptCmd::DeserializeCmd or by ScriptCmd::DeserializeV40Cmd
	_isFromVersion40 = version < 41;
	if (_isFromVersion40)
	{
		// Deserialize Version40::typeJoinRequest
		GlobalId scriptId = in.GetLong ();
	}
	_memberInfo.Deserialize (in, version);
}
