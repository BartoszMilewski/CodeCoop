#if !defined (VERIFICATIONREPORT_H)
#define VERIFICATIONREPORT_H
//------------------------------------
//  (c) Reliable Software, 1999 - 2008
//------------------------------------

#include "GlobalId.h"
#include <MultiMap.h>

class VerificationReport
{
public:
	enum ItemKind
	{
		MissingFolder,	// Folder missing from disk
		AbsentFolder,	// Folder missing from the project (in state none when its contents is controlled)
		MissingCheckedout,
		MissingNew,
		MissingReadOnlyAttribute,
		PreservedLocalEdits,
		IllegalName,
		DirtyUncontrolled,
		SyncAreaOrphan,
		Corrupted
	};

private:
	typedef std::multimap<ItemKind, GlobalId> ItemMap;

	class IsEqualGid : public std::unary_function<ItemMap::const_iterator, bool>
	{
	public:
		IsEqualGid (GlobalId gid)
			: _gid (gid)
		{}

		bool operator () (std::pair<ItemKind, GlobalId> pair) const
		{
			return pair.second == _gid;
		}
	private:
		GlobalId	_gid;
	};

public:
	typedef MmCountRangeSequencer<std::multimap<ItemKind, GlobalId> > Sequencer;

	void Remember (VerificationReport::ItemKind itemKind, GlobalId gid)
	{
		_report.insert (std::make_pair(itemKind, gid));
	}

	bool IsEmpty () const { return _report.empty (); }
	bool IsPresent (VerificationReport::ItemKind itemKind, GlobalId gid) const
	{
		typedef ItemMap::const_iterator Iterator;
		std::pair<Iterator, Iterator> range = _report.equal_range (itemKind);
		if (range.first == range.second)
			return false;
		else
			return std::find_if (range.first, range.second, IsEqualGid (gid)) != range.second;
	}

	Sequencer GetSequencer (VerificationReport::ItemKind itemKind) const
	{
		return Sequencer (_report.equal_range (itemKind));
	}

private:
	ItemMap	_report;
};

#endif
