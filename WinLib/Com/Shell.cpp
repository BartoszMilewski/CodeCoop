//------------------------------------
//  (c) Reliable Software, 1997 - 2007
//------------------------------------
#include <WinLibBase.h>
#include "Shell.h"

#include <Win/Message.h>
#include <Ex/Error.h>
#include <File/Path.h>
#include <File/File.h>
#include <Sys/WinString.h>
#include <Sys/Process.h>

#include <StringOp.h>

#include <Shlwapi.h>
#include <sstream>

using namespace ShellMan;

bool ShellMan::CopyToDesktop(std::string const & srcFilePath, std::string const & targetName)
{
	FilePath userDesktopPath;
	VirtualDesktopFolder userDesktop;
	userDesktop.GetPath (userDesktopPath);

	if (File::Exists(srcFilePath))
	{
		::FileInfo info(srcFilePath);
		if (info.GetSize().IsZero())
			return false;
	}
	else
		return false;

	std::string targetPath = File::CreateUniqueName(userDesktopPath.GetFilePath(targetName));
	return File::CopyNoEx(srcFilePath.c_str(), targetPath.c_str());
}


Path::Path (char const * path)
{
	Desktop desktop;
	Init (desktop, path);
}

Path::Path (Com::IfacePtr<IShellFolder> & folder, char const * path)
{
	Init (folder, path);
}

void Path::Init (Com::IfacePtr<IShellFolder> & folder, char const * path)
{
	ULONG lenParsed = 0;
	// Convert path to UNICODE
	std::wstring wPath = ToWString (path);
	HRESULT hr = folder->ParseDisplayName (0, 0, &wPath [0], &lenParsed, &_p, 0);
	if (FAILED (hr))
		throw Com::Exception (hr, "Cannot access shell folder", path);
}

FolderBrowser::FolderBrowser (Win::Dow::Handle winOwner,
							  Ptr<ITEMIDLIST> & pidlRoot,
							  char const * userInstructions,
							  char const * dlgWinCaption,
							  char const * startupFolder)
{
	std::pair<char const *, char const *> initData (dlgWinCaption, startupFolder);
    _displayName [0] = '\0';
    _fullPath [0] = '\0';
    _browseInfo.hwndOwner = winOwner.ToNative ();
	_browseInfo.pidlRoot = pidlRoot;
    _browseInfo.pszDisplayName = _displayName;
    _browseInfo.lpszTitle = userInstructions; 
    _browseInfo.ulFlags = BIF_RETURNONLYFSDIRS  | BIF_NEWDIALOGSTYLE; 
	_browseInfo.lpfn = ShellMan::FolderBrowser::BrowseCallbackProc; 
    _browseInfo.lParam = reinterpret_cast<LPARAM>(&initData);
    _browseInfo.iImage = 0;
    _p = SHBrowseForFolder (&_browseInfo);
    if (_p != 0)
        SHGetPathFromIDList (_p, _fullPath);
}

int CALLBACK FolderBrowser::BrowseCallbackProc (HWND winBrowseDlg,
												UINT uMsg,
												LPARAM lParam,
												LPARAM lpData)
{
	if (uMsg == BFFM_INITIALIZED && lpData != 0)
	{
		// lpData == _browseInfo.lParam == pointer to the initialization data
		std::pair<char const *, char const *> const * initData =
			reinterpret_cast<std::pair<char const *, char const *> const *>(lpData);
		Win::Dow::Handle dlgWin (winBrowseDlg);
		if (initData->first != 0)
		{
			// Window caption specified -- display it
			dlgWin.SetText (initData->first);
		}
		if (initData->second != 0)
		{
			// Startup folder specified -- expand it
			// Convert path to UNICODE
			std::wstring wPath = ::ToWString (initData->second);
			Win::Message msg (BFFM_SETEXPANDED, TRUE, reinterpret_cast<LPARAM>(wPath.c_str ()));
			dlgWin.SendMsg (msg);
		}
	}
	return 0;
}

struct ShellMan::Folder::ShId const ShellMan::Folder::_shFolders [] =
{
	{ "Alternate Common Startup folder",			CSIDL_COMMON_ALTSTARTUP		},
	{ "Alternate User Startup folder",				CSIDL_ALTSTARTUP			},
	{ "Application Data folder",					CSIDL_APPDATA				},
	{ "Control Panel virtual folder",				CSIDL_CONTROLS				},
	{ "Cookies folder",								CSIDL_COOKIES				},
	{ "Desktop (namespace root)",					CSIDL_DESKTOP				},
	{ "Desktop folder (All Users profile)",			CSIDL_COMMON_DESKTOPDIRECTORY },
	{ "Desktop folder",								CSIDL_DESKTOPDIRECTORY		},
	{ "Favorites folder (All Users profile)",		CSIDL_COMMON_FAVORITES		},
	{ "Favorites folder",							CSIDL_FAVORITES				},
	{ "Fonts virtual folder",						CSIDL_FONTS					},
	{ "History folder",								CSIDL_HISTORY				},
	{ "Internet Cache folder",						CSIDL_INTERNET_CACHE		},
	{ "Internet virtual folder",					CSIDL_INTERNET				},
	{ "My Computer virtual folder",					CSIDL_DRIVES				},
	{ "Network Neighborhood root",					CSIDL_NETWORK				},
	{ "Network Neighborhood directory",				CSIDL_NETHOOD				},
	{ "Personal folder",							CSIDL_PERSONAL				},
	{ "Printers virtual folder",					CSIDL_PRINTERS				},
	{ "PrintHood folder",							CSIDL_PRINTHOOD				},
	{ "Programs folder (under Start menu in All Users profile)",CSIDL_COMMON_PROGRAMS },
	{ "Programs folder (under Start menu in user profile)", CSIDL_PROGRAMS		},
	{ "Recent folder",								CSIDL_RECENT				},
	{ "Recycle Bin folder",							CSIDL_BITBUCKET				},
	{ "SendTo folder",								CSIDL_SENDTO				},
	{ "Start menu (All Users profile)",				CSIDL_COMMON_STARTMENU		},
	{ "Start menu (user profile)",					CSIDL_STARTMENU				},
	{ "Startup folder (All Users profile)",			CSIDL_COMMON_STARTUP		},
	{ "Startup folder (user profile)",				CSIDL_STARTUP				},
	{ "Templates folder",							CSIDL_TEMPLATES				}
};

ShellMan::Folder::Folder (int folderId, int alternativeFolderId)
{
    if (::SHGetSpecialFolderLocation (0, folderId, &_p) != NOERROR)
    {
		// Try alternative id if present
        if (alternativeFolderId != -1)
        {
			if (::SHGetSpecialFolderLocation (0, alternativeFolderId, &_p) != NOERROR)
			{
                throw Win::Exception ("Internal error: Cannot access shell folder", GetShellFolderName (alternativeFolderId));
			}
        }
        else
		{
            throw Win::Exception ("Internal error: Cannot access shell folder", GetShellFolderName (folderId));
		}
    }
}

char const * ShellMan::Folder::GetShellFolderName (int id) const
{
	for (int i = 0; i < sizeof (_shFolders) / sizeof (_shFolders [0]); ++i)
	{
		if (_shFolders [i].id == id)
			return _shFolders [i].name;
	}
	return 0;
}

bool ShellMan::Folder::GetPath (FilePath & path)
{
    char pathBuf [MAX_PATH];
    if (::SHGetPathFromIDList (_p, pathBuf))
    {
        path.Change (pathBuf);
        return true;
    }
    return false;
}

void ShortCut::Save (char const * linkPath)
{
	PersistFile linkFile (_link);
	// Convert link file path to UNICODE
	std::wstring wLinkPath = ToWString (linkPath);
	if (SUCCEEDED (linkFile->Save (&wLinkPath [0], TRUE)))
		linkFile->SaveCompleted (&wLinkPath [0]);
	else
		throw Win::Exception ("Internal error: Cannot create application shortcut");
}

ShellMan::FileInfo::FileInfo (std::string const & path)
{
	if (!::SHGetFileInfo (path.c_str (),
						  0,
						  &_shellFileInfo,
						  sizeof (SHFILEINFO),
						  SHGFI_ICON | SHGFI_LARGEICON | SHGFI_DISPLAYNAME | SHGFI_TYPENAME))
		throw Win::Exception ("Internal error: Cannot read file information", path.c_str ());
}

#if 0	//	::AssocQueryString not available on some Win 95 or NT 4.0 platforms;
		//	excluding this code allows debug builds to run on those platforms

ShellMan::AssociatedCommand::AssociatedCommand (std::string const & fileNameExtension,
												std::string const & verb)
{
	_cmd.resize (MAX_PATH);
	unsigned long bufSize = _cmd.size ();
	HRESULT hr = ::AssocQueryString (ASSOCF_NOTRUNCATE |	// Do not truncate the return string. Return an error value and the required size for the complete string.
										 ASSOCF_REMAPRUNDLL,	// If a command uses Rundll.exe, setting this flag tells the method to ignore Rundll.exe and return information about its target.
										 ASSOCSTR_EXECUTABLE,	// Executable from a Shell verb command string.
										 fileNameExtension.c_str (),
										 verb.c_str (),
										 &_cmd [0],
										 &bufSize);
	if (hr == E_POINTER)
	{
		// Buffer too small
		_cmd.resize (bufSize);
		hr = ::AssocQueryString (ASSOCF_NOTRUNCATE |	// Do not truncate the return string. Return an error value and the required size for the complete string.
									 ASSOCF_REMAPRUNDLL,	// If a command uses Rundll.exe, setting this flag tells the method to ignore Rundll.exe and return information about its target.
									 ASSOCSTR_EXECUTABLE,	// Executable from a Shell verb command string.
									 fileNameExtension.c_str (),
									 verb.c_str (),
									 &_cmd [0],
									 &bufSize);
	}

	if (hr != S_OK)
	{
		std::ostringstream info;
		info << "File name extension: " << fileNameExtension.c_str () << "; verb: " << verb.c_str ();
		throw Com::Exception (hr, "Cannot retrieve Shell verb command string", info.str ().c_str ());
	}
}

void ShellMan::AssociatedCommand::Execute (std::string const & path)
{
	std::ostringstream cmdLine;
	cmdLine << _cmd.c_str () << " \"" << path.c_str () << "\"";
	Win::ChildProcess assocApp (cmdLine.str ().c_str (), true);	// Inherit parent's handles
	assocApp.SetAppName (_cmd);
	assocApp.ShowNormal ();
	assocApp.Create ();
}

#endif

// ShellMan helper functions

// Transform HINSTANCE returned by ::ShellExecute to error code
// Returns errCode or ShellMan::Success
int ErrCode (Win::Instance hInst)
{
    int errCode = reinterpret_cast<int> (static_cast<HINSTANCE> (hInst));
    if (errCode > 32)
	{
		// No problems
		Win::ClearError ();	// Clear any errors set by shell
        errCode = ShellMan::Success;
	}
    return errCode;
}

// returns ShellMan::Success if success
int ShellAction (Win::Dow::Handle win, char const * action, char const * path, char const * arguments = 0, char const * directory = 0)
{
	HINSTANCE hInst = ::ShellExecute (win.ToNative (), action, path, arguments, directory, SW_SHOWNORMAL);
	return ErrCode (hInst);
}

namespace ShellMan
{
	int Search (Win::Dow::Handle win, char const * path)
	{
		return ShellAction (win, "find", path);
	}
	int Explore (Win::Dow::Handle win, char const * path)
	{ 
		return ShellAction (win, "explore", path); 
	}
    int Open    (Win::Dow::Handle win, char const * path) 
	{ 
		return ShellAction (win, 0, path); // default verb (usually "open")
	}
#if 0  // SHObjectProperties is only available in versions of the shell DLLs 5.0 and later, indicating it may not run on all windows systems
	bool Properties (Win::Dow::Handle win, char const * path, char const * page)
    {
		std::wstring wPath = ToWString (path);
		std::wstring wPage = ToWString (page);
		BOOL result = ::SHObjectProperties (win.ToNative (),
									SHOP_FILEPATH,
									&wPath [0],
									&wPage [0]);
		return result != FALSE;
    }
#endif
    int Execute (Win::Dow::Handle win, char const * path, char const * arguments, char const * directory)
    {
        return ShellAction (win, "open", path, arguments, directory);
    }
    int FindExe (char const * filePath, char * exePath)
    {
        HINSTANCE hInst = ::FindExecutable (filePath, 0, exePath);
        return ErrCode (hInst);
    }

	std::string HtmlOpenError (int errCode, char const * what, char const * path)
	{
		std::ostringstream info;
        if (errCode == ERROR_FILE_NOT_FOUND)
        {
            info << "File not found\n" << path;
        }
        else if (errCode == SE_ERR_NOASSOC)
        {
            info << "To view " << what << " you have to have a Web Browser installed";
        }
        else if (errCode == 0 || errCode == SE_ERR_OOM)
        {
            info << "Cannot show " << what <<  "due to low system resources\n"
                    "Close some windows to free system resources";
        }
        else
        {
            SysMsg errInfo (errCode);
            info << "Problem showing " << what << ".  System tells us:\n" << errInfo.Text ();
        }
		return info.str ();
    }

	void Delete (Win::Dow::Handle win, char const * path, FILEOP_FLAGS flags)
	{
		if (!File::Exists (path))
			return;

		// SHFileOperation requires that the from path is ended with double '\0'
		// WARNING: path cannot be allocated on the heap -- SHFileOperation will fail
		char fromPath [MAX_PATH + 2];
		memset (fromPath, 0, sizeof (fromPath));
		SHFILEOPSTRUCT fileInfo;
		memset (&fileInfo, 0, sizeof (fileInfo));
		fileInfo.hwnd = win.ToNative (); 
		fileInfo.wFunc = FO_DELETE; 
		fileInfo.pFrom = fromPath;
		fileInfo.fFlags = flags;
		strcpy (fromPath, path);
		if (::SHFileOperation (&fileInfo))
		{
			if (fileInfo.fAnyOperationsAborted)
				throw Win::Exception ();
			else
				throw Win::Exception ("Internal error: Cannot delete file or folder.", fromPath);
		}
		if (fileInfo.fAnyOperationsAborted)
			throw Win::Exception ();
		Win::ClearError ();	// Clear any error code set by the shell
	}

	// Doesn't throw when delete fails -- returns false instead
	bool QuietDelete (Win::Dow::Handle win, char const * path) throw ()
	{
		// SHFileOperation requires that the from path is ended with double '\0'
		// WARNING: path cannot be allocated on the heap -- SHFileOperation will fail
		char fromPath [MAX_PATH + 2];
		memset (fromPath, 0, sizeof (fromPath));
		SHFILEOPSTRUCT fileInfo;
		memset (&fileInfo, 0, sizeof (fileInfo));
		fileInfo.hwnd = win.ToNative (); 
		fileInfo.wFunc = FO_DELETE; 
		fileInfo.pFrom = fromPath;
		fileInfo.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI;
		strcpy (fromPath, path);
		int result = ::SHFileOperation (&fileInfo);
		Win::ClearError ();	// Clear any error code set by the shell
		return (result == 0) && !fileInfo.fAnyOperationsAborted;
	}

	void DeleteContents (Win::Dow::Handle win, char const * path, FILEOP_FLAGS flags)
	{
		if (!File::Exists (path))
			return;

		FilePath folderContent (path);
		// SHFileOperation requires that the from path is ended with double '\0'
		// WARNING: path cannot be allocated on the heap -- SHFileOperation will fail
		char fromPath [MAX_PATH + 2];
		memset (fromPath, 0, sizeof (fromPath));
		SHFILEOPSTRUCT fileInfo;
		memset (&fileInfo, 0, sizeof (fileInfo));
		fileInfo.wFunc = FO_DELETE; 
		fileInfo.pFrom = fromPath;
		fileInfo.fFlags = flags;
		strcpy (fromPath, folderContent.GetAllFilesPath ());
		if (::SHFileOperation (&fileInfo))
		{
			if (fileInfo.fAnyOperationsAborted)
				throw Win::Exception ();
			else
				throw Win::Exception ("Internal error: Cannot delete folder contents", fromPath);
		}
		if (fileInfo.fAnyOperationsAborted)
			throw Win::Exception ();
		Win::ClearError ();	// Clear any error code set by the shell
	}

	void CopyContents (Win::Dow::Handle win,
					   char const * fromPath, 
					   char const * toPath, 
					   FILEOP_FLAGS flags)
	{
		FilePath fromFolderContent (fromPath);
		// SHFileOperation requires that the from path is ended with double '\0'
		// WARNING: paths cannot be allocated on the heap -- SHFileOperation will fail
		char srcPath [MAX_PATH + 2];
		char tgtPath [MAX_PATH + 2];
		memset (srcPath, 0, sizeof (srcPath));
		memset (tgtPath, 0, sizeof (tgtPath));
		strcpy (srcPath, fromFolderContent.GetAllFilesPath ());
		strcpy (tgtPath, toPath);
		SHFILEOPSTRUCT fileInfo;
		memset (&fileInfo, 0, sizeof (fileInfo));
		fileInfo.hwnd = win.ToNative ();
		fileInfo.wFunc = FO_COPY; 
		fileInfo.pFrom = srcPath;
		fileInfo.pTo = tgtPath;
		fileInfo.fFlags = flags;
		if (::SHFileOperation (&fileInfo))
		{
			if (fileInfo.fAnyOperationsAborted)
				throw Win::Exception ();
			else
				throw Win::Exception ("Internal error: Cannot copy folder contents", fromPath);
		}
		if (fileInfo.fAnyOperationsAborted)
			throw Win::Exception ();
		Win::ClearError ();	// Clear any error code set by the shell
	}

	// Revisit: File list should be something else than char const * -- too confusing
	// SHFileOperation requires that the file list is ended with double '\0'
	// File list and toPath can contain multiple file paths separated by '\0'
	void CopyFiles (Win::Dow::Handle win,
					char const * fileList,
					char const * toPath,
					FILEOP_FLAGS flags,
					char const * title)
	{
		SHFILEOPSTRUCT fileInfo;
		memset (&fileInfo, 0, sizeof (fileInfo));
		fileInfo.hwnd = win.ToNative ();
		fileInfo.wFunc = FO_COPY; 
		fileInfo.pFrom = fileList;
		fileInfo.pTo = toPath;
		fileInfo.fFlags = flags;
		fileInfo.lpszProgressTitle = title;
        int err = ::SHFileOperation(&fileInfo);
        if (err)
		{
			if (fileInfo.fAnyOperationsAborted)
				throw Win::Exception ();
			else
				throw Win::Exception ("Internal error: Cannot copy files to:", toPath);
		}
		if (fileInfo.fAnyOperationsAborted)
			throw Win::Exception ();
		Win::ClearError ();	// Clear any error code set by the shell
	}

	// Revisit: File list should be something else than char const * -- too confusing
	// SHFileOperation requires that the file list is ended with double '\0'
	// File list can contain multiple file paths separated by '\0'
	bool DeleteFiles (Win::Dow::Handle win,
					  char const * fileList,
					  FILEOP_FLAGS flags,
					  char const * title,
					  bool quiet)
	{
		SHFILEOPSTRUCT fileInfo;
		memset (&fileInfo, 0, sizeof (fileInfo));
		fileInfo.hwnd = win.ToNative ();
		fileInfo.wFunc = FO_DELETE; 
		fileInfo.pFrom = fileList;
		fileInfo.fFlags = flags;
		fileInfo.lpszProgressTitle = title;
		int result = ::SHFileOperation (&fileInfo);
		if (quiet)
		{
			Win::ClearError ();	// Clear any error code set by the shell
			return (result == 0) && !fileInfo.fAnyOperationsAborted;
		}
		else
		{
			if (fileInfo.fAnyOperationsAborted)
				throw Win::Exception ();
			if (result != 0)
			{
				if ((flags & FOF_NOERRORUI) != 0)	// Throw exception only if Shell was asked not to display error information
					throw Win::Exception ("Internal error: Cannot delete files");
				else
					return false;
			}
		}
		Win::ClearError ();	// Clear any error code set by the shell
		return true;
	}

	void FileMove (Win::Dow::Handle win,
				   char const * oldPath,
				   char const * newPath,
				   FILEOP_FLAGS flags)
	{
		char srcPath [MAX_PATH + 2];
		char tgtPath [MAX_PATH + 2];
		memset (srcPath, 0, sizeof (srcPath));
		memset (tgtPath, 0, sizeof (tgtPath));
		strcpy (srcPath, oldPath);
		strcpy (tgtPath, newPath);
		SHFILEOPSTRUCT fileInfo;
		memset (&fileInfo, 0, sizeof (fileInfo));
		fileInfo.hwnd = win.ToNative ();
		fileInfo.wFunc = FO_MOVE; 
		fileInfo.pFrom = srcPath;
		fileInfo.pTo = tgtPath;
		fileInfo.fFlags = flags;
		if (::SHFileOperation (&fileInfo))
		{
			if (fileInfo.fAnyOperationsAborted)
				throw Win::Exception ();
			else
				throw Win::Exception ("Internal error: Cannot move file", oldPath);
		}
		if (fileInfo.fAnyOperationsAborted)
			throw Win::Exception ();
		Win::ClearError ();	// Clear any error code set by the shell
	}
};
