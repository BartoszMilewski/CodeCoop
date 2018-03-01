//---------------------------------------------
// (c) Reliable Software 2005
//---------------------------------------------

#include <WinLibBase.h>
#include "Monitor.h"

#include <Sys/Process.h>
#include <Win/Message.h>

#include <iomanip>
#include <sstream>

using namespace Dbg;

Monitor::Msg::Msg (std::string const & str)
	: _msg (str)
{
	if (!IsCurrentVersion ())
	{
		if (_msg.length () <= 2)
		{
			// Bogus log message
			_msg.clear ();
			return;
		}
		// Try unpack previous formatting
		std::string tmp ("1");
		if (_msg [0] == 1)
		{
			if (_msg [1] == 0)
				tmp += 'I';
			else
				tmp += 'H';
			//os << (unsigned char) 1;
			//os << (unsigned char) _kind;
			//os << _component << ':';
			//os << _threadId << ':';
			//os << _time;
			//os << "::";
			std::string sourceId;
			std::string threadId;
			std::string timeStamp;
			std::string info;
			unsigned int start = 2;
			unsigned int pos = _msg.find (':', start);
			if (pos != std::string::npos)
			{
				sourceId = _msg.substr (start, pos - start);
				start = pos + 1;
			}
			pos = _msg.find (':', start);
			if (pos != std::string::npos)
			{
				threadId = _msg.substr (start, pos - start);
				start = pos + 1;
			}
			pos = _msg.find ("::", start);
			if (pos != std::string::npos)
			{
				timeStamp = _msg.substr (start, pos - start);
				start = pos + 2;
			}
			info = _msg.substr (start, _msg.length () - start);
			tmp += timeStamp;
			tmp += ' ';
			tmp += sourceId;
			tmp += " (";
			tmp += threadId;
			tmp += ") ";
			tmp += info;
			_msg = tmp;
		}
	}
}

const char Monitor::CLASS_NAME [] = "Reliable Software Debug Output Monitor";
// Display line in the debug output monitor
// wParam = line length; lParam = pointer to the line
const char Monitor::UM_PUT_LINE [] = "UM_PUT_LINE";
const char Monitor::UM_DBG_MON_START [] = "UM_DBG_MON_START";
const char Monitor::UM_DBG_MON_STOP [] = "UM_DBG_MON_STOP";

Monitor::Monitor ()
{
	FindMonitorWindow ();
}

Monitor::Monitor (std::string const & sourceId)
	: _sourceId (sourceId)
{
	FindMonitorWindow ();
}

void Monitor::Attach (std::string const & sourceId)
{
	_sourceId = sourceId;

	if (_monitorWin.IsNull ())
		FindMonitorWindow ();

	if (!_monitorWin.IsNull ())
	{
		AttachMsg attach (_sourceId);
		Send (attach);
	}
}

void Monitor::Detach ()
{
	if (!_monitorWin.IsNull ())
	{
		DetachMsg detach (_sourceId);
		Send (detach);
	}
}

void Monitor::Write (std::string const & msg) const
{
	if (!_monitorWin.IsNull () && !msg.empty ())
	{
		InfoMsg info (_sourceId, msg);
		Send (info);
	}
}

void Monitor::Write (Dbg::Monitor::Msg const & msg) const
{
	if (!_monitorWin.IsNull ())
		Send (msg);
}

void Monitor::FindMonitorWindow ()
{
	Win::ProcessProxy monitorProxy (CLASS_NAME);
	_monitorWin = monitorProxy.GetWin ();
}

void Monitor::Send (Dbg::Monitor::Msg const & msg) const
{
	Assert (!_monitorWin.IsNull ());
	Win::RegisteredMessage winMsg (UM_PUT_LINE);
	winMsg.SetWParam (msg.length ());
	winMsg.SetLParam (msg.Get ());
	_monitorWin.SendInterprocessPackage (winMsg);
}

Monitor::Msg::Msg (std::string const & sourceId, char type)
	: _msg ("2")
{
	// Debug message header format:
	//
	// Character position
	//            1         2 
	//  01234567890123456789012
	// '1I08:19:13:421 (0x0d7c) Server Code Co-op: '
	//
	// Position 0 - message format version
	// Position 1 - message type - I - information; H - header
	// Position 16 - start of thread id in hex (format version 2)
	_msg += type;
	std::ostringstream out;
	SYSTEMTIME now;
	::GetSystemTime (&now);
	out << std::setw (2) << std::setfill ('0') << now.wHour << ':';
	out << std::setw (2) << std::setfill ('0') << now.wMinute << ':';
	out << std::setw (2) << std::setfill ('0') << now.wSecond << ':';
	out << std::setw (3) << std::setfill ('0') << now.wMilliseconds;
	unsigned long threadId = ::GetCurrentThreadId ();
	out << " (0x" << std::setw (4) << std::setfill ('0') << std::hex << threadId << ") ";
	out << sourceId << ": ";
	_msg += out.str ();
}

std::string Monitor::Msg::GetThreadId () const
{
	if (length () > 2)
	{
		if (_msg [0] == '2')
			return _msg.substr (16, 6);
		else if (_msg [0] == '1')
		{
			std::string::size_type parenPos = _msg.find ('(');
			if (parenPos != std::string::npos)
				return _msg.substr (parenPos + 1, 6);
		}
	}
	return std::string ();
}
