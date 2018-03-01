//
// (c) Reliable Software 1998, 99, 2000, 01
//

#include "precompiled.h"
#include "SimpleMapiEx.h"

#include <Sys/WinString.h>

#include <mapi.h>
#include <mapicode.h>

using namespace SimpleMapi;

Exception::Exception (char const * msg, HRESULT hRes, char const * obj)
	: Win::Exception (msg)
{
	_err = 0;
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

	// Errors from MAPI.H

	case MAPI_USER_ABORT:
		strcpy (_objName, "User aborted operation.");
		break;
	case MAPI_E_FAILURE:
		strcpy (_objName, "One or more unspecified errors occurred while working with MAPI");
		break;
	case MAPI_E_LOGON_FAILURE:
		strcpy (_objName, "There was no default logon, and the user failed to log on successfully\r\n"
						  "when the logon dialog box was displayed.");
		break;
	case MAPI_E_DISK_FULL:
		strcpy (_objName, "An attachment could not be written to a temporary file because\r\n"
						  "there was not enough space on the disk.");
		break;
	case MAPI_E_INSUFFICIENT_MEMORY:
		strcpy (_objName, "There was insufficient memory to procced with MAPI operation.");
		break;
	case MAPI_E_ACCESS_DENIED:
		strcpy (_objName, "Acess denied");
		break;
	case MAPI_E_TOO_MANY_SESSIONS:
		strcpy (_objName, "There are too many MAPI sessions open simultaneously.");
		break;
	case MAPI_E_TOO_MANY_FILES:
		strcpy (_objName, "There were too many file attachments in the message.");
		break;
	case MAPI_E_TOO_MANY_RECIPIENTS:
		strcpy (_objName, "There were too many recipients of the message.");
		break;
	case MAPI_E_ATTACHMENT_NOT_FOUND:
		strcpy (_objName, "The specified attachment was not found.");
		break;
	case MAPI_E_ATTACHMENT_OPEN_FAILURE:
		strcpy (_objName, "The specified attachment could not be opened.");
		break;
	case MAPI_E_ATTACHMENT_WRITE_FAILURE:
		strcpy (_objName, "Could not write to the message attachement.");
		break;
	case MAPI_E_UNKNOWN_RECIPIENT:
		strcpy (_objName, "A recipient did not appear in the address list.");
		break;
	case MAPI_E_BAD_RECIPTYPE:
		strcpy (_objName, "The type of a recipient was not TO, CC, or BCC.");
		break;
	case MAPI_E_NO_MESSAGES:
		strcpy (_objName, "No messages with specified attributes.");
		break;
	case MAPI_E_INVALID_MESSAGE:
		strcpy (_objName, "An invalid message identifier was passed to the MAPI.");
		break;
	case MAPI_E_TEXT_TOO_LARGE:
		strcpy (_objName, "The text in the message was too large.");
		break;
	case MAPI_E_INVALID_SESSION:
		strcpy (_objName, "An invalid session handle was passed to the MAPI.");
		break;
	case MAPI_E_TYPE_NOT_SUPPORTED:
		strcpy (_objName, "Type not supported.");
		break;
	case MAPI_E_AMBIGUOUS_RECIPIENT:
		strcpy (_objName, "A recipient was specified ambiguously.");
		break;
	case MAPI_E_MESSAGE_IN_USE:
		strcpy (_objName, "Message in use.");
		break;
	case MAPI_E_NETWORK_FAILURE:
		strcpy (_objName, "Network failure.");
		break;
	case MAPI_E_INVALID_EDITFIELDS:
		strcpy (_objName, "Invalid edit fields.");
		break;
	case MAPI_E_INVALID_RECIPS:
		strcpy (_objName, "One or more recipients were invalid or did not resolve to any address.");
		break;
	case MAPI_E_NOT_SUPPORTED:
		strcpy (_objName, "The operation was not supported by the underlying messaging system.");
		break;

	default:
		_err = hRes;
		break;
	}

	if (obj != 0)
	{
		size_t len = strlen (_objName);
		if (len > MAX_PATH - 2)
			return;
		_objName [len] = '\n';
		++len;
		_objName [len] = '"';
		++len;
		_objName [len] = '\0';
		strncpy (_objName + len, obj, MAX_PATH - len);
		len = strlen (_objName);
		if (len > MAX_PATH - 1)
			return;
		_objName [len] = '"';
		++len;
		_objName [len] = '\0';
	}
}
