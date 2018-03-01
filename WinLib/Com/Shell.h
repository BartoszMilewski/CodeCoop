#if !defined SHELL_H
#define SHELL_H
//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------
#include <Com/Com.h>
#include <Graph/Icon.h>

class FilePath;

namespace ShellMan
{
	bool CopyToDesktop(std::string const & srcFilePath, std::string const & targetName);

	enum { Success = -1 };

	template <class T>
	class Ptr
	{
	public:
		~Ptr ()
		{
			Free ();
			_shellMalloc->Release ();
		}
		T * operator->() { return _p; }
		T const * operator->() const { return _p; }
		operator T const * () const { return _p; }
		T const & GetAccess () const { return *_p; }

	protected:
		Ptr () : _p (0) 
		{
			// Obtain malloc here, rather than
			// in the destructor. 
			// Destructor must be fail-proof.
			// Revisit: Would static LPMALLOC _shellMalloc work?
			if (SHGetMalloc (&_shellMalloc) == E_FAIL)
				throw Win::Exception ("Internal error: Cannot obtain shell Malloc"); 
		}
		void Free ()
		{
			if (_p != 0)
				_shellMalloc->Free (_p);
			_p = 0;
		}

		T       * _p;
		IMalloc * _shellMalloc;
	private:
		Ptr (Ptr const & p) {}
		void operator = (Ptr const & p) {}
	};

	class Desktop: public Com::IfacePtr<IShellFolder>
	{
	public:
		Desktop ()
		{
			if (SHGetDesktopFolder (&(*this)) != NOERROR)
				throw Win::Exception ("Internal error: Cannot access shell Desktop folder");
			// Revisit: use ComException with HRESULT error code
		}
	};

	// Shell Path: Shell uses this pre-parsed form of path in its operations
	class Path: public Ptr<ITEMIDLIST>
	{
	public:
		Path (char const * path);
		Path (Com::IfacePtr<IShellFolder> & folder, char const * path);

	private:
		void Init (Com::IfacePtr<IShellFolder> & folder, char const * path);
	};

	class FolderBrowser: public Ptr<ITEMIDLIST>
	{
	public:
		FolderBrowser (Win::Dow::Handle winOwner,
					   Ptr<ITEMIDLIST> & pidlRoot,
					   char const * userInstructions,
					   char const * dlgWinCaption = 0,
					   char const * startupFolder = 0);

		char const * GetDisplayName () { return _displayName; }
		char const * GetPath ()        { return _fullPath; }
		bool IsOK() const              { return _p != 0; };

	private:
		static int CALLBACK BrowseCallbackProc (HWND winBrowseDlg,
												UINT uMsg,
												LPARAM lParam,
												LPARAM lpData);

	private:
		char       _displayName [MAX_PATH];
		char       _fullPath [MAX_PATH];
		BROWSEINFO _browseInfo;
	};

	class Folder: public Ptr<ITEMIDLIST>
	{
	public:
		bool GetPath (FilePath & path);

	protected:
		Folder (int folderId, int alternativeFolderId = -1);

	private:
		char const * GetShellFolderName (int id) const;

	private:
		struct ShId
		{
			char const * name;
			int			 id;
		};

		static struct ShId const _shFolders [];
	};

	class DrivesPath : public Folder
	{
	public:
		DrivesPath ()
			: Folder (CSIDL_DRIVES)
		{}
	};

	class NetworkPath : public Folder
	{
	public:
		NetworkPath ()
			: Folder (CSIDL_NETWORK)
		{}
	};

	class CommonProgramsFolder : public Folder
	{
	public:
		CommonProgramsFolder ()
			: Folder (CSIDL_COMMON_PROGRAMS, CSIDL_PROGRAMS)
		{}
	};

	class UserProgramsFolder : public Folder
	{
	public:
		UserProgramsFolder ()
			: Folder (CSIDL_PROGRAMS, CSIDL_PROGRAMS)
		{}
	};

	class DesktopFolder : public Folder
	{
	public:
		DesktopFolder ()
			: Folder (CSIDL_DESKTOPDIRECTORY)
		{}
	};

	class VirtualDesktopFolder : public Folder
	{
	public:
		VirtualDesktopFolder ()
			: Folder (CSIDL_DESKTOP)
		{}
	};

	class CommonDesktopFolder : public Folder
	{
	public:
		CommonDesktopFolder ()
			: Folder (CSIDL_COMMON_DESKTOPDIRECTORY, CSIDL_DESKTOPDIRECTORY)
		{}
	};

	class UserDesktopFolder : public Folder
	{
	public:
		UserDesktopFolder ()
			: Folder (CSIDL_DESKTOPDIRECTORY, CSIDL_DESKTOPDIRECTORY)
		{}
	};

	class CommonStartupFolder : public Folder
	{
	public:
		CommonStartupFolder ()
			: Folder (CSIDL_COMMON_STARTUP, CSIDL_STARTUP)
		{}
	};

	class UserStartupFolder : public Folder
	{
	public:
		UserStartupFolder ()
			: Folder (CSIDL_STARTUP)
		{}
	};

	class PersonalFolder : public Folder
	{
	public:
		PersonalFolder ()
			: Folder(CSIDL_PERSONAL)
		{}
	};

	class Link : public Com::IfacePtr<IShellLink>
	{
	public:
		Link ()
		{
			HRESULT hres;
			hres = CoCreateInstance (CLSID_ShellLink,
									 NULL,
									 CLSCTX_INPROC_SERVER,
									 IID_IShellLink,
									 reinterpret_cast<void **>(&_p));
			if (!SUCCEEDED (hres))
				throw Win::Exception ("Internal error: Cannot obtain Shell Link interface");
		}
	};

	class PersistFile : public Com::IfacePtr<IPersistFile>
	{
	public:
		PersistFile (Com::IfacePtr<IShellLink> & iface)
		{
			HRESULT hres;
			hres = iface->QueryInterface (IID_IPersistFile, reinterpret_cast<void **>(&_p));
			if (!SUCCEEDED (hres))
				throw Win::Exception ("Internal error: Cannot obtain Persistent File interface");
		}
	};

	class ShortCut
	{
	public:
		ShortCut ()
			: _link ()
		{}

		void SetObject (char const * objPath) { _link->SetPath (objPath); }
		void SetDescription (char const * description) { _link->SetDescription (description); }
		void Save (char const * linkPath);

	private:
		Link	_link;
	};

	class FileInfo
	{
	public:
		FileInfo (std::string const & path);

		Icon::Handle GetIcon () const { return _shellFileInfo.hIcon; }

	private:
		SHFILEINFO	_shellFileInfo;
	};

	class AssociatedCommand
	{
	public:
		AssociatedCommand (std::string const & fileNameExtension, std::string const & verb);
		void Execute (std::string const & path);

	private:
		std::string	_cmd;
	};

	enum Error
	{
		NoResource = 0,
		NoFile = ERROR_FILE_NOT_FOUND,
		NoAssoc = SE_ERR_NOASSOC,
		NoMemory = SE_ERR_OOM
	};

	enum Mode
	{
		Silent = FOF_NOERRORUI | FOF_SILENT | FOF_NOCONFIRMATION,
		SilentAllowUndo = Silent | FOF_ALLOWUNDO,
		UiNoConfirmation = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR,
		SilentNoConfirmSafeCopy = UiNoConfirmation | Silent | FOF_RENAMEONCOLLISION,
		UiNoConfirmationAllowUndo = UiNoConfirmation | FOF_ALLOWUNDO
	};

    // Shell action functions return errCode or ShellMan::Success
	int Explore (Win::Dow::Handle win, char const * path);
    int Open    (Win::Dow::Handle win, char const * path);
	int Search  (Win::Dow::Handle win, char const * path);
	bool Properties (Win::Dow::Handle win, char const * path, char const * page);
    int Execute (Win::Dow::Handle win, char const * path, char const * arguments, char const * directory = 0);
    int FindExe (char const * filePath, char * exePath);

	std::string HtmlOpenError (int errCode, char const * what, char const * path);

	void Delete (Win::Dow::Handle win, char const * path,
				 FILEOP_FLAGS flags = FOF_SILENT | FOF_NOCONFIRMATION);
	bool QuietDelete (Win::Dow::Handle win, char const * path) throw ();
	void DeleteContents (Win::Dow::Handle win, char const * path,
						 FILEOP_FLAGS flags = FOF_SILENT | FOF_NOCONFIRMATION);
	void CopyContents (Win::Dow::Handle win, char const * fromPath, char const * toPath,
					   FILEOP_FLAGS flags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR);
	void CopyFiles (Win::Dow::Handle win, char const * fileList, char const * toPath,
					FILEOP_FLAGS flags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR,
					char const * title = 0);
	bool DeleteFiles (Win::Dow::Handle win, char const * fileList,
					  FILEOP_FLAGS flags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION,
					  char const * title = 0, bool quiet = false);
	void FileMove (Win::Dow::Handle win, char const * oldPath, char const * newPath,
					  FILEOP_FLAGS flags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION);
}

#endif
