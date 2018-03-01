// ----------------------------------
// (c) Reliable Software, 2003 - 2007
// ----------------------------------

#include <WinLibBase.h>

#include "BitsDownloader.h"
#include <StringOp.h>
#include "File/Path.h"

namespace BITS
{
	Downloader::Downloader (std::string const & jobName,
							CopyProgressSink & sink,
							std::string const & server)
		: _callbacks (new StatusCallbacks (sink))
		, _jobName (jobName)
	{
		_url = "http://" + server + "/";
		try
		{
			_bits.reset (new BITS::Service ());
		}
		catch (...)
		{
			// BITS not installed or failed on very basic level
			_bits.reset ();
		}
	}

	unsigned int Downloader::CountDownloads ()
	{
		BITS::JobSeq  seq (*_bits.get (), _jobName);
		return seq.Count ();
	}

	void Downloader::StartGetFile (std::string const & sourcePath,
								   std::string const & targetPath)
	{
		try
		{
			_job.reset (new Job (*_bits, _jobName));
			std::string remoteFileUrl (_url);
			remoteFileUrl += sourcePath;
			_job->AddFile (remoteFileUrl, targetPath);
			_job->SetCallbacks (_callbacks);
			_job->Resume ();
		}
		catch (...)
		{
			if (_job.get () != 0)
				_job->Cancel ();

			throw;
		}
	}

	bool Downloader::Continue (std::string const & localFile)
	{
		for (BITS::JobSeq  seq (*_bits.get (), _jobName); !seq.AtEnd (); seq.Advance ())
		{
			BITS::FileSeq fileSeq (seq.GetCurr ());
			if (fileSeq.GetFileCount () == 1)
			{
				if (IsNocaseEqual (fileSeq.GetLocalFileName (), localFile))
				{
					_job.reset (new Job (seq.GetCurr ()));
					break;
				}
			}
		}
		if (_job.get () == 0)
			return false;

		_job->SetCallbacks (_callbacks);
		return true;
	}

	void Downloader::CancelCurrent ()
	{
		if (_job.get () != 0)
			_job->Cancel ();
	}

	void Downloader::CancelAll ()
	{
		for (BITS::JobSeq  seq (*_bits.get (), _jobName); !seq.AtEnd (); seq.Advance ())
		{
			seq.Cancel ();
		}
	}
	
	void Downloader::SetTransientErrorIsFatal (bool isFatal)
	{
		Assert (_job.get () != 0);
		if (isFatal)
			_job->SetNoProgressTimeout (0);
		else
			_job->SetNoProgressTimeout (1209600); // default Windows value
	}
	void Downloader::SetForegroundPriority ()
	{
		Assert (_job.get () != 0);
		_job->SetForegroundPriority ();
	}
	void Downloader::SetNormalPriority ()
	{
		Assert (_job.get () != 0);
		_job->SetNormalPriority ();
	}
}
