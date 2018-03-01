//------------------------------------
// (c) Reliable Software 1997
//------------------------------------
#include <WinLibBase.h>
#include "FileGet.h"
#include <Sys/SysVer.h>

FileGetter::FileGetter ()
{
    // Revisit: fix dynamic allocation
    _bufLen = 256;
    _buf = new char [_bufLen];
    memset (_buf, 0, _bufLen);
    _buf [0] = 0;

	SystemVersion windows;
	if (windows.MajorVer () >= 5)
	    lStructSize = sizeof (OPENFILENAME);
	else
		lStructSize = OPENFILENAME_SIZE_VERSION_400;
    hwndOwner = 0;          // owner window 
    hInstance = 0;          // instance handle 
    lpstrFilter = 0;        // e.g. "Text (*.txt *.me)\0*.txt;*.me\0"
    lpstrCustomFilter = 0; 
    nMaxCustFilter = 0; 
    nFilterIndex = 1;       // which filter to use? First is 1. 
    lpstrFile = _buf;       // buffer with file name and return buffer 
    nMaxFile = _bufLen; 
    lpstrFileTitle = 0; 
    nMaxFileTitle = 0; 
    lpstrInitialDir = 0;    // initial directory path (0 for current dir)
    lpstrTitle = 0; 
    Flags = OFN_EXPLORER; 
    nFileOffset = 0;        // out: offset of file name in buf 
    nFileExtension = 0;     // out: offset of extension
    lpstrDefExt = 0;        // default extension to append
    lCustData = (long)this; // the "this" pointer
    lpfnHook = 0; 
    lpTemplateName = 0; 
}

bool FileGetter::GetExistingFile (Win::Dow::Handle hwnd, char const * title)
{
    hwndOwner = hwnd.ToNative ();
    lpstrTitle = title;
    Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY; 
    if (::GetOpenFileName (this) == FALSE)
    {
		// Check if Cancel or error
		DWORD errCode = ::CommDlgExtendedError ();
		if (errCode != 0)
			throw Win::CommDlgException (errCode, title);
		else
			return false;	// User Canceled dialog
    }
    return true;
}

bool FileGetter::GetNewFile (Win::Dow::Handle hwnd, char const * title)
{
    hwndOwner = hwnd.ToNative ();
    lpstrTitle = title;
    Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY; 
    if (::GetSaveFileName (this) == FALSE)
    {
		// Check if Cancel or error
		DWORD errCode = ::CommDlgExtendedError ();
		if (errCode != 0)
			throw Win::CommDlgException (errCode, title);
		else
			return false;	// User Canceled dialog
    }
    return true;
}

UINT msgFileOk = 0;

bool FileGetter::GetMultipleFiles (Win::Dow::Handle hwnd, char const * title)
{
    if (msgFileOk == 0)
        msgFileOk = ::RegisterWindowMessage (FILEOKSTRING);

    hwndOwner = hwnd.ToNative ();
    lpstrTitle = title;
    Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT; 
    if (::GetOpenFileName (this) == FALSE)
    {
		// Check if Cancel or error
		DWORD errCode = ::CommDlgExtendedError ();
		if (errCode != 0)
			throw Win::CommDlgException (errCode, title);
		else
			return false;	// User Canceled dialog
    }
    return true;
}

PathIter::PathIter (FileGetter const & getter)
: _buf (getter.GetPath ()), _iName (0)
{
    Advance ();
    if (AtEnd () && _iName != 0)
    {
        // There was only single selection
        _iPath = _iName;
        _iName = 0;
    }
    else
    {
        _iPath = 0;
    }
}

void PathIter::Advance ()
{
    while (_buf [_iName] != 0)
        _iName++;
    _iName++;
}

char const * PathIter::GetPath (char * bufPath, int size)
{
    int lenPath = strlen (&_buf [_iPath]);
    if (lenPath + 1 + (int)strlen (&_buf [_iName]) + 1 > size)
        return 0;

    strcpy (bufPath, &_buf [_iPath]);
    if (lenPath > 0 && bufPath [lenPath-1] != '\\')
    {
        bufPath [lenPath] = '\\';
        lenPath++;
    }
    strcpy (bufPath + lenPath, &_buf [_iName]);
    return bufPath;
}

