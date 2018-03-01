//
// (cbreak; Reliable Software 1998
//

#include "CmcEx.h"

#include <Sys\WinString.h>

CmcException::CmcException (char const * msg, CMC_return_code cmcCode)
	: WinException (msg)
{
	switch (cmcCode)
	{
	case CMC_E_AMBIGUOUS_RECIPIENT:
		strcpy (_objName, "CMC_E_AMBIGUOUS_RECIPIENT");
		break;
	case CMC_E_ATTACHMENT_NOT_FOUND:
		strcpy (_objName, "CMC_E_ATTACHMENT_NOT_FOUND");
		break;
	case CMC_E_ATTACHMENT_OPEN_FAILURE:
		strcpy (_objName, "CMC_E_ATTACHMENT_OPEN_FAILURE");
		break;
	case CMC_E_ATTACHMENT_READ_FAILURE:
		strcpy (_objName, "CMC_E_ATTACHMENT_READ_FAILURE");
		break;
	case CMC_E_ATTACHMENT_WRITE_FAILURE:
		strcpy (_objName, "CMC_E_ATTACHMENT_WRITE_FAILURE");
		break;
	case CMC_E_COUNTED_STRING_UNSUPPORTED:
		strcpy (_objName, "CMC_E_COUNTED_STRING_UNSUPPORTED");
		break;
	case CMC_E_DISK_FULL:
		strcpy (_objName, "CMC_E_DISK_FULL");
		break;
	case CMC_E_FAILURE:
		strcpy (_objName, "CMC_E_FAILURE");
		break;
	case CMC_E_INSUFFICIENT_MEMORY:
		strcpy (_objName, "CMC_E_INSUFFICIENT_MEMORY");
		break;
	case CMC_E_INVALID_CONFIGURATION:
		strcpy (_objName, "CMC_E_INVALID_CONFIGURATION");
		break;
	case CMC_E_INVALID_ENUM:
		strcpy (_objName, "CMC_E_INVALID_ENUM");
		break;
	case CMC_E_INVALID_FLAG:
		strcpy (_objName, "CMC_E_INVALID_FLAG");
		break;
	case CMC_E_INVALID_MEMORY:
		strcpy (_objName, "CMC_E_INVALID_MEMORY");
		break;
	case CMC_E_INVALID_MESSAGE_PARAMETER:
		strcpy (_objName, "CMC_E_INVALID_MESSAGE_PARAMETER");
		break;
	case CMC_E_INVALID_MESSAGE_REFERENCE:
		strcpy (_objName, "CMC_E_INVALID_MESSAGE_REFERENCE");
		break;
	case CMC_E_INVALID_PARAMETER:
		strcpy (_objName, "CMC_E_INVALID_PARAMETER");
		break;
	case CMC_E_INVALID_SESSION_ID:
		strcpy (_objName, "CMC_E_INVALID_SESSION_ID");
		break;
	case CMC_E_INVALID_UI_ID:
		strcpy (_objName, "CMC_E_INVALID_UI_ID");
		break;
	case CMC_E_LOGON_FAILURE:
		strcpy (_objName, "CMC_E_LOGON_FAILURE");
		break;
	case CMC_E_MESSAGE_IN_USE:
		strcpy (_objName, "CMC_E_MESSAGE_IN_USE");
		break;
	case CMC_E_NOT_SUPPORTED:
		strcpy (_objName, "CMC_E_NOT_SUPPORTED");
		break;
	case CMC_E_PASSWORD_REQUIRED:
		strcpy (_objName, "CMC_E_PASSWORD_REQUIRED");
		break;
	case CMC_E_RECIPIENT_NOT_FOUND:
		strcpy (_objName, "CMC_E_RECIPIENT_NOT_FOUND");
		break;
	case CMC_E_SERVICE_UNAVAILABLE:
		strcpy (_objName, "CMC_E_SERVICE_UNAVAILABLE");
		break;
	case CMC_E_TEXT_TOO_LARGE:
		strcpy (_objName, "CMC_E_TEXT_TOO_LARGE");
		break;
	case CMC_E_TOO_MANY_FILES:
		strcpy (_objName, "CMC_E_TOO_MANY_FILES");
		break;
	case CMC_E_TOO_MANY_RECIPIENTS:
		strcpy (_objName, "CMC_E_TOO_MANY_RECIPIENTS");
		break;
	case CMC_E_UNABLE_TO_NOT_MARK_AS_READ:
		strcpy (_objName, "CMC_E_UNABLE_TO_NOT_MARK_AS_READ");
		break;
	case CMC_E_UNRECOGNIZED_MESSAGE_TYPE:
		strcpy (_objName, "CMC_E_UNRECOGNIZED_MESSAGE_TYPE");
		break;
	case CMC_E_UNSUPPORTED_ACTION:
		strcpy (_objName, "CMC_E_UNSUPPORTED_ACTION");
		break;
	case CMC_E_UNSUPPORTED_CHARACTER_SET:
		strcpy (_objName, "CMC_E_UNSUPPORTED_CHARACTER_SET");
		break;
	case CMC_E_UNSUPPORTED_DATA_EXT:
		strcpy (_objName, "CMC_E_UNSUPPORTED_DATA_EXT");
		break;
	case CMC_E_UNSUPPORTED_FLAG:
		strcpy (_objName, "CMC_E_UNSUPPORTED_FLAG");
		break;
	case CMC_E_UNSUPPORTED_FUNCTION_EXT:
		strcpy (_objName, "CMC_E_UNSUPPORTED_FUNCTION_EXT");
		break;
	case CMC_E_UNSUPPORTED_VERSION:
		strcpy (_objName, "CMC_E_UNSUPPORTED_VERSION");
		break;
	case CMC_E_USER_CANCEL:
		strcpy (_objName, "CMC_E_USER_CANCEL");
		break;
	case CMC_E_USER_NOT_LOGGED_ON:
		strcpy (_objName, "CMC_E_USER_NOT_LOGGED_ON");
		break;
	default:
		{
			Msg info;
			info << "(UNKNOWN CMC RETURN CODE: 0x" << std::hex << cmcCode << ")";
			strcpy (_objName, info.c_str ());
		}
		break;
	}
}

