#if !defined (ERRORLOG_H)
#define ERRORLOG_H
//------------------------------------
//  (c) Reliable Software, 1997 - 2006
//------------------------------------

//
//	Single-threaded error logging support
//
//	To send output to the error log just use the 'TheErrorLog' as an ostream:
//		E.g.:  TheErrorLog << "Just a simple message " << n << std::endl;
//  Warning: don't use before or after main()
//  Always end log output with std::endl 

#if defined (DIAGNOSTIC)

#include "Path.h"
#include "FileIo.h"

#include <iostream>
#include <streambuf>
#include <fstream>

namespace Dbg
{
	class LogStreamManipulator
	{
	public:
		LogStreamManipulator (std::string const & str)
			: _str (str)
		{}

		virtual std::ostream & Execute (std::ostream & os) = 0;

	protected:
		std::string const & _str;
	};

	class LogStream : public std::ostream
	{
	public:
		LogStream ()
			: _outputBuf (_log, _state),
			  std::ostream (&_outputBuf)
		{}

		void SetLogPath (std::string const & path);
		void WriteError (std::string const & err);
		void PushState (std::string const & str);
		void PopState ();
		void Clear ();

	private:
		class OutputBuf : public std::streambuf
		{
		public:
			OutputBuf (std::vector<std::string> & log, std::vector<int> const &	state)
				: _log (log),
				  _state (state)
			{
				doallocate ();
			}

			virtual int sync ();
			virtual int overflow (int ch = EOF);
			virtual int doallocate ();

		private:
			//	Size doesn't really matter;  we'll flush if we need more room
			enum { BufferSize = 1024 };

		private:
			//	The buffer needs an extra character so we can append a NULL to the data
			char						_buffer [BufferSize + 1];
			std::vector<std::string> &	_log;
			std::vector<int> const &	_state;
		};

	private:
		static char _logName [];

		OutputBuf					_outputBuf;	
		OutStream					_logFile;
		FilePath					_logPath;
		std::vector<std::string>	_log;
		std::vector<int>			_state;
	};

	extern LogStream TheLogStream;

	#define TheErrorLog Dbg::TheLogStream
}

class SetLogPath : public Dbg::LogStreamManipulator
{
public:
	SetLogPath (std::string const & state)
		: Dbg::LogStreamManipulator (state)
	{}

	std::ostream & Execute (std::ostream & os);
};

class PushState : public Dbg::LogStreamManipulator
{
public:
	PushState (std::string const & state)
		: Dbg::LogStreamManipulator (state)
	{}

	std::ostream & Execute (std::ostream & os);
};

class PopState : public Dbg::LogStreamManipulator
{
public:
	PopState ()
		: Dbg::LogStreamManipulator (std::string ())
	{}

	std::ostream & Execute (std::ostream & os);
};

class WriteError : public Dbg::LogStreamManipulator
{
public:
	WriteError (std::string const & state)
		: Dbg::LogStreamManipulator (state)
	{}

	std::ostream & Execute (std::ostream & os);
};

class ClearLogStack : public Dbg::LogStreamManipulator
{
public:
	ClearLogStack ()
		: Dbg::LogStreamManipulator (std::string ())
	{}

	std::ostream & Execute (std::ostream & os);
};

#else

//	In non-debug builds we want any calls to the log stream to disappear.

//	Make sure that in non-debug builds we completely get rid of calls to TheErrorLog,
//	even if we're calling functions and trying to send the results.  E.g.,
//
//		dbg << pFoo->SomeFunction(...)
//
//	should disappear completely.  It's not enough to have the log stream do
//	nothing.  We make sure the call to the stream is on the right of a
//	logical and so that it never gets evaluated.  And since it's never used,
//	we don't even need a real object.  Here we construct an ostream out of
//	nothing (literally).
//

#define TheErrorLog	0 && (*((std::ostream *) 0))

namespace Dbg
{
	class LogStreamManipulator
	{
	public:
		LogStreamManipulator () {}
		std::ostream & Execute (std::ostream & os) {};
	};

	int dummy ();
}

#define SetLogPath(x)	#x

#define PushState(x)	#x

#define PopState		Dbg::dummy

#define WriteError(x)	#x

#define ClearLogStack	Dbg::dummy

#endif

std::ostream & operator<<(std::ostream & os, Dbg::LogStreamManipulator & manip);

#endif
