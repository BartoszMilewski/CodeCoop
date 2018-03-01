#if !defined (SYSVER_H)
#define SYSVER_H
//
// (c) Reliable Software 1997, 1998
//

#include <ostream>

class SystemVersion: public OSVERSIONINFO
{
public:
    SystemVersion ()
    {
        memset (this, 0, sizeof (OSVERSIONINFO));
        dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
        _isOK = ::GetVersionEx (this) != 0;
    }

    bool IsOK () const { return _isOK; }
    bool IsWin95 () const { return dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && dwMinorVersion == 0; }
    bool IsWin98 () const { return dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && dwMinorVersion == 10; }
    bool IsWin32Windows () const { return dwPlatformId == VER_PLATFORM_WIN32_WINDOWS; }
    bool IsWinNT () const { return dwPlatformId == VER_PLATFORM_WIN32_NT; }
    int  MajorVer () const { return dwMajorVersion; }
    int  MinorVer () const { return dwMinorVersion; }
	int  BuildNum () const { return IsWinNT () ? dwBuildNumber : LOWORD(dwBuildNumber); }
	char const *CSDVersion () const { return szCSDVersion; }

private:
    bool _isOK;
};

inline std::ostream& operator<< (std::ostream& os, SystemVersion& sysVer)
{
	os << std::dec << "System Version: " << sysVer.MajorVer () << "." << sysVer.MinorVer ();
	os << " (" << sysVer.BuildNum () << ") [" << sysVer.CSDVersion () << "]";
	return os;
}

#endif
