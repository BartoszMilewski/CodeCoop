// ----------------------------------
// (c) Reliable Software, 2003 - 2007
// ----------------------------------

#include <WinLibBase.h>

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "BitService.h"
#include "Download.h"
#include "StringOp.h"
#include <File/ActiveCopy.h>

namespace BITS
{
	Service::Service ()
	{
		dbg << "BITS: Service constructor" << std::endl;
		HRESULT hres;
		hres = ::CoCreateInstance (CLSID_BackgroundCopyManager,
								   NULL,
								   CLSCTX_LOCAL_SERVER,
								   IID_IBackgroundCopyManager,
								   reinterpret_cast<void **>(&_service));
		if (!SUCCEEDED (hres))
			throw Com::Exception (hres, "Cannot obtain Background Copy Manager interface");
	}
	Com::IfacePtr<IBackgroundCopyJob> Service::CreateJob (std::string const & name)
	{
		dbg << "BITS: creating job" << std::endl;
		HRESULT hres;
		GUID guid;
		Com::IfacePtr<IBackgroundCopyJob> job;
		std::wstring wName = ToWString (name);
		hres = _service->CreateJob (wName.c_str (),
							        BG_JOB_TYPE_DOWNLOAD,
							        &guid,
							        &job);
		if (!SUCCEEDED(hres))
			throw Com::Exception (hres, "Internal error: Cannot obtain Background Copy Job interface");

		return job;
	}
	Com::IfacePtr<IEnumBackgroundCopyJobs> Service::GetJobs ()
	{
		dbg << "BITS: enumerating jobs" << std::endl;
		Com::IfacePtr<IEnumBackgroundCopyJobs> jobs;
		HRESULT hr = _service->EnumJobs (0, &jobs);
		if (!SUCCEEDED (hr))
			throw Com::Exception (hr, "Internal error: Cannot obtain Background Copy Job enumerator");

		return jobs;
	}

	// Job

	Job::Job (Service & service, std::string const & name)
		: _job (service.CreateJob (name))
	{
	}
	Job::Job (Com::IfacePtr<IBackgroundCopyJob> job)
		: _job (job)
	{}

	void Job::SetCallbacks (IBackgroundCopyCallback * callbacks)
	{
		dbg << "BITS: set callbacks" << std::endl;
		HRESULT hr = _job->SetNotifyInterface (callbacks);
		if (!SUCCEEDED(hr))
			throw Com::Exception (hr, "Internal error: Cannot register status notifiers of Background Copy Job");
		
		hr = _job->SetNotifyFlags(BG_NOTIFY_JOB_TRANSFERRED |
						          BG_NOTIFY_JOB_ERROR |
                                  BG_NOTIFY_JOB_MODIFICATION);
		if (!SUCCEEDED(hr))
			throw Win::Exception ("Internal error: Cannot set notification flags of Background Copy Job");
	}
	void Job::Cancel ()
	{
		dbg << "BITS: cancel job" << std::endl;
		_job->Cancel ();
	}

	void Job::AddFile (std::string const & remoteFileUrl, std::string const & localFile)
	{
		dbg << "BITS: add file " << remoteFileUrl.c_str () << std::endl;
		std::wstring wRemoteFileUrl = ToWString (remoteFileUrl);
		std::wstring wLocalFile = ToWString (localFile);
		
		HRESULT hr = _job->AddFile (wRemoteFileUrl.c_str (), wLocalFile.c_str ()); 

		if (!SUCCEEDED(hr))
			throw Com::Exception (hr, "Internal error: Cannot add file to Background Copy Job");
	}
	void Job::Resume ()
	{
		dbg << "BITS: resume job" << std::endl;
		HRESULT hr = _job->Resume ();
		if (!SUCCEEDED(hr))
			throw Com::Exception (hr, "Internal error: Cannot resume Background Copy Job");
	}

	void Job::SetNoProgressTimeout (unsigned long timeout)
	{
		dbg << "BITS: set no progress timeout for " << timeout << " seconds" << std::endl;
		_job->SetNoProgressTimeout (timeout);
	}

	void Job::SetForegroundPriority ()
	{
		dbg << "BITS: set foreground priority for a job" << std::endl;
		_job->SetPriority (BG_JOB_PRIORITY_FOREGROUND);
	}
	void Job::SetNormalPriority ()
	{
		dbg << "BITS: set normal priority for a job" << std::endl;
		_job->SetPriority (BG_JOB_PRIORITY_NORMAL);
	}
	// JobSeq

	JobSeq::JobSeq (Service & service, std::string const & name)
		: _name (name), _jobIter (service.GetJobs ()), _totalJobCount (0), _curr (-1)
	{
		dbg << "BITS: job enumerator" << std::endl;
		HRESULT hr = _jobIter->GetCount (&_totalJobCount);
		dbg << "BITS: found " << _totalJobCount << " jobs" << std::endl;
		if (!SUCCEEDED (hr))
			throw Com::Exception (hr, "Internal error: Cannot obtain Background Copy Man job number");

		Advance ();
	}

	void JobSeq::Advance ()
	{
		dbg << "BITS: advancing enumerator " << std::endl;
		Assert (!AtEnd ());
		++_curr;
		while (!AtEnd ())
		{
			HRESULT hr = _jobIter->Next (1, &_currJob, NULL);
			if (!SUCCEEDED (hr))
				throw Com::Exception (hr, "Internal error: Cannot advance Background Copy Job enumerator");

			if (_name.empty ())
				break; // don't skip any jobs

			if (IsNocaseEqual (GetName (), _name))
				break; // stop on a job with the same name

			++_curr;
		};
	}

	unsigned int JobSeq::Count ()
	{
		if (_name.empty ())
			return _totalJobCount;

		Reset ();
		unsigned int count = 0;
		for ( ; !AtEnd (); Advance ())
		{
			++count;
		}
		Reset ();	

		return count;
	}

	void JobSeq::Reset ()
	{
		_jobIter->Reset ();
		_curr = -1;
		Advance ();
	}

	void JobSeq::Cancel ()
	{
		Assert (!AtEnd ());
		_currJob->Cancel ();
	}

	std::string JobSeq::GetName ()
	{
		Assert (!AtEnd ());
		std::string currName;
		wchar_t* jobName = NULL;
		HRESULT hr = _currJob->GetDisplayName (&jobName);
		if (SUCCEEDED (hr))
		{
			currName = ToMBString (jobName);
			::CoTaskMemFree (jobName);
			dbg << "BITS: job name is " << currName << " jobs" << std::endl;
		}
		return currName;
	}

	// FileSeq

	FileSeq::FileSeq (Com::IfacePtr<IBackgroundCopyJob> job)
		: _fileCount (0), _curr (-1)
	{
		dbg << "BITS: file enumerator" << std::endl;
		HRESULT hr = job->EnumFiles (&_fileIter);
		if (!SUCCEEDED (hr))
			throw Com::Exception (hr, "Internal error: Cannot obtain Background Copy Files enumerator");

		hr = _fileIter->GetCount (&_fileCount);
		if (!SUCCEEDED (hr))
			throw Win::Exception ("Internal error: Cannot obtain Background Copy Job files number");

		dbg << "BITS: file enumerator found " << _fileCount << " files" << std::endl;
		Advance ();
	}
	void FileSeq::Advance ()
	{
		dbg << "BITS: advancing file enumerator" << std::endl;
		Assert (!AtEnd ());
		_curr++;
		if (AtEnd ())
			return;

		HRESULT hr = _fileIter->Next (1, &_currFile, NULL);
		if (!SUCCEEDED (hr))
			throw Com::Exception (hr, "Internal error: Cannot advance Background Copy File enumerator");
	}
	int FileSeq::GetFileCount ()
	{
		return _fileCount;
	}
	std::string FileSeq::GetLocalFileName ()
	{
		Assert (!AtEnd ());
		std::string localFileName;
		wchar_t* pLocalFileName = NULL;
		HRESULT hr = _currFile->GetLocalName (&pLocalFileName);
		if (SUCCEEDED(hr))
		{
			localFileName = ToMBString (pLocalFileName);
			::CoTaskMemFree (pLocalFileName); 
			dbg << "BITS: current file is " << localFileName << std::endl;
		}
		return localFileName;
	}

	// StatusCallbacks

	HRESULT StatusCallbacks::JobTransferred (IBackgroundCopyJob* pJob)
	{
		// 1. This callback is not always called on completion.
		//	  For example, if the download finished when a client app was not running,
		//    the client is notified through JobModification callback.
		dbg << "BITS: Job Transferred." << std::endl;
		HRESULT result = pJob->Complete ();
		// 2. To handle repeated notifications handle BG_E_INVALID_STATE error.
		//	  BG_E_INVALID_STATE: The requested action is not allowed in the current job state.
		//	  The job might have been canceled or completed transferring. It is in a read-only state now.
		// 3. BITS removes the job from the transfer queue if
		//    the HRESULT is S_OK or BG_S_PARTIAL_COMPLETE.
		//    The job remains in the transfer queue if BITS was unable to rename
		//	  all of the temporary files. The job remains in the queue until the application
		//    is able to fix the problem, and calls the Complete method again or Cancel method.
		//    Files that were renamed successfully are available to the user.
		CopyProgressSink::CompletionStatus status = CopyProgressSink::Unrecognized;
		if (result == S_OK)
		{
			dbg << "BITS job transferred: successfully." << std::endl;
			status = CopyProgressSink::Success;
		}
		else if (result == BG_E_INVALID_STATE)
		{
			dbg << "BITS Job transferred: job already cancelled or completed." << std::endl;
			status = CopyProgressSink::AlreadyHandled;
		}
		else if (BG_S_PARTIAL_COMPLETE)
		{
			dbg << "BITS job transferred: partial complete." << std::endl;
			status = CopyProgressSink::PartialSuccess;
		}
		else if (BG_S_UNABLE_TO_DELETE_FILES)
		{
			dbg << "BITS job transferred: cannot delete temporary files." << std::endl;
			status = CopyProgressSink::CannotDeleteTempFiles;
		}
		else
		{
			dbg << "BITS job transferred: unrecognized response." << std::endl;
			status = CopyProgressSink::Unrecognized;
		}

		if (!_sink.OnCompleted (status))
			pJob->Cancel ();

		return S_OK;
	}

	HRESULT StatusCallbacks::JobError (IBackgroundCopyJob* pJob, IBackgroundCopyError* pError)
	{
		// Transient errors do not generate calls to the JobError method
		dbg << "BITS: job error." << std::endl;
		BG_ERROR_CONTEXT errContext;
		HRESULT err;
		if (S_OK != pError->GetError (&errContext, &err))
		{
			dbg << "BITS: cancelling job." << std::endl;
			pJob->Cancel ();
			Win::InternalException ex ("BITS: unknown job exception.");
			_sink.OnException (ex);
			return S_OK;
		}
		WCHAR* pszDescription = NULL;
		pError->GetErrorDescription(GetThreadLocale(), &pszDescription);
		Win::InternalException ex ("BITS: job error.", ::ToMBString (pszDescription).c_str ());
		CoTaskMemFree(pszDescription);

		switch (errContext)
		{
		case BG_ERROR_CONTEXT_NONE:
			break;
		case BG_ERROR_CONTEXT_UNKNOWN:
			break;
		case BG_ERROR_CONTEXT_GENERAL_QUEUE_MANAGER:
			break;
		case BG_ERROR_CONTEXT_QUEUE_MANAGER_NOTIFICATION:
			break;
		case BG_ERROR_CONTEXT_LOCAL_FILE:
			break;
		case BG_ERROR_CONTEXT_REMOTE_FILE:
			break;
		case BG_ERROR_CONTEXT_GENERAL_TRANSPORT:
			break;
		};

		dbg << "BITS: cancelling job." << std::endl;
		pJob->Cancel ();
		_sink.OnException (ex);
		return S_OK;
	}

	HRESULT StatusCallbacks::JobModification (IBackgroundCopyJob* pJob, DWORD dwReserved)
	{
		// Your implementation may not receive all modification events
		// under maximum resource load conditions.
		dbg << "BITS: job modification." << std::endl;

		BG_JOB_PROGRESS progress;
		BG_JOB_STATE state;

		if (S_OK != pJob->GetProgress(&progress) ||
		    S_OK != pJob->GetState(&state))
		{
			dbg << "BITS: cancelling job." << std::endl;
			pJob->Cancel ();
			Win::InternalException ex ("BITS: cannot retrieve job progress or state.");
			_sink.OnException (ex);
			return S_OK;
		}
		switch (state)
		{
		case BG_JOB_STATE_QUEUED:
			dbg << "BITS job modification: job queued." << std::endl;
			break;
		case BG_JOB_STATE_CONNECTING:
			dbg << "BITS job modification: connecting." << std::endl;
			if (!_sink.OnConnecting ())
			{
				dbg << "BITS job modification: cancelling job." << std::endl;
				pJob->Cancel ();
			}
			break;
		case BG_JOB_STATE_TRANSFERRING:
			dbg << "BITS job modification: transferring." << std::endl;
			// Revisit: we are limiting ourselves to file sizes < 2GB
			if (!_sink.OnProgress (static_cast<int> (progress.BytesTotal),
								   static_cast<int> (progress.BytesTransferred)))
			{
				dbg << "BITS job modification: cancelling job." << std::endl;
				pJob->Cancel ();
			}
			break;
		case BG_JOB_STATE_SUSPENDED:
			dbg << "BITS job modification: job suspended." << std::endl;
			break;
		case BG_JOB_STATE_ERROR:
			{
				dbg << "BITS job modification: state error." << std::endl;
				// Documentation says: 
				// "BITS does not generate a modify event when the state of the job
				// changes to BG_JOB_STATE_ERROR."
				dbg << "BITS job modification: cancel the job." << std::endl;
				pJob->Cancel ();
				Win::InternalException ex ("BITS: job state error.");
				_sink.OnException (ex);
			}
			break;
		case BG_JOB_STATE_TRANSIENT_ERROR:
			dbg << "BITS job modification: transient error." << std::endl;
			_sink.OnTransientError ();
			break;
		case BG_JOB_STATE_TRANSFERRED:
			dbg << "BITS job modification: job transferred." << std::endl;
			// Documentation says: 
			// "BITS does not generate a modify event when the state of the job 
			// changes to BG_JOB_STATE_TRANSFERRED."
			// This is not true!
			{
				HRESULT result = pJob->Complete ();
				// 1. To handle repeated notifications handle BG_E_INVALID_STATE error.
				//	  BG_E_INVALID_STATE: The requested action is not allowed in the current job state.
				//	  The job might have been canceled or completed transferring. It is in a read-only state now.
				// 2. BITS removes the job from the transfer queue if
				//    the HRESULT is S_OK or BG_S_PARTIAL_COMPLETE.
				//    The job remains in the transfer queue if BITS was unable to rename
				//	  all of the temporary files. The job remains in the queue until the application
				//    is able to fix the problem, and calls the Complete method again or Cancel method.
				//    Files that were renamed successfully are available to the user.
				CopyProgressSink::CompletionStatus status = CopyProgressSink::Unrecognized;
				if (result == S_OK)
				{
					dbg << "BITS job modification: transferred successfully." << std::endl;
					status = CopyProgressSink::Success;
				}
				else if (result == BG_E_INVALID_STATE)
				{
					dbg << "BITS job modification: job already cancelled or completed." << std::endl;
					status = CopyProgressSink::AlreadyHandled;
				}
				else if (BG_S_PARTIAL_COMPLETE)
				{
					dbg << "BITS job modification: partial complete." << std::endl;
					status = CopyProgressSink::PartialSuccess;
				}
				else if (BG_S_UNABLE_TO_DELETE_FILES)
				{
					dbg << "BITS job modification: cannot delete temporary files." << std::endl;
					status = CopyProgressSink::CannotDeleteTempFiles;
				}
				else
				{
					dbg << "BITS job modification: unrecognized response." << std::endl;
					status = CopyProgressSink::Unrecognized;
				}

				if (!_sink.OnCompleted (status))
					pJob->Cancel ();
			}
			break;
		case BG_JOB_STATE_ACKNOWLEDGED:
			dbg << "BITS job modification: job acknowledged." << std::endl;
			break;
		case BG_JOB_STATE_CANCELLED:
			dbg << "BITS job modification: job cancelled." << std::endl;
			break;
		};

		return S_OK;
	}
}
