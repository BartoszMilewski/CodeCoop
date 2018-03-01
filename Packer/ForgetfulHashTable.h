#if !defined FORGETFULHASHTABLE_H
#define FORGETFULHASHTABLE_H
//-------------------------------------
// (c) Reliable Software 1999, 2000, 01
// ------------------------------------

#include <auto_vector.h>

class ForgetfulHashTable
{
public:
	ForgetfulHashTable ()
	   : _size (HashVectorSize)
	{
		_lists.resize (_size, 0);
	}

	void Clear ();

private:
	struct Link
	{
		Link *	_next;
	};

	class PositionItem : public Link 
	{ 
	public:
		void Init (int position, Link * next)
		{
			_position = position;
			_next = next;
		}

		int GetPosition () const { return _position; }
		PositionItem * Next () const { return reinterpret_cast<PositionItem *>(_next); }
		void SetNext (Link * next) { _next = next; }

	private:
		int	_position;
	};

public:
	class PositionListIter
	{
	public:
		PositionListIter (PositionItem * head)
			: _cur (head)
		{}

		bool AtEnd () const { return _cur == 0; }
		void Advance () { _cur = _cur->Next (); }

		int GetPosition () const { return _cur->GetPosition (); }
		void ShortenList ()
		{
			_linkAllocator.Free (_cur->Next ());
			_cur->SetNext (0);
		}
 
	private:
		PositionItem *	_cur;
	};

	friend class ForgetfulHashTable::PositionListIter;

	PositionListIter Save (unsigned long hashValue, int position)
	{
		unsigned int hashIdx = hashValue % _size;
		PositionItem * oldHead = _lists [hashIdx];
		PositionItem * newItem = _linkAllocator.Alloc ();
		newItem->Init (position, oldHead);
		_lists [hashIdx] = newItem;
		// Return position list already stored in the hash table
		return PositionListIter (oldHead);	
	}

private:
	enum
	{
		HashVectorSize = 86371
	};

	class LinkAllocator
	{
	public :
		LinkAllocator ()
			: _free (0)
		{}
		~LinkAllocator ()
		{
			Purge ();
		}

   		void Free (PositionItem * list);

		void Purge ()
		{
			_pool.clear ();
			_free = 0;
		}

		PositionItem * Alloc ()
		{
			if (_free == 0)
			{
				std::unique_ptr<Block> newBlock (new Block (sizeof (PositionItem)));
				_free = newBlock->GetFirstLink ();
				_pool.push_back (std::move(newBlock));
			}
			PositionItem * newLink = reinterpret_cast<PositionItem *>(_free);
			_free = _free->_next;
			return newLink;
		}

	private:
		class Block
		{
		public:
			Block (unsigned int itemSize);

			Link * GetFirstLink () { return reinterpret_cast<Link *>(&_mem [0]); }

		private:
			enum
			{
				BlockSize = 79 * 1024	// 79kb holds approx. 20 000 PositionItems
			};

		private:
			char	_mem [BlockSize];
		};

	private:
		 Link *				_free;
		 auto_vector<Block>	_pool;
	};

private:
	std::vector <PositionItem *>	_lists;
	const unsigned int				_size;
	static LinkAllocator			_linkAllocator;
};

#endif 
