#if !defined (SCRIPTCOMMANDS_H)
#define SCRIPTCOMMANDS_H
//------------------------------------
//  (c) Reliable Software, 2003 - 2008
//------------------------------------

#include "ScriptCmd.h"
#include "GlobalId.h"
#include "FileData.h"
#include "MemberInfo.h"
#include "SynchKind.h"
#include "DiffRecord.h"
#include "Cluster.h"
#include "Lineage.h"
#include "SerVector.h"

#include <File/File.h>

class DumpWindow;
class PathFinder;
class CmdFileExec;
class CmdCtrlExec;
class CmdMemberExec;
class CmdJoinExec;
class Catalog;
class DataBase;
class AckBox;
namespace History
{
	class Db;
}

class FileCmd : public ScriptCmd
{
public:
	class LazyFileBuf;
public:
	static std::unique_ptr<FileCmd> DeserializeCmd (Deserializer & in, int version);

	FileCmd (FileData const & fileData)
		: _fileData (fileData)
	{}
	FileCmd ()
	{}

	// ScriptCmd interface
	void Dump (DumpWindow & dumpWin, PathFinder const & pathFinder) const;
	bool Verify () const;
	bool IsEqual (ScriptCmd const & cmd) const;

	// Serializable interface
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);

	// File command interface
	virtual File::Size GetOldFileSize () const = 0;
	virtual File::Size GetNewFileSize () const = 0;
	virtual bool IsContentsChanged () const { return true; }
	virtual SynchKind GetSynchKind () const = 0;
	virtual std::unique_ptr<CmdFileExec> CreateExec () const = 0;

	FileData const & GetFileData () const { return _fileData; }
	GlobalId GetGlobalId () const { return _fileData.GetGlobalId (); }
	UniqueName const & GetUniqueName () const { return _fileData.GetUniqueName (); }
	char const * GetName () const { return _fileData.GetName ().c_str (); }
	FileType GetFileType () const { return _fileData.GetType (); }
	CheckSum GetOldCheckSum () const { return _fileData.GetCheckSum ();	}
	CheckSum GetNewCheckSum () const { return _newCheckSum; }

	void SetNewCheckSum (CheckSum cs)	{ _newCheckSum = cs; }
	void SetUniqueName (UniqueName const & newUname) { _fileData.SetName (newUname); }

protected:
	FileData		_fileData;
	CheckSum		_newCheckSum;
};

class FileCmd::LazyFileBuf : public Serializable
{
public:
	LazyFileBuf ()
		: _buf (0),
		  _fileSize (0, 0),
		  _fileOffset (0, 0),
		  _deleteFile (false)
	{}
	~LazyFileBuf ()
	{
		delete []_buf;
		if (_deleteFile)
			File::DeleteNoEx (_filePath);
	}

	void CopyIn (char const * buf, File::Size len);
	void SetSource (std::string const & filePath,
					File::Size len,
					File::Offset offset = File::Offset (0, 0),
					bool deleteFile = false);
	void Write (Serializer& out) const;
	std::string const & GetPath () const { return _filePath; }
	File::Size FileSize () const { return _fileSize; }
	File::Offset GetOffset () const { return _fileOffset; }
	void Dump (DumpWindow & dumpWin) const;
	// Serializable interface
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);

private:
	File::Size		_fileSize;
	File::Offset	_fileOffset;
	char *			_buf;
	std::string		_filePath;		// For lazy evaluation
	bool			_deleteFile;	// Perform cleanup
};

class WholeFileCmd : public FileCmd
{
public:
	WholeFileCmd (FileData const & fileData)
		: FileCmd (fileData)
	{
		_fileData.SetCheckSum (CheckSum ()); // old checksum
	}
	WholeFileCmd () {}

	// ScriptCmd interface
	ScriptCmdType GetType () const { return typeWholeFile; }
	void Dump (DumpWindow & dumpWin, PathFinder const & pathFinder) const;

	// Serializable interface
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);

	// FileCmd interface
	File::Size GetNewFileSize () const { return _buf.FileSize (); }
	File::Size GetOldFileSize () const { return File::Size (0, 0); }
	SynchKind GetSynchKind () const { return synchNew; }
	std::unique_ptr<CmdFileExec> CreateExec () const;

	void SaveTo (std::string const & path) const;
	// REVISIT: this method is called only during full sync creation and
	// can be avoided if restored files from the temporary area are kept
	// on disk a little bit longer.
	void CopyIn (char const * buf, File::Size len)
	{
		_buf.CopyIn (buf, len);
	}
	// Lazy version: don't use with temporary (restored) files
	void SetSource (std::string const & filePath, File::Size len)
	{
		_buf.SetSource (filePath, len);
	}
	std::string const & GetPath () const { return _buf.GetPath (); }
	File::Offset GetOffset () const { return _buf.GetOffset (); }

private:
	FileCmd::LazyFileBuf	_buf;
};

class DeleteCmd : public FileCmd
{
	friend class DeleteFileExec;
public:
	DeleteCmd (FileData const & fileData)
		: FileCmd (fileData)
	{
		_newCheckSum = fileData.GetCheckSum ();
	}
	DeleteCmd () {}

	// ScriptCmd interface
	ScriptCmdType GetType () const { return typeDeletedFile; }
	void Dump (DumpWindow & dumpWin, PathFinder const & pathFinder) const;

	// Serializable interface
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);

	// FileCmd interface
	File::Size GetNewFileSize () const { return File::Size (0, 0); }
	File::Size GetOldFileSize () const { return _buf.FileSize (); }
	SynchKind GetSynchKind () const;
	std::unique_ptr<CmdFileExec> CreateExec () const;
	void SaveTo (std::string const & path) const;
	// Lazy version: don't use with temporary (restored) files
	void SetSource (std::string const & filePath, File::Size len, bool doDelete)
	{
		_buf.SetSource (filePath, len, File::Offset (0, 0), doDelete);
	}
private:
	FileCmd::LazyFileBuf _buf;
};

class LineArray: public Serializable
{
public:
	LineArray ()
		: _bufLen (0),
		  _end (0),
		  _buf (0)
	{}
	~LineArray ()
	{
		delete []_buf;
	}

	void Add (int lineNo, int len, char const * buf);
	int LineNumber (int i) const { return _lineNo [i]; }
	int GetCount () const {return _lineNo.size (); }
	char const * GetLine (int i) const
	{
		int offset = _offsets [i];
		return &_buf [offset];
	}
	unsigned int GetLineLen (unsigned int i) const
	{
		Assert (i < _offsets.size ());
		int endOffset = (i == _offsets.size () - 1)? _end: _offsets [i + 1];
		endOffset--; // skip trailing null
		return endOffset - _offsets [i];
	}
	int FindLine (int lineNo) const;
	bool IsEqual (LineArray const & lineArray) const;

	// Serializable interface
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);

private:
	void Grow (int newSize);

	std::vector<long>	_offsets;
	std::vector<long>	_lineNo;
	char *				_buf;
	int 				_bufLen;
	int 				_end;
};

class Cluster;
class SimpleLine;
class LineBuf;

class DiffCmd: public FileCmd, public DiffRecorder
{
	friend class DiffFileExec;
	friend class TextDiffFileExec;
	friend class BinDiffFileExec;
	friend class BlameReport;

public:
	DiffCmd (FileData const & fileData)
		: FileCmd (fileData)
	{
		_newCheckSum = fileData.GetCheckSum ();
	}
	DiffCmd ()
	{}

	// FileCmd interface
	File::Size GetNewFileSize () const { return _newFileSize; }
	File::Size GetOldFileSize () const { return _oldFileSize; }
	bool IsContentsChanged () const { return _clusters.size () != 0; }
	SynchKind GetSynchKind () const;
	bool IsEqual (ScriptCmd const & cmd) const;

	// Serializable interface
    void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);

	void SetNewCheckSum (CheckSum checkSum) { FileCmd::SetNewCheckSum (checkSum); }
	void SetOldFileSize (File::Size size) { _oldFileSize = size; }
	void SetNewFileSize (File::Size size) { _newFileSize = size; }
	void AddCluster (Cluster const & cluster);
	void AddOldLine (int oldNum, SimpleLine const & line);
	void AddNewLine (int newNum, SimpleLine const & line);

protected:
	void DisplayChanges (DumpWindow & dumpWin, PathFinder const & pathFinder) const;

protected:
	File::Size 				_oldFileSize;
	File::Size				_newFileSize;
	std::vector<Cluster> 	_clusters;
	LineArray				_newLines;
	LineArray				_oldLines;
};

class TextDiffCmd: public DiffCmd
{
	friend class DiffFileExec;
	friend class TextDiffFileExec;

public :
	TextDiffCmd (FileData const & fileData)
		: DiffCmd (fileData)
	{}
	TextDiffCmd ()
	{}

	// ScriptCmd interface
	ScriptCmdType GetType () const { return typeTextDiffFile; }
	void Dump (DumpWindow & dumpWin, PathFinder const & pathFinder) const;

	// FileCmd interface
	std::unique_ptr<CmdFileExec> CreateExec () const;
};

class BinDiffCmd: public DiffCmd
{
	friend class DiffFileExec;
	friend class BinDiffFileExec;

public :
	BinDiffCmd (FileData const & fileData)
		: DiffCmd (fileData)
	{}
	BinDiffCmd ()
	{}

	// ScriptCmd interface
    ScriptCmdType GetType () const { return typeBinDiffFile; }
	void Dump (DumpWindow & dumpWin, PathFinder const & pathFinder) const;

	// FileCmd interface
	std::unique_ptr<CmdFileExec> CreateExec () const;
};

class NewFolderCmd: public FileCmd
{
	friend class NewFolderExec;

public:
	NewFolderCmd (FileData const & folderData)
		: FileCmd (folderData)
	{}
	NewFolderCmd ()
	{}

	// ScriptCmd interface
	ScriptCmdType GetType () const { return typeNewFolder; }
	void Dump (DumpWindow & dumpWin, PathFinder const & pathFinder) const;

	// FileCmd interface
	File::Size GetNewFileSize () const { return File::Size (0, 0); }
	File::Size GetOldFileSize () const { return File::Size (0, 0); }
	bool IsContentsChanged () const { return false; }
	SynchKind GetSynchKind () const { return synchNew; }
	std::unique_ptr<CmdFileExec> CreateExec () const;
};

class DeleteFolderCmd: public FileCmd
{
	friend class DeleteFolderExec;

public:
	DeleteFolderCmd (FileData const & folderData)
		: FileCmd (folderData)
	{}
	DeleteFolderCmd ()
	{}

	// ScriptCmd interface
	ScriptCmdType GetType () const { return typeDeleteFolder; }
	void Dump (DumpWindow & dumpWin, PathFinder const & pathFinder) const;

	// FileCmd interface
	File::Size GetNewFileSize () const { return File::Size (0, 0); }
	File::Size GetOldFileSize () const { return File::Size (0, 0); }
	bool IsContentsChanged () const { return false; }
	SynchKind GetSynchKind () const { return synchDelete; }
	std::unique_ptr<CmdFileExec> CreateExec () const;
};

// Needed only when we are importing version 4.2 history
class MembershipPlaceholderCmd : public FileCmd
{
public:
	MembershipPlaceholderCmd ()
	{}

	// Serializable interface
	void Deserialize (Deserializer& in, int version);

	// ScriptCmd interface
	ScriptCmdType GetType () const { return typeUserCmd; }

	// FileCmd interface
	File::Size GetNewFileSize () const { return File::Size (0, 0); }
	File::Size GetOldFileSize () const { return File::Size (0, 0); }
	SynchKind GetSynchKind () const { return synchNone; }
	std::unique_ptr<CmdFileExec> CreateExec () const;

private:
	MemberInfo	_userInfo;
};

class CtrlCmd : public ScriptCmd
{
public:
	CtrlCmd (GlobalId scriptId)
		: _scriptId (scriptId)
	{}
	CtrlCmd ()
		: _scriptId (gidInvalid)
	{}

	// ScriptCmd interface
	void Dump (DumpWindow & dumpWin, PathFinder const & pathFinder) const {}

	// Serializable interface
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);

	// Control command interface
	virtual std::unique_ptr<CmdCtrlExec> CreateExec (History::Db & history, UserId sender) const = 0;

	GlobalId GetScriptId () const { return _scriptId; }

protected:
	GlobalId	_scriptId;
};

class AckCmd : public CtrlCmd
{
public:
	AckCmd (GlobalId scriptId)
		: CtrlCmd (scriptId),
		  _isV40MembershipUpdateAck (false)
	{}
	AckCmd (bool isV40MembershipUpdateAck = false)
		: _isV40MembershipUpdateAck (isV40MembershipUpdateAck)
	{}

	// ScriptCmd interface
	ScriptCmdType GetType () const { return typeAck; }

	// CtrlCmd interface
	std::unique_ptr<CmdCtrlExec> CreateExec (History::Db & history, UserId sender) const;

	bool IsV40MembershipUpdateAck () const { return _isV40MembershipUpdateAck; }

private:
	bool	_isV40MembershipUpdateAck;
};

class MakeReferenceCmd : public CtrlCmd
{
public:
	MakeReferenceCmd (GlobalId scriptId)
		: CtrlCmd (scriptId)
	{}
	MakeReferenceCmd ()
	{}

	// ScriptCmd interface
	ScriptCmdType GetType () const { return typeMakeReference; }

	// CtrlCmd interface
	std::unique_ptr<CmdCtrlExec> CreateExec (History::Db & history, UserId sender) const;
};

class ResendRequestCmd : public CtrlCmd
{
public:
	ResendRequestCmd (GlobalId scriptId)
		: CtrlCmd (scriptId)
	{}
	ResendRequestCmd ()
	{}

	// ScriptCmd interface
	ScriptCmdType GetType () const { return typeResendRequest; }

	// CtrlCmd interface
	std::unique_ptr<CmdCtrlExec> CreateExec (History::Db & history, UserId sender) const;
};

class ResendFullSynchRequestCmd : public CtrlCmd
{
public:
	ResendFullSynchRequestCmd (GlobalId scriptId, 
								Lineage & setScripts, 
								auto_vector<UnitLineage> & membershipScripts)
		: CtrlCmd (scriptId)
	{
		_setScripts.swap (setScripts);
		_membershipScripts.swap (membershipScripts);
	}
	ResendFullSynchRequestCmd ()
	{}
	Lineage const & GetSetScripts () const { return _setScripts; }
	auto_vector<UnitLineage> const & GetMembershipScripts () const { return _membershipScripts; }
	Lineage & GetSetScripts () { return _setScripts; }
	auto_vector<UnitLineage> & GetMembershipScripts () { return _membershipScripts; }
	// ScriptCmd interface
	ScriptCmdType GetType () const { return typeResendFullSynchRequest; }
	// CtrlCmd interface
	std::unique_ptr<CmdCtrlExec> CreateExec (History::Db & history, UserId sender) const;
	// Serializable interface
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);
private:
	Lineage _setScripts;
	auto_vector<UnitLineage> _membershipScripts;
};

class VerificationRequestCmd : public CtrlCmd
{
public:
	VerificationRequestCmd (GlobalId myReferenceId, GidList const & knownDeadMembers)
		: CtrlCmd (myReferenceId),
		  _knownDeadMembers (knownDeadMembers)
	{}
	VerificationRequestCmd ()
	{}

	GidList const & GetKnownDeadMembers () const { return _knownDeadMembers; }

	// ScriptCmd interface
	ScriptCmdType GetType () const { return typeVerificationRequest; }
	// CtrlCmd interface
	std::unique_ptr<CmdCtrlExec> CreateExec (History::Db & history, UserId sender) const;
	// Serializable interface
	void Serialize (Serializer & out) const;
	void Deserialize (Deserializer & in, int version);

private:
	SerVector<UserId>	_knownDeadMembers;
};

class MemberCmd : public ScriptCmd
{
public:
	// Member command interface
	virtual std::unique_ptr<CmdMemberExec> CreateExec (Project::Db & projectDb) const = 0;
	virtual void VerifyLicense () const = 0;
	virtual UserId GetUserId () const = 0;
};

class NewMemberCmd : public MemberCmd
{
public:
	NewMemberCmd (MemberInfo const & newMember)
		: _memberInfo (newMember)
	{}
	NewMemberCmd ()
	{}

	// ScriptCmd interface
	ScriptCmdType GetType () const { return typeNewMember; }
	void Dump (DumpWindow & dumpWin, PathFinder const & pathFinder) const;
	void Dump (std::ostream & os) const;
	bool IsEqual (ScriptCmd const & cmd) const;

	// Serializable interface
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);

	// Member command interface
	std::unique_ptr<CmdMemberExec> CreateExec (Project::Db & projectDb) const;
	void VerifyLicense () const;
	UserId GetUserId () const { return _memberInfo.Id (); }

	MemberInfo const & GetMemberInfo () const { return _memberInfo; }
	void SetPrehistoricScriptId (GlobalId gid) { _memberInfo.SetPreHistoricScript (gid); }
	void SetMostRecentScriptId (GlobalId gid) { _memberInfo.SetMostRecentScript (gid); }
	void ResetScriptMarkers () { _memberInfo.ResetScriptMarkers (); }

private:
	MemberInfo	_memberInfo;
};

class DeleteMemberCmd : public MemberCmd
{
public:
	DeleteMemberCmd (MemberInfo const & deletedMember)
		: _memberInfo (deletedMember)
	{}
	DeleteMemberCmd ()
	{}

	// ScriptCmd interface
	ScriptCmdType GetType () const { return typeDeleteMember; }
	void Dump (DumpWindow & dumpWin, PathFinder const & pathFinder) const;
	void Dump (std::ostream & os) const;
	bool IsEqual (ScriptCmd const & cmd) const;

	// Serializable interface
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);

	// Member command interface
	std::unique_ptr<CmdMemberExec> CreateExec (Project::Db & projectDb) const;
	void VerifyLicense () const { /* Do nothing for defecting member */ };
	UserId GetUserId () const { return _memberInfo.Id (); }

	MemberInfo const & GetMemberInfo () const { return _memberInfo; }

private:
	MemberInfo	_memberInfo;
};

class EditMemberCmd : public MemberCmd
{
public:
	EditMemberCmd (MemberInfo const & oldInfo, MemberInfo const & newInfo)
		: _oldMemberInfo (oldInfo),
		  _newMemberInfo (newInfo)
	{}
	EditMemberCmd ()
	{}

	// ScriptCmd interface
	ScriptCmdType GetType () const { return typeEditMember; }
	void Dump (DumpWindow & dumpWin, PathFinder const & pathFinder) const;
	void Dump (std::ostream & os) const;
	bool IsEqual (ScriptCmd const & cmd) const;

	// Serializable interface
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);

	// Member command interface
	std::unique_ptr<CmdMemberExec> CreateExec (Project::Db & projectDb) const;
	void VerifyLicense () const;
	UserId GetUserId () const { return _newMemberInfo.Id (); }	// Edit member comand converted from version 4.2 doesn't have valid _oldMemberInfo

	MemberInfo const & GetOldMemberInfo () const { return _oldMemberInfo; }
	MemberInfo const & GetNewMemberInfo () const { return _newMemberInfo; }

	bool IsVersion40EmergencyAdminElection () const;
	bool IsVersion40Defect () const;

private:
	MemberInfo	_oldMemberInfo;
	MemberInfo	_newMemberInfo;
};

class JoinRequestCmd : public ScriptCmd
{
public:
	JoinRequestCmd ()
		: _isFromVersion40 (false)
	{}
	JoinRequestCmd (MemberInfo const & info)
		: _memberInfo (info),
		  _isFromVersion40 (false)
	{}

	// ScriptCmd interface
	ScriptCmdType GetType () const { return typeJoinRequest; }
	void Dump (DumpWindow & dumpWin, PathFinder const & pathFinder) const;

	// Serializable interface
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);

	MemberInfo const & GetMemberInfo () const { return _memberInfo; }
	bool IsFromVersion40 () const { return _isFromVersion40; }

private:
	MemberInfo	_memberInfo;
	bool		_isFromVersion40;
};

#endif
