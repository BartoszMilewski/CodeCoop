//---------------------------
// (c) Reliable Software 2007
//---------------------------
#include <WinLibBase.h>
#include "ActiveCopy.h"

#include <File/Path.h>
#include <File/FileIo.h>
#include <Sys/GlobalUniqueName.h>

LocalFile::LocalFile (std::string const & path)
	: _targetPath (path),
	  _tmpPath (GlobalUniqueName ().GetName () + ".tmp")
{
}

void LocalFile::OpenWrite ()
{
	_file.reset (new FileIo (_tmpPath.GetFilePath (), ::File::CreateAlwaysMode ()));
}

void LocalFile::OpenRead ()
{
	_file.reset (new FileIo (_targetPath, ::File::ReadOnlyMode ()));
}

::File::Size LocalFile::GetSize()
{
	if (_file.get() != 0)
		return _file->GetSize();
	else
	{
		FileIo openFile(_targetPath, ::File::ReadOnlyMode());
		return openFile.GetSize();
	}
}

void LocalFile::Read (char * buf, unsigned long & size)
{
	_file->FillBuf (buf, size);
}

void LocalFile::Write (char const * buf, unsigned long size)
{
	_file->Write (buf, size);
}

LocalFile::~LocalFile ()
{
	if (_file.get () != 0)
		_file->Close ();
}

void LocalFile::Commit ()
{
	_file->Close ();
	if (::File::Exists (_targetPath))
		::File::Delete (_targetPath);
	::File::Move (_tmpPath.GetFilePath (), _targetPath.c_str ());
}

//-----------
// ActiveCopy
//-----------
void ActiveCopy::StartCopy (ActiveCopyFile & srcFile,
							ActiveCopyFile & destFile)
{
	_worker.reset (new Worker (srcFile, destFile, _sink));
	_worker.SetWaitForDeath (30 * 1000);
}

//-------------------
// ActiveCopy::Worker
//-------------------
ActiveCopy::Worker::Worker (ActiveCopyFile & srcFile,
							ActiveCopyFile & destFile,
							CopyProgressSink & sink)
	: _sourceFile (srcFile),
	  _destFile (destFile),
	  _sink (sink)
{
}

ActiveCopy::Worker::~Worker ()
{}

int const ChunkSize = 256 * 1024;

void ActiveCopy::Worker::Run ()
{
	if (IsDying ())
		return;

	try
	{
		_sink.OnConnecting ();
		// GetSize must be called before opening the file
		::File::Size sourceFileSize = _sourceFile.GetSize ();
		if (sourceFileSize.IsLarge ())
			throw Win::InternalException ("Cannot copy file bigger then 4GB.",
										  _sourceFile.GetFilePath ().c_str ());

		_sourceFile.OpenRead ();
		if (IsDying ())
			return;

		_destFile.OpenWrite ();
		if (IsDying ())
			return;

		unsigned long srcFileSize = sourceFileSize.Low ();
		char buf [ChunkSize];
		unsigned long size = 0;
		int sizeTransferred = 0;
		do
		{
			size = sizeof (buf);
			// modifies size
			_sourceFile.Read (buf, size);

			if (IsDying ())
				return;
			
			if (size == 0)
				break;

			sizeTransferred += size;
			_destFile.Write (buf, size);

			if (!_sink.OnProgress (srcFileSize, sizeTransferred))
				return;

		} while (true);

		_destFile.Commit ();
		_sink.OnCompleted (CopyProgressSink::Success);
	}
	catch (Win::Exception ex)
	{
		_sink.OnException (ex);
	}
	catch (...)
	{
		Win::InternalException ex ("Unknown exception during file copy.",
								   _sourceFile.GetFilePath ().c_str ());
		_sink.OnException (ex);
	}
}

