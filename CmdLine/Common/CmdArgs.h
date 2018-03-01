#if !defined (CMDARGS_H)
#define CMDARGS_H
//------------------------------------
//  (c) Reliable Software, 1999 - 2007
//------------------------------------

#include "CmdFlags.h"

#include <File/Path.h>
#include <array_vector.h>

class FilePath;

class CmdArgs
{
public:
	CmdArgs (int count, char * args [], CmdSwitch validSwitch, bool pathsOnly = false);
	CmdArgs (char const * path);
	CmdArgs (FilePath const & path);

	bool IsHelp () const { return _options.IsHelp (); }
	bool IsAllInProject () const { return _options.IsAllInProject (); }
	bool IsKeepCheckedOut () const { return _options.IsKeepCheckedOut (); }
	bool IsComment () const { return _options.IsComment (); }
	bool IsCommand () const { return _options.IsCommand (); }
	bool IsHeaderFile () const { return _options.IsTypeHeader (); }
	bool IsSourceFile () const { return _options.IsTypeSource (); }
	bool IsTextFile () const { return _options.IsTypeText (); }
	bool IsBinaryFile () const { return _options.IsTypeBinary (); }
	bool IsListing () const { return _options.IsListing (); }
	bool IsVersionId () const { return _options.IsVersionId (); }
	bool IsOverwrite () const { return _options.IsOverwrite (); }
	bool IsLocalEdits () const { return _options.IsLocalEdits (); }
	bool IsDeleteFromDisk () const { return _options.IsDeleteFromDisk (); }
	bool IsDoCheckIn () const { return _options.IsDoCheckIn (); }
	bool IsDumpVersion () const	{ return _options.IsDumpVersion (); }
	bool IsDumpCatalog () const	{ return _options.IsDumpCatalog (); }
	bool IsDumpMembership () const { return _options.IsDumpMembership (); }
	bool IsDumpHistory () const { return _options.IsDumpHistory (); }
	bool IsDumpAll () const { return _options.IsDumpAll (); }
	bool IsNotifySink () const { return _options.IsNotifySink (); }
	bool IsPickLicense () const { return _options.IsPickLicense (); }
	bool IsForce () const { return _options.IsForce (); }
	bool IsFtpServer () const { return _options.IsFtpServer (); }
	bool IsFtpFolder () const { return _options.IsFtpFolder (); }
	bool IsFtpUser () const	{ return _options.IsFtpUser (); }
	bool IsFtpPassword () const { return _options.IsFtpPassword (); }
	bool IsEmpty () const { return Size () == 0 && DeepSize () == 0 && _projId == -1; }

	unsigned int Size () const { return _paths.size (); }
	char const ** GetFilePaths () const { return _paths.get (); }
	unsigned int DeepSize () const { return _deepPaths.size (); }
	char const ** GetDeepPaths () const { return _deepPaths.get (); }
	std::string const & GetComment () const { return _options.GetComment (); }
	std::string const & GetCommand () const { return _options.GetCommand (); }
	std::string const & GetVersionId () const { return _options.GetVersionId (); }
	std::string const & GetUserId () const { return _options.GetUserId (); }
	std::string const & GetUserName () const { return _options.GetUserName (); }
	std::string const & GetUserState () const { return _options.GetUserState (); }
	std::string const & GetFtpServer   () const { return _options.GetFtpServer (); }
	std::string const & GetFtpFolder   () const { return _options.GetFtpFolder (); }
	std::string const & GetFtpUser () const { return _options.GetFtpUser (); }
	std::string const & GetFtpPassword () const { return _options.GetFtpPassword (); }
	int GetProjectId () const { return _projId; }
	unsigned GetNotifySink () const { return _notifySink; }

private:
	class FileFinder
	{
	public:
		FileFinder (std::string const & path,
					std::string const & filePattern,
					bool isRecursive,
					StringArray & files);

		void ListFolders (StringArray & files);

	private:
		void ListFiles (bool isRecursive, StringArray & files);

	private:
		std::string	_filePattern;
		FilePath	_curPath;
	};

private:
	void SelectOneFileFromFolder (FilePath const & path);

private:
	CmdOptions	_options;
	StringArray	_paths;
	StringArray	_deepPaths;
	int			_projId;
	unsigned	_notifySink;
};

#endif
