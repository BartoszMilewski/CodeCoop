#if !defined (LONGSEQUENCE_H)
#define LONGSEQUENCE_H
//---------------------------
// (c) Reliable Software 2005
//---------------------------
#include <File/MemFile.h>
#include "PtrUtil.h"
#include <iterator>

const unsigned PageSize = 0x10000; // 64k (allocation granularity on x86 processors
//const unsigned PageSize = 0x100000; // 1M

// A container that can store more than 4GB of data by using backing store
// and mapping it on demand. Supports adding elements to the end and iterating
// forward only.

template<class T>
class LongSequence: public MappableFile
{
	class Page;
	class TopPage;
public:
	class iterator;
public:
	LongSequence (std::string const & path, 
					File::Mode mode = File::OpenAlwaysMode ());
	LongSequence (File::Size reservedSize);
	~LongSequence ();
	void push_back (T val);
	iterator begin () const
	{
		return iterator (this, 0, 0);
	}
	iterator end () const
	{
		return iterator (PageCount (), LastPageRecordCount ());
	}
	void Resize (File::Size size);
	File::Size CurSize () const;
private:
	void InitSize (File::Size size);
	static unsigned RecordsPerPage () { return PageSize / sizeof (T); }
	static unsigned UsablePageSize () { return sizeof (T) * RecordsPerPage (); }
	unsigned PageCount () const;
	unsigned LastPageRecordCount () const;
private:
	LongSequence (LongSequence const &);
	LongSequence & operator= (LongSequence const &);
private:
	unsigned	_originalPageCount;
	unsigned	_origLastPageRecordCount;
	bool		_isWritable;
	mutable std::unique_ptr<TopPage>	_topPage;
};

// Page

// Top page of the file, where appending can be done
template<class T>
class LongSequence<T>::TopPage
{
public:
	TopPage (LongSequence<T> & file, unsigned pageNo, unsigned count)
		: _buf (0), _top (0), _end (0), _pageNo (pageNo), _file (file)
	{
		File::Offset startOffset (pageNo);
		startOffset *= PageSize;
		if (file.IsFile ())
		{
			File::Size newSize (0, 0);
			newSize += startOffset;
			newSize += PageSize;
			// We are always at the end of the file
			file.CloseMap ();
			file.MakeMap (newSize, false); // not read only
		}
		_buf = reinterpret_cast<T *> (::MapViewOfFile (
			file.ToNative (),
			FILE_MAP_WRITE,
			startOffset.High (),
			startOffset.Low (),
			PageSize));
		if (_buf == 0)
			throw Win::Exception ("LongSequence: cannot map file for writing");
		_top = _buf + count;
		_end = _buf + PageSize / sizeof (T);
	}
	~TopPage ()
	{
		if (_buf)
			::UnmapViewOfFile (_buf);
	}
	bool IsFile () const { return _file.IsFile (); }
	unsigned PageNo () const { return _pageNo; }
	unsigned Count () const { return _top - _buf; }
	void push_back (T value)
	{
		Assert (_top != _end);
		*_top = value;
		++_top;
	}
	bool IsFull () const { return _top == _end; }
private:
	TopPage (Page const &);
	TopPage & operator= (Page const &);
private:
	LongSequence<T> & _file;
	unsigned _pageNo;
	T * _top;
	T * _end;
	T * _buf;
};

template<class T>
class LongSequence<T>::Page: public RefCounted
{
public:
	Page (LongSequence<T> const * file, unsigned pageNo) 
		: _pageNo (pageNo), _buf (0), _file (file)
	{
		if (_file->ToNative () != 0)
		{
			Map ();
		}
	}
	Page (Page * prevPage)
		: _pageNo (prevPage->_pageNo + 1), _buf (0), _file (prevPage->_file)
	{
		Map ();
	}
	~Page ()
	{
		if (_buf)
			::UnmapViewOfFile (_buf);
	}
	unsigned PageNo () const { return _pageNo; }
	T const * Get (unsigned idx) const { return &_buf [idx]; }
	T * Get (unsigned idx) { return &_buf [idx]; }
private:
	void Map ()
	{
		File::Offset offset (_pageNo);
		offset *= PageSize;
		unsigned bufSize = PageSize;
		LargeInteger sizeLeft (_file->CurSize ().ToMath () - offset.ToMath ());
		if (!sizeLeft.IsLarge () && sizeLeft.Low () < PageSize)
			bufSize = sizeLeft.Low ();
		if (bufSize == 0)
		{
			_buf = 0;
			return;
		}

		_buf = reinterpret_cast<T *> (::MapViewOfFile (
			_file->ToNative (),
			FILE_MAP_READ,
			offset.High (),
			offset.Low (),
			bufSize));
		if (_buf == 0)
			throw Win::Exception ("LongSequence Page: cannot map file for reading");
	}
private:
	File::Size	_totalSize;
	unsigned	_pageNo;
	T *			_buf;
	LongSequence<T> const * _file;
};

// LongSequence iterator

template<class T>
class LongSequence<T>::iterator : public std::iterator<std::input_iterator_tag, T>
{
	typedef typename LongSequence<T>::Page Page;
public:
	iterator (unsigned pageNo, unsigned recordCount) 
		: _pageNo (pageNo), _curIdx (recordCount)
	{
	}
	iterator (LongSequence<T> const * file, unsigned pageNo, unsigned idx) 
		: _pageNo (pageNo), _curIdx (idx)
	{
		_page.reset (new Page (file, pageNo));
	}
	T operator * () const { return *_page->Get (_curIdx); }
	T const * operator-> () const { return _page->Get (_curIdx); }
	iterator operator++ ()
	{
		++_curIdx;
		if (_curIdx == RecordsPerPage ())
		{
			_page.reset (new Page (_page.get ()));
			++_pageNo;
			_curIdx = 0;
		}
		return *this;
	}
	bool operator != (iterator const & other) const
	{ 
		return !operator == (other); 
	}
	bool operator == (iterator const & other) const
	{ 
		return _pageNo == other._pageNo && _curIdx == other._curIdx; 
	}
private:
	unsigned		_pageNo;
	unsigned		_curIdx;
	RefPtr<Page>	_page;
};

template<class T>
LongSequence<T>::LongSequence (std::string const & path, 
							   File::Mode mode)
	: MappableFile (path, mode), _isWritable (!mode.IsReadOnly ())
{
	InitSize (GetSize ());
}

template<class T>
LongSequence<T>::LongSequence (File::Size reservedSize)
	: _isWritable (true), _originalPageCount (0), _origLastPageRecordCount (0)
{
	long long x = PageSize - 1;
	long long y = (x + reservedSize.ToMath ()) / PageSize;
	y *= PageSize;
	MakeMap (File::Size (y), false);
}

template<class T>
void LongSequence<T>::InitSize (File::Size size)
{
	// Calculate the number of pages
	LargeInteger iPageCount (size);
	iPageCount /= UsablePageSize ();
	if (iPageCount.IsLarge ())
		throw Win::InternalException ("LongSequence: File too big");
	_originalPageCount = iPageCount.Low ();

	// Calculate how many records in the last page
	LargeInteger iRoundedSize (_originalPageCount);
	iRoundedSize *= UsablePageSize ();
	LargeInteger iLeft (size.ToMath () - iRoundedSize.ToMath ());
	Assert (!iLeft.IsLarge ());
	_origLastPageRecordCount = iLeft.Low () / sizeof (T);
}

template<class T>
LongSequence<T>::~LongSequence ()
{
	try
	{
		if (_topPage.get () != 0 && _topPage->IsFile ())
		{
			// Shrink file back to actual size of data
			File::Size size (_topPage->PageNo (), 0);
			size *= PageSize;
			size += _topPage->Count () * sizeof (T);
			_topPage.reset ();
			CloseMap ();
			File::Resize (size);
		}
	}
	catch (...)
	{}
}

template<class T>
void LongSequence<T>::Resize (File::Size size)
{
	_topPage.reset ();
	CloseMap ();
	InitSize (size);
	MakeMap (size, false); // not read only
}

template<class T>
unsigned LongSequence<T>::PageCount () const
{
	if (_topPage.get () == 0)
		return _originalPageCount;
	unsigned pageNo = _topPage->PageNo ();
	if (_topPage->IsFull ())
		++pageNo;
	return pageNo;
}

template<class T>
unsigned LongSequence<T>::LastPageRecordCount () const
{
	if (_topPage.get () == 0)
		return _origLastPageRecordCount;

	unsigned count = _topPage->Count ();
	if (_topPage->IsFull ())
		count = 0;
	return count;
}

template<class T>
File::Size LongSequence<T>::CurSize () const
{
	if (_topPage.get () != 0)
	{
		File::Size size (_topPage->PageNo ());
		size *= PageSize;
		size += _topPage->Count () * sizeof (T);
		return size;
	}
	else
	{
		File::Size size (_originalPageCount);
		size *= PageSize;
		size += _origLastPageRecordCount * sizeof (T);
		return size;
	}
}

template<class T>
void LongSequence<T>::push_back (T val)
{
	if (_topPage.get () == 0)
	{
		_topPage.reset (new TopPage (*this, _originalPageCount, _origLastPageRecordCount));
	}
	else if (_topPage->IsFull ())
	{
		unsigned pageNo = _topPage->PageNo () + 1;
		_topPage.reset (new TopPage (*this, pageNo, 0));
	}
	_topPage->push_back (val);
}

#endif
