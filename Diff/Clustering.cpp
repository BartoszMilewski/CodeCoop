//----------------------------------
// (c) Reliable Software 1998 - 2007
//----------------------------------

#include "precompiled.h"
#include "Clustering.h"
#include "CluLines.h"
#include "LineBuf.h"

#include <Ctrl/ProgressMeter.h>

Clusterer::Clusterer (LineBuf & linesOld, LineBuf & linesNew)
	: _linesOld (linesOld),
	  _linesNew (linesNew)
{}

void Clusterer::PickUniqueClusters (Progress::Meter & meter, std::vector<bool> & badLines, Comparator & comp)
{
	// If using progress meter, preserve previous range
	// because we will many times set New range.
	int oldMin, oldMax, oldStep;
	meter.GetRange (oldMin, oldMax, oldStep);
	// Iterate over lines from New file
	// and cluster them by shifts
	int count = _linesNew.Count ();
	meter.SetRange (0, count);
	meter.SetActivity ("Finding changed lines");
	for (int i = 0; i < count; i++)
	{
		meter.StepAndCheck ();
		DoLine (i, badLines [i]);
	}
	// maybe some clusters can be extetended up 
	if (std::find (badLines.begin (), badLines.end (), true) != badLines.end ())
		TryExtendUpClusters (badLines, comp);
	// Sort by cluster length
	meter.SetActivity ("Sorting and processing changes");
	int clusterCount = _allClusters.size ();
	// We can have really huge cluster count.
	// To display meanigfull progress feedback we have
	// to compress the range a little bit.
	// Set range so the initial heap creation accounts for 10% progress
	int maxi = clusterCount / 1024;
	if (maxi == 0)
	{
		maxi = clusterCount;
	}
	meter.SetRange (maxi / 16, maxi);

	ClusterHeap clusterHeap (_allClusters);
	
	// Pick the longest cluster and process it
	for (Cluster * cluster = clusterHeap.PopBest (); 
		 cluster != 0; 
		 cluster = clusterHeap.PopBest ())
	{
		meter.StepAndCheck ();
		// Shrink all clusters that overlap with
		// this one
		AcceptCluster (cluster);
		_finalClusters.push_back (cluster);
		// Have to reheap because AcceptCluster spoiled the ordering
		clusterHeap.MakeHeap ();
	}
	// Each line of the New and old file is now in some unique cluster
	// except for those lines that were deleted or added
	FindAdded (meter);
	FindDeleted (meter);
	// Now we have all lines uniquely clustered
	meter.SetRange (oldMin, oldMax, oldStep);
	meter.SetActivity ("");
}

void Clusterer::DoLine (int lineNo, bool isBad)
{
	Line * line = _linesNew.GetLine (lineNo);
	// Current line's list of shifts (distances to similar lines)
	if (line->GetShifts ().size () == 0)
		return;
	std::vector<int> shifts;
	line->SwapShifts (shifts);
	// Current line's list of clusters -- to be filled
	std::vector<Cluster *> & clusters = line->GetClusters ();
	Assert (clusters.size () == 0);
	clusters.reserve (shifts.size ());

	// Previous line's (if any) list of clusters
	std::vector<Cluster *> empty;
	std::vector<Cluster *> & prevClusters = (lineNo > 0)?
		_linesNew.GetLine (lineNo - 1)->GetClusters () : empty;

	//----------------------------------------------------------------
	// Go in parallel over the list of current shifts and previous
	// clusters (both are sorted!). Shifts that don't match any of the
	// previous clusters are NEW clusters introduced by this line
	// We add them to the ClusterOwner sink.
	// Shifts that match a previous cluster EXTEND that cluster by
	// one more line.
	//----------------------------------------------------------------

	typedef std::vector<int>::const_iterator ShiftIter;

	ShiftIter shit = shifts.begin ();
	CluIter clit = prevClusters.begin ();
	while (shit != shifts.end ())
	{
		Cluster * cluster = 0;
		int shift = *shit;

		if (clit == prevClusters.end () || shift < (*clit)->Shift ())
		{
			// This is a New cluster
			if (isBad)
			{
				// optimalisation: a "bad" line can't start the cluster (at this moment)
				++shit;
				continue;
			}
			Cluster clu (lineNo - shift, lineNo);
			cluster = _allClusters.push_back (clu);
			// next shift in current line
			++shit;
		}
		else if (shift == (*clit)->Shift ())
		{	
			// this is a continuation of the same cluster
			cluster = *clit;
			cluster->Extend ();
			++shit;
			++clit;
		}
		else
		{
			// shift > cluShift && clit != prevClusters.end ()
			// skip this cluster
			++clit;
			continue;
		}
	
		// We have a cluster (make sure they are sorted)
		Assert (cluster != 0);
		Assert (clusters.size () == 0 || clusters.back ()->Shift () < cluster->Shift ());
		// Add it to current line
		clusters.push_back (cluster);
		// Find the corresponding old line and add it to it too
		int oldLineNo = cluster->OldLineNo () + (lineNo - cluster->NewLineNo ());
		Line * oldLine = _linesOld.GetLine (oldLineNo);
		oldLine->PushCluster (cluster);				
	}
}

void Clusterer::FindAdded (Progress::Meter & meter)
{
	int count = _linesNew.Count ();
	meter.SetActivity ("Finding added lines");
	meter.SetRange (0, count);
	Cluster * cluster = 0;
	for (int i = 0; i < count; i++)
	{
		meter.StepAndCheck ();
		Line * line = _linesNew.GetLine (i);
		if (line->GetFinalCluster () == 0)
		{
			if (cluster == 0)
			{
				Cluster clu (-1, i);
				cluster = AddCluster (clu);
			}
			else
			{
				cluster->Extend ();
			}
			line->SetFinalCluster (cluster);
		}
		else
			cluster = 0;
	}
}

void Clusterer::FindDeleted (Progress::Meter & meter)
{
	int count = _linesOld.Count ();
	meter.SetActivity ("Finding deleted lines");
	meter.SetRange (0, count);
	Cluster * cluster = 0;
	for (int i = 0; i < count; i++)
	{
		meter.StepAndCheck ();
		Line * line = _linesOld.GetLine (i);
		if (line->GetFinalCluster () == 0)
		{
			if (cluster == 0)
			{
				Cluster clu (i, -1);
				cluster = AddCluster (clu);
			}
			else
			{
				cluster->Extend ();
			}
			line->SetFinalCluster (cluster);
		}
		else
			cluster = 0;
	}
}

// Assign given cluster to the lines of the New file
// that are in it. Reduce length of all the other clusters
// that overlap with it.
void Clusterer::AcceptCluster (Cluster * cluster)
{
	int newLineNo = cluster->NewLineNo ();
	int oldLineNo = cluster->OldLineNo ();
	int len = cluster->Len ();
	for (int i = 0; i < len; i++)
	{
		Line * lineNew = _linesNew.GetLine (newLineNo + i);
		AcceptLineNew (lineNew, newLineNo + i, cluster);
		Line * lineOld = _linesOld.GetLine (oldLineNo + i);
		AcceptLineOld (lineOld, oldLineNo + i, cluster);
	}
}

void Clusterer::AcceptLineNew (Line * lineNew, int newLineNo, Cluster * cluster)
{
	std::vector<Cluster *> & clusters = lineNew->GetClusters ();
	for (CluIter it = clusters.begin (); it != clusters.end (); ++it)
	{
		Cluster * cluOther = *it;
		if (cluOther != cluster && cluOther->IsNewIn (newLineNo))
		{
			// Shorten the other cluster
			if (cluOther->NewLineNo () < cluster->NewLineNo ())
				cluOther->ShortenFromEnd (cluOther->NewLineNo () + cluOther->Len () - newLineNo);
			else
				cluOther->ShortenFromFront (cluster->NewLineNo () + cluster->Len () - newLineNo);
		}
	}
	lineNew->SetFinalCluster (cluster);
}

void Clusterer::AcceptLineOld (Line * lineOld, int oldLineNo, Cluster * cluster)
{
	std::vector<Cluster *> & clusters = lineOld->GetClusters ();
	for (CluIter it = clusters.begin (); it != clusters.end (); ++it)
	{
		Cluster * cluOther = *it;
		if (cluOther != cluster && cluOther->IsOldIn (oldLineNo))
		{
			// Shorten the other cluster
			if (cluOther->OldLineNo () < cluster->OldLineNo ())
				cluOther->ShortenFromEnd (cluOther->OldLineNo () + cluOther->Len () - oldLineNo);
			else
				cluOther->ShortenFromFront (cluster->OldLineNo () + cluster->Len () - oldLineNo);
		}
	}
	lineOld->SetFinalCluster (cluster);
}

void Clusterer::TryExtendUpClusters (std::vector<bool> const & badLines, Comparator const & comp)
{
	for (ClusterOwner::iterator it = _allClusters.begin (); it != _allClusters.end (); ++it)
	{
		if (it->NewLineNo () > 0 && it->OldLineNo () > 0 && badLines [it->NewLineNo () - 1])
		{
			for (int k = it->NewLineNo () - 1, i = it->OldLineNo () - 1; k >= 0 && i >= 0; --k, --i) 
			{
				Line * lineNew = _linesNew.GetLine (k);
				Line * lineOld = _linesOld.GetLine (i);
				if (comp.IsSimilar (lineNew, lineOld))
				{
					it->ExtendUp ();
					Cluster * cluster = &(*it);
					lineNew->PushCluster (cluster);
					lineOld->PushCluster (cluster);
				}
				else
					break;
			}
		}
	}
}

//
// Cluster Sort
//

//--------------------------------------------------------
// Back inserter iterator that converts values to pointers
// The iterator is called with values of type T
// while the container accepts pointers to T
//--------------------------------------------------------
template<class T, class Cont>
class ValToPtrInserter: public std::iterator<std::output_iterator_tag, T>
{
public:
	explicit ValToPtrInserter (Cont & cont) : _cont (&cont) {}
	ValToPtrInserter& operator= (T const & val)
	{
		_cont->push_back (&val);
		return *this;
	}
	ValToPtrInserter & operator++ () { return *this; }
	ValToPtrInserter operator++ (int) { return *this; }
	ValToPtrInserter & operator * () { return *this; }
private:
	Cont * _cont;
};


ClusterSort::ClusterSort (std::vector<Cluster> const & clusters)
	: _count (clusters.size ()),
	  _cur (0)
{
	reserve (_count);
	std::copy (clusters.begin (), clusters.end (), ValToPtrInserter<Cluster, ClusterSort> (*this));
}

ClusterSort::ClusterSort (CluSeq cluSeq)
	: _count (0),
	  _cur (0)
{
	for (; !cluSeq.AtEnd (); cluSeq.Advance ())
	{
		push_back (cluSeq.Get ());
		++_count;
	}
}


void ClusterSort::SortByNewLineNo ()
{
	std::sort (begin (), end (), CmpNewLines ());
}

void ClusterSort::SortByOldLineNo ()
{
	std::sort (begin (), end (), CmpOldLines ());
}

//
// Cluster Heap
//

ClusterHeap::ClusterHeap (ClusterOwner & cluOwner)
{
	int count = cluOwner.size ();
	_vector.reserve (count);

	ClusterOwner::iterator it = cluOwner.begin ();
	ClusterOwner::iterator end = cluOwner.end ();
	for (int i = 0; it != end; ++it, ++i)
	{
		Assert (i < count);
		Cluster * clu = &(*it);
		_vector.push_back (clu);
	}
	_begin = _vector.begin ();
	_end = _vector.end ();
	MakeHeap ();
}

void ClusterHeap::MakeHeap ()
{
	std::make_heap (_begin, _end, CmpLength ());
	// remove zero length clusters
	typedef std::vector<Cluster *>::reverse_iterator RevIter;
	RevIter begin (_end);
	RevIter end (_begin);
	RevIter found = std::find_if (begin, end, NotZeroLength ());
	// convert to straight iterator
	_end = found.base ();
}

Cluster * ClusterHeap::PopBest ()
{
	if (_begin == _end || (*_begin)->Len () == 0)
		return 0;

	return *_begin++;
}

#if !defined (NDEBUG)
void ClusterOwnerTest ()
{
	{
		unmovable_vector<Cluster, 2> owner;
		Assert (!(owner.begin () != owner.end ()));

		Cluster clu0 (0, 0);
		// 1
		Cluster * pClu = owner.push_back (clu0);
		Assert (pClu->OldLineNo () == clu0.OldLineNo () && pClu->NewLineNo () == clu0.NewLineNo ());
		unmovable_vector<Cluster, 2>::iterator it = owner.begin ();
		unmovable_vector<Cluster, 2>::iterator end = owner.end ();
		Assert (it != end);
		++it;
		Assert (!(it != end));

		// 2
		Cluster clu1 (1, 1);
		pClu = owner.push_back (clu1);
		Assert (pClu->OldLineNo () == clu1.OldLineNo () && pClu->NewLineNo () == clu1.NewLineNo ());
		it = owner.begin ();
		end = owner.end ();
		Assert (it != end);
		++it;
		Assert (it != end);
		++it;
		Assert (!(it != end));

		// 3
		Cluster clu2 (2, 2);
		pClu = owner.push_back (clu2);
		Assert (pClu->OldLineNo () == clu2.OldLineNo () && pClu->NewLineNo () == clu2.NewLineNo ());
		it = owner.begin (); // 0
		end = owner.end ();
		Assert (it != end);
		++it; // 1
		Assert (it != end);
		++it; // 2
		Assert (it != end);
		++it; // end
		Assert (!(it != end));
	}
	unmovable_vector<Cluster, 2> owner;
	for (int i = 0; i < 1024; ++i)
	{
		Cluster clu (i, i);
		owner.push_back (clu);
	}
	int n = 0;
	unmovable_vector<Cluster, 2>::iterator it = owner.begin ();
	unmovable_vector<Cluster, 2>::iterator end = owner.end ();
	while (it != end)
	{
		Assert (n < 1024);
		Cluster & c = *it;
		Assert (c.OldLineNo () == n);
		Assert (c.NewLineNo () == n);
		++n;
		++it;
	}
	Assert (n == 1024);
}
#endif
