#if !defined (DLL_H)
#define DLL_H
// -----------------------------------
// (c) Reliable Software, 1999 -- 2002
// -----------------------------------

#include <Win/Instance.h>

#include <shlwapi.h>
#include <string>

class Dll: public Win::Instance
{
public:
	Dll (std::string const & filename, bool quietLoad = false);
	~Dll ();

	std::string const & GetFilename () const { return _filename; }

	void * GetFunction (std::string const & funcName) const;

	template <class T>
	void GetFunction (std::string const & funcName, T & funPointer)
	{
		funPointer = static_cast<T> (GetFunction (funcName));
	}

private:
	std::string const _filename;
};

class DllVersion
{
public:
	DllVersion (Dll const & dll);

	bool IsOk () const { return _isOk; }
	int GetMajorVer () const { return _version.dwMajorVersion; }
	int GetMinorVer () const { return _version.dwMinorVersion; }

private:
	DLLVERSIONINFO	_version;
	bool			_isOk;
};

#endif
