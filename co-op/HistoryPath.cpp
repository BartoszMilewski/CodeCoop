//------------------------------------
//  (c) Reliable Software, 2006 - 2007
//------------------------------------

#include "precompiled.h"
#include "HistoryPath.h"
#include "HistoryRange.h"
#include "Workspace.h"

#include <Ctrl/ProgressMeter.h>

History::Path::Sequencer::Sequencer (History::Path const & path)
	: _cur (path._segments.begin ()),
	  _end (path._segments.end ())
{}

History::Path::Path ()
{}

void History::Path::AddUndoRange (History::Db const & history,
								  History::Range const & range,
								  GidSet const & selectedFiles)
{
	AddSegment (history, range, selectedFiles, false);	// Backward segment
}

void History::Path::AddRedoRange (History::Db const & history,
								  History::Range const & range,
								  GidSet const & selectedFiles)
{
	AddSegment (history, range, selectedFiles, true);	// Forward segment
}

void History::Path::AddSegment (History::Db const & history,
								History::Range const & range,
								GidSet const & selectedFiles,
								bool isRedo)
{
	if (range.Size () == 0)
		return;

	Progress::Meter dummy;
	std::unique_ptr<Workspace::HistorySelection> selection;
	if (selectedFiles.empty ())
	{
		selection.reset (new Workspace::HistoryRangeSelection (
			history,
			range,
			isRedo,
			dummy));
	}
	else
	{
		selection.reset (new Workspace::FilteredHistoryRangeSelection (
			history,
			range,
			selectedFiles,
			isRedo,
			dummy));
	}
	_segments.push_back (std::move(selection));
}
