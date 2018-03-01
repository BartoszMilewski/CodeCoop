//-----------------------------------
// (c) Reliable Software 2008
//-----------------------------------

#include "precompiled.h"

#include <File/File.h>
#include <File/Path.h>
#include <Ex/Error.h>
#include <Ex/Winex.h>
#include <StringOp.h>
#include <Ctrl/Output.h>

#include <iostream>

class LocalPipePath: public FilePath
{
public:
	LocalPipePath ()
	{
		_buf.resize (3);
		_buf [0] = '\\';
		_buf [1] = '\\';
		_buf [2] = '.';
		_prefix = _buf.length ();
		DirDown ("pipe");
	}
};

class Pipe
{
public:
	Pipe (std::string const & name)
		: _hPipe (INVALID_HANDLE_VALUE)
	{
		LocalPipePath path;
		_hPipe = ::CreateNamedPipe( 
				  path.GetFilePath(name),   // pipe name 
				  PIPE_ACCESS_DUPLEX,       // read/write access 
				  PIPE_TYPE_MESSAGE |       // message type pipe 
				  PIPE_READMODE_MESSAGE |   // message-read mode 
				  PIPE_WAIT,                // blocking mode 
				  PIPE_UNLIMITED_INSTANCES, // max. instances  
				  BUFFER_SIZE,              // output buffer size 
				  BUFFER_SIZE,              // input buffer size 
				  0,                        // client time-out 
				  NULL);                    // default security attribute 

		if (_hPipe == INVALID_HANDLE_VALUE)
			throw Win::Exception ("Cannot create named pipe", path.GetFilePath (name));
	}

	~Pipe () throw () { Close (); }

	HANDLE ToNative () const { return _hPipe; }
	
	bool PipeOk () const throw () { return _hPipe != INVALID_HANDLE_VALUE; }

	void Close () throw ()
	{
		if (PipeOk())
		{
			BOOL b = ::CloseHandle (_hPipe);
			Assert (b != FALSE);
			_hPipe = INVALID_HANDLE_VALUE;
		}
	}

	bool WaitForConnection ()
	{
		// Wait for the client to connect; if it succeeds, 
		// the function returns a nonzero value. If the function
		// returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 

		if (ConnectNamedPipe(_hPipe, NULL) != 0)
			return true;
		if (GetLastError() == ERROR_PIPE_CONNECTED)
			return true;

		return false;
	}

private:
	static int const BUFFER_SIZE = 4096;

private:
	HANDLE	_hPipe;
};

int main (int argc, char * argv [])
{
	try
	{
		Pipe pipe ("CodeCoopHub");
		if (pipe.WaitForConnection ())
		{
			// Handle client connection
		}
	}
	catch (Win::Exception ex)
	{
		std::cerr << "Win::Exception: " << Out::Sink::FormatExceptionMsg (ex) << std::endl;
	}
	catch ( ... )
	{
		std::cerr << "Unknow exception" << std::endl;
	}
	return 1;
}
