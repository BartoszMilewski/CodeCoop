//------------------------------------
//  (c) Reliable Software, 2006 - 2007
//------------------------------------

#include "precompiled.h"
#include "BuiltInMerger.h"
#include "PhysicalFile.h"
#include "HistoricalFiles.h"
#include "Diff.h"
#include "Merge.h"

#include <Ctrl/ProgressMeter.h>
#include <File/MemFile.h>

BuiltInMerger::BuiltInMerger (PhysicalFile & file)
	: _file (&file)
{
	Assert (file.IsTextual ());
	FileState state = file.GetState ();
	Assert (state.IsRelevantIn (Area::Original));
	Assert (state.IsPresentIn (Area::Original));
	Assert (state.IsPresentIn (Area::Project));
	Assert (state.IsPresentIn (Area::Synch));
	Assert (state.IsPresentIn (Area::Reference));

	_projectVersion.assign (file.GetFullPath (Area::Project));
	_referenceVersion.assign (file.GetFullPath (Area::Reference));
	_syncVersion.assign (file.GetFullPath (Area::Synch));
}

BuiltInMerger::BuiltInMerger (PhysicalFile & file, Restorer const & restorer)
	: _file (&file)
{
	Assert (file.IsTextual ());
	FileState state = file.GetState ();
	Assert (state.IsRelevantIn (Area::Original));
	Assert (state.IsPresentIn (Area::Original));
	Assert (state.IsPresentIn (Area::Project));

	_projectVersion.assign (file.GetFullPath (Area::Project));
	TmpProjectArea const & beforeArea = restorer.GetBeforeArea ();
	_referenceVersion.assign (file.GetFullPath (beforeArea.GetAreaId ()));
	TmpProjectArea const & afterArea = restorer.GetAfterArea ();
	_syncVersion.assign (file.GetFullPath (afterArea.GetAreaId ()));
}

void BuiltInMerger::MergeFiles (Progress::Meter & meter)
{
	// Store current progress meter range
	// because we will set New one
	int oldMin, oldMax, oldStep;
	meter.GetRange (oldMin, oldMax, oldStep);
	std::vector<char> mergeResultBuf;
	DoMerge (mergeResultBuf, meter);
	Assert (_file != 0);
	_file->OverwriteInProject (mergeResultBuf);
	meter.SetRange (oldMin, oldMax, oldStep);
}

void BuiltInMerger::MergeFiles (std::string const & mergeResultPath)
{
	Progress::Meter dummy;
	std::vector<char> mergeResultBuf;
	DoMerge (mergeResultBuf, dummy);

	MemFileAlways file (mergeResultPath);
	File::Size newSize (mergeResultBuf.size (), 0);
	file.ResizeFile (newSize);
	std::copy (mergeResultBuf.begin (), mergeResultBuf.end (), file.GetBuf ());
}

void BuiltInMerger::DoMerge (std::vector<char> & mergeResultBuf, Progress::Meter & meter)
{
	// Merge Synch area with Project Area
	MemFileReadOnly userFile (_projectVersion);
	MemFileReadOnly syncFile (_syncVersion);
	MemFileReadOnly refFile (_referenceVersion);

	StrictComparator comp;
	Differ diffOut (refFile.GetBuf (), refFile.GetSize ().Low (), 
					userFile.GetBuf (), userFile.GetSize ().Low (), 
					comp, meter, EditStyle::chngUser);
	Differ diffSync (refFile.GetBuf (), refFile.GetSize ().Low (), 
					 syncFile.GetBuf (), syncFile.GetSize ().Low (), 
					 comp, meter, EditStyle::chngSynch);

	Merger merger (diffOut, diffSync, meter);
	merger.Dump (mergeResultBuf);
}
