//--------------------------------------
//  (c) Reliable Software, 1999 -- 2005
//--------------------------------------

#include "precompiled.h"
#include "FileTypes.h"

#include <Ex/WinEx.h>
#include <Dbg/Assert.h>

char const * FileType::GetName () const
{
	if (!IsRecoverable ())
	{
		Assert (!IsFolder () && !IsRoot ()); 
		if (IsHeader ())
			return "Header (unrecoverable)";
		else if (IsSource ())
			return "Source (unrecoverable)";
		else if (IsText ())
			return "Text (unrecoverable)";
		else if (IsBinary ())
			return "Binary (unrecoverable)";
		else if (IsInvalid ())
			return "Invalid (unrecoverable)";
		else
			return "?";
	}
	else
	{
		if (IsHeader ())
			return "Header";
		else if (IsSource ())
			return "Source";
		else if (IsText ())
			return "Text";
		else if (IsBinary ())
			return "Binary";
		else if (IsFolder ())
			return "Folder";
		else if (IsInvalid ())
			return "Invalid";
		else if (IsRoot ())
			return "Project Root Folder";
		else
			return "?";
	}
}

bool FileType::Verify () const
{
	switch (_bits._T)
	{
	case typeHeader:
	case typeSource:
	case typeText:
	case typeBinary:
	case typeFolder:
	case typeRoot:
	case typeWiki:
	case typeInvalid:
		break;
	default:
		return false;
	}
	return true;
}
