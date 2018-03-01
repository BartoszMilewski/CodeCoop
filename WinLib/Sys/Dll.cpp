// -----------------------------------
// (c) Reliable Software, 1999 -- 2004
// -----------------------------------
#include <WinLibBase.h>
#include "Dll.h"

Dll::Dll (std::string const & filename, bool quietLoad)
	: Win::Instance (::LoadLibrary (filename.c_str ())),
	  _filename (filename)
{
	if (IsNull () && !quietLoad)
		throw Win::Exception ("Cannot load dynamic link library", _filename.c_str ());
}

Dll::~Dll ()
{
	if (!IsNull ())
	{
		try
		{
			::FreeLibrary (H ());
		}
		catch (...) {}
	}
}

FunPtr Dll::GetFunction (std::string const & funcName) const
{
	FunPtr pFunction = reinterpret_cast<FunPtr> (::GetProcAddress (*this, funcName.c_str ()));
	if (pFunction == 0)
	{
		std::string info = _filename + " (" + funcName + ")";
		throw Win::Exception ("Cannot find function in the dynamic link library", info.c_str ());
	}
	return pFunction;
}

DllVersion::DllVersion (Dll const & dll)
	: _isOk (false)
{
	memset (&_version, 0, sizeof (DLLVERSIONINFO));
	_version.cbSize = sizeof (DLLVERSIONINFO);
	DLLGETVERSIONPROC dllGetVersion =
		reinterpret_cast<DLLGETVERSIONPROC>(::GetProcAddress (dll, "DllGetVersion"));
   
    if (dllGetVersion != 0)
    {
        HRESULT hr = (*dllGetVersion)(&_version);
        if (SUCCEEDED (hr))
			_isOk = true;
	}
}
