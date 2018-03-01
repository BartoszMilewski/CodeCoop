// (c) Reliable Software 2003
#include "precompiled.h"
#include "Lister.h"
#include "OutSink.h"
#include <File/Dir.h>
#include <File/Drives.h>
#include <Win/Win.h>

enum 
{
	MAX_FILES_TO_LIST = 16,
	MAX_FILES_TO_COMPARE = 4 
};

Lister::Lister (Data::ListQuery const & query, Data::Sink * sink, int id)
	: Data::Provider (sink, id),
	  _query (query)
{
}

void Lister::Run ()
{
	try
	{
		int i = 0;
		FileSeq seq (_query.GetPath ().GetAllFilesPath ());
		while (!seq.AtEnd () && _sink != 0)
		{
			if (IsDying ())
				return;

			Data::ChunkHolder result (new Data::Chunk);
			do
			{
				char const * name = seq.GetName ();
				Data::Item::Info info (seq.IsFolder ());
				result->Add (name, info);
				seq.Advance ();
				if (result->size () == MAX_FILES_TO_LIST)
					break;
			} while (!seq.AtEnd ());
			// pass chunk to sink
			if (_sink != 0)
			{
				_sink->DataReady (result, seq.AtEnd (), _id);
			}
		}
	}
	catch (Win::Exception & e)
	{
		TheOutput.Display (e);
	}
	catch (...)
	{}
}

DriveLister::DriveLister (Data::Sink * sink, int id)
	: Data::Provider (sink, id)
{
}

void DriveLister::Run ()
{
	try
	{
		Data::ChunkHolder result (new Data::Chunk);
		Data::Item::Info info;
		info.SetDrive ();
		for (DriveSeq drives; !drives.AtEnd (); drives.Advance ())
		{
			if (IsDying ())
				return;
			result->Add (drives.GetDriveString (), info);
		}
		if (_sink != 0)
		{
			_sink->DataReady (result, true, _id);
		}
	}
	catch (...)
	{}
}

//------------
// Accumulator
//------------

Accumulator::Accumulator (Data::ListQuery const & query, Data::Sink * sink, int id)
: Data::Pipe (sink, id),
		_query (query)
{
	_lister.reset (new Lister (query, this, id));
}

void Accumulator::Run ()
{
	try
	{
		while (_sink != 0)
		{
			_event.Wait ();
			if (IsDying ())
				return;
			MergeChunks ();
		}
	}
	catch (...)
	{}
}

void Accumulator::DataReady (Data::ChunkHolder data, bool isDone, int srcId)
{
	Win::Lock lock (_critSect);
	// dbg << "Accumulator: from " << srcId << ", done = " << isDone << std::endl;
	_sources.Push (data);
	if (isDone)
		_lister.reset ();
	_event.Release ();
}

void Accumulator::MergeChunks ()
{
	Data::Processor straight;
	Data::Merger merger (straight);
	{
		Win::Lock lock (_critSect);
		_sources.EndBatch (); 
		if (!_prevResult.Empty ())
			merger.AddSource (_prevResult.Get ());
		merger.AddSources (_sources.begin (), _sources.end ());
	}

	if (merger.Start ())
	{
		merger.Do ();

		{//----------------------------
			Win::Lock lock (_critSect);
			_prevResult = straight.GetResult ();
			if (_sink != 0)
				_sink->DataReady (_prevResult, _lister.empty () && _sources.IsDone (), _id);
		}
	}
}

//-----------
// Combiner: combines the results from two listers
//-----------
Combiner::Combiner (Data::CmpQuery const & query, Data::Sink * sink, int id)
	: Data::Pipe (sink, id)
{
	Data::ListQuery query1 (query.GetOldPath ());
	Data::ListQuery query2 (query.GetNewPath ());
	_lister1.reset (new Lister (query1, this, 0));
	_lister2.reset (new Lister (query2, this, 1));
}

void Combiner::DataReady (Data::ChunkHolder data, bool isDone, int srcId)
{
	Win::Lock lock (_critSect);
	// dbg << "Combiner: from " << srcId << ", done = " << isDone << std::endl;
	if (srcId == 0)
	{
		data->SetPresentInOld ();
		_sources1.Push (data);
		if (isDone)
			_lister1.reset ();
	}
	else
	{
		Assert (srcId == 1);
		data->SetPresentInNew ();
		_sources2.Push (data);
		if (isDone)
			_lister2.reset ();
	}
	_event.Release ();
}

void Combiner::Run ()
{
	try
	{
		while (_sink != 0)
		{
			_event.Wait ();
			if (IsDying ())
				return;
			MergeChunks ();
		}
	}
	catch (...)
	{}
}

void Combiner::MergeChunks ()
{
	Data::Processor straight;
	Data::Merger merger (straight);

	{
		Win::Lock lock (_critSect);
		_sources1.EndBatch ();
		_sources2.EndBatch ();
		if (!_prevResult.Empty ())
			merger.AddSource (_prevResult.Get ());
		merger.AddSources (_sources1.begin (), _sources1.end ());
		merger.AddSources (_sources2.begin (), _sources2.end ());
	}

	if (merger.Start ())
	{
		merger.Do ();

		{
			Win::Lock lock (_critSect);
			_prevResult = straight.GetResult ();
			if (_sink != 0)
				_sink->DataReady (_prevResult, _lister1.empty () 
											&& _lister2.empty () 
											&& _sources1.IsDone () 
											&& _sources2.IsDone (), _id);
		}
	}
}

// FileCatcher, accumulates files that must be compared
class FileCatcher: public Data::Processor
{
public:
	FileCatcher () : _toBeCompared (new Data::Chunk) {}
	bool Put (Data::Item & item);
	Data::ChunkHolder GetTheCatch () { return _toBeCompared; }
	bool HasCatch () const { return !_toBeCompared.Get ()->empty (); }
private:
	Data::ChunkHolder _toBeCompared;
};

bool FileCatcher::Put (Data::Item & item)
{
	Data::Processor::Put (item);
	Data::Item::Info const & info = item.GetInfo ();
	if (!info.IsFolder () && !info.IsCompared () && info.IsPresentInNew () && info.IsPresentInOld ())
	{
		item.GetInfo ().SetCompared ();
		_toBeCompared->AddItem (item);
	}
	return true;
}

class FileCompare: public Data::Processor
{
public:
	FileCompare (Data::CmpQuery const & query, int maxCount) 
		: _query (query), _maxCount (maxCount), _count (0) {}
	bool Put (Data::Item & item)
	{
		// dbg << "    " << item.Name () << std::endl;
		bool diff = FilesDifferent (item.Name ());
		item.GetInfo ().SetDifferent (diff);
		item.SetDiffState ();
		Data::Processor::Put (item);
		return _count++ < _maxCount;
	}
	void ResetCount () { _count = 0; }
private:
	bool FilesDifferent (std::string const & name);
private:
	int _count;
	int _maxCount;
	Data::CmpQuery	const & _query;
};

bool FileCompare::FilesDifferent (std::string const & name)
{
	bool isDiff = true;
	try
	{
		// dbg << name << std::endl;
		std::string path1 (_query.GetOldPath ().GetFilePath (name));
		std::string path2 (_query.GetNewPath ().GetFilePath (name));
		isDiff = !File::IsContentsEqual (path1.c_str (), path2.c_str ());
	}
	catch (...)
	{}
	return isDiff;
}

//----------------------------------------
// Comparator has two inputs and two sinks
//
// |Combiner|->|Comparator| -> sink
//                ^    |
//                |    v
//          |ContentComparator|
//----------------------------------------

// Stage 1: Data keeps arriving from Combiner. 
// During merge, items that are present in both directories are stored in FileCatcher
// Merge result is passed to the sink for partial display
// and data from FileCather are passed to ContentComparator.
// Data from ContentComparator is passed to the sink and also
// back for merging (with the state "compared", so that those items will not
// be sent back to ContentComparator)
// End Stage 1: The last chunk from Combiner arrives. Combiner is killed.
//
// Stage 2: Data from Combiner are still awaiting to be merged, data from ContentComparator
// are still being added to merge.
// End Stage 2: The last chunk from Combiner is picked up by merge. 
//
// Stage 3: Data from ContentComparator no longer go back to merge. When merge finishes,
// the last (possibly empty) end chunk is put through ContentComparator
// End Stage 3: End chunk arrives from ContentComparator

Comparator::Comparator (Data::CmpQuery const & query, Data::Sink * sink, int id)
	: Data::Pipe (sink, id),
	  _state (stReady)
{
	// Create the combiner based on CmpQuery
	_combiner.reset (new Combiner (query, this, 0));
	// Create the content comparator, make us its sink
	_contentComparator.reset (new ContentComparator (query, this, id));
}

void Comparator::DataReady (Data::ChunkHolder data, bool isDone, int srcId)
{
	Win::Lock lock (_critSect);
	if (srcId == 0) // from combiner
	{
		Assert (_state == stReady);
		dbg << "Comparator: from Combiner, done = " << isDone << std::endl;
		_sources.Push (data);
		if (isDone) // from combiner
		{
			// Kill the combiner, Content comparator is still working
			dbg << "Kill the combiner" << std::endl;
			_combiner.reset ();
			_state = stSourceEnd;
		}
	}
	else // from ContentComparator
	{
		dbg << "Comparator: from Content Comparator, done = " << isDone << std::endl;
		_sink->DataReady (data, isDone, srcId);
		if (_state != stMergeEnd)
		{
			dbg << "->Back for merging" << std::endl;
			_sources.Push (data);
		}
	}
	_event.Release ();
}

void Comparator::Run ()
{
	try
	{
		while (_sink != 0)
		{
			_event.Wait ();
			if (IsDying ())
				return;
			ProcessChunks ();
		}
	}
	catch (...)
	{}
}

// Merge data that came from the Combiner and from the ContentComparator
void Comparator::ProcessChunks ()
{
	FileCatcher catcher;
	Data::Merger merger (catcher);
	{
		Win::Lock lock (_critSect);
		_sources.EndBatch ();
		if (!_prevResult.Empty ())
		{
			Assert (_state != stMergeEnd);
			merger.AddSource (_prevResult.Get ());
		}
		if (_sources.begin () != _sources.end ())
		{
			Assert (_state != stMergeEnd);
			merger.AddSources (_sources.begin (), _sources.end ());
		}
		if (_state == stSourceEnd) // last merge
		{
			dbg << "Comparator: Entering last merge" << std::endl;
			_state = stMergeEnd;
		}
	}

	if (merger.Start ())
	{
		merger.Do ();

		{//----------------------------
			Win::Lock lock (_critSect);
			_prevResult = catcher.GetResult ();
			// Order is important
			// 1. Flush our data to the sink
			if (_sink != 0)
			{
				_sink->DataReady (_prevResult, false, _id);
			}
			// 2. Load the processor with files to compare
			if (_state == stMergeEnd)
			{
				// Make sure we don't restart the merge
				_prevResult.Clear ();
				// we are done, send last chunk (maybe empty)
				dbg << "Comparator after final merge, sends END chunk to content comparator" << std::endl;
				_contentComparator.get ()->DataReady (catcher.GetTheCatch (), true, _id);
			}
			else if (catcher.HasCatch ())
			{
				dbg << "Comparator after merge, sends chunk to content comparator" << std::endl;
				_contentComparator.get ()->DataReady (catcher.GetTheCatch (), false, _id);
			}
		}
	}
}

// ->|Comparator| -> sink
//    ^    |
//    |    v
//   |ContentComparator|

// Stage 1: no data yet
// End Stage 1:
//    (a) non-end chunk arrives -- go to Stage 2
//    (b) non-empty end chunk arrives -- go to Stage 3
//    (c) empty end chunk arrives -- pass it directly to sink, END
// Stage 2:
// End Stage 2:
//    (b) non-empty end chunk arrives -- go to Stage 3
//    (c) empty end chunk arrives -- go to Stage 3
// Stage 3:
//     finish all merging, send end chunk to sink

ContentComparator::ContentComparator (Data::CmpQuery const & query, Data::Sink * sink,  int id)
	: Data::Pipe (sink, id),
	_query (query),
	_state (stReady)
{
}

void ContentComparator::DataReady (Data::ChunkHolder data, bool isDone, int srcId)
{
	Win::Lock lock (_critSect);
	if (!data->empty ())
	{
		Assert (_state == stReady || _state == stBusy);
		dbg << "ContentComparator: files for further processing from " << srcId << ", done = " << isDone << std::endl;
		_state = stBusy;
		_sources.Push (data);
	}

	if (isDone)
	{
		if (_state == stBusy)
		{
			dbg << "ContentComparator: enter finishing stage" << std::endl;
			_state = stFinishing;
		}
		else if (_state == stReady && data->empty ())
		{
			// Nothing to do--shut down
			dbg << "ContentComparator resending empty chunk" << std::endl;
			// send empty final chunk
			_sink->DataReady (data, true, _id);
		}
	}
	_event.Release ();
}

void ContentComparator::Run ()
{
	try
	{
		for (;;)
		{
			_event.Wait ();
			if (IsDying ())
				return;
			ProcessChunks ();
		}
	}
	catch (...)
	{}
}

void ContentComparator::ProcessChunks ()
{
	FileCompare cmp (_query, MAX_FILES_TO_COMPARE);
	Data::Merger merger (cmp);
	{
		Win::Lock lock (_critSect);
		_sources.EndBatch ();
		merger.AddSources (_sources.begin (), _sources.end ());
	}
	if (merger.Start ())
	{
		bool done = false;
		do
		{
			merger.Do ();

			{//----------------------------
				Win::Lock lock (_critSect);
				if (_sink != 0)
				{
					bool isEnd = (_state == stFinishing) && merger.IsDone () && _sources.IsDone ();
					dbg << "ContentComparator: sending merged chunk, isEnd = " << isEnd << std::endl;
					dbg << "_state = " << _state << ", merger.IsDone () = " << merger.IsDone ()
						<< "_sources.IsDone () = " << _sources.IsDone () << std::endl;
					_sink->DataReady (cmp.GetResult (), isEnd, _id);
					cmp.ResetCount ();
				}
				// else dbg << "Sink disconnected" << std::endl;
			}
		} while (!merger.IsDone () && !IsDying ());
	}
}
