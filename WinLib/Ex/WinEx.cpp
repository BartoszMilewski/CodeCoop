//------------------------------------
// (c) Reliable Software 1997 -- 2007
//------------------------------------

#include <WinLibBase.h>
#include "WinEx.h"
#include "Error.h"
#include <sstream>

using namespace Win;

Exception::Exception (char const * msg, char const * objName)
    : _msg (msg), 
	  _hModule (0)
{
	_err = ::GetLastError ();
	::SetLastError (0);
	InitObjName (objName);
}

Exception::Exception (char const * msg, char const * objName, DWORD err, HINSTANCE hModule)
    : _msg (msg), 
	  _err (err),
	  _hModule (hModule)
{
	::SetLastError (0);
	InitObjName (objName);
}

void Exception::InitObjName (char const * objName)
{
	if (objName != 0)
	{
		unsigned len = std::min(strlen(objName), sizeof(_objName) - 1);
		if (len != 0)
			strncpy (_objName, objName, len);
		_objName [len] = '\0';
	}
	else
		_objName [0] = '\0';
}

CommDlgException::CommDlgException (DWORD commonDlgExtendedErr, char const * objName)
	: Exception (0, objName)
{
    SetLastError (0);
	if (objName != 0)
	{
		strncpy (_objName, objName, sizeof (_objName) - 1);
		_objName [sizeof (_objName) - 1] = '\0';
	}
	else
		_objName [0] = '\0';

	switch (commonDlgExtendedErr)
	{
		// General common dialog errors

	case CDERR_DIALOGFAILURE:
		_msg = "The dialog box could not be created.";
		break;
	case CDERR_FINDRESFAILURE:
		_msg = "The common dialog box function failed to find a specified resource.";
		break;
	case CDERR_INITIALIZATION:
		_msg = "The common dialog box function failed during initialization.";
		break;
	case CDERR_LOADRESFAILURE:
		_msg = "The common dialog box function failed to load a specified resource.";
		break;
	case CDERR_LOADSTRFAILURE:
		_msg = "The common dialog box function failed to load a specified string.";
		break;
	case CDERR_LOCKRESFAILURE:
		_msg = "The common dialog box function failed to lock a specified resource.";
		break;
	case CDERR_MEMALLOCFAILURE:
		_msg = "The common dialog box function was unable to allocate memory for internal structures.";
		break;
	case CDERR_MEMLOCKFAILURE:
		_msg = "The common dialog box function was unable to lock the memory associated with a handle.";
		break;
	case CDERR_NOHINSTANCE:
		_msg = "The ENABLETEMPLATE flag was set in the Flags member of the initialization structure\n"
			   "for the corresponding common dialog box, but you failed to provide a corresponding instance handle.";
		break;
	case CDERR_NOHOOK:
		_msg = "The ENABLEHOOK flag was set in the Flags member of the initialization structure\n"
			   "for the corresponding common dialog box, but you failed to provide a pointer to a corresponding hook procedure.";
		break;
	case CDERR_NOTEMPLATE:
		_msg = "The ENABLETEMPLATE flag was set in the Flags member of the initialization structure\n"
			   "for the corresponding common dialog box, but you failed to provide a corresponding template.";
		break;
	case CDERR_REGISTERMSGFAIL:
		_msg = "The RegisterWindowMessage function returned an error code when it was called by the common dialog box function.";
		break;
	case CDERR_STRUCTSIZE:
		_msg = "The lStructSize member of the initialization structure for the corresponding common dialog box is invalid.";
		break;

		// The following error codes can be returned for the PrintDlg function:

	case PDERR_CREATEICFAILURE:
		_msg = "The PrintDlg function failed when it attempted to create an information context.";
		break;
	case PDERR_DEFAULTDIFFERENT:
		_msg = "You called the PrintDlg function with the DN_DEFAULTPRN flag specified in the wDefault\n"
			   "member of the DEVNAMES structure, but the printer described by the other structure members\n"
			   "did not match the current default printer. (This error occurs when you store the DEVNAMES\n"
			   "structure and the user changes the default printer by using the Control Panel.)\n"
			   "To use the printer described by the DEVNAMES structure, clear the DN_DEFAULTPRN flag\n"
			   "and call PrintDlg again. To use the default printer, replace the DEVNAMES structure\n"
			   "(and the DEVMODE structure, if one exists) with NULL; and call PrintDlg again.";
		break;
	case PDERR_DNDMMISMATCH:
		_msg = "The data in the DEVMODE and DEVNAMES structures describes two different printers.";
		break;
	case PDERR_GETDEVMODEFAIL:
		_msg = "The printer driver failed to initialize a DEVMODE structure.";
		break;
	case PDERR_INITFAILURE:
		_msg = "The PrintDlg function failed during initialization\n"
			   "and there is no more specific extended error code to describe the failure.";
		break;
	case PDERR_LOADDRVFAILURE:
		_msg = "The PrintDlg function failed to load the device driver for the specified printer.";
		break;
	case PDERR_NODEFAULTPRN:
		_msg = "A default printer does not exist.";
		break;
	case PDERR_NODEVICES:
		_msg = "No printer drivers were found.";
		break;
	case PDERR_PARSEFAILURE:
		_msg = "The PrintDlg function failed to parse the strings in the [devices] section of the WIN.INI file.";
		break;
	case PDERR_PRINTERNOTFOUND:
		_msg = "The [devices] section of the WIN.INI file did not contain an entry for the requested printer.";
		break;
	case PDERR_RETDEFFAILURE:
		_msg = "The PD_RETURNDEFAULT flag was specified in the Flags member of the PRINTDLG structure,\n"
			   "but the hDevMode or hDevNames member was not NULL.";
		break;
	case PDERR_SETUPFAILURE:
		_msg = "The PrintDlg function failed to load the required resources.";
		break;

		// The following error codes can be returned for the ChooseFont function:

	case CFERR_MAXLESSTHANMIN:
		_msg = "The size specified in the nSizeMax member of the CHOOSEFONT structure is less\n"
			   "than the size specified in the nSizeMin member.";
		break;
	case CFERR_NOFONTS:
		_msg = "No fonts exist.";
		break;

		// The following error codes can be returned for the GetOpenFileName and GetSaveFileName functions:

	case FNERR_BUFFERTOOSMALL:
		_msg = "The buffer pointed to by the lpstrFile member of the OPENFILENAME structure is\n"
			   "too small for the filename specified by the user.";
		break;
	case FNERR_INVALIDFILENAME:
		_msg = "A filename is invalid.";
		break;
	case FNERR_SUBCLASSFAILURE:
		_msg = "An attempt to subclass a list box failed because sufficient memory was not available.";
		break;

		// The following error code can be returned for the FindText and ReplaceText functions:

	case FRERR_BUFFERLENGTHZERO:
		_msg = "A member of the FINDREPLACE structure points to an invalid buffer.";
		break;
	
	default:
		_msg = "Unknown common dialog error.";
		break;
	}
}

void ThrowAssumption (char const * file, unsigned line, char const * msg, char const * info)
{
	int const bufSize = 512;
	static char buf [bufSize + 1];
	std::ostringstream out;
	out << "Internal error in " << file << ", line " << line << "\n";
	out << msg;
	strncpy (buf, out.str ().c_str (), bufSize);
	throw Win::InternalException (buf, info);
}
