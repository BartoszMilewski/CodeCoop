//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "precompiled.h"
#include "Diff.h"
#include "DiffRecord.h"
#include "CluSum.h"
#include "LineCounter.h"
#include "ListingWin.h"

#include <Ctrl/ProgressMeter.h>
#include <File/MemFile.h>
#include <Ex/WinEx.h>
#include <HashTable.h>

int MakePrimish (int x)
{
	// Note: Mersenne primes are numbers that
	// are one less than 2 to the power of a prime number.
	// They are not always primes and here
	// we don't even raise 2 to a prime power, but so what!
	// It's good enough for a primish number
	int i;
	for (i = 0; i < sizeof (int) * 8; i++)
	{
		if (x)
			x >>= 1;
		else
			break;
	}
	return (1 << i) - 1; 
}

Differ::Differ (char const * bufOld, 
				int lenOld, 
				char const * bufNew, 
				int lenNew, 
				Comparator & comp, 
				Progress::Meter & meter,
				EditStyle::Source src)
	: _linesOld (bufOld, lenOld), 
	  _linesNew (bufNew, lenNew), 
	  _clusterer (_linesOld, _linesNew),
	  _comp (comp),
	  _lenOld (lenOld),
	  _lenNew (lenNew),
	  _src (src),
	  _iStart (0),
	  _iTail (0),
	  _badLines (_linesNew.Count (), false)
{
	int oldCount = _linesOld.Count ();
	int newCount = _linesNew.Count ();	
	// skip initial similar lines
	int iEnd = std::min (oldCount, newCount);
	while (_iStart < iEnd)
	{
		Line * line1 = _linesOld.GetLine (_iStart);
		Line * line2 = _linesNew.GetLine (_iStart);
		if (!comp.IsSimilar (line1, line2))
			break;
		line2->AddSimilar (0);
		++_iStart;
	}
	// skip trailing similar lines
	while (oldCount - _iTail > _iStart && newCount - _iTail > _iStart)
	{
		Line * line1 = _linesOld.GetLine (oldCount - _iTail - 1);
		Line * line2 = _linesNew.GetLine (newCount - _iTail - 1);
		if (!comp.IsSimilar (line1, line2))
			break;
		line2->AddSimilar (newCount - oldCount);
		++_iTail;
	}
	CompareLines (oldCount, newCount, comp, meter);
	if (newCount > THRESHOLD_LINE_COUNT)
		PrepareOptimisation ();
	// do the clustering
	_clusterer.PickUniqueClusters (meter, _badLines, comp);
}

void Differ::CompareLines (int oldCount, int newCount, Comparator & comp, Progress::Meter & meter)
{
	int size = 1;
	if (oldCount - _iStart - _iTail > 0)
		size = MakePrimish (oldCount - _iStart - _iTail);
	// hash all the lines of the old file
	Hash::Table hTable (size);
	for (int i = _iStart; i < oldCount - _iTail; ++i)
	{
		Line * line = _linesOld.GetLine (i);
		hTable.Add (i, line->Buf (), line->Len ());
	}
	// find old lines similar to New lines
	meter.SetActivity ("Finding similar lines");
	meter.SetRange (0, newCount - _iTail - _iStart, 1);
	for (int j = _iStart; j < newCount - _iTail; ++j)
	{
		Line * lineNew = _linesNew.GetLine (j);
		using Hash::ShortIter;
		for (ShortIter it (hTable, lineNew->Buf (), lineNew->Len ());
			 !it.AtEnd ();
			 it.Advance ())
		{						
			if (newCount > CRITICAL_LINE_COUNT)
			{
				// optimisation for larges files:
				// Above CRITICAL_LINE_COUNT lines, the search simimilar lines is limited to |shift| <= CRITICAL_SHIFT
				if (std::abs (j - it.GetValue ()) > CRITICAL_SHIFT)
					continue;
			}
			// it.GetValue () returns line number
			Line * lineOld = _linesOld.GetLine (it.GetValue ());
			if (comp.IsSimilar (lineOld, lineNew))
			{
				// distance to similar line
				lineNew->AddSimilar (j - it.GetValue ());
			}
		}
		meter.StepAndCheck ();		
	}
}


void Differ::Record (DiffRecorder & recorder)
{
	ClusterSort sortedClusters (_clusterer.GetClusterSeq ());
	sortedClusters.SortByNewLineNo ();
	int count = sortedClusters.Count ();
	for (int i = 0; i < count; i++)
	{
		Cluster const * cluster = sortedClusters [i];
		int oldL = cluster->OldLineNo ();
		int newL = cluster->NewLineNo ();
		int len  = cluster->Len ();
		if (oldL == -1)
		{
			recorder.AddCluster (*cluster);
			// an added line
			for (int j = 0; j < len; j++)
			{
				Line * line = _linesNew.GetLine (newL + j);
				recorder.AddNewLine (newL + j, *line);
			}
		}
		else if (newL == -1)
		{
			// store deleted lines for redundancy
			// and to be able to go back
			recorder.AddCluster (*cluster);
			// a deleted line
			for (int j = 0; j < len; j++)
			{
				Line * line = _linesOld.GetLine (oldL + j);
				recorder.AddOldLine (oldL + j, *line);
			}
		}
		else
		{
			recorder.AddCluster (*cluster);
		}
	}
}

void Differ::Dump (ListingWindow & listWin, LineCounter & counter, Progress::Meter & meter)
{
	ClusterSum cluSum (*this);
	meter.SetActivity ("Merging lines");
	meter.SetRange (0, cluSum.TotalCount (), 1);
	while (!cluSum.AtEnd ())
	{
		cluSum.DumpCluster (listWin, _src, counter);
		cluSum.Advance ();
		meter.StepAndCheck ();
	}
}

void Differ::PrepareOptimisation ()
{
	int const totalCount = _linesNew.Count ();
	Assert (totalCount > THRESHOLD_LINE_COUNT);
	// find the "bad" lines (with frequency over 2%)
	// Bad line: 100 * similarCount / totalCount > THRESHOLD_PERCENTAGE
	// or: similarCount > THRESHOLD_PERCENTAGE * totalCount / 100
	unsigned maxSimilar = totalCount / 100;
	Assert (maxSimilar > 0);
	maxSimilar *= THRESHOLD_PERCENTAGE;
	
	std::vector<bool>::iterator it = _badLines.begin ();
	for (int i = 0; it != _badLines.end (); ++it, ++i)
	{
		Line const * lineNew = _linesNew.GetLine (i);
		unsigned similarCount = lineNew->GetShifts ().size ();
		if ( similarCount > 0 && similarCount > maxSimilar)
		{
			*it = true; // this line is bad
		}
	}

	// protection from exceptional event : too many bad lines
	for (int k = 0; k < totalCount - MAX_BAD_LINE_STRETCH; k += MAX_BAD_LINE_STRETCH)
	{
		std::vector<bool>::iterator itStart = _badLines.begin () + k;
		std::vector<bool>::iterator itEnd = _badLines.begin () + k + MAX_BAD_LINE_STRETCH;
		
		if (std::find (itStart, itEnd, false) == itEnd) // All "bad"
			*itStart = false; // mark starting line of the bad stretch as good
	}
}
