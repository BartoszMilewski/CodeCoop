//
// (c) Reliable Software 1998
//

#include "MapiEx.h"
#include "MapiBuffer.h"

#include <Sys/WinString.h>
#include <LightString.h>

#include <mapix.h>

MapiException::MapiException (char const * msg, HRESULT hRes, char const * obj)
: Win::Exception (msg)
{
	switch (hRes)
	{
	/* General errors (used by more than one MAPI object) */

	case MAPI_E_CALL_FAILED:
		strcpy (_objName, "MAPI_E_CALL_FAILED");
		break;
	case MAPI_E_NOT_ENOUGH_MEMORY:
		strcpy (_objName, "MAPI_E_NOT_ENOUGH_MEMORY");
		break;
	case MAPI_E_INVALID_PARAMETER:
		strcpy (_objName, "MAPI_E_INVALID_PARAMETER");
		break;
	case MAPI_E_INTERFACE_NOT_SUPPORTED:
		strcpy (_objName, "MAPI_E_INTERFACE_NOT_SUPPORTED");
		break;
	case MAPI_E_NO_ACCESS:
		strcpy (_objName, "MAPI_E_NO_ACCESS");
		break;
	case MAPI_E_NO_SUPPORT:
		strcpy (_objName, "MAPI_E_NO_SUPPORT");
		break;
	case MAPI_E_BAD_CHARWIDTH:
		strcpy (_objName, "MAPI_E_BAD_CHARWIDTH");
		break;
	case MAPI_E_STRING_TOO_LONG:
		strcpy (_objName, "MAPI_E_STRING_TOO_LONG");
		break;
	case MAPI_E_UNKNOWN_FLAGS:
		strcpy (_objName, "MAPI_E_UNKNOWN_FLAGS");
		break;
	case MAPI_E_INVALID_ENTRYID:
		strcpy (_objName, "MAPI_E_INVALID_ENTRYID");
		break;
	case MAPI_E_INVALID_OBJECT:
		strcpy (_objName, "MAPI_E_INVALID_OBJECT");
		break;
	case MAPI_E_OBJECT_CHANGED:
		strcpy (_objName, "MAPI_E_OBJECT_CHANGED");
		break;
	case MAPI_E_OBJECT_DELETED:
		strcpy (_objName, "MAPI_E_OBJECT_DELETED");
		break;
	case MAPI_E_BUSY:
		strcpy (_objName, "MAPI_E_BUSY");
		break;
	case MAPI_E_NOT_ENOUGH_DISK:
		strcpy (_objName, "MAPI_E_NOT_ENOUGH_DISK");
		break;
	case MAPI_E_NOT_ENOUGH_RESOURCES:
		strcpy (_objName, "MAPI_E_NOT_ENOUGH_RESOURCES");
		break;
	case MAPI_E_NOT_FOUND:
		strcpy (_objName, "MAPI_E_NOT_FOUND");
		break;
	case MAPI_E_VERSION:
		strcpy (_objName, "MAPI_E_VERSION");
		break;
	case MAPI_E_LOGON_FAILED:
		strcpy (_objName, "MAPI_E_LOGON_FAILED");
		break;
	case MAPI_E_SESSION_LIMIT:
		strcpy (_objName, "MAPI_E_SESSION_LIMIT");
		break;
	case MAPI_E_USER_CANCEL:
		strcpy (_objName, "MAPI_E_USER_CANCEL");
		break;
	case MAPI_E_UNABLE_TO_ABORT:
		strcpy (_objName, "MAPI_E_UNABLE_TO_ABORT");
		break;
	case MAPI_E_NETWORK_ERROR:
		strcpy (_objName, "MAPI_E_NETWORK_ERROR");
		break;
	case MAPI_E_DISK_ERROR:
		strcpy (_objName, "MAPI_E_DISK_ERROR");
		break;
	case MAPI_E_TOO_COMPLEX:
		strcpy (_objName, "MAPI_E_TOO_COMPLEX");
		break;
	case MAPI_E_BAD_COLUMN:
		strcpy (_objName, "MAPI_E_BAD_COLUMN");
		break;
	case MAPI_E_EXTENDED_ERROR:
		if (obj == 0)
		{
			strcpy (_objName, "MAPI_E_EXTENDED_ERROR -- no extended error information available");
		}
		else
		{
			strcpy (_objName, obj);
		}
		break;
	case MAPI_E_COMPUTED:
		strcpy (_objName, "MAPI_E_COMPUTED");
		break;
	case MAPI_E_CORRUPT_DATA:
		strcpy (_objName, "MAPI_E_CORRUPT_DATA");
		break;
	case MAPI_E_UNCONFIGURED:
		strcpy (_objName, "MAPI_E_UNCONFIGURED");
		break;
	case MAPI_E_FAILONEPROVIDER:
		strcpy (_objName, "MAPI_E_FAILONEPROVIDER");
		break;
	case MAPI_E_UNKNOWN_CPID:
		strcpy (_objName, "MAPI_E_UNKNOWN_CPID");
		break;
	case MAPI_E_UNKNOWN_LCID:
		strcpy (_objName, "MAPI_E_UNKNOWN_LCID");
		break;
	/* Flavors of E_ACCESSDENIED, used at logon */

	case MAPI_E_PASSWORD_CHANGE_REQUIRED:
		strcpy (_objName, "MAPI_E_PASSWORD_CHANGE_REQUIRED");
		break;
	case MAPI_E_PASSWORD_EXPIRED:
		strcpy (_objName, "MAPI_E_PASSWORD_EXPIRED");
		break;
	case MAPI_E_INVALID_WORKSTATION_ACCOUNT:
		strcpy (_objName, "MAPI_E_INVALID_WORKSTATION_ACCOUNT");
		break;
	case MAPI_E_INVALID_ACCESS_TIME:
		strcpy (_objName, "MAPI_E_INVALID_ACCESS_TIME");
		break;
	case MAPI_E_ACCOUNT_DISABLED:
		strcpy (_objName, "MAPI_E_ACCOUNT_DISABLED");
		break;

	/* MAPI base function and status object specific errors and warnings */

	case MAPI_E_END_OF_SESSION:
		strcpy (_objName, "MAPI_E_END_OF_SESSION");
		break;
	case MAPI_E_UNKNOWN_ENTRYID:
		strcpy (_objName, "MAPI_E_UNKNOWN_ENTRYID");
		break;
	case MAPI_E_MISSING_REQUIRED_COLUMN:
		strcpy (_objName, "MAPI_E_MISSING_REQUIRED_COLUMN");
		break;
	case MAPI_W_NO_SERVICE:
		strcpy (_objName, "MAPI_W_NO_SERVICE");
		break;

	/* Property specific errors and warnings */

	case MAPI_E_BAD_VALUE:
		strcpy (_objName, "MAPI_E_BAD_VALUE");
		break;
	case MAPI_E_INVALID_TYPE:
		strcpy (_objName, "MAPI_E_INVALID_TYPE");
		break;
	case MAPI_E_TYPE_NO_SUPPORT:
		strcpy (_objName, "MAPI_E_TYPE_NO_SUPPORT");
		break;
	case MAPI_E_UNEXPECTED_TYPE:
		strcpy (_objName, "MAPI_E_UNEXPECTED_TYPE");
		break;
	case MAPI_E_TOO_BIG:
		strcpy (_objName, "MAPI_E_TOO_BIG");
		break;
	case MAPI_E_DECLINE_COPY:
		strcpy (_objName, "MAPI_E_DECLINE_COPY");
		break;
	case MAPI_E_UNEXPECTED_ID:
		strcpy (_objName, "MAPI_E_UNEXPECTED_ID");
		break;
	case MAPI_W_ERRORS_RETURNED:
		strcpy (_objName, "MAPI_W_ERRORS_RETURNED");
		break;

	/* Table specific errors and warnings */

	case MAPI_E_UNABLE_TO_COMPLETE:
		strcpy (_objName, "MAPI_E_UNABLE_TO_COMPLETE");
		break;
	case MAPI_E_TIMEOUT:
		strcpy (_objName, "MAPI_E_TIMEOUT");
		break;
	case MAPI_E_TABLE_EMPTY:
		strcpy (_objName, "MAPI_E_TABLE_EMPTY");
		break;
	case MAPI_E_TABLE_TOO_BIG:
		strcpy (_objName, "MAPI_E_TABLE_TOO_BIG");
		break;

	case MAPI_E_INVALID_BOOKMARK:
		strcpy (_objName, "MAPI_E_INVALID_BOOKMARK");
		break;

	case MAPI_W_POSITION_CHANGED:
		strcpy (_objName, "MAPI_W_POSITION_CHANGED");
		break;
	case MAPI_W_APPROX_COUNT:
		strcpy (_objName, "MAPI_W_APPROX_COUNT");
		break;

	/* Transport specific errors and warnings */

	case MAPI_E_WAIT:
		strcpy (_objName, "MAPI_E_WAIT");
		break;
	case MAPI_E_CANCEL:
		strcpy (_objName, "MAPI_E_CANCEL");
		break;
	case MAPI_E_NOT_ME:
		strcpy (_objName, "MAPI_E_NOT_ME");
		break;

	case MAPI_W_CANCEL_MESSAGE:
		strcpy (_objName, "MAPI_W_CANCEL_MESSAGE");
		break;

	/* Message Store, Folder, and Message specific errors and warnings */

	case MAPI_E_CORRUPT_STORE:
		strcpy (_objName, "MAPI_E_CORRUPT_STORE");
		break;
	case MAPI_E_NOT_IN_QUEUE:
		strcpy (_objName, "MAPI_E_NOT_IN_QUEUE");
		break;
	case MAPI_E_NO_SUPPRESS:
		strcpy (_objName, "MAPI_E_NO_SUPPRESS");
		break;
	case MAPI_E_COLLISION:
		strcpy (_objName, "MAPI_E_COLLISION");
		break;
	case MAPI_E_NOT_INITIALIZED:
		strcpy (_objName, "MAPI_E_NOT_INITIALIZED");
		break;
	case MAPI_E_NON_STANDARD:
		strcpy (_objName, "MAPI_E_NON_STANDARD");
		break;
	case MAPI_E_NO_RECIPIENTS:
		strcpy (_objName, "MAPI_E_NO_RECIPIENTS");
		break;
	case MAPI_E_SUBMITTED:
		strcpy (_objName, "MAPI_E_SUBMITTED");
		break;
	case MAPI_E_HAS_FOLDERS:
		strcpy (_objName, "MAPI_E_HAS_FOLDERS");
		break;
	case MAPI_E_HAS_MESSAGES:
		strcpy (_objName, "MAPI_E_HAS_MESSAGES");
		break;
	case MAPI_E_FOLDER_CYCLE:
		strcpy (_objName, "MAPI_E_FOLDER_CYCLE");
		break;

	case MAPI_W_PARTIAL_COMPLETION:
		strcpy (_objName, "MAPI_W_PARTIAL_COMPLETION");
		break;

	/* Address Book specific errors and warnings */

	case MAPI_E_AMBIGUOUS_RECIP:
		strcpy (_objName, "MAPI_E_AMBIGUOUS_RECIP");
		break;
	default:
		{
			Msg info;
			info << "(HRESULT code: 0x" << std::hex << hRes << ")";
			strcpy (_objName, info.c_str ());
		}
		break;
	}
}

char const * MapiExtendedError (HRESULT hRes, struct IMAPIProp * pIf)
{
	MapiBuffer<MAPIERROR> extMapiErr;
	HRESULT hr = pIf->GetLastError (hRes, 0, extMapiErr.GetFillBuf ());
	if (FAILED (hr))
	{
		return "MAPI_E_EXTENDED_ERROR -- Failed to get extended error information";
	}
	else
	{
		Msg info;
		info << "MAPI_E_EXTENDED_ERROR -- Extended error: ";
		if (extMapiErr->lpszError != 0)
			info <<  extMapiErr->lpszError;
		else
			info << "not specified";
		info << "\nComponent: ";
		if (extMapiErr->lpszComponent != 0)
			info <<  extMapiErr->lpszComponent;
		else
			info <<  "not specified";
		info << "\nLow level error code: " << std::hex << extMapiErr->ulLowLevelError;
		info << "\nContext code: " << std::hex << extMapiErr->ulContext;
		return info.c_str ();
	}
}
