#if !defined (BITSERVICE_H)
#define BITSERVICE_H
// ---------------------------
// (c) Reliable Software, 2003
// ---------------------------
#include <Bits.h>
#include <Com/Com.h>

class CopyProgressSink;

namespace BITS
{
	class Service
	{
	public:
		Service ();
		Com::IfacePtr<IBackgroundCopyJob> CreateJob (std::string const & name);
		Com::IfacePtr<IEnumBackgroundCopyJobs> GetJobs ();
	private:
		 Com::IfacePtr<IBackgroundCopyManager> _service;
	};

	class Job
	{
	public:
		Job (Service & service, std::string const & name);
		Job (Com::IfacePtr<IBackgroundCopyJob> iJob);
		void SetCallbacks (IBackgroundCopyCallback * callbacks);
		void AddFile (std::string const & remoteFileUrl, std::string const & localFile);
		void Resume ();
		void Cancel ();

		void SetNoProgressTimeout (unsigned long timeout);
		void SetForegroundPriority ();
		void SetNormalPriority ();
	private:
		 Com::IfacePtr<IBackgroundCopyJob> _job;
	};

	class JobSeq
	{
	public:
		JobSeq (Service & service, std::string const & name); // specify empty name to iterate over all jobs
		bool AtEnd () const { return _curr == _totalJobCount; }
		void Advance ();
		void Reset ();
		unsigned int Count (); // resets the sequencer
		void Cancel ();

		Com::IfacePtr<IBackgroundCopyJob> GetCurr ()
		{
			Assert (!AtEnd ());
			return _currJob;
		}
	private:
		std::string GetName ();
	private:
		std::string const _name;
		Com::IfacePtr<IEnumBackgroundCopyJobs> _jobIter;
		Com::IfacePtr<IBackgroundCopyJob>      _currJob;
		unsigned long						   _totalJobCount;
		unsigned long						   _curr;
	};

	class FileSeq
	{
	public:
		FileSeq (Com::IfacePtr<IBackgroundCopyJob> job);
		void Advance ();
		bool AtEnd () const { return _curr == _fileCount; }
		int GetFileCount ();
		std::string GetLocalFileName ();
	private:
		Com::IfacePtr<IEnumBackgroundCopyFiles> _fileIter;
		Com::IfacePtr<IBackgroundCopyFile>      _currFile;
		unsigned long _fileCount; 
		unsigned long _curr;
	};

	class StatusCallbacks : public IBackgroundCopyCallback
	{
	public:
		StatusCallbacks (CopyProgressSink & sink)
			: _sink (sink),
			  _refCount (1)
		{}

		// IUnknown
		HRESULT __stdcall QueryInterface (REFIID riid, LPVOID *ppvObj)
		{
			if( riid == IID_IUnknown || riid == IID_IBackgroundCopyCallback ) 
			{
				*ppvObj = this;
			}
			else
			{
				*ppvObj = NULL;
				return E_NOINTERFACE;
			}

			AddRef();
			return S_OK;
		}
		ULONG __stdcall AddRef  ()
		{
			return ::InterlockedIncrement (&_refCount);
		}
		ULONG __stdcall Release ()
		{
			ULONG  count = ::InterlockedDecrement (&_refCount);
			if (count == 0)
				delete this;

			return count;
		}

		// IBackgroundCopyCallback methods
		HRESULT __stdcall JobTransferred (IBackgroundCopyJob* job);
		HRESULT __stdcall JobError (IBackgroundCopyJob* pJob, IBackgroundCopyError* pError);
		HRESULT __stdcall JobModification (IBackgroundCopyJob* pJob, DWORD dwReserved);
	private:
		CopyProgressSink & _sink;
		LONG volatile  _refCount;
	};
}

#endif
