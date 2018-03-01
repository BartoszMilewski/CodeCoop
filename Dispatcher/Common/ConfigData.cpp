// ---------------------------
// (c) Reliable Software, 2002
// ---------------------------
#include "precompiled.h"
#include "ConfigData.h"
#include "AppInfo.h"
#include "OutputSink.h"
#include <StringOp.h>
#include <Net/NetShare.h>

std::unique_ptr<TransportValidator> TheTransportValidator;

bool TransportValidator::Validate (Transport const & transport, Win::Dow::Handle win) const
{
	if (transport.IsUnknown ())
	{
		TheOutput.Display ("Please specify the network path to \nthe hub's Public Inbox share.",
							Out::Information, win);
		return false;
	}

	if (transport.GetMethod () != Transport::Network)
		return true;

	FilePath fwdPath (transport.GetRoute ());
	if (!FilePath::IsAbsolute (fwdPath.GetDir ()))
	{
		TheOutput.Display ("Please specify a full network path to \nthe hub's Public Inbox share.",
							Out::Information, win);
		return false;
	}
	
	if (!FilePath::IsValid (fwdPath.GetDir ()))
	{
		TheOutput.Display ("The network path contains some illegal characters.\n"
							"Please specify a valid path.",
							Out::Information, win);
		return false;
	}
	return true;
}

bool TransportValidator::ValidateRemoteHub (Transport const & transport, Win::Dow::Handle win) const
{
	if (transport.IsUnknown ())
	{
		TheOutput.Display ("Please specify the hub's email address.",
							Out::Information, win);
		return false;
	}
	return true;
}

bool TransportValidator::ValidateExcludePublicInbox (
								Transport const & transport,
								char const * errmsg,
								Win::Dow::Handle win) const
{
	if (!Validate (transport, win))
		return false;

	if (_pi.IsEqualDir (transport.GetRoute ()) ||
		_myTransports.Get (transport.GetMethod ()) == transport)
	{
		TheOutput.Display (errmsg, Out::Information, win);
		return false;
	}

	return true;
}
