//------------------------------
// (c) Reliable Software 2000-03
//------------------------------
#include <WinLibBase.h>
#include "SharedMem.h"
#include "GlobalUniqueName.h"

void SharedMem::Create (unsigned int size, std::string const & name, bool quiet)
{
	if (name.empty ())
	{
		// If name not provided create globaly unique name
		GlobalUniqueName uname;
		_name = uname.GetName ();
	}
	else
	{
		_name = name;
	}
	_size = size;
	Assert (!_name.empty ());
	SECURITY_ATTRIBUTES security;
	security.nLength = sizeof (SECURITY_ATTRIBUTES);
	security.lpSecurityDescriptor = 0;
	security.bInheritHandle = TRUE;
	_handle = ::CreateFileMapping (INVALID_HANDLE_VALUE,	// Not a real file 
								   &security,				// Default security 
								   PAGE_READWRITE,			// Read/write permission 
								   0,						// high-order DWORD of size 
								   _size,					// low-order DWORD of size 
								   _name.c_str ());			// Name of mapping object
	if (_handle == 0)
	{
		if (!quiet)
			throw Win::Exception ("Internal error: Cannot allocate shared memory");
	}
	else
	{
		Map (quiet);
	}
}

void SharedMem::Open (std::string const & name, unsigned int size, bool quiet)
{
	_handle = ::OpenFileMapping (FILE_MAP_ALL_ACCESS,	// Access rigths
								 FALSE,					// Handle will not be inherited
								 name.c_str ());		// Name of mapping object
	if (_handle == 0)
	{
		if (!quiet)
			throw Win::Exception ("Internal error: Cannot open shared memory");
	}
	else
	{
		_name = name;
		_size = size;
		Map (quiet);
	}
}

void SharedMem::Map (unsigned int handle)
{
	_handle = reinterpret_cast<HANDLE> (handle);
	Map ();
}

void SharedMem::Map (bool quiet)
{
	void * p = ::MapViewOfFile (_handle,				// Handle to mapping object
								FILE_MAP_ALL_ACCESS,	// Access mode
								0,						// High-order DWORD of start offset
								0,						// Low-order DWORD of start offset
								0);						// Map all bytes
	if (p == 0)
	{
		if (!quiet)
			throw Win::Exception ("Internal error: Cannot map shared memory");
	}
	else
	{
		_buf = static_cast<char *>(p);
	}
}

void SharedMem::Close ()
{
	if (_buf != 0)
		::UnmapViewOfFile (_buf);
	if (_handle != 0)
		::CloseHandle (_handle);
}
