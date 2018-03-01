//----------------------------
// (c) Reliable Software 1999
//----------------------------
#include "precompiled.h"
#include "ScriptStatus.h"
#include <Dbg/Assert.h>

char const * ScriptStatus::LongDescription () const
{
	Read::Bits r = GetReadStatus ();
	if (r != Read::Ok)
	{
		switch (r)
		{
		case Read::Ok:
			return "Ok";
			break;
		case Read::Absent:
			return "Script was deleted";
			break;
		case Read::NoHeader:
			return "Missing script header";
			break;
		case Read::AccessDenied:
			return "Access denied";
			break;
		case Read::Corrupted:
			return "Corrupted script file";
			break;
		default:
			Assert (!"Unknown status");
		}
	}
	switch (GetDispatchStatus ())
	{
	case Dispatch::ToBeDone:
		return "Waiting to be processed";
		break;
	case Dispatch::InProgress:
		return "In process of being dispatched";
		break;
	case Dispatch::NoNetwork:
		return "Network path not accessible";
		break;
	case Dispatch::NoEmail:
		return "Waiting to be emailed";
		break;
	case Dispatch::ServerTimeout:
		return "E-mail server timed out";
		break;
	case Dispatch::NoDisk:
		return "Disk error";
		break;
	case Dispatch::NoTmpCopy:
		return "Out of disk space to create temporary copy";
		break;
	case Dispatch::Ignored:
		return "Ignored: requires user action";
		break;
	default:
		Assert (!"Unknown status");
	}
	return "Error";
}

char const * ScriptStatus::ShortDescription () const
{
	Read::Bits r = GetReadStatus ();
	if (r != Read::Ok)
	{
		switch (r)
		{
		case Read::Ok:
			return "Ok";
			break;
		case Read::Absent:
			return "Deleted";
			break;
		case Read::NoHeader:
			return "No header";
			break;
		case Read::AccessDenied:
			return "Access";
			break;
		case Read::Corrupted:
			return "Corrupted";
			break;
		default:
			Assert (!"Unknown status");
		}
	}
	switch (GetDispatchStatus ())
	{
	case Dispatch::ToBeDone:
		return "Waiting";
		break;
	case Dispatch::InProgress:
		return "In Progress";
		break;
	case Dispatch::NoNetwork:
		return "Network";
		break;
	case Dispatch::NoEmail:
		return "Email";
		break;
	case Dispatch::ServerTimeout:
		return "Timeout";
		break;
	case Dispatch::NoDisk:
		return "Disk";
		break;
	case Dispatch::NoTmpCopy:
		return "Disk Space";
		break;
	case Dispatch::Ignored:
		return "Ignored";
		break;
	default:
		Assert (!"Unknown status");
	}
	return "Error";
}

char const * ScriptStatus::GetDirectionStr () const
{
	Read::Bits r = GetReadStatus ();
	if (r != Read::Ok)
		return "Unknown";

	switch (GetDirection ())
	{
	case Direction::Unknown:
		return "Unknown";
		break;
	case Direction::In:
		return "In";
		break;
	case Direction::Out:
		return "Out";
		break;
	default:
		Assert (!"Unknown direction");
		return "Error";
	};
}
