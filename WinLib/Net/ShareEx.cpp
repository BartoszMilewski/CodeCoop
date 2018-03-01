//---------------------------
// (c) Reliable Software 2000
//---------------------------
#include <WinLibBase.h>
#include "ShareEx.h"
#include <lmerr.h>
#include <WinError.h>

using namespace Net;

char const * ShareException::GetReason () const
{
	char const * reason = 0;

	switch (_errCode)
	{
	case ERROR_ACCESS_DENIED:
		reason = "The user does not have access to the requested information";
		break;
	case ERROR_NOT_ENOUGH_MEMORY:
		reason = "Insufficient memory is available";
		break;
	case ERROR_INVALID_LEVEL:
		reason = "The system call level is not correct";
		break;
	case ERROR_INVALID_NAME:
		reason = "The filename, directory name, or volume label syntax is incorrect";
		break;
	case ERROR_INVALID_PARAMETER:
		reason = "The specified parameter is invalid";
		break;
	case ERROR_FILE_NOT_FOUND:
		reason = "The specified directory not found";
		break;
	case ERROR_DIRECTORY:
		reason = "The directory name is invalid";
		break;
	case NERR_NetNotStarted:	// 2102
		reason = "The workstation driver is not installed";
		break;
	case NERR_ServerNotStarted:	// 2114
		reason = "The Server service is not started";
		break;
	case NERR_UnknownDevDir:	// 2116
		reason = "The device or directory does not exist";
		break;
	case NERR_RedirectedPath:	// 2117
		reason = "The operation is invalid for a redirected resource. " \
				 "The specified device name is assigned to a shared resource";
		break;
	case NERR_DuplicateShare:	// 2118
		reason = "The sharename is already in use on this server";
		break;
	case NERR_BufTooSmall:		// 2123
		reason = "The API return buffer is too small";
		break;
	case NERR_NetNameNotFound:	// 2310
		reason = "The sharename does not exist";
		break;
	case NERR_InvalidComputer:	// 2351
		reason = "This computer name is invalid";
		break;
	case NERR_ShareNotFound:	// 2392
		reason = "The shared resource you are connected to could not be found";
		break;
	default:
		// reason = 0;
		break;
	}
	return reason;
}

