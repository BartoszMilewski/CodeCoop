#if !defined (CMDEXEC_H)
#define CMDEXEC_H
//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "GlobalId.h"
#include "SynchKind.h"
#include "Lineage.h"
#include "ScriptHeader.h"
#include "ScriptList.h"
#include "ScriptCommandList.h"

#include <auto_array.h>

class ScriptCmd;
class WholeFileCmd;
class DiffCmd;
class DeleteCmd;
class NewFolderCmd;
class DeleteFolderCmd;
class CtrlCmd;
class JoinRequestCmd;
class NewMemberCmd;
class DeleteMemberCmd;
class EditMemberCmd;
class DataBase;
class ProjectDb;
class ScriptMailer;
class CommandList;
class AckBox;
namespace History
{
	class Db;
}
namespace Project
{
	class Db;
}
class PathFinder;
class TransactionFileList;
class Catalog;
class LineBuf;
class MemberInfo;
class Transformer;
class ThisUserAgent;

namespace Undo
{
	class Buffer
	{
	public:
		~Buffer () {}

		virtual void SaveTo (std::string const & path) const {};
		virtual CheckSum GetChecksum () const { return CheckSum (); }
	};

	class DiffBuffer : public Buffer
	{
	public:
		DiffBuffer (File::Size oldFileSize)
			: _recBuf (oldFileSize.Low ()),
			  _size (oldFileSize.Low ())
		{}

		void SaveTo (std::string const & path) const;
		CheckSum GetChecksum () const;

		char * Get () const { return _recBuf.get (); }

	private:
		auto_array<char>	_recBuf;
		unsigned long		_size;
	};

	class NewBuffer : public Buffer
	{
	public:
		void SaveTo (std::string const & path) const;
	};

	class DeleteBuffer : public Buffer
	{
	public:
		DeleteBuffer (CheckSum checkSum)
			: _checkSum (checkSum)
		{}

		CheckSum GetChecksum () const { return _checkSum; }

	private:
		CheckSum	_checkSum;
	};
}

class ScriptCorruptException {};

//
// File command execs
//

class CmdFileExec
{
public:
	virtual ~CmdFileExec ()
	{}

	// Acting on Reference create Synch
	virtual void Do (DataBase & database, 
					 PathFinder & pathFinder, 
					 TransactionFileList & fileList,
					 bool inPlace = false)	// By default preserve Reference, in place means
											// place operation result back in Reference
	{}

	// Undo edits in the Reference
	virtual std::unique_ptr<Undo::Buffer> Undo (std::string const & refFilePath) const;
	virtual SynchKind GetSynchKind () const { return synchNone; }

protected:
	// Helpers
	void VerifyParent (DataBase const & dataBase, Transformer const & trans) const throw (Win::Exception);
};

class WholeFileExec : public CmdFileExec
{
public:
	WholeFileExec (WholeFileCmd const & command)
		: _command (command)
	{}

	void Do (DataBase & database, 
			 PathFinder & pathFinder, 
			 TransactionFileList & fileList,
			 bool inPlace);
	std::unique_ptr<Undo::Buffer> Undo (std::string const & refFilePath) const;
	SynchKind GetSynchKind () const { return synchNew; }

private:
	WholeFileCmd const & _command;
};

class DiffFileExec : public CmdFileExec
{
public:
	DiffFileExec (DiffCmd const & command)
		: _command (command)
	{}

	void Do (DataBase & database, 
			 PathFinder & pathFinder, 
			 TransactionFileList & fileList,
			 bool inPlace);
	SynchKind GetSynchKind () const { return synchEdit; }

protected:	
	virtual void Reconstruct (char * newBuf, int newLen, 
							const char * refBuf, int refLen) const = 0;

protected:
	DiffCmd const & _command;
};

class TextDiffFileExec : public DiffFileExec
{
public:
	TextDiffFileExec (DiffCmd const & command)
		: DiffFileExec (command)
	{}

	std::unique_ptr<Undo::Buffer> Undo (std::string const & refFilePath) const;

private:	
	void Reconstruct (char * newBuf, int newLen, 
					const char * refBuf, int refLen) const;
};

// Warning: We cannot deal with files > 2GB
class BinDiffFileExec : public DiffFileExec
{
public:
	BinDiffFileExec (DiffCmd const & command)
		: DiffFileExec (command)
	{}

	std::unique_ptr<Undo::Buffer> Undo (std::string const & refFilePath) const;

private:
	void Reconstruct (char * newBuf, int newLen, 
					const char * refBuf, int refLen) const;
};

class DeleteFileExec : public CmdFileExec
{
public:
	DeleteFileExec (DeleteCmd const & command)
		: _command (command)
	{}

	void Do (DataBase & database, 
			 PathFinder & pathFinder, 
			 TransactionFileList & fileList,
			 bool inPlace); 
	std::unique_ptr<Undo::Buffer> Undo (std::string const & refFilePath) const;
	SynchKind GetSynchKind () const { return synchDelete; }

private:
	DeleteCmd const & _command;
};

class NewFolderExec : public CmdFileExec
{
public:
	NewFolderExec (NewFolderCmd const & command)
		: _command (command)
	{}
	void Do (DataBase & database, 
			 PathFinder & pathFinder, 
			 TransactionFileList & fileList,
			 bool inPlace);

	SynchKind GetSynchKind () const { return synchNew; }

private:
	NewFolderCmd const & _command;
};

class DeleteFolderExec : public CmdFileExec
{
public:
	DeleteFolderExec (DeleteFolderCmd const & command)
		: _command (command)
	{}

	void Do (DataBase & database, 
			 PathFinder & pathFinder, 
			 TransactionFileList & fileList,
			 bool inPlace); 

	SynchKind GetSynchKind () const { return synchDelete; }

private:
	DeleteFolderCmd const & _command;
};

//
// Control command execs
//

class CmdCtrlExec
{
public:
	CmdCtrlExec (CtrlCmd const & command, History::Db & history, UserId sender)
		: _command (command),
		  _history (history),
		  _sender (sender)
	{}
	virtual ~CmdCtrlExec ()
	{}

	virtual void Do (Unit::Type unitType, AckBox & ackBox, bool isSenderReceiver) = 0;
	ScriptHeader & GetScriptHeader () { return _hdr; }
	CommandList  & GetCommandList () { return _cmdList; }
	ScriptList & GetScriptList () { return _scriptList; }

protected:
	CtrlCmd const &	_command;
	History::Db &	_history;
	UserId			_sender;
	ScriptHeader	_hdr;
	CommandList		_cmdList;
	ScriptList		_scriptList;
};

class CmdAckExec : public CmdCtrlExec
{
public:
	CmdAckExec (CtrlCmd const & command, History::Db & history, UserId sender)
		: CmdCtrlExec (command, history, sender)
	{}

	void Do (Unit::Type unitType, AckBox & ackBox, bool isSenderReceiver);
};

class CmdMakeReferenceExec : public CmdCtrlExec
{
public:
	CmdMakeReferenceExec (CtrlCmd const & command, History::Db & history, UserId sender)
		: CmdCtrlExec (command, history, sender)
	{}

	void Do (Unit::Type unitType, AckBox & ackBox, bool isSenderReceiver);
};

class CmdResendRequestExec : public CmdCtrlExec
{
public:
	CmdResendRequestExec (CtrlCmd const & command, History::Db & history, UserId sender)
		: CmdCtrlExec (command, history, sender)
	{}

	void Do (Unit::Type unitType, AckBox & ackBox, bool isSenderReceiver);
};

class CmdVerificationRequestExec : public CmdCtrlExec
{
public:
	CmdVerificationRequestExec (CtrlCmd const & command, 
								History::Db & history, 
								UserId sender)
		: CmdCtrlExec (command, history, sender)
	{}

	void Do (Unit::Type unitType, AckBox & ackBox, bool isSenderReceiver);
};

//
// Member command execs
//

class CmdMemberExec
{
public:
	CmdMemberExec (Project::Db & projectDb)
		: _projectDb (projectDb)
	{}
	virtual ~CmdMemberExec ()
	{}

	virtual void Do (ThisUserAgent & agent) = 0;

protected:
	Project::Db &	_projectDb;
};

class CmdNewMemberExec : public CmdMemberExec
{
public:
	CmdNewMemberExec (NewMemberCmd const & command, Project::Db & projectDb)
		: CmdMemberExec (projectDb),
		  _command (command)
	{}

	void Do (ThisUserAgent & agent);

private:
	NewMemberCmd const &	_command;
};

class CmdDeleteMemberExec : public CmdMemberExec
{
public:
	CmdDeleteMemberExec (DeleteMemberCmd const & command, Project::Db & projectDb)
		: CmdMemberExec (projectDb),
		  _command (command)
	{}

	void Do (ThisUserAgent & agent);

private:
	DeleteMemberCmd const &	_command;
};

class CmdEditMemberExec : public CmdMemberExec
{
public:
	CmdEditMemberExec (EditMemberCmd const & command, Project::Db & projectDb)
		: CmdMemberExec (projectDb),
		  _command (command)
	{}

	void Do (ThisUserAgent & agent);

private:
	void ExecuteV40Cmd (ThisUserAgent & agent);
	void ChangeUserData (MemberInfo const & oldInfo, MemberInfo const & newInfo, ThisUserAgent & agent);

private:
	EditMemberCmd const & _command;
};

#endif
