#if !defined (MEMORYLOG_H)
#define MEMORYLOG_H
//---------------------------------------------
// (c) Reliable Software 2004
//---------------------------------------------

#include <vector>
#include <string>
#include <iostream>
#include <streambuf>

class MemoryLog  : public std::ostream
{
public:
	MemoryLog ()
		: _outputBuf (_log),
		  std::ostream (&_outputBuf)
	{}

	void Save (std::string const & path);

private:
	class OutputBuf : public std::streambuf
	{
	public:
		OutputBuf (std::vector<std::string> & log)
			: _log (log)
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
	};

private:
	OutputBuf					_outputBuf;	
	std::vector<std::string>	_log;
};

class DbgTracer : public MemoryLog
{
public:
	DbgTracer (std::string const & logFileName)
		: _logFileName (logFileName)
	{}

	void SaveLog ();

private:
	std::string	_logFileName;
};

class DbgTrace
{
public:
	DbgTrace (DbgTracer & tracer, std::string const & apiName)
		: _tracer (tracer),
		  _apiName (apiName)
	{
		_tracer << "--> " << _apiName << std::endl;
	};
	~DbgTrace ()
	{
		_tracer << "<-- " << _apiName << std::endl;
	};
private:
	DbgTracer &	_tracer;
	std::string	_apiName;
};

#endif
