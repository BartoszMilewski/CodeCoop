#if !defined (MAPIBUFFER_H)
#define MAPIBUFFER_H
//
// (c) Reliable Software 1998
//

#include <Ex\WinEx.h>

#include <mapix.h>

template <class T>
class MapiBuffer
{
public:
	MapiBuffer ()
		: _p (0)
	{}
	MapiBuffer (int elementCount);
	MapiBuffer (void * pBaseBlock, int elementCount);
	~MapiBuffer ()
	{
		Free ();
	}

	T * Release ()
	{
		T * tmp = _p;
		_p = 0;
		return tmp;
	}
	T * GetBuf() const { return _p; }
    T & operator [] (int i) { return _p [i]; }
	T * operator ->() { return _p; }
	T ** GetFillBuf ()
	{
		// Return pointer to a pointer, so MAPI
		// can allocate buffer and fill it in
		// MAPI allocated and filled buffer CANNOT
		// be user with MapiBufferSerializer
		Free ();
		return &_p;
	}

private:
	MapiBuffer (MapiBuffer const & buf);
	MapiBuffer & operator = (MapiBuffer const & buf);
	void Free ();

private:
	T * _p;
};

template <class T>
MapiBuffer<T>::MapiBuffer<T> (int elementCount)
{
	SCODE sRes = ::MAPIAllocateBuffer (elementCount * sizeof (T), reinterpret_cast<LPVOID *>(&_p));
	if (FAILED (sRes))
		throw WinException ("Cannot allocate MAPI memory");
	memset (_p, 0, elementCount * sizeof (T));
}

template <class T>
MapiBuffer<T>::MapiBuffer<T> (void * pBaseBlock, int elementCount)
{
	SCODE sRes = ::MAPIAllocateMore (elementCount * sizeof (T), pBaseBlock, reinterpret_cast<LPVOID *>(&_p));
	if (FAILED (sRes))
		throw WinException ("Cannot allocate MAPI memory");
	memset (_p, 0, elementCount * sizeof (T));
}

template <class T>
void MapiBuffer<T>::Free ()
{
	if (_p != 0)
	{
		::MAPIFreeBuffer (_p);
		_p = 0;
	}
}

template <class T>
class MapiBufferSerializer
{
public:
	MapiBufferSerializer (MapiBuffer<T> const & buf)
		: _pWrite (buf.GetBuf ())
	{}

	void Write (T * element)
	{
		memcpy (_pWrite, element, sizeof (T));
		_pWrite++;
	}

private:
	T * _pWrite;
};

//
// Mapi ADRLIST
//

class MapiRecipient;
struct _ADRLIST;

class MapiAddrList
{
public:
	MapiAddrList ()
		: _addrList (0),
		  _top (0)
	{}
	MapiAddrList (int recipientCount);
	MapiAddrList (MapiAddrList & buf);
	~MapiAddrList ();

	MapiAddrList & operator = (MapiAddrList & buf);
	void AddRecipient (MapiRecipient const & recipient);
	struct _ADRLIST * GetBuf () const { return _addrList; }

	void Dump (char const * title) const;

private:
	struct _ADRLIST * Release ();
	void Free ();

private:
	struct _ADRLIST *	_addrList;
	unsigned int		_top;
};

//
// MAPI SRowSet
//

struct _SRowSet;

class MapiRows
{
public:
	MapiRows ()
		: _rows (0)
	{}
	MapiRows (MapiRows & buf);
	~MapiRows ();

	MapiRows & operator = (MapiRows & buf);
	struct _SRowSet ** FillIn () { return &_rows; }
	int Count () const { return _rows->cRows; }
	SBinary const * GetEntryId (int i) const;
	char const * GetDisplayName (int i) const;
	bool IsFolder (int i) const;

	void Dump (char const * title) const;

private:
	struct _SRowSet * Release ();
	void Free ();

private:
	struct _SRowSet *	_rows;
};

#endif
