#if !defined (WINGLOBALMEM_H)
#define WINGLOBALMEM_H
//-----------------------------------
// (c) Reliable Software 1998 -- 2003
//-----------------------------------

class GlobalMem 
{
    friend class GlobalBuf;
public:
    GlobalMem ()
		: _hMem (0)
    {}
    GlobalMem (unsigned int len)
		: _hMem (0)
    {
		Allocate (len);
    }
    ~GlobalMem ()
    {
		Free ();
    }
	void Allocate (unsigned int len)
	{
		Free ();
        _hMem = ::GlobalAlloc (GHND, len);
        if (_hMem == 0)
            throw Win::Exception ("Cannot allocate global memory");
	}
	HGLOBAL GetHandle () const
	{
		return _hMem;
	}
    HGLOBAL Acquire ()
    {
        HGLOBAL tmp = _hMem;
        _hMem = 0;
        return tmp;
    }
private:
	void Free ()
	{
        if (_hMem != 0)
            ::GlobalFree (_hMem);
	}

private:
    HGLOBAL _hMem;
};

class GlobalBuf
{
public:
    GlobalBuf (GlobalMem & mem)
        : _handle (mem._hMem)
    {
        _buf = reinterpret_cast<char *> (::GlobalLock (_handle));
        if (_buf == 0)
            throw Win::Exception ("Internal error: Cannot lock global memory");
    }
    GlobalBuf (HGLOBAL handle)
        : _handle (handle)
    {
        _buf = reinterpret_cast<char *> (::GlobalLock (_handle));
        if (_buf == 0)
            throw Win::Exception ("Internal error: Cannot lock global memory");
    }
    char & operator [] (int i) { return _buf [i]; }
    char const & operator [] (int i) const { return _buf [i]; }
	char const * Get () const { return _buf; }
    ~GlobalBuf ()
    {
        ::GlobalUnlock (_handle);
    }
protected:
    char *  _buf;
private:
    HGLOBAL _handle;
};

#endif
