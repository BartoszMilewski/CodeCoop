//-----------------------------------
// (c) Reliable Software 2004
//-----------------------------------
#include <WinLibBase.h>
#include "File.h"
#include <Sys/SysVer.h>

void File::InitSizer ()
{
	Assert (_hFile != INVALID_HANDLE_VALUE);
	SystemVersion ver;
	if (!ver.IsOK ())
		throw Win::Exception ("Get system version failed");

	// create implementation depending on the operating system
	else if (ver.IsWinNT () && ver.MajorVer () >= 5)
		_sizer.reset (new File::Sizer64 (_hFile));
	else
		_sizer.reset (new File::Sizer32 (_hFile));
}

File::Sizer64::Sizer64 (HANDLE hFile)
	: File::Sizer (hFile), _dll ("Kernel32.dll")
{}

bool File::Sizer64::Rewind ()
{
	typedef BOOL (WINAPI * SetFilePointerExT)
	(
		HANDLE hFile,
		LARGE_INTEGER liDistanceToMove,
		PLARGE_INTEGER lpNewFilePointer,
		DWORD dwMoveMethod
	);
	SetFilePointerExT fun;
	_dll.GetFunction ("SetFilePointerEx", fun);

	File::Size zero (0, 0);
	return (*fun) (_hFile, zero.ToNative (), 0, FILE_BEGIN) != 0;
}

File::Size File::Sizer64::GetSize () const
{
	typedef BOOL (WINAPI * GetFileSizeExT)
	(
		HANDLE hFile,
		PLARGE_INTEGER lpFileSize
	);
	GetFileSizeExT fun;
	_dll.GetFunction ("GetFileSizeEx", fun);

	File::Size size;
	if ((*fun) (_hFile, size.ToPtr ()) == 0)
	{
		throw Win::Exception ("Get file size failed.");
	}
	return size;
}

File::Offset File::Sizer64::GetPosition () const
{
	typedef BOOL (WINAPI * SetFilePointerExT)
	(
		HANDLE hFile,
		LARGE_INTEGER liDistanceToMove,
		PLARGE_INTEGER lpNewFilePointer,
		DWORD dwMoveMethod
	);
	SetFilePointerExT fun;
	_dll.GetFunction ("SetFilePointerEx", fun);

	File::Offset zero (0, 0);
	File::Offset position;
	if ((*fun) (_hFile, zero.ToNative (), position.ToPtr (), FILE_CURRENT) == 0)
	{
		DWORD lastError = ::GetLastError ();
		if (lastError == ERROR_HANDLE_EOF)
		{
			Win::ClearError ();
			position = File::Offset::Invalid;
		}
		else
			throw Win::Exception ("Cannot get file position.");
	}
	return position;
}

File::Offset File::Sizer64::SetPosition (File::Offset pos)
{
	typedef BOOL (WINAPI * SetFilePointerExT)
	(
		HANDLE hFile,
		LARGE_INTEGER liDistanceToMove,
		PLARGE_INTEGER lpNewFilePointer,
		DWORD dwMoveMethod
	);
	SetFilePointerExT fun;
	_dll.GetFunction ("SetFilePointerEx", fun);

	File::Offset newPos;
	if ((*fun) (_hFile, pos.ToNative (), newPos.ToPtr (), FILE_BEGIN) == 0)
	{
		DWORD lastError = ::GetLastError ();
		if (lastError != ERROR_HANDLE_EOF)
			throw Win::Exception ("Cannot set file position.");
		else
			Win::ClearError ();
	}
	return newPos;
}

// 32-bit version temporary stubs

File::Sizer32::Sizer32 (HANDLE hFile)
	: File::Sizer (hFile)
{}

bool File::Sizer32::Rewind ()
{
	long high = 0;
	return ::SetFilePointer (_hFile, 0, &high, FILE_BEGIN) != INVALID_SET_FILE_POINTER;
}

File::Size File::Sizer32::GetSize () const
{
	DWORD high = 0;
	long size = ::GetFileSize (_hFile, &high);
	if (size == INVALID_FILE_SIZE && ::GetLastError() != NO_ERROR)
	{
		throw Win::Exception ("Get file size failed.");
	}
	return File::Size (size, high);
}

File::Offset File::Sizer32::GetPosition () const
{
	long high = 0;
	long low = ::SetFilePointer (_hFile, 0, &high, FILE_CURRENT);
	if (low == INVALID_SET_FILE_POINTER && ::GetLastError() != NO_ERROR)
	{
		DWORD lastError = ::GetLastError ();
		if (lastError == ERROR_HANDLE_EOF)
		{
			Win::ClearError ();
			return File::Offset::Invalid;
		}
		else
			throw Win::Exception ("Cannot get file position.");
	}
	return File::Offset (low, high);
}

File::Offset File::Sizer32::SetPosition (File::Offset pos)
{
	long high = pos.High ();
	long low = ::SetFilePointer (_hFile, pos.Low (), &high, FILE_BEGIN);
	
	if (low == INVALID_SET_FILE_POINTER && ::GetLastError() != NO_ERROR)
	{
		DWORD lastError = ::GetLastError ();
		if (lastError != ERROR_HANDLE_EOF)
			throw Win::Exception ("Cannot set file position.");
		else
			Win::ClearError ();
	}
	return File::Offset (low, high);
}

