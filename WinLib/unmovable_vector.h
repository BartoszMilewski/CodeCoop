#if !defined (UNMOVABLE_VECTOR_H)
#define UNMOVABLE_VECTOR_H
//---------------------------
// (c) Reliable Software 2005
//---------------------------
#include <auto_vector.h>
#include <iterator>

// A container that stores elements by value
// You can use pointers to elements, because they never move

template<class T, unsigned block_size=1000>
class unmovable_vector
{
	class Block
	{
	public:
		Block (): _top (0)
		{}
		bool IsFull () const { return _top == block_size; }
		T * push_back (T const & elem)
		{
			Assert (!IsFull ());
			_arr [_top] = elem;
			++_top;
			return & _arr [_top - 1];
		}
		void clear () { _top = 0; }
		static unsigned capacity () { return block_size; }
		unsigned size () const { return _top; }
		typedef T * iterator;
		iterator begin () { return &_arr [0]; }
		iterator end () { return &_arr [_top]; }
		static iterator null_iter () { return 0; }
	private:
		unsigned	_top;
		T			_arr [block_size];
	};

protected:
	typedef typename auto_vector<typename unmovable_vector<T, block_size>::Block> block_vector;

	typedef typename block_vector::const_iterator BlocksIter;

public:
	class const_iterator: public std::iterator<std::input_iterator_tag, T>
	{
	public:
		const_iterator (BlocksIter blocksIter, BlocksIter blocksEnd, 
			typename Block::iterator it, typename Block::iterator endBlock)
			: _blocksIter (blocksIter), _blocksEnd (blocksEnd), _it (it), _endBlock (endBlock)
		{}
		const_iterator operator++ ()
		{
			Advance ();
			return *this;
		}
		T const * operator-> () const { return &(*_it); }
		T const & operator * () const { return (*_it); }

		bool operator != (const_iterator const & other) const
		{ 
			return _it != other._it; 
		}
		bool operator == (const_iterator const & other) const
		{ 
			return _it == other._it; 
		}
	protected:
		void Advance ()
		{
			++_it;
			if (_it == _endBlock)
			{
				++_blocksIter;
				if (_blocksIter != _blocksEnd)
				{
					_it = (*_blocksIter)->begin ();
					_endBlock = (*_blocksIter)->end ();
				}
			}
		}
	protected:
		BlocksIter		_blocksIter;
		BlocksIter		_blocksEnd;
		typename Block::iterator _it;
		typename Block::iterator _endBlock;
	};

	class iterator: public const_iterator
	{
	public:
		iterator (BlocksIter blocksIter, BlocksIter blocksEnd, 
			typename Block::iterator it, typename Block::iterator endBlock)
			: const_iterator (blocksIter, blocksEnd, it, endBlock)
		{}
		iterator operator++ ()
		{
			Advance ();
			return *this;
		}
		T * operator-> () { return &(*_it); }
		T & operator * () { return (*_it); }
	};
public:
	unmovable_vector ();
	T * push_back (T const & elem);
	unsigned size () const;
	void clear ();
	const_iterator begin () const
	{
		return iterator (_blocks.begin (), 
						_blocks.end (), 
						(*_blocks.begin ())->begin (), 
						(*_blocks.begin ())->end ());
	}
	const_iterator end () const
	{
		BlocksIter lastBlock = _blocks.end ();
		--lastBlock;
		return iterator (_blocks.end (), _blocks.end (), (*lastBlock)->end (), (*lastBlock)->end ());
	}

	iterator begin ()
	{
		return iterator (_blocks.begin (), 
						_blocks.end (), 
						(*_blocks.begin ())->begin (), 
						(*_blocks.begin ())->end ());
	}
	iterator end ()
	{
		BlocksIter lastBlock = _blocks.end ();
		--lastBlock;
		return iterator (_blocks.end (), _blocks.end (), (*lastBlock)->end (), (*lastBlock)->end ());
	}

protected:
	block_vector _blocks;
};

// Implementation

template<class T, unsigned block_size>
unmovable_vector<T, block_size>::unmovable_vector ()
{
	// Always at least one block!
	std::unique_ptr<Block> block (new Block);
	_blocks.push_back (std::move(block));
}

template<class T, unsigned block_size>
unsigned unmovable_vector<T, block_size>::size () const
{
	return (_blocks.size () - 1) * Block::capacity () + _blocks.back ()->size ();
}

template<class T, unsigned block_size>
void unmovable_vector<T, block_size>::clear ()
{
	// Always at least one block!
	_blocks.resize (1);
	_blocks [0]->clear ();
}

template<class T, unsigned block_size>
T * unmovable_vector<T, block_size>::push_back (T const & elem)
{
	Assert (_blocks.back () != 0);
	if (_blocks.back ()->IsFull ())
	{
		std::unique_ptr<Block> block (new Block);
		_blocks.push_back (std::move(block));
	}
	return _blocks.back ()->push_back (elem);
}

#endif
