// (c) Reliable Software 2003
#include "precompiled.h"
#include "DataChunk.h"

namespace Data
{
	bool Item::Info::Merge (Info const & info)
	{
		if (_bits != info._bits)
		{
			_bits |= info._bits;
			return true;
		}
		return false;
	}

	bool Chunk::Contains (Chunk const & subChunk) const
	{
		for (const_iterator it = subChunk.begin (); it != subChunk.end (); ++it)
			if (_items.find (*it) == _items.end ())
				return false;
		return true;
	}

	void Chunk::SetPresentInOld ()
	{
		for (iterator it = begin (); it != end (); ++it)
		{
			it->GetInfo ().SetPresentInOld ();
			it->SetDiffState ();
		}
	}

	void Chunk::SetPresentInNew ()
	{
		for (iterator it = begin (); it != end (); ++it)
		{
			it->GetInfo ().SetPresentInNew ();
			it->SetDiffState ();
		}
	}

	void Merger::Sequencer::AddSource (Data::Chunk * chunk)
	{
		if (chunk != 0 && !chunk->empty ())
		{
			_sources.push_back (std::make_pair (chunk->begin (), chunk->end ()));
		}
	}

	void Merger::Sequencer::Advance ()
	{
		_lowest.clear ();
		// Iterate over sources to find the lowest items (all equal)
		RangeList::iterator it = _sources.begin ();
		while (it != _sources.end ())
		{
			// Remove empty sources
			if (it->first == it->second)
			{
				it = _sources.erase (it);
				continue;
			}

			if (_lowest.empty ()) // first candidate
			{
				_lowest.push_back (&it->first);
			}
			else if (*it->first < **_lowest.back ()) // found lower
			{
				_lowest.clear ();
				_lowest.push_back (&it->first);
			}
			else if (!(**_lowest.back () < *it->first)) // found equal
			{
				_lowest.push_back (&it->first);
			}
			++it;
		}

		if (!_lowest.empty ())
		{
			// Merge the lowest items
			std::vector<Data::Chunk::iterator *>::iterator it = _lowest.begin ();
			Data::Chunk::iterator * pChunkIter = *it;
			_item = **pChunkIter;
			++(*pChunkIter);
			++it;
			while (it != _lowest.end ())
			{
				pChunkIter = *it;
				_item.Merge (**pChunkIter);
				++(*pChunkIter);
				++it;
			}
		}
	}

	void Merger::AddSources (Data::ChunkIter begin, Data::ChunkIter end)
	{
		for (Data::ChunkIter it = begin; it != end; ++it)
		{
			_sequencer.AddSource (it->Get ());
		}
	}

	void Merger::Do ()
	{
		if (_sequencer.Done ())
			return;
		_processor.Reload ();
		do
		{
			bool keepWorking = _processor.Put (_sequencer.Get ());
			_sequencer.Advance ();
			if (!keepWorking)
				break;
		} while (!_sequencer.Done ());
	}
}