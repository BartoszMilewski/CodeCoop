#if !defined (MEMCHECK_H)
#define MEMCHECK_H
//-----------------------------------------
//  MemCheck.h
//  (c) Reliable Software, 1999
//-----------------------------------------

class HeapCheck
{
public:
	HeapCheck ()
	{
		int crtDbgFlags = _CrtSetDbgFlag (_CRTDBG_REPORT_FLAG);
		// Turn on debug allocation
		crtDbgFlags |= _CRTDBG_ALLOC_MEM_DF;
		// Turn off C-Run-time block checking
		crtDbgFlags &= ~_CRTDBG_CHECK_CRT_DF;
		_CrtSetDbgFlag (crtDbgFlags);
		// Remeber heap state at program startup
		_CrtMemCheckpoint (&_begin);
	}
	~HeapCheck ()
	{
		// Remember heap state at program end
		_CrtMemCheckpoint (&_end);
		if (_CrtMemDifference (&_diff, &_begin, &_end))
			_CrtMemDumpStatistics (&_diff);
		_CrtDumpMemoryLeaks ();
	}

private:
	_CrtMemState	_begin;
	_CrtMemState	_end;
	_CrtMemState	_diff;
};

#endif
