#if !defined HTAB_H
#define HTAB_H
//------------------------------------
// (c) Reliable Software 1997
//------------------------------------
#include <array_vector.h>

namespace Hash
{
	// A singly linked link

	class ILink
	{
	public:
		ILink () {}
		ILink (ILink * next, int value)
			: _next (next), _value (value) 
		{}

		ILink * GetNext () { return _next; }
		ILink const * GetNext () const { return _next; }
		int GetValue () const { return _value; }

		void  SetValue (int value) 
		{
			_value = value; 
		}
		void  SetNext (ILink * next)
		{
			_next = next;
		}
	private:
		ILink   *_next; // pointer to next
		int     _value; // stored value
	};


	// Singly linked list

	class IList
	{
		friend class ListIterC;
		friend class ListIter;
	public:
		IList () : _head (0) {}

		// No destructor: the list doesn't own links
		void push_front (ILink * link);
		int  count ();
		void clear () {_head = 0;}

	private:

		ILink * GetHead () { return _head; }
		ILink const * GetHead () const { return _head; }
	private:

		ILink *_head; // head of the linked list
	};

	// IList iterator. Must be subclassed
	// Only objects of derived classes 
	// can be created

	class ListIterC
	{
	public:
		bool AtEnd () const { return _link == 0; }
		void Advance () { _link = _link->GetNext(); }
		int GetValue () const { return _link->GetValue (); }
	protected:
		// Constructor is protected
		ListIterC (IList const & list)
			: _link (list.GetHead()) {}
	private:
		ILink const *_link;
	};

	class ListIter
	{
	public:
		bool AtEnd () const { return _link == 0; }
		void Advance () { _link = _link->GetNext(); }
		int GetValue () const { return _link->GetValue (); }
		void SetValue (int value) { _link->SetValue (value); }
	protected:
		// Constructor is protected
		ListIter (IList & list)
			: _link (list.GetHead()) {}
	private:
		ILink *_link;
	};

	// Bulk allocator of links

	class LinkAlloc
	{
		enum { BLOCK_SIZE = 1000 };
	public:
		LinkAlloc () : _curBlock (0), _curLink (0) {}
		ILink * AllocLink (int value);
		void Reset ();
	private:
		// allocate blocks of ILink

		array_vector<ILink>	_storage;
		unsigned int	    _curBlock;
		int					_curLink;
	};


	// Hash table
	// implemented as an array of short lists.
	// Search returns a short list, the caller
	// is supposed to find (or not) the actual entry
	// in that list.

	class Table
	{
		friend class ShortIter;
	public:
		Table () : _size (0), _aList (0) {}

		Table (int size)
			: _aList (0)
		{
			Init (size);
		}

		~Table ()
		{
			delete [] _aList;
		}

		void Init (int size)
		{
			assert (_aList == 0);
			_aList = new IList [size];
			_size = size;
		}

		void Add (int id, const char* str, int len);

	private:

		IList & Find (const char* str, int len) const;
		int hash (const char* str, int len) const;

		int          _size;    // size of the array
		IList		*_aList;   // array of lists

		LinkAlloc	 _storage;
	};

	// The short list iterator
	// The client creates this iterator
	// to search for a given string

	class ShortIter: public ListIterC
	{
	public:
		ShortIter ( IList &list)
			: ListIterC (list) {}

		ShortIter (Table const & htab, const char* str, int len)
			: ListIterC (htab.Find (str, len)) {}
	};
};

#endif
