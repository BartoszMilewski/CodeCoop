#if !defined (HISTORYPATH_H)
#define HISTORYPATH_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "GlobalId.h"

#include <auto_vector.h>

namespace Workspace
{
	class HistorySelection;
}
namespace History
{
	class Db;
	class Range;
}

namespace History
{
	class Path
	{
	public:
		class Sequencer
		{
		public:
			Sequencer (History::Path const & path);

			bool AtEnd () const { return _cur == _end; }
			void Advance () { ++_cur; }
			Workspace::HistorySelection & GetSegment () const { return *(*_cur); }
		private:
			auto_vector<Workspace::HistorySelection>::const_iterator	_cur;
			auto_vector<Workspace::HistorySelection>::const_iterator	_end;
		};

	public:
		Path ();
		bool IsEmpty () const { return _segments.size () == 0; }
		unsigned SegmentCount () const { return _segments.size (); }
		Workspace::HistorySelection const & GetFirstSegment () const { return *_segments.front (); }
		Workspace::HistorySelection const & GetLastSegment () const { return *_segments.back (); }

		void AddUndoRange (History::Db const & history,
						   History::Range const & range,
						   GidSet const & selectedFiles);
		void AddRedoRange (History::Db const & history,
						   History::Range const & range,
						   GidSet const & selectedFiles);

	private:
		void AddSegment (History::Db const & history,
						 History::Range const & range,
						 GidSet const & selectedFiles,
						 bool isRedo);
		
	private:
		Path (Path const &);
		Path & operator= (Path const &);
	private:
		auto_vector<Workspace::HistorySelection>	_segments;
	};
}

#endif
