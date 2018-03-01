#if !defined (MULITMAP_H)
#define MULITMAP_H
//---------------------------
// (c) Reliable Software 2007
//---------------------------

template<class ItemMap>
class MmRangeSequencer
{
public:
	typedef typename ItemMap::mapped_type ValueType;
	typedef typename ItemMap::const_iterator MmIter;

	MmRangeSequencer (std::pair<MmIter, MmIter> range)
		: _cur (range.first),
		  _end (range.second)
	{}

	bool AtEnd () const { return _cur == _end; }
	void Advance () { ++_cur; }

	ValueType Get () const { return _cur->second; }

private:
	MmIter	_cur;
	MmIter	_end;
};

template<class ItemMap>
class MmCountRangeSequencer: public MmRangeSequencer<ItemMap>
{
public:
	MmCountRangeSequencer (std::pair<MmIter, MmIter> range)
		: MmRangeSequencer<ItemMap> (range),
		  _count (std::distance (range.first, range.second))
	{}
	unsigned Count () const { return _count; }
private:
	unsigned	_count;
};

#endif
