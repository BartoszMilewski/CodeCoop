// ---------------------------
// (c) Reliable Software, 2003
// ---------------------------
#include "precompiled.h"
#include "TestController.h"
#include <StringOp.h>
#include <Net/BITService.h>
#include <Sys/Synchro.h>
#include <File/Path.h>

Out::Sink TheOutput;

bool TestController::OnCreate (Win::CreateData const * create, bool & success) throw ()
{
	TheOutput.SetParent (_h);
	Win::EditMaker editMaker (_h, 1);
	editMaker.Style () << Win::Edit::Style::MultiLine << Win::Edit::Style::AutoVScroll << Win::Style::Ex::AddClientEdge;
	_edit.Reset (editMaker.Create ());
	_timer.Attach (_h);
	_timer.Set (100);
	success = true;
	return true;
}

bool TestController::OnTimer (int id) throw ()
{
	_timer.Kill ();
	try
	{
//		GetSingleFileCallbackPureAPI ();
		GetSingleFileCallback ();
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (char const * msg)
	{
		TheOutput.Display (msg, Out::Error);
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Internal error: Unknown exception", Out::Error);
	}
	return true;
}

// ---------------------------------------------------
// based on Platform SDK bits_ie sample

class StatCallbacks : public IBackgroundCopyCallback
{
public:
	StatCallbacks ()
	{}
	void Init (DownloadSink * sink)
	{
		  _sink = sink;
	}
	// IUnknown
	HRESULT __stdcall QueryInterface (REFIID riid, LPVOID *ppvObj)
	{
		if( riid == IID_IUnknown)
		{
			*ppvObj = (IUnknown*)this;
		}
		else if (riid == IID_IBackgroundCopyCallback ) 
		{
			*ppvObj = (IBackgroundCopyCallback*)this;
		}
		else
		{
			*ppvObj = NULL;
			return E_NOINTERFACE;
		}

		return S_OK;
	}
    HRESULT STDMETHODCALLTYPE CreateInstance(
        IUnknown *pUnkOuter,
        REFIID riid,
        void **ppvObject )
    {

        if ( pUnkOuter )
            return CLASS_E_NOAGGREGATION;

        return QueryInterface( riid, ppvObject );

    }
	ULONG __stdcall AddRef  () {return 0; }
	ULONG __stdcall Release () {return 0; }

	// IBackgroundCopyCallback methods
	HRESULT __stdcall JobTransferred (IBackgroundCopyJob* pJob)
	{
		// 2. To handle repeated notifications handle BG_E_INVALID_STATE error.
		//	  BG_E_INVALID_STATE: The requested action is not allowed in the current job state.
		//	  The job might have been canceled or completed transferring. It is in a read-only state now.
		// 3. BITS removes the job from the transfer queue if
		//    the HRESULT is S_OK or BG_S_PARTIAL_COMPLETE.
		//    The job remains in the transfer queue if BITS was unable to rename
		//	  all of the temporary files. The job remains in the queue until the application
		//    is able to fix the problem, and calls the Complete method again or Cancel method.
		//    Files that were renamed successfully are available to the user.
		HRESULT result = pJob->Complete ();
		DownloadSink::CompletionStatus status = CompleteResult2Status (result);
		if (!_sink->OnCompleted (status))
			pJob->Cancel ();
		return S_OK;
	}
	HRESULT __stdcall JobError (IBackgroundCopyJob* pJob, IBackgroundCopyError* pError)
	{
		BG_ERROR_CONTEXT errContext;
		HRESULT err;
		if (S_OK != pError->GetError (&errContext, &err))
		{
			pJob->Cancel ();
			_sink->OnFatalError ();
			return S_OK;
		}
		WCHAR* pszDescription = NULL;
		pError->GetErrorDescription(GetThreadLocale(), &pszDescription);
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

		pJob->Cancel ();
		_sink->OnFatalError ();
		return S_OK;
	}
	HRESULT __stdcall JobModification (IBackgroundCopyJob* pJob, DWORD dwReserved)
	{
		BG_JOB_PROGRESS progress;
		BG_JOB_STATE state;

		if (S_OK != pJob->GetProgress(&progress) ||
		    S_OK != pJob->GetState(&state))
		{
			pJob->Cancel ();
			_sink->OnFatalError ();
			return S_OK;
		}
		switch (state)
		{
		case BG_JOB_STATE_QUEUED:
			break;
		case BG_JOB_STATE_CONNECTING:
			_sink->OnConnecting ();
			break;
		case BG_JOB_STATE_TRANSFERRING:
			// Revisit: we are limiting ourselves to file sizes < 2GB
			if (!_sink->OnProgress (static_cast<int> (progress.BytesTotal),
								   static_cast<int> (progress.BytesTransferred)))
			{
				pJob->Cancel ();
			}
			break;
		case BG_JOB_STATE_SUSPENDED:
			break;
		case BG_JOB_STATE_ERROR:
			break;
		case BG_JOB_STATE_TRANSIENT_ERROR:
			{
				BG_ERROR_CONTEXT Context;
				IBackgroundCopyError* pError = NULL;
				HRESULT hr = 0;
				HRESULT hrError = 0;
				WCHAR* pszDescription = NULL;

				hr = pJob->GetError(&pError);
				if (SUCCEEDED(hr))
				{
					pError->GetError(&Context, &hrError);  
					hr = pError->GetErrorDescription(GetThreadLocale(), &pszDescription);
					CoTaskMemFree(pszDescription);
				}
				pError->Release();
			}
			_sink->OnTransientError ();
			break;
		case BG_JOB_STATE_TRANSFERRED:
			{
				HRESULT result = pJob->Complete ();
				_sink->OnCompleted (CompleteResult2Status (result));
			}
			break;
		case BG_JOB_STATE_ACKNOWLEDGED:
			break;
		case BG_JOB_STATE_CANCELLED:
			break;
		};

		return S_OK;
	}
private:
	DownloadSink::CompletionStatus CompleteResult2Status (HRESULT result) const
	{
		DownloadSink::CompletionStatus status = DownloadSink::Unrecognized;
		if (result == S_OK)
		{
			dbg << "BITS job transferred: successfully." << std::endl;
			status = DownloadSink::Success;
		}
		else if (result == BG_E_INVALID_STATE)
		{
			dbg << "BITS Job transferred: job already cancelled or completed." << std::endl;
			status = DownloadSink::AlreadyHandled;
		}
		else if (BG_S_PARTIAL_COMPLETE)
		{
			dbg << "BITS job transferred: partial complete." << std::endl;
			status = DownloadSink::PartialSuccess;
		}
		else if (BG_S_UNABLE_TO_DELETE_FILES)
		{
			dbg << "BITS job transferred: cannot delete temporary files." << std::endl;
			status = DownloadSink::CannotDeleteTempFiles;
		}
		else
		{
			dbg << "BITS job transferred: unrecognized response." << std::endl;
			status = DownloadSink::Unrecognized;
		}
		return status;
	}
private:
	DownloadSink * _sink;
	LONG volatile  _refCount;
} callbacks;

void TestController::GetSingleFileCallbackPureAPI ()
{
	callbacks.Init (this);
	HRESULT hres;
	hres = ::CoCreateInstance (CLSID_BackgroundCopyManager,
								NULL,
								CLSCTX_LOCAL_SERVER,
								IID_IBackgroundCopyManager,
								reinterpret_cast<void **>(&service));
	if (!SUCCEEDED (hres))
		throw Win::Exception ("Internal error: Cannot obtain Background Copy Manager interface");

	GUID guid;
	std::wstring wName = ToWString ("Code Co-op Dispatcher");
	hres = service->CreateJob (wName.c_str (),
							   BG_JOB_TYPE_DOWNLOAD,
							   &guid,
							   &job);
	if (!SUCCEEDED(hres))
		throw Win::Exception ("Internal error: Cannot obtain Background Copy Job interface");

	std::wstring wRemoteFileUrl = ToWString ("http://www.bartosz.com/index.htm");
	std::wstring wLocalFile = ToWString ("C:\\version.xml");
	
	hres = job->AddFile (wRemoteFileUrl.c_str (), wLocalFile.c_str ()); 
	if (!SUCCEEDED(hres))
		throw Win::Exception ("Internal error: Cannot add file to Background Copy Job");

	HRESULT hr = job->SetNotifyInterface (&callbacks);
	if (!SUCCEEDED(hr))
		throw Win::Exception ("Internal error: Cannot register status notifiers of Background Copy Job");
	
	hr = job->SetNotifyFlags(BG_NOTIFY_JOB_TRANSFERRED |
						        BG_NOTIFY_JOB_ERROR |
                                BG_NOTIFY_JOB_MODIFICATION);
	if (!SUCCEEDED(hr))
		throw Win::Exception ("Internal error: Cannot set notification flags of Background Copy Job");

	hres = job->Resume ();
	if (!SUCCEEDED(hr))
		throw Win::Exception ("Internal error: Cannot resume Background Copy Job");
}

// -----------------------------------------
void TestController::GetSingleFileCallback ()
{
	PutLine ("Get single file, callback version");
	_downloader.reset (new BITS::Downloader ("Code Co-op Dispatcher", *this));
	PutLine ("Start downloading");
	std::string localFolder ("c:\\");
	FilePath localFolderPath (localFolder);
	_downloader->StartGetFile (
		"relisoft.com",
		"version.xml",
		localFolderPath,
		"version.xml");

	_downloader->CancelCurrent ();
	// wait for some time so that download completes
//	Win::Event event;
//	event.Wait (10*1000);

//	_downloader.reset ();
//	event.Wait (10*1000);

	// break and reconnect to BITS
	PutLine ("Break and reconnect to BITS");
	_downloader.reset (new BITS::Downloader ("Code Co-op Dispatcher", *this));
	if (_downloader->Continue ("c:\\version.xml"))
		PutLine ("Continuing download");
	else
	{
		PutLine ("Restarting download");
		_downloader->StartGetFile (
		"relisoft.com",
		"version.xml",
		localFolderPath,
		"version.xml");
	}
}

// ----------------------
// DownloadSink interface
bool TestController::OnConnecting ()
{
	PutLine ("Connecting");
	return true;
}

// returns false, if the download should be terminated
bool TestController::OnProgress (int bytesTotal, int bytesTransferred)
{
	PutLine ("Progress");
	return true;
}

bool TestController::OnCompleted (CompletionStatus status)
{
	PutLine ("Completed!");
	return true;
}

void TestController::OnTransientError ()
{
	// just wait
	PutLine ("Transient Error");
	_downloader->CancelCurrent ();
}

void TestController::OnFatalError ()
{
	// switch to FTP
	PutLine ("Fatal error!");
}
