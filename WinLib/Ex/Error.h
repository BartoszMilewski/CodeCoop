#if !defined (ERROR_H)
#define ERROR_H
//----------------------------------
// (c) Reliable Software 2003 - 2007
//----------------------------------

#include <WinInet.h>

class LastSysErr
{
public:
    LastSysErr ();
    ~LastSysErr ();

	operator char const * () const  { return _msg; }
    char const * Text () const { return _msg; }
    DWORD Code () const { return _err; }
	bool IsFileNotFound () const { return _err == ERROR_FILE_NOT_FOUND; }
	bool IsPathNotFound () const { return _err == ERROR_PATH_NOT_FOUND; }
	bool IsNoMoreFiles () const { return _err == ERROR_NO_MORE_FILES; }
	bool IsInternetError () const { return _err == ERROR_INTERNET_EXTENDED_ERROR; }
	bool IsIOPending () const { return _err == ERROR_IO_PENDING; }

private:
    char * _msg;
    DWORD  _err;
};

class LastInternetError
{
public:
	LastInternetError ();

	std::string const & Text () const { return _msg; }

private:
	unsigned long	_code;
	std::string		_msg;
};

class SysMsg
{
public:
    SysMsg (DWORD errCode, HINSTANCE hModule = 0);
    ~SysMsg ();
 
	operator char const * () const  { return _msg; }
    char const * Text () const { return _msg; }

private:
    char * _msg;
};

#endif
