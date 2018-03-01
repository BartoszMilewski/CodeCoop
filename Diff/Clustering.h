#if !defined (CLUSTERING_H)
#define CLUSTERING_H
//----------------------------------
// (c) Reliable Software 1998 - 2007
//----------------------------------

#include "Cluster.h"
#include <unmovable_vector.h>

class Line;
class LineBuf;
namespace Progress { class Meter; }

class CluSeq
{
	typedef std::vector<Cluster *>::const_iterator Iter;
public:
	CluSeq (Iter begin, Iter end)
		: _cur (begin), _end (end)
	{}
	bool AtEnd () const { return _cur == _end; }
	void Advance () { ++_cur; }
	Cluster * Get () const { return *_cur; }
private:
	Iter _cur;
	Iter _end;
};

// Clusters a buffer of lines
class Comparator;

// Stores clusters by value, never reallocates, so pointers to cluster
// are never invalidated
typedef unmovable_vector<Cluster, 1000> ClusterOwner;

class Clusterer
{
public:
	Clusterer (LineBuf & linesOld, LineBuf & linesNew);
	void PickUniqueClusters (Progress::Meter & meter, std::vector<bool>  & badLines, Comparator & comp);
	int  GetClusterCount () const { return _finalClusters.size (); }

	Cluster * AddCluster (Cluster const & cluster) 
	{ 
		
		Cluster * clu = _allClusters.push_back (cluster);
		_finalClusters.push_back (clu);
		return clu;
	}
	CluSeq GetClusterSeq () const 
	{
		return CluSeq (_finalClusters.begin (), _finalClusters.end ());
	}
	std::vector<Cluster *>::const_iterator begin ()
	{
		return _finalClusters.begin ();
	}
	std::vector<Cluster *>::const_iterator end ()
	{
		return _finalClusters.end ();
	}
private:
	void DoLine (int lineNo, bool isBad);
	void TryExtendUpClusters (std::vector<bool> const & badLines, Comparator const & comp);

	void AcceptCluster (Cluster * cluster);
	void AcceptLineNew (Line * lineNew, int newLineNo, Cluster * cluster);
	void AcceptLineOld (Line * lineOld, int oldLineNo, Cluster * cluster);
	void FindAdded (Progress::Meter & meter);
	void FindDeleted (Progress::Meter & meter);

	LineBuf 	  & _linesOld;
	LineBuf 	  & _linesNew;
	ClusterOwner	_allClusters;

	std::vector<Cluster *> _finalClusters;
};

//
// Sorts list of clusters by New or old line numbers
//
class ClusterSort: public std::vector<Cluster const *>
{
public:

	ClusterSort (CluSeq cluSeq);
	ClusterSort (std::vector<Cluster> const & clusters);
	int  Count () const { return _count; }
	void SortByNewLineNo ();
	void SortByOldLineNo ();
	Cluster const * GetCur ()
	{
		if (_cur < _count)
			return (*this) [_cur];
		else
			return 0;
	}
	void Advance () { _cur++; }
private:
	int _count;
	int _cur;
};

class ClusterHeap
{
public:
	ClusterHeap (ClusterOwner & cluOwner);
	Cluster * PopBest ();
	void MakeHeap ();
private:
	CluIter 			_begin;
	CluIter 			_end;
	std::vector<Cluster *>	_vector;
};

#endif
