#if !defined (MAPIBUFFER_H)
#define MAPIBUFFER_H
//
// (c) Reliable Software 1998 -- 2002
//

#include <Ex\WinEx.h>

#include <mapix.h>

namespace Mapi
{
	class MsgStore;
	class Message;

	template <class T>
	class Buffer
	{
	public:
		Buffer ()
			: _p (0)
		{}
		Buffer (int elementCount);
		Buffer (void * pBaseBlock, int elementCount);
		~Buffer ()
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
		T ** operator & () { return &_p; }
		T & operator [] (int i) { return _p [i]; }
		T * operator ->() { return _p; }

	private:
		Buffer (Buffer const & buf);
		Buffer & operator = (Buffer const & buf);
		void Free ();

	protected:
		T * _p;
	};

	template <class T>
	Buffer<T>::Buffer (int elementCount)
	{
		SCODE sRes = ::MAPIAllocateBuffer (elementCount * sizeof (T), reinterpret_cast<LPVOID *>(&_p));
		if (FAILED (sRes))
			throw Win::Exception ("Cannot allocate MAPI memory");
		memset (_p, 0, elementCount * sizeof (T));
	}

	template <class T>
	Buffer<T>::Buffer (void * pBaseBlock, int elementCount)
	{
		SCODE sRes = ::MAPIAllocateMore (elementCount * sizeof (T), pBaseBlock, reinterpret_cast<LPVOID *>(&_p));
		if (FAILED (sRes))
			throw Win::Exception ("Cannot allocate MAPI memory");
		memset (_p, 0, elementCount * sizeof (T));
	}

	template <class T>
	void Buffer<T>::Free ()
	{
		if (_p != 0)
		{
			::MAPIFreeBuffer (_p);
			_p = 0;
		}
	}

	//
	// MAPI SRowSet
	//

	class RowSet
	{
	public:
		RowSet ()
			: _rows (0)
		{}
		RowSet (RowSet & buf);
		~RowSet ();

		RowSet & operator = (RowSet & buf);
		LPSRowSet * operator & () { return &_rows; }
		LPSRowSet Get () { return _rows; }
		unsigned long Count () const { return _rows->cRows; }
		SBinary const * GetEntryId (unsigned int i) const;
		char const * GetDisplayName (unsigned int i) const;
		unsigned long GetAttachNum (unsigned int i) const;
		bool IsFolder (unsigned int i) const;

		LPSRowSet Release ();
		void Dump (char const * title) const;

	private:
		void Free ();

	private:
		LPSRowSet	_rows;
	};

	class EntryId
	{
		friend class Session;
		friend class MsgStore;

	public:
		EntryId ()
			: _size (0),
			  _id (0)
		{}
		~EntryId ()
		{
			if (_id != 0)
			{
				::MAPIFreeBuffer (_id);
				_id = 0;
			}
		}

		unsigned long GetSize () const { return _size; }
		ENTRYID * Get () const { return _id; } 

	protected:
		unsigned long	_size;
		ENTRYID *		_id;
	};

	class PrimaryIdentityId : public EntryId
	{
	public:
		void SetValid (bool flag) { _isValid = flag; }
		bool IsValid () const { return _isValid; }

	private:
		bool	_isValid;
	};

	class FolderEntryId : public Buffer<SPropValue>
	{
	public:
		FolderEntryId () {}
		FolderEntryId (MsgStore & msgStore, unsigned long folderIdTag)
		{
			Init (msgStore, folderIdTag);
		}

		void Init (MsgStore & msgStore, unsigned long folderIdTag);

		bool IsValid () const { return _p != 0 && _p->Value.bin.cb != 0; }

		unsigned long GetSize () const { return _p->Value.bin.cb; }
		ENTRYID * Get () const { return reinterpret_cast<ENTRYID *>(_p->Value.bin.lpb); } 
	};

	class MsgEntryId : public Buffer<SPropValue>
	{
	public:
		MsgEntryId (Message & msg);

		unsigned long GetSize () const { return _p->Value.bin.cb; }
		ENTRYID * Get () const { return reinterpret_cast<ENTRYID *>(_p->Value.bin.lpb); } 
		SBinary * GetBinary () const { return &_p->Value.bin; }
	};

	class ReceiveFolderId : public EntryId
	{
	public:
		ReceiveFolderId (MsgStore & msgStore);
	};
}

#endif
