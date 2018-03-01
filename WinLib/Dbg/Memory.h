#if !defined(DBG_MEMORY_H)
#define DBG_MEMORY_H
//------------------------------
// (c) Reliable Software 2001-03
//------------------------------

//	LeakTrack : Use CRT routines to track memory allocations
//
//	declare object Dbg::LeakTrack leaktrack globally for leak tracking

#if !defined (NDEBUG)
#include <Sys\Synchro.h>
#endif

namespace Dbg
{
	class LeakTrack
	{
#if !defined(NDEBUG)

	private:
		int _crtFlags;

	public:
		LeakTrack()
		{
			// Get current flag
			_crtFlags = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );

			int newFlags = _crtFlags;

			// Turn on leak-checking bit
			newFlags |= _CRTDBG_LEAK_CHECK_DF;
			// Turn off CRT block checking bit
			newFlags &= ~_CRTDBG_CHECK_CRT_DF;
			// Set flag to the new value
			_CrtSetDbgFlag( newFlags );
		}

		~LeakTrack()
		{
	//		_CrtSetDbgFlag( _crtFlags );
		}

#endif
	};

	class NoMemoryAllocations
	{
#if !defined (NDEBUG)
	public:
		NoMemoryAllocations ();
		~NoMemoryAllocations ();

	private:
		static int AllocHook (int allocType, void *userData, size_t size, int blockType, 
							long requestNumber, const unsigned char *filename, int lineNumber);

		static Win::Mutex					_mutex;
		static std::list<DWORD>				_tidList;
		static std::list<DWORD>::size_type	_tidListSize;
		static _CRT_ALLOC_HOOK volatile		_oldHook;
#endif
	};
}

#endif
