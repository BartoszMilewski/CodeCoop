#if !defined (APPINFO_H)
#define APPINFO_H
//------------------------------------
//  (c) Reliable Software, 1997 - 2008
//------------------------------------

#include "GlobalFileNames.h"

#include <Win/Win.h>
#include <File/Path.h>

class AppInformation;
extern AppInformation TheAppInfo;

class AppInformation
{
public:
	class Initializer
	{
	public:
		Initializer () { TheAppInfo.InitPaths (); }
		~Initializer () { TheAppInfo.Clear (); }
	};

    AppInformation ()
        : _isFirstInstance (true), _win (0)
    {}

    void InitPaths ();
    void SetFirstInstanceFlag (bool isFirstInstance) { _isFirstInstance = isFirstInstance; }
	void SetWindow (Win::Dow::Handle win) { _win = win; }
	void Clear () { _pgmPath.Clear (); }

    bool IsFirstInstance () const { return _isFirstInstance; }
	bool IsTemporaryUpdate () const;
#if defined (COOP_PRO)
	bool IsFtpSupportEnabled () const { return true; }
#else
	bool IsFtpSupportEnabled () const { return false; }
#endif
	FilePath const & GetProgramPath () const { return _pgmPath; }
    char const * GetDifferPath () const { return _pgmPath.GetFilePath (DifferExeName); }
	char const * GetCabArcPath () const { return _pgmPath.GetFilePath (CabArcExe); }
	char const * GetFtpAppletPath () const { return _pgmPath.GetFilePath (FtpAppExe); }
    char const * GetUninstallerPath () const { return _pgmPath.GetFilePath (UninstallExeName); }
	Win::Dow::Handle GetWindow () const { return _win; }

private:
    bool				_isFirstInstance;
    FilePath			_pgmPath;
	Win::Dow::Handle	_win;
};

#endif
