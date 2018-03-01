#if !defined (CLULINES_H)
#define CLULINES_H
//-----------------------------------
// (c) Reliable Software 1998 -- 2002
//-----------------------------------

#include "Cluster.h"

#include <Dbg/Assert.h>

class LineCounter;

class SimpleLine
{
public:
	SimpleLine (char const * buf, size_t len = 0)
		: _buf (buf), _len (len)
	{}
	void Init (char const * buf, size_t len)
	{
		_buf = buf;
		_len = len;
	}
	void SetLen (size_t len)
	{
		_len = len;
	}
	int Len () const { return _len; }
	char const * Buf () const { return _buf; }
protected:
	size_t			_len;
	char const	  * _buf;
};

class Line: public SimpleLine
{
public:
	Line (char const * buf)
		: SimpleLine (buf), _cluster (0)
	{}
	void SetLen (size_t len);
	void AddSimilar (int dist)
	{
		// They are supposed to be sorted
		Assert (_similar.size () == 0 || _similar.back () < dist);
		_similar.push_back (dist);
	}
	// No ownership transfer!
	void PushCluster (Cluster * cluster)
	{
		_clusterList.push_back (cluster);
	}

	std::vector<int> const & GetShifts ()  const { return _similar; }
	void SwapShifts (std::vector<int> & shifts) { shifts.swap (_similar); }
	std::vector<Cluster *> & GetClusters () { return _clusterList; }

	void SetFinalCluster (Cluster const * cluster) { _cluster = cluster; }
	Cluster const * GetFinalCluster () const { return _cluster; }

private:
	// array of shifts (New line # - old line #)
	std::vector<int> 	   _similar;
	// corresponding array of clusters
	std::vector<Cluster *> _clusterList;
	Cluster const		 * _cluster;
};

class Comparator
{
public:
	virtual ~Comparator () {}

	virtual bool IsSimilar (Line const * line1, Line const * line2) const = 0;
};

class NullComparator: public Comparator
{
public:
	bool IsSimilar (Line const * line1, Line const * line2) const;
};

class FuzzyComparator: public Comparator
{
public:
	bool IsSimilar (Line const * line1, Line const * line2) const;
};

class StrictComparator: public Comparator
{
public:
	bool IsSimilar (Line const * line1, Line const * line2) const;
};

#endif
