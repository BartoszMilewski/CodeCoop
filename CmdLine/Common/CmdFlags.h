#if !defined (CMDFLAGS_H)
#define CMDFLAGS_H
//------------------------------------
//  (c) Reliable Software, 2000 - 2007
//------------------------------------

#include <string>
#include <bitset>

class CmdSwitch
{
public:
    void SetRecursive ()		{ _switch.set (recursive); }
    void SetAllInProject ()		{ _switch.set (all); }
	void SetKeepCheckedOut ()	{ _switch.set (keepCheckedOut); }
	void SetHelp ()				{ _switch.set (help); }
	void SetTypeHeader ()		{ _switch.set (headerFile); }
	void SetTypeSource ()		{ _switch.set (sourceFile); }
	void SetTypeText ()			{ _switch.set (textFile); }
	void SetTypeBinary ()		{ _switch.set (binaryFile); }
	void SetListing ()			{ _switch.set (listing); }
	void SetComment ()			{ _switch.set (comment); }
	void SetVersion ()			{ _switch.set (version); }
	void SetCommand ()			{ _switch.set (command); }
	void SetProject ()			{ _switch.set (project); }
	void SetOverwrite ()		{ _switch.set (overwrite); }
	void SetLocalEdits ()		{ _switch.set (localedits); }
	void SetDeleteFromDisk ()	{ _switch.set (deleteFromDisk); }
	void SetUserId ()			{ _switch.set (userId); }
	void SetUserName ()			{ _switch.set (userName); }
	void SetUserState ()		{ _switch.set (userState); }
	void SetDoCheckIn ()		{ _switch.set (doCheckIn); }
	void SetDumpVersion ()		{ _switch.set (dumpVersion); }
	void SetDumpCatalog ()		{ _switch.set (dumpCatalog); }
	void SetDumpMembership ()	{ _switch.set (dumpMembership); }
	void SetDumpHistory ()		{ _switch.set (dumpHistory); }
	void SetDumpAll ()			{ _switch.set (dumpAll); }
	void SetNotifySink ()		{ _switch.set (notifySink); }
	void SetPickLicense ()		{ _switch.set (pickLicense); }
	void SetForce ()			{ _switch.set (force); }
	void SetFtpServer ()		{ _switch.set (ftpServer); }
	void SetFtpFolder ()		{ _switch.set (ftpFolder); }
	void SetFtpUser ()			{ _switch.set (ftpUser); }
	void SetFtpPassword ()		{ _switch.set (ftpPassword); }

    bool IsRecursive () const		{ return _switch.test (recursive); }
    bool IsAllInProject () const	{ return _switch.test (all); }
	bool IsKeepCheckedOut () const	{ return _switch.test (keepCheckedOut); }
	bool IsHelp () const			{ return _switch.test (help); }
	bool IsTypeHeader () const		{ return _switch.test (headerFile); }
	bool IsTypeSource () const		{ return _switch.test (sourceFile); }
	bool IsTypeText () const		{ return _switch.test (textFile); }
	bool IsTypeBinary () const		{ return _switch.test (binaryFile); }
	bool IsListing () const			{ return _switch.test (listing); }
	bool IsComment () const			{ return _switch.test (comment); }
	bool IsVersion () const			{ return _switch.test (version); }
	bool IsCommand () const			{ return _switch.test (command); }
	bool IsProject () const			{ return _switch.test (project); }
	bool IsOverwrite () const		{ return _switch.test (overwrite); }
	bool IsLocalEdits () const		{ return _switch.test (localedits); }
	bool IsDeleteFromDisk () const  { return _switch.test (deleteFromDisk); }
	bool IsUserId () const			{ return _switch.test (userId); }
	bool IsUserName () const		{ return _switch.test (userName); }
	bool IsUserState () const		{ return _switch.test (userState); }
	bool IsDoCheckIn ()	const		{ return _switch.test (doCheckIn); }
	bool IsDumpVersion () const		{ return _switch.test (dumpVersion); }
	bool IsDumpCatalog () const		{ return _switch.test (dumpCatalog); }
	bool IsDumpMembership () const	{ return _switch.test (dumpMembership); }
	bool IsDumpHistory () const		{ return _switch.test (dumpHistory); }
	bool IsDumpAll () const			{ return _switch.test (dumpAll); }
	bool IsNotifySink () const		{ return _switch.test (notifySink); }
	bool IsPickLicense () const		{ return _switch.test (pickLicense); }
	bool IsForce () const			{ return _switch.test (force); }
	bool IsFtpServer () const		{ return _switch.test (ftpServer); }
	bool IsFtpFolder () const		{ return _switch.test (ftpFolder); }
	bool IsFtpUser () const			{ return _switch.test (ftpUser); }
	bool IsFtpPassword () const		{ return _switch.test (ftpPassword); }

private:
	enum
	{
		recursive,
		all,
		keepCheckedOut,
		help,
		headerFile,
		sourceFile,
		textFile,
		binaryFile,
		listing,
		comment,
		version,
		command,
		project,
		overwrite,
		localedits,
		deleteFromDisk,
		userId,
		userName,
		userState,
		doCheckIn,
		dumpVersion,
		dumpCatalog,
		dumpMembership,
		dumpHistory,
		dumpAll,
		notifySink,
		pickLicense,
		force,
		ftpServer,
		ftpFolder,
		ftpUser,
		ftpPassword,
		flagCount
	};

private:
	std::bitset<flagCount>	_switch;		// Command switches
};

class CmdOptions : public CmdSwitch
{
public:
	void SetComment (std::string const & comment)
	{
		_comment = comment;
		CmdSwitch::SetComment ();
	}
	void SetCommand (std::string const & command)
	{
		_command = command;
		CmdSwitch::SetCommand ();
	}
	void SetVersionId (std::string const & versionId)
	{
		_versionId = versionId;
		CmdSwitch::SetVersion ();
	}
	bool IsComment () const	{ return CmdSwitch::IsComment () && !_comment.empty (); }
	bool IsCommand () const	{ return CmdSwitch::IsCommand () && !_command.empty (); }
	bool IsVersionId () const { return CmdSwitch::IsVersion () && !_versionId.empty (); }

	void SetUserId (std::string const & id)
	{
		_userId = id;
		CmdSwitch::SetUserId ();
	}
	void SetUserName (std::string const & name)
	{
		_userName = name;
		CmdSwitch::SetUserName ();
	}
	void SetUserState (std::string const & state)
	{
		_userState = state;
		CmdSwitch::SetUserState ();
	}

	void SetFtpServer (std::string const & server)
	{
		_ftpServer = server;
		CmdSwitch::SetFtpServer ();
	}
	void SetFtpFolder (std::string const & folder)
	{
		_ftpFolder = folder;
		CmdSwitch::SetFtpFolder ();
	}
	void SetFtpUser (std::string const & user)
	{
		_ftpUser = user;
		CmdSwitch::SetFtpUser ();
	}
	void SetFtpPassword (std::string const & password)
	{
		_ftpPassword = password;
		CmdSwitch::SetFtpPassword ();
	}

	std::string const & GetComment () const { return _comment; }
	std::string const & GetCommand () const { return _command; }
	std::string const & GetVersionId () const { return _versionId; }

	std::string const & GetUserId   () const { return _userId;    }
	std::string const & GetUserName () const { return _userName;  }
	std::string const & GetUserState() const { return _userState; }

	std::string const & GetFtpServer   () const { return _ftpServer;   }
	std::string const & GetFtpFolder   () const { return _ftpFolder;   }
	std::string const & GetFtpUser () const { return _ftpUser;  }
	std::string const & GetFtpPassword () const { return _ftpPassword; }

private:
	std::string	_comment;	// Command comment
	std::string	_versionId;	// Version id
	std::string _command;	// Co-op command

	// project member
	std::string _userId;
	std::string _userName;
	std::string _userState;

	// coop backup
	std::string	_ftpServer;
	std::string _ftpFolder;
	std::string	_ftpUser;
	std::string	_ftpPassword;
};

class StandardSwitch : public CmdSwitch
{
public:
	StandardSwitch ()
	{
		SetHelp ();
	}
};

#endif
