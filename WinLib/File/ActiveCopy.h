#if !defined (ACTIVECOPY_H)
#define ACTIVECOPY_H
//---------------------------
// (c) Reliable Software 2007
//---------------------------

#include "ActiveCopyFile.h"

#include <Sys/Active.h>
#include <File/SafePaths.h>

class FileIo;

class CopyProgressSink
{
	public:
		enum CompletionStatus
		{
			Unrecognized,
			Success,
			AlreadyHandled,
			PartialSuccess,
			CannotDeleteTempFiles
		};
public:
	virtual ~CopyProgressSink () {}
	// all methods returning boolean value
	// return false, if the download should be terminated
	virtual bool OnConnecting () = 0;
	virtual bool OnProgress (int bytesTotal, int bytesTransferred) = 0;
	virtual bool OnCompleted (CompletionStatus status) = 0;
	virtual void OnTransientError () = 0;
	virtual void OnException (Win::Exception & ex) = 0;
};

class ActiveCopy
{
private:
	class Worker : public ActiveObject
	{
	public:
		Worker (ActiveCopyFile & srcFile,
				ActiveCopyFile & destFile,
				CopyProgressSink & sink);
		~Worker ();
		// Active Object
		void Run ();
		void FlushThread () {}
	private:
		ActiveCopyFile & _sourceFile;
		ActiveCopyFile & _destFile;
		CopyProgressSink & _sink;
	};

public:
	ActiveCopy (CopyProgressSink & sink)
		: _sink (sink)
	{}

	bool IsWorking () const
	{
		if (_worker.empty ())
			return false;

		return _worker.IsAlive ();
	}
	void StartCopy (ActiveCopyFile & srcFile,
					ActiveCopyFile & destFile);
	void StopCopy () { _worker.reset (); }

private:
	CopyProgressSink &	_sink;
	auto_active<Worker>	_worker;
};

class LocalFile : public ActiveCopyFile
{
public:
	LocalFile (std::string const & path);
	~LocalFile ();

	void OpenWrite ();
	void OpenRead ();

	::File::Size GetSize ();
	std::string const & GetFilePath () const { return _targetPath; }

	void Read (char * buf, unsigned long & size);
	void Write (char const * buf, unsigned long size);

	void Commit ();

private:
	std::string				_targetPath;
	SafeTmpFile				_tmpPath;
	std::unique_ptr<FileIo>	_file;
};

#endif
