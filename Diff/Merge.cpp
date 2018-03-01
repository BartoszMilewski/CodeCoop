//----------------------------------
// Reliable Software (c) 1998 - 2007
//----------------------------------

#include "precompiled.h"
#include "Merge.h"
#include "Diff.h"
#include "CluSum.h"
#include "ListingWin.h"

#include <Ctrl/ProgressMeter.h>
#include <Dbg/Assert.h>

Merger::Merger (Differ & diffUser, Differ & diffSynch, Progress::Meter & meter)
	: _diffUser (diffUser), 
	  _diffSynch (diffSynch), 
	  _userClusterer (diffUser.GetClusterer ()),
	  _synchClusterer (diffSynch.GetClusterer ())
{
	meter.SetRange (0, _userClusterer.GetClusterCount () + _synchClusterer.GetClusterCount () + 1);
	MergeDiffs (meter);
}

void Merger::Dump (ListingWindow & listWin, LineCounter & counter, Progress::Meter & meter)
{
	DiffSum diffSum (_diffUser, _diffSynch);
	while (!diffSum.AtEnd ())
	{
		diffSum.DumpCluster (listWin, counter);
		diffSum.Advance ();
	}
}

void Merger::Dump (std::vector<char> & buf)
{
	DiffSum diffSum (_diffUser, _diffSynch);
	while (!diffSum.AtEnd ())
	{
		diffSum.DumpCluster (buf);
		diffSum.Advance ();
	}
}

void Merger::MergeDiffs (Progress::Meter & meter)
{
	// Sort clusters by old (reference) line numbers

	std::list<Cluster *> sortedUserClusters;
	meter.SetActivity ("Sorting user changes");
	std::copy (_userClusterer.begin (), _userClusterer.end (), std::back_inserter (sortedUserClusters));
	sortedUserClusters.sort (CmpOldLines ());

	std::list<Cluster *> sortedSynchClusters;
	meter.SetActivity ("Sorting script changes");
	std::copy (_synchClusterer.begin (), _synchClusterer.end (), std::back_inserter (sortedSynchClusters));
	sortedSynchClusters.sort (CmpOldLines ());

	typedef std::list<Cluster *>::iterator CluListPos;

	// skip New clusters
	CluListPos userClusters = std::find_if (
		sortedUserClusters.begin (), 
		sortedUserClusters.end (), 
		NotAdded ());

	CluListPos synchClusters = std::find_if (
		sortedSynchClusters.begin (), 
		sortedSynchClusters.end (), 
		NotAdded ());

	meter.SetActivity ("Splitting clusters of lines");
	meter.SetRange (0, 
		sortedUserClusters.size () + sortedSynchClusters.size () + 1);

	// Split overlapping clusters, so that both cluster lists
	// are identical as far as old line numbers go.
	while (userClusters != sortedUserClusters.end () 
		&& synchClusters != sortedSynchClusters.end ())
	{
		Cluster * userClu  = *userClusters;
		Cluster * synchClu = *synchClusters;

		int limUser  = userClu->OldLineNo ();
		int limSynch = synchClu->OldLineNo ();
		if (limUser < limSynch)
		{
			Cluster sp = userClu->SplitCluster (limSynch - limUser);
			// insert after current
			++userClusters;
			meter.StepIt ();
			Cluster * clu = _userClusterer.AddCluster (sp);
			userClusters = sortedUserClusters.insert (userClusters, clu);
			// iterator still valid: no reallocation allowed
			Assert (*userClusters == clu);
		}
		else if (limUser > limSynch)
		{
			Cluster sp = synchClu->SplitCluster (limUser - limSynch);
			// insert after current
			++synchClusters;
			meter.StepIt ();
			Cluster * clu = _synchClusterer.AddCluster (sp);
			synchClusters = sortedSynchClusters.insert (synchClusters, clu);
			// iterator still valid: no reallocation allowed
			Assert (*synchClusters == clu);
		}
		else // both clusters start at the same offset
		{
			limUser  += userClu->Len ();
			limSynch += synchClu->Len ();

			if (userClusters != sortedUserClusters.end ()&& limUser <= limSynch)
			{
				++userClusters;
				meter.StepIt ();
			}

			if (synchClusters != sortedSynchClusters.end ()&& limUser >= limSynch)
			{
				++synchClusters;
				meter.StepIt ();
			}
		}
	}
#if !defined NDEBUG
	{

		// skip New clusters
		CluListPos userClusters = std::find_if (
			sortedUserClusters.begin (), 
			sortedUserClusters.end (), 
			NotAdded ());

		CluListPos synchClusters = std::find_if (
			sortedSynchClusters.begin (), 
			sortedSynchClusters.end (), 
			NotAdded ());

		while (userClusters != sortedUserClusters.end () 
			&& synchClusters != sortedSynchClusters.end ())
		{
			int oldUserLine = (*userClusters)->OldLineNo ();
			int oldSynchLine = (*synchClusters)->OldLineNo ();
			Assert (oldUserLine == oldSynchLine);
			++userClusters;
			++synchClusters;
		} 
	}
#endif
	meter.SetActivity ("");
}
