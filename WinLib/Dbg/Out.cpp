//----------------------------------
// (c) Reliable Software 2001 - 2005
//----------------------------------

#include <WinLibBase.h>

#if !defined(NDEBUG)
#include "Out.h"
#include "Log.h"

#include <File/ErrorLog.h>
#include <Sys/Synchro.h>

#include <ostream>

namespace Dbg
{
	//	DbgLogStreamBuf : a simple stream buffer implementation that allows us to
	//	write to OutputDebugString using all the benefits of an ostream

	class LockHelper
	{
	public:
		void Acquire ()
		{
			if (_mtLocking)
			{
				_critSect.Acquire ();
				++_lockCounter;
			}
		}
		void ReleaseFull ()
		{
			unsigned int count = _lockCounter;
			_lockCounter = 0;
			while (count != 0)
			{
				--count;
				_critSect.Release ();
			}
		}

	private:
		Win::CritSection _critSect;
		unsigned long _lockCounter;
		static bool _mtLocking;
	};

	//	set this to false in debugger to help debug output locking
	bool LockHelper::_mtLocking = true;

	LockHelper TheLockHelper;

	class StreamBuf : public std::streambuf
	{
	public:
		virtual int sync ();
		virtual int overflow (int nCh = EOF);
		virtual int doallocate ();

		void SetLogging (bool isLogging) { _isLogging = isLogging; }
		void SetComponent (char const * component) { _component = component; }
	private:
		//	nBufferSize doesn't really matter;  we'll flush if we need more room
		enum { nPutAreaSize = 1024 };

		//	The buffer needs an extra character so we can append a NULL to the data
		char _buffer [nPutAreaSize + 1];

		bool		 _isLogging;
		char const * _component;
	};

	// Global objects
	StreamBuf TheStreamBuf;
	std::ostream TheOutStream (&TheStreamBuf);

	int StreamBuf::sync ()
	{
		char *pch = pptr();
		if (pch != 0)
		{
			//	We've got data in the buffer
			*pch = '\0';
			if (_isLogging)
			{
				::OutputDebugString (_buffer);
				TheLog.Write (_component, _buffer);
			}
		}

		doallocate ();
		TheLockHelper.ReleaseFull ();
		return 0;
	}

	int StreamBuf::overflow (int nCh)
	{
		sync ();
		if (nCh != EOF)
		{
			_buffer [0] = static_cast<char> (nCh);
			pbump (1);
		}
		return 0;
	}

	int StreamBuf::doallocate ()
	{
		setp (_buffer, _buffer + nPutAreaSize);
		return nPutAreaSize;
	}

	//
	//	CrtReport hooking
	//

	class ReportHook
	{
	public:
		ReportHook ();
		~ReportHook ();
	private:
		static int SendToDbg (int reportType, char *message, int *returnValue);
	private:
		_CRT_REPORT_HOOK _prevHook;
	};

	int ReportHook::SendToDbg (int reportType, char *message, int *returnValue)
	{
		Dbg::TheOutStream << "*** " << message << std::flush;
		*returnValue = 0; // don't start the debugger
		if (reportType == _CRT_ASSERT)
			TheErrorLog << WriteError (message);
		return FALSE; // let _CrtDbgReport output the message in normal way
	}

	ReportHook::ReportHook ()
		: _prevHook (_CrtSetReportHook (ReportHook::SendToDbg))
	{}

	ReportHook::~ReportHook ()
	{
		_CrtSetReportHook (_prevHook);
	}

	// make sure report hook is installed before main
	ReportHook TheReportHook;

	// ostream operator << for Dbg::Lock
	std::ostream& operator << (std::ostream& os, Dbg::Lock & lock)
	{
		Dbg::TheLockHelper.Acquire ();
		Dbg::TheStreamBuf.SetLogging (lock._isLogging);
		Dbg::TheStreamBuf.SetComponent (lock._component);
		return os;
	}
}

#endif
