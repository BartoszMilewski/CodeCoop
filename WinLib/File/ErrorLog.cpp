//------------------------------------
//  (c) Reliable Software, 1997 - 2009
//------------------------------------

#include <WinLibBase.h>
#include "ErrorLog.h"
#include <HtmlTag.h>
#include <StringOp.h>

#if defined (DIAGNOSTIC)

#include <TimeStamp.h>

namespace Dbg
{
	LogStream TheLogStream;

	char LogStream::_logName [] = "ErrorLog.html";

	void LogStream::SetLogPath (std::string const & path)
	{
		_logPath.Change (path);
	}

	void LogStream::WriteError (std::string const & err)
	{
		if (_logPath.IsDirStrEmpty ())
			return;

		try
		{
			char const * fullPath = _logPath.GetFilePath (_logName);
			_logFile.Open (fullPath, std::ios_base::app);
			// Format error log in HTML
			{
				Html::Document doc (_logFile);
				// Head
				{
					Html::Head head (doc);
					{
						Html::Title title (head);
						_logFile << "Code Co-op Error Log";
					}
				}
				// Body
				{
					Html::Body body (doc);
					// Heading 2
					{
						Html::Heading2 heading (body);
						StrTime timeStamp (CurrentTime ());
						std::string logEntry (timeStamp.GetString ());
						logEntry += "<p>";
						logEntry += err;
						logEntry += '\n';
						_logFile << logEntry;
					}
					// Write memory log
					for (size_t i = 0; i < _log.size (); i++)
					{
						_logFile << _log [i];
					}
					_logFile << "<p>-----END OF ERROR REPORT----" << std::endl;
				}
			}
			_logFile.close ();
		}
		catch (...) 
		{
			if (_logFile.is_open ())
				_logFile.close ();
		}
		LogStream::Clear ();
	}

	void LogStream::PushState (std::string const & str)
	{
		_state.push_back (_log.size ());
		std::ostringstream msg ("<p>");
		// Heading 2
		{
			msg << "<p>";
			Html::Heading2 h2 (msg);
			msg << _state.size () << ": " << str << std::endl;
		}
		_log.push_back (msg.str ());
	}

	void LogStream::PopState ()
	{
		if (!_state.empty ())
		{
			int count = _log.size () - _state.back ();
			_state.pop_back ();
			// Clear lines logged in the poped state
			for (int i = 0; i < count; i++)
			{
				_log.pop_back ();
			}
		}
	}

	void LogStream::Clear ()
	{
		_log.clear ();
		_state.clear ();
	}

	int LogStream::OutputBuf::sync ()
	{
		char * pch = pptr();
		if (pch != 0)
		{
			//	We've got data in the buffer
			*pch = '\0';
			std::string msg ("<br>");
			msg += ToString (_state.size ());
			msg += ": ";
			msg += _buffer;
			_log.push_back (msg);
		}
		doallocate ();
		return 0;
	}

	int LogStream::OutputBuf::overflow (int ch)
	{
		sync ();
		if (ch != EOF)
		{
			_buffer [0] = static_cast<char> (ch);
			pbump (1);
		}
		return 0;
	}

	int LogStream::OutputBuf::doallocate ()
	{
		setp (_buffer, _buffer + BufferSize);
		return BufferSize;
	}
}

std::ostream & SetLogPath::Execute (std::ostream & os)
{
	Dbg::TheLogStream.SetLogPath (_str);
	return os;
}

std::ostream & PushState::Execute (std::ostream & os)
{
	Dbg::TheLogStream.PushState (_str);
	return os;
}

std::ostream & PopState::Execute (std::ostream & os)
{
	Dbg::TheLogStream.PopState ();
	return os;
}

std::ostream & WriteError::Execute (std::ostream & os)
{
	Dbg::TheLogStream.WriteError (_str);
	return os;
}

std::ostream & ClearLogStack::Execute (std::ostream & os)
{
	Dbg::TheLogStream.Clear ();
	return os;
}

std::ostream & operator<<(std::ostream & os, Dbg::LogStreamManipulator & manip)
{
	return manip.Execute (os);
}

#else

namespace Dbg
{
	int dummy () { return 0; }
}

std::ostream & operator<<(std::ostream & os, Dbg::LogStreamManipulator & manip)
{
	return os;
}

#endif
