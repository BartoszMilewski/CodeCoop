#if !defined (LOG_H)
#define LOG_H
//------------------------------------
//  (c) Reliable Software, 2000 - 2006
//------------------------------------

#include <Dbg/Monitor.h>
#include <File/FileIo.h>

namespace Dbg
{
	// To turn on the logging of debug output, call TheLog.Open 
	// with file name and (optional) directory. All debug statements
	// dbg << ...
	// will be copied to the log file.

	class Log
	{
	public:
		Log ()
			: _isOn (false)
		{}
		~Log ();
		void Open (char const * fileName, char const * path = 0);
		void DbgMonAttach (std::string const & program) { _monitor.Attach (program); }
		void DbgMonDetach () { _monitor.Detach (); }
		void Close ();
		bool IsOn () const { return _isOn; }

		void Write (std::string const & component, char const * msg);

	private:
		std::string FormatLogName (std::string const & name) const;

	private:
		Dbg::Monitor	_monitor;
		OutStream		_out;
		// state
		bool			_isOn;
	};

#if defined DIAGNOSTIC
	extern Log TheLog;
#endif
}

#endif
