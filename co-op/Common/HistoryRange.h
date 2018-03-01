#if !defined (HISTORYRANGE_H)
#define HISTORYRANGE_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "GlobalId.h"

namespace History
{
	// A list of script ids ordered by the predecessor relation
	class Range
	{
	public:
		Range ()
			: _fromCurrentVersion (false)
		{}

		void SetFromCurrentVersion (bool flag) { _fromCurrentVersion = flag; }
		bool IsFromCurrentVersion () const { return _fromCurrentVersion; }

		unsigned Size () const { return _ids.size (); }
		GlobalId GetYoungestId () const { return _ids.size () == 0 ? gidInvalid : _ids.front (); }
		GlobalId GetOldestId () const { return _ids.size () == 0 ? gidInvalid :_ids.back (); }
		bool IsEqual (Range const & range) const;
		bool IsSubsetOf (GidList const & ids) const;

		void AddScriptId (GlobalId id) { _ids.push_back (id); }
		void RemoveYoungestId ()
		{
			Assert (_ids.size () != 0);
			_ids.erase (_ids.begin ());
		}
		void RemoveOldestId ()
		{
			Assert (_ids.size () != 0);
			_ids.pop_back ();
		}
		void Clear () { _ids.clear (); }
		GlobalId operator[] (unsigned idx) const
		{
			Assert (idx < _ids.size ());
			return _ids [idx];
		}

		GidList::const_iterator begin () const { return _ids.begin (); }
		GidList::const_iterator end () const { return _ids.end (); }

	private:
		GidList	_ids;
		bool	_fromCurrentVersion;
	};
}

#endif
