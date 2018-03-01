#if !defined PIPE_H
#define PIPE_H
//----------------------------------
// (c) Reliable Software 2009
//----------------------------------

class Pipe
{
public:
	Pipe(bool doInheritHandle = false)
		: _hRead(INVALID_HANDLE_VALUE),
		  _hWrite(INVALID_HANDLE_VALUE)
	{
		SECURITY_ATTRIBUTES securityAttr;
   		securityAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
   		securityAttr.bInheritHandle = doInheritHandle? TRUE: FALSE; 
   		securityAttr.lpSecurityDescriptor = NULL;
		
		if (!CreatePipe(&_hRead, &_hWrite, &securityAttr, 0))
			throw Win::Exception("Cannot open pipe");
		
	}
	~Pipe()
	{
		if (_hRead != INVALID_HANDLE_VALUE)
			CloseHandle(_hRead);
		if (_hWrite != INVALID_HANDLE_VALUE)
			CloseHandle(_hWrite);
	}
	bool Read (void * buf, unsigned long & size)
	{
		ULONG sizeAsked = size;
		return ::ReadFile (_hRead, buf, sizeAsked, &size, NULL)	!= FALSE;
	}
	bool Write (void const * buf, unsigned long & size)
	{
		ULONG sizeAsked = size;
		return ::WriteFile (_hWrite, buf, sizeAsked, &size, NULL) != FALSE;
	}
	void CloseReadEnd()
	{
		CloseHandle(_hRead);
		_hRead = INVALID_HANDLE_VALUE;
	}
	void CloseWriteEnd()
	{
		CloseHandle(_hWrite);
		_hWrite = INVALID_HANDLE_VALUE;
	}
	HANDLE ToNativeWrite() const { return _hWrite; }
	HANDLE ToNativeRead() const { return _hRead; }
private:
	HANDLE _hRead;
	HANDLE _hWrite;
};

#endif

