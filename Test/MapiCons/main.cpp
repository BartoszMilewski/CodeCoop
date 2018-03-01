#include <windows.h>
#include <mapi.h>
#include <iostream>

class DbgGuiLeak
{
public:
	explicit DbgGuiLeak ()
	{
		_guiResCount = ::GetGuiResources (::GetCurrentProcess (), GR_GDIOBJECTS);
	}
	~DbgGuiLeak ()
	{
		int leaks = ::GetGuiResources (::GetCurrentProcess (), GR_GDIOBJECTS) - _guiResCount;
		if (leaks != 0)
		{
			std::cout << "Gui Resources Leaked: " << leaks << std::endl;
		}
	}
private:
	unsigned _guiResCount;
};

class Dll
{
public:
	Dll (std::string const & filename);
	~Dll ();

	std::string const & GetFilename () const { return _filename; }

	template <class T>
	void GetFunction (std::string const & funcName, T & funPointer)
	{
		funPointer = static_cast<T> (GetFunction (funcName));
	}

private:
	void * GetFunction (std::string const & funcName) const;

	std::string const _filename;
	HINSTANCE _h;
};

Dll::Dll (std::string const & filename)
	: _filename (filename),
	  _h (::LoadLibrary (filename.c_str ()))
	  
{
	if (_h == 0)
		throw "Cannot load dynamic link library";
}

Dll::~Dll ()
{
	if (_h != 0)
	{
		::FreeLibrary (_h);
	}
}

void * Dll::GetFunction (std::string const & funcName) const
{
	void * pFunction = ::GetProcAddress (_h, funcName.c_str ());
	if (pFunction == 0)
	{
		throw "Cannot find function in the dynamic link library";
	}
	return pFunction;
}

namespace SimpleMapi
{
	class Session
	{
	public:
		Session (Dll & mapi);
		~Session ();
	private:
		Dll		&_mapi;
		LHANDLE	_h;
	private:
		typedef ULONG (FAR PASCAL *Logon) (ULONG ulUIParam,
										LPTSTR lpszProfileName,
										LPTSTR lpszPassword,
										FLAGS flFlags,
										ULONG ulReserved,
										LPLHANDLE lplhSession);
		typedef ULONG (FAR PASCAL *Logoff) (LHANDLE session,
											ULONG ulUIParam,
											FLAGS flFlags,
											ULONG reserved);
	};

	Session::Session (Dll & mapi)
		: _mapi (mapi)
	{
		Logon logon;
		_mapi.GetFunction ("MAPILogon", logon);
		std::cout << "Mapi Logon" << std::endl;
		ULONG rCode = logon (0,		// Handle to window displaying Simple MAPI dialogs
							0,		// Use default profile
							0,		// No password
							0,		// No flags
							0,		// Reserved -- must be zero
							&_h);	// Session handle
		if (rCode != SUCCESS_SUCCESS)
			throw "Logon failed";
	}

	Session::~Session ()
	{
		Logoff logoff;
		_mapi.GetFunction ("MAPILogoff", logoff);
		std::cout << "Mapi Logoff" << std::endl;
		ULONG rCode = logoff (_h, 0, 0, 0);

		if (rCode != SUCCESS_SUCCESS)
			throw "Logoff failed";
	}
}

void TestMapi ()
{
	DbgGuiLeak leak;
	Dll mapi ("mapi32.dll");
	SimpleMapi::Session session (mapi);
}

int main ()
{
	try
	{
		TestMapi ();
		TestMapi ();
		TestMapi ();
	}
	catch (char const * msg)
	{
		std::cerr << msg << std::endl;
	}
}

