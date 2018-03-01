#if !defined (DATACHUNK_H)
#define DATACHUNK_H
// (c) Reliable Software 2003
#include <StringOp.h>
#include <Sys/Synchro.h>
#include <Dbg/Out.h>
#include <auto_vector.h>
#include <sstream>

enum DiffState
{
	stateNew,
	stateDel,
	stateSame,
	stateDiff,
	stateInvalid
};

namespace Data
{
	// Represents a folder or a file (or a pair of those, being compared)
	class Item
	{
	public:
		// Item::Info contains comparison data
		class Info
		{
			enum
			{
				bitFolder = 0,
				bitDrive,
				bitPresentInOld,
				bitPresentInNew,
				bitCompared,
				bitDifferent
			};
		public:
			Info () {}
			Info (bool isFolder)
			{
				_bits.set (bitFolder, isFolder);
			}
			bool IsFolder () const { return _bits.test (bitFolder); }
			bool IsDrive () const { return _bits.test (bitDrive); }
			bool IsPresentInOld () const { return _bits.test (bitPresentInOld); }
			bool IsPresentInNew () const { return _bits.test (bitPresentInNew); }
			bool IsCompared () const { return _bits.test (bitCompared); }
			bool IsDifferent () const { return _bits.test (bitDifferent); }

			void SetDrive (bool value = true) { _bits.set (bitDrive, value); }
			void SetCompared (bool value = true)
			{
				_bits.set (bitCompared, value);
			}
			void SetDifferent (bool value = true)
			{
				_bits.set (bitDifferent, value);
				SetCompared ();
			}
			void SetPresentInNew (bool value = true)
			{
				_bits.set (bitPresentInNew, value);
			}
			void SetPresentInOld (bool value = true)
			{
				_bits.set (bitPresentInOld, value);
			}
			// returns true if changed
			bool Merge (Info const & info);
		private:
			std::bitset<32> _bits;
		};
	public:
		Item () : _diffState (stateInvalid) {}
		Item (std::string const & name, Info info)
			: _name (name), _info (info)
		{
			SetDiffState ();
		}
		Info & GetInfo () { return _info; }
		Info GetInfo () const { return _info; }
		bool IsFolder () const { return _info.IsFolder (); }
		bool IsDrive () const { return _info.IsDrive (); }
		
		std::string const & Name () const { return _name; }
		bool Merge (Item const & item)
		{
			if (_info.Merge (item._info))
			{
				SetDiffState ();
				return true;
			}
			return false;
		}
		void SetDiffState () const
		{
			if (!_info.IsPresentInOld () && _info.IsPresentInNew ())
				_diffState = stateNew;
			else if (_info.IsPresentInOld () && !_info.IsPresentInNew ())
				_diffState = stateDel;
			else if (_info.IsPresentInOld () && _info.IsPresentInNew ())
			{
				if (_info.IsCompared ())
					_diffState = _info.IsDifferent ()? stateDiff: stateSame;
				else
					_diffState = stateSame;
			}
		}
		DiffState GetDiffState () const { return _diffState; }
		bool operator < (Item const & item) const
		{
			if (IsFolder ())
				return !item.IsFolder () || IsNocaseLess (_name, item._name);
			else
				return !item.IsFolder () && IsNocaseLess (_name, item._name);
		}
	private:
		std::string			_name;
		Data::Item::Info	_info;
		mutable DiffState	_diffState;
	};

	// A set of items
	class Chunk
	{
	public:
		typedef std::set<Item>::const_iterator const_iterator;
		typedef std::set<Item>::iterator iterator;
	public:
		unsigned size () const { return _items.size (); }
		bool empty () const { return _items.empty (); }
		iterator begin () { return _items.begin (); }
		iterator end () { return _items.end (); }
		const_iterator begin () const { return _items.begin (); }
		const_iterator end () const { return _items.end (); }
		void Add (std::string const & name, Item::Info info) 
		{
			_items.insert (Item (name, info));
		}
		void AddItem (Item const & item)
		{
			_items.insert (item);
		}
		void SetPresentInNew ();
		void SetPresentInOld ();
		// RefCounted
		void Acquire () const { _refCount.Inc (); }
		long Release () const { return _refCount.Dec (); }
		// Debugging
		bool Contains (Chunk const & subChunk) const;
	private:
		std::set<Item>		_items;
		Win::AtomicCounter  mutable _refCount;
	};

	// A smart (ref-counting) auto chunk
	class ChunkHolder
	{
	public:
		ChunkHolder (Chunk * chunk = 0)
			: _chunk (chunk)
		{
			if (_chunk)
				_chunk->Acquire ();
		}
		ChunkHolder (ChunkHolder const & src)
		{
			if (!src.Empty ())
				src._chunk->Acquire ();
			_chunk = src._chunk;
		}
		~ChunkHolder ()
		{
			Clear ();
		}
		Chunk * Get () { return _chunk; }
		Chunk const * Get () const { return _chunk; }
		Chunk const & operator * () const { return *_chunk; }
		void Clear ()
		{
			// Race condition?
			if (_chunk != 0)
			{
				if (_chunk->Release () == 0)
					delete _chunk;
			}
			_chunk = 0;
		}
		bool Empty () const { return _chunk == 0; }
		void Reset (Chunk * chunk)
		{
			Assert (chunk != 0);
			Assert (chunk != _chunk);
			chunk->Acquire ();
			Clear ();
			_chunk = chunk;
		}
		ChunkHolder & operator = (ChunkHolder const & src)
		{
			Assert (&src != this);
			Clear ();
			if (src._chunk != 0)
				src._chunk->Acquire ();
			_chunk = src._chunk;
			return *this;
		}
		Chunk * operator-> () { return _chunk; }
	private:
		Chunk * _chunk;
	};

	// A vector of ref-counted chunks
	typedef std::vector<ChunkHolder> ChunkStore;
	typedef std::vector<ChunkHolder>::iterator ChunkIter;
	typedef std::vector<ChunkHolder>::const_iterator ConstChunkIter;

	// Buffers up chunks for batch processing.
	// Call EndBatch when you're done processing current batch.
	class ChunkBuffer
	{
	public:
		void Push (Data::ChunkHolder data)
		{
			_nextBatch.push_back (data); 
		}
		void EndBatch ()
		{
			_currentBatch.swap (_nextBatch);
			_nextBatch.clear ();
		}
		void Clear ()
		{
			_currentBatch.clear ();
			_nextBatch.clear ();
		}
		bool IsDone () const { return _nextBatch.empty (); }
		Data::ChunkIter begin () { return _currentBatch.begin (); }
		Data::ChunkIter end () { return _currentBatch.end (); }
	private:
		Data::ChunkStore	_currentBatch;
		Data::ChunkStore	_nextBatch;
	};

	// Does not accumulate chunks for processing
	// Only stores the latest chunk
	class SingleChunkBuffer
	{
	public:
		// when next data is a superset of previous data
		void Replace (Data::ChunkHolder data)
		{
			_next = data;
		}
		void EndBatch ()
		{
			_current = _next;
			_next.Clear ();
		}
		void Clear ()
		{
			_current.Clear ();
			_next.Clear ();
		}
		bool Empty () const { return _current.Get () == 0; }
		Data::Chunk * GetChunk () { return _current.Get (); }
		bool IsDone () const { return _next.Get () == 0; }
	private:
		Data::ChunkHolder	_current;
		Data::ChunkHolder	_next;
	};


	// The processor and a store for items--used as a sink for a merger
	// The base class simply stores items in a chunk
	class Processor
	{
	public:
		virtual ~Processor () {}
		// return true if can accomodate more
		virtual bool Put (Data::Item & item)
		{
			_result->AddItem (item);
			return true;
		}
		Data::ChunkHolder GetResult ()
		{
			return _result;
		}
		void Reload ()
		{
			_result.Reset (new Data::Chunk);
		}
	protected:
		Data::ChunkHolder _result;
	};

	// Merge a set of ordered chunks
	// Chunks that are equal are merged
	// The results are stored in the processor
	// If the processor has finite capacity, repeat Do until merger Is Done
	class Merger
	{
		// Sequences in parallel through a set of ordered source chunks
		class Sequencer
		{
			typedef std::pair<Data::Chunk::iterator, Data::Chunk::iterator> Range;
			typedef std::list<Range> RangeList;
		public:
			Sequencer () : _lowest (0) {}
			bool Empty () const { return _sources.empty (); }
			void Clear ()
			{
				_sources.clear ();
				_lowest.clear ();
			}
			void AddSource (Data::Chunk * chunk);
			void Start ()
			{
				Advance ();
			}
			// return true if there's more
			bool Done () const
			{
				return _lowest.empty ();
			}
			void Advance ();
			Data::Item & Get ()
			{
				return _item;
			}
		private:
			// set of sources: iterators pointing to beginnings and ends of chunks
			RangeList _sources;
			// list of ptrs to iterators pointing to the items equal to the lowest item
			// we need pointers, because we have to increment the actual iterators
			std::vector<Data::Chunk::iterator *> _lowest;
			Data::Item _item;
		};
	public:
		Merger (Data::Processor & processor) : _processor (processor) {}
		void AddSource (Data::Chunk * chunk) { _sequencer.AddSource (chunk); }
		void AddSources (Data::ChunkIter begin, Data::ChunkIter end);
		// return true if not empty
		bool Start ()
		{
			if (_sequencer.Empty ())
				return false;
			_sequencer.Start ();
			return true;
		}
		void Do ();
		bool IsDone () const { return _sequencer.Done (); }
		void Clear () { _sequencer.Clear (); }
	private:
		Data::Processor & _processor;
		Sequencer _sequencer;
	};

}

inline std::ostream& operator << (std::ostream& out, Data::Item::Info info)
{
	out << (info.IsPresentInOld ()
		? (info.IsPresentInNew ()? "on" : "o")
		: (info.IsPresentInNew ()? "n" : "-"));
	return out;
}

inline std::ostream& operator << (std::ostream& out, Data::Item const & item)
{
	return out << item.Name () << " " << item.GetInfo () << " ";
}

inline std::ostream& operator<< (std::ostream& out, Data::Chunk const & chunk)
{
	if (chunk.empty ())
		return out << "empty";
	else
	{
		for (Data::Chunk::const_iterator it = chunk.begin (); it != chunk.end (); ++it)
		{
			out << " | " << *it << std::endl;
		}
	}
	return out;
}

inline std::ostream& operator<< (std::ostream& out, Data::ChunkHolder const & chunk)
{
	return out << *chunk;
}

inline std::ostream& operator<< (std::ostream& out, Data::Merger const & merger)
{
	return out;// << merger.GetComment ();
}

#endif
