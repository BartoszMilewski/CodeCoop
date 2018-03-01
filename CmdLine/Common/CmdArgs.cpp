//------------------------------------
//  (c) Reliable Software, 1999 - 2007
//------------------------------------

#include "precompiled.h"
#include "CmdArgs.h"

#include <File/File.h>
#include <File/Path.h>
#include <File/Dir.h>
#include <Ex/WinEx.h>
#include <Dbg/Assert.h>
#include <StringOp.h>

#include <Shlwapi.h>

#include <sstream>
#include <locale>
#include <iostream>

CmdArgs::CmdArgs (int count, char * args [], CmdSwitch validSwitch, bool pathsOnly)
	: _projId (-1),
	  _notifySink (-1)
{
	for (int i = 1; i < count; i++)
	{
		char const * arg = args [i];
		if (arg [0] == '-' || arg [0] == '/')
		{
			if (arg [1] == 'r' && validSwitch.IsRecursive ())
			{
				_options.SetRecursive ();
			}
			else if (arg [1] == 'a' && validSwitch.IsAllInProject ())
			{
				_options.SetAllInProject ();
			}
			else if (arg [1] == 'f')
			{
				if (validSwitch.IsForce ())
				{
					_options.SetForce ();
				}
				else if (validSwitch.IsFtpFolder ())
				{
					// -f:<ftp server path>
					if (arg [2] == ':')
					{
						if (arg [3] != '\0')
							_options.SetFtpFolder (&arg [3]);
						else
							std::cerr << arg << " - missing ftp server path." << std::endl;
					}
					else
					{
						std::cerr << "Ignoring unrecognized switch " << arg << std::endl;
					}
				}
				else
				{
					std::cerr << "Ignoring unrecognized switch " << arg << std::endl;
				}
			}
			else if (arg [1] == 'o')
			{
				if (validSwitch.IsKeepCheckedOut ())
					_options.SetKeepCheckedOut ();
				else if (validSwitch.IsOverwrite ())
					_options.SetOverwrite ();
			}
			else if ((arg [1] == '?' || arg [1] == 'h') && validSwitch.IsHelp ())
			{
				_options.SetHelp ();
			}
			else if (arg [1] == 'c')
			{
				if (validSwitch.IsComment ())
				{
					// -c:<comment>
					if (arg [2] == ':')
					{
						if (arg [3] != '\0')
							_options.SetComment (&arg [3]);
						else
							std::cerr << arg << " - missing comment." << std::endl;
					}
				}
				else if (validSwitch.IsCommand ())
				{
					// -c:<command>
					if (arg [2] == ':')
					{
						if (arg [3] != '\0')
							_options.SetCommand (&arg [3]);
						else
							std::cerr << arg << " - missing command." << std::endl;
					}
				}
				else
				{
					std::cerr << "Ignoring unrecognized switch " << arg << std::endl;
				}
			}
			else if (arg [1] == 't')
			{
				// -t:<file type>
				if (arg [2] == ':')
				{
					if (arg [3] != '\0')
					{
						std::string type (&arg [3]);
						if (type == "header" && validSwitch.IsTypeHeader ())
							_options.SetTypeHeader ();
						else if (type == "source" && validSwitch.IsTypeSource ())
							_options.SetTypeSource ();
						else if (type == "text" && validSwitch.IsTypeText ())
							_options.SetTypeText ();
						else if (type == "binary" && validSwitch.IsTypeBinary ())
							_options.SetTypeBinary ();
						else
							std::cerr << "Ignoring unrecognized file type '" << type << "'" << std::endl;
					}
					else
					{
						std::cerr << arg << " - missing file type."  << std::endl;
					}
				}
				else
				{
					std::cerr << "Ignoring unrecognized switch " << arg << std::endl;
				}
			}
			else if (arg [1] == 'i')
			{
				if (validSwitch.IsDoCheckIn ())
					_options.SetDoCheckIn ();
			}
			else if (arg [1] == 'l')
			{
				if (validSwitch.IsListing ())
					_options.SetListing ();
				else if (validSwitch.IsLocalEdits ())
					_options.SetLocalEdits ();
			}
			else if (arg [1] == 'v' && validSwitch.IsVersion ())
			{
				// -v:<version id>
				if (arg [2] == ':')
				{
					if (arg [3] != '\0')
						_options.SetVersionId (&arg [3]);
					else
						std::cerr << arg << " - missing version id."  << std::endl;
				}
				else
				{
					std::cerr << "Ignoring unrecognized switch " << arg << std::endl;
				}
			}
			else if (arg[1] == 'p')
			{
				if (arg [2] == ':')
				{
					if (validSwitch.IsProject ())
					{
						if (std::isdigit (arg [3], std::locale ("C")))
						{
							std::istringstream in (&arg[3]);
							in >> _projId;
						}
						else
						{
							std::cerr << arg << " - missing project id (expected -p:n where n is project id)."  << std::endl;
						}
					}
					else if (validSwitch.IsFtpPassword ())
					{
						if (arg [3] != '\0')
						{
							_options.SetFtpPassword (&arg [3]);
						}
						else
						{
							std::cerr << arg << " - missing ftp password."  << std::endl;
						}
					}
				}
				else
				{
					std::cerr << "Ignoring unrecognized switch " << arg << std::endl;
				}
			}
			else if (arg [1] == 'd' && validSwitch.IsDeleteFromDisk ())
			{
				_options.SetDeleteFromDisk ();
			}
			else if (arg [1] == 'u')
			{
				// -u:<user id>
				if (arg [2] == ':')
				{
					if (validSwitch.IsUserId ())
					{
						if (arg [3] != '\0')
							_options.SetUserId (&arg [3]);
						else
							std::cerr << arg << " - missing user id."  << std::endl;
					}
					else if (validSwitch.IsFtpUser ())
					{
						if (arg [3] != '\0')
							_options.SetFtpUser (&arg [3]);
						else
							std::cerr << arg << " - missing ftp user name."  << std::endl;
					}
				}
				else
				{
					std::cerr << "Ignoring unrecognized switch " << arg << std::endl;
				}
			}
			else if (arg [1] == 'n' && validSwitch.IsUserName ())
			{
				// -n:<user name>
				if (arg [2] == ':')
				{
					if (arg [3] != '\0')
						_options.SetUserName (&arg [3]);
					else
						std::cerr << arg << " - missing user name."  << std::endl;
				}
				else
				{
					std::cerr << "Ignoring unrecognized switch " << arg << std::endl;
				}
			}
			else if (arg [1] == 's')
			{
				// -s:<user state>
				if (arg [2] == ':')
				{
					if (validSwitch.IsUserState ())
					{
						if (arg [3] != '\0')
						{
							std::string state (&arg [3]);
							if (state == "voting"   ||
								state == "observer" ||
								state == "remove"   ||
								state == "admin")
								_options.SetUserState (state);
							else
								std::cerr << "Ignoring unrecognized user state '" << state << "'" << std::endl;
						}
						else
						{
							std::cerr << arg << " - missing user state."  << std::endl;
						}
					}
					else if (validSwitch.IsFtpServer ())
					{
						if (arg [3] != '\0')
						{
							_options.SetFtpServer (&arg [3]);
						}
						else
						{
							std::cerr << arg << " - missing ftp server."  << std::endl;
						}
					}
				}
				else
				{
					std::cerr << "Ignoring unrecognized switch " << arg << std::endl;
				}
			}
			else if (arg [1] == 'd')
			{
				// -d:<dump type>
				if (arg [2] == ':')
				{
					if (arg [3] != '\0')
					{
						std::string type (&arg [3]);
						if (type == "version" && validSwitch.IsDumpVersion ())
							_options.SetDumpVersion ();
						else if (type == "catalog" && validSwitch.IsDumpCatalog ())
							_options.SetDumpCatalog ();
						else if (type == "membership" && validSwitch.IsDumpMembership ())
							_options.SetDumpMembership ();
						else if (type == "history" && validSwitch.IsDumpHistory ())
							_options.SetDumpHistory ();
						else if (type == "all" && validSwitch.IsDumpAll ())
							_options.SetDumpAll ();
						else if (type == "pick_license" && validSwitch.IsPickLicense ())
							_options.SetPickLicense ();
						else if (type.find_first_not_of ("0123456789abcdefABCDEF") == std::string::npos && validSwitch.IsNotifySink ())
						{
							unsigned long sink;
							if (HexStrToUnsigned (type.c_str (), sink))
							{
								_options.SetNotifySink ();
								_notifySink = sink;
							}
							else
								std::cerr << "Ignoring unrecognized notify sink id '" << type << "'"  << std::endl;
						}
						else
							std::cerr << "Ignoring unrecognized diagnostic information type '" << type << "'" << std::endl;
					}
					else
					{
						std::cerr << arg << " - diagnostic information type."  << std::endl;
					}
				}
				else
				{
					std::cerr << "Ignoring unrecognized switch " << arg << std::endl;
				}
			}
			else
			{
				std::cerr << "Ignoring unrecognized switch " << arg << std::endl;
			}
		}
		else
		{
			// Found file -- canonicalize path
			std::string argPath;
			if (FilePath::IsAbsolute (arg))
			{
				argPath = arg;
			}
			else
			{
				// Use current folder
				CurrentFolder curFolder;
				if (FilePath::IsSeparator (arg [0]))
				{
					// use current drive
					argPath = curFolder.GetDrive ();
					argPath += arg;
				}
				else
				{
					argPath = curFolder.GetFilePath (arg);
				}
			}
			char canonicalPath [MAX_PATH];
			if (!::PathCanonicalize (canonicalPath, argPath.c_str ()))
				throw Win::Exception ("Cannot canonicalize path.", argPath.c_str ());
			if (pathsOnly)
			{
				_paths.push_back (canonicalPath);
			}
			else
			{
				if (File::IsFolder (canonicalPath))
				{
					_paths.push_back (canonicalPath);
				}
				else
				{
					// File path or path with file pattern
					if (FilePath::IsValid (canonicalPath))
					{
						_paths.push_back (canonicalPath);
					}
					else
					{

						PathSplitter splitter (canonicalPath);
						std::string filePattern = splitter.GetFileName ();
						filePattern += splitter.GetExtension ();
						std::string path = splitter.GetDrive ();
						path += splitter.GetDir ();
						if (_options.IsRecursive ())
						{
							if (filePattern == "*.*" || filePattern == "*")
							{
								// Special case of recursive listing -- select only
								// first level and Co-op will select all deeper levels.
								FileFinder finder (path, filePattern, false, _deepPaths);
								finder.ListFolders (_deepPaths);
								continue;
							}
						}
						FileFinder finder (path, filePattern, _options.IsRecursive (), _paths);
					}
				}
			}
		}
	}
	if (_options.IsAllInProject () && _paths.size () == 0 && _deepPaths.size () == 0)
	{
		// User specified only -a option without file or folder -- select the current folder.
		// This will allow Code Co-op to identify the project.
		CurrentFolder curFolder;
		_paths.push_back (curFolder.GetDir ());
	}
}

CmdArgs::CmdArgs (char const * path)
	: _projId (-1)
{
	char canonicalPath [MAX_PATH];
	if (!::PathCanonicalize (canonicalPath, path))
		throw Win::Exception ("Cannot canonicalize path.", path);
	if (File::IsFolder (canonicalPath))
	{
		FileFinder finder (canonicalPath, "*.*", true, _paths);
	}
	else
	{
		PathSplitter splitter (canonicalPath);
		std::string rootPath = splitter.GetDrive ();
		rootPath += splitter.GetDir ();
		// Recursively list root path contents
		FileFinder finder (rootPath, "*.*", true, _paths);
	}
}

CmdArgs::CmdArgs (FilePath const & path)
	: _projId (-1)
{
	// Recursively list root path contents
	FileFinder finder (path.GetDir (), "*.*", true, _paths);
}

CmdArgs::FileFinder::FileFinder (std::string const & path,
								 std::string const & filePattern,
								 bool isRecursive,
								 StringArray & files)
	: _filePattern (filePattern),
	  _curPath (path)
{
	Assert (FilePath::IsAbsolute (_curPath.GetDir ()));
	ListFiles (isRecursive, files);
}

void CmdArgs::FileFinder::ListFiles (bool isRecursive, StringArray & files)
{
	for (FileSeq seq (_curPath.GetFilePath (_filePattern)); !seq.AtEnd (); seq.Advance ())
	{
		if (!seq.IsFolder ())
		{
			FilePath workPath (_curPath.GetFilePath (seq.GetName ()));
			workPath.ConvertToLongPath ();
			files.push_back (workPath.GetDir ());
		}
	}
	if (isRecursive)
	{
		for (DirSeq seq (_curPath.GetAllFilesPath ()); !seq.AtEnd (); seq.Advance ())
		{
			_curPath.DirDown (seq.GetName ());
			ListFiles (isRecursive, files);
			_curPath.DirUp ();
		}
	}
}

void CmdArgs::FileFinder::ListFolders (StringArray & files)
{
	for (DirSeq seq (_curPath.GetAllFilesPath ()); !seq.AtEnd (); seq.Advance ())
	{
		FilePath workPath (_curPath.GetFilePath (seq.GetName ()));
		workPath.ConvertToLongPath ();
		files.push_back (workPath.GetDir ());
	}
}
