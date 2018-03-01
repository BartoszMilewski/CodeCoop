#if !defined (CLUSTER_H)
#define CLUSTER_H
//------------------------------------
// (c) Reliable Software 1997
//------------------------------------

// A cluster of lines that were moved together
class Cluster
{
public:

	Cluster (): _len (0) {}
	Cluster (int oldLineNo, int newLineNo, int len = 1)
		: _oldLineNo (oldLineNo), _newLineNo (newLineNo), _len (len)
	{}
	void Extend () { _len++; }
	void ExtendUp ();
	int Len () const { return _len; }
	bool IsEmpty () const { return _len == 0; }
	int NewLineNo () const { return _newLineNo; }
	int OldLineNo () const { return _oldLineNo; }
	int Shift () const { return _newLineNo - _oldLineNo; }
	bool BetterThan (Cluster * other) const
	{
		return Len () > other->Len ();
	}
	bool IsNewIn (int newLineNo) const 
	{ 
		return newLineNo >= _newLineNo && newLineNo < _newLineNo + _len; 
	}
	bool IsOldIn (int oldLineNo) const 
	{ 
		return oldLineNo >= _oldLineNo && oldLineNo < _oldLineNo + _len; 
	}
	void ShortenFromFront (int len)
	{
		if (len >= _len)
			_len = 0;
		else
		{
			_newLineNo += len;
			_oldLineNo += len;
			_len -= len;
		}
	}
	void ShortenFromEnd (int len)
	{
		_len -= len;
	}
	Cluster SplitCluster (int offset);
private:
	int _oldLineNo;
	int _newLineNo;
	int _len;
};

// Iterator

typedef std::vector<Cluster *>::iterator CluIter;

// Functors

class CmpOldLines: public std::binary_function<Cluster const *, Cluster const *, bool>
{
public:
	bool operator () (Cluster const * const & clu1, Cluster const * const & clu2) const
	{
		return clu1->OldLineNo () < clu2->OldLineNo ();
	}
};

class CmpNewLines: public std::binary_function<Cluster const *, Cluster const *, bool>
{
public:
	bool operator () (Cluster const * const & clu1, Cluster const * const & clu2) const
	{
		return clu1->NewLineNo () < clu2->NewLineNo ();
	}
};

class CmpLength: public std::binary_function<Cluster const *, Cluster const *, bool>
{
public:
	bool operator () (Cluster const * const & clu1, Cluster const * const & clu2) const
	{
		return clu1->Len () < clu2->Len ();
	}
};

class NotAdded: public std::unary_function<Cluster const *, bool>
{
public:
	bool operator () (Cluster * clu) const 
	{ 
		return clu->OldLineNo () != -1; 
	}
};

class NotZeroLength: public std::unary_function<Cluster const *, bool>
{
public:
	bool operator () (Cluster * clu) const 
	{ 
		return clu->Len () != 0; 
	}
};

#endif
