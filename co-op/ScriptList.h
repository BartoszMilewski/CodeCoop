#if !defined (SCRIPTLIST_H)
#define SCRIPTLIST_H
//----------------------------------
//  (c) Reliable Software, 2003-2004
//----------------------------------

#include "ScriptSerialize.h"
#include "ScriptCmd.h"
#include "ScriptHeader.h"
#include "ScriptCommandList.h"
#include "Params.h"

#include <auto_vector.h>

#include <Dbg/Assert.h>

class ScriptList : public ScriptSerializable
{
public:
	ScriptList () {}
    ScriptList (Deserializer& in)
	{
		Read (in);
	}

	void push_back (std::unique_ptr<ScriptHeader> hdr,
					std::unique_ptr<CommandList> cmdList)
	{
		_hdrs.push_back (std::move(hdr));
		_cmdLists.push_back (std::move(cmdList));
		Assert (_hdrs.size () == _cmdLists.size ());
	}
	unsigned size () const { return _cmdLists.size (); }

	void Append (ScriptList & list);
    bool IsSection () const { return true; }
    int  SectionId () const { return 'SLST'; }
    int  VersionNo () const { return scriptVersion; }
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);

public:
	class Sequencer
	{
	public:
		Sequencer (ScriptList const & list)
			: _curHdr (list._hdrs.begin ()),
			  _curCmdList (list._cmdLists.begin ()),
			  _end (list._hdrs.end ())
		{}

		bool AtEnd () const { return _curHdr == _end; }
		void Advance ()
		{
			++_curHdr;
			++_curCmdList;
		}

		ScriptHeader const & GetHeader () const { return **_curHdr; }
		CommandList const & GetCmdList () const { return **_curCmdList; }

	private:
		auto_vector<ScriptHeader>::const_iterator	_curHdr;
		auto_vector<CommandList>::const_iterator	_curCmdList;
		auto_vector<ScriptHeader>::const_iterator	_end;
	};

	class EditSequencer
	{
	public:
		EditSequencer (ScriptList & list)
			: _curHdr (list._hdrs.begin ()),
			  _curCmdList (list._cmdLists.begin ()),
			  _end (list._hdrs.end ())
		{
			if (!AtEnd ())
				_cmdListSeq.Restart (**_curCmdList);
		}

		bool AtEnd () const { return _curHdr == _end; }
		void Advance ()
		{
			++_curHdr;
			++_curCmdList;
			if (!AtEnd ())
				_cmdListSeq.Restart (**_curCmdList);
		}

		ScriptHeader & GetHeader () const { return **_curHdr; }
		CommandList & GetCmdList () const { return **_curCmdList; }
		unsigned GetCmdListSize () const { return (*_curCmdList)->size (); }
		CommandList::EditSequencer GetCmdSequencer () const
		{
			Assert (!AtEnd ());
			return _cmdListSeq;
		}
	private:
		auto_vector<ScriptHeader>::iterator	_curHdr;
		auto_vector<CommandList>::iterator	_curCmdList;
		auto_vector<ScriptHeader>::iterator	_end;
		CommandList::EditSequencer			_cmdListSeq;
	};

	friend class Sequencer;
	friend class EditSequencer;

private:
	auto_vector<ScriptHeader>	_hdrs;
	auto_vector<CommandList>	_cmdLists;
};

class FullSynchData
{
public:
	void SwapLineages (Lineage & setScriptIds, auto_vector<UnitLineage> & membershipScriptIds)
	{
		_mainLineage.swap (setScriptIds);
		_sideLineages.swap (membershipScriptIds);
	}
	ScriptList & GetSetScriptList () { return _setScriptList; }
	ScriptList & GetMembershipScriptList () { return _membershipScriptList; }
	Lineage & GetMainLineage () { return _mainLineage; }
	auto_vector<UnitLineage> & GetSideLineages () { return _sideLineages; }
	void InitLineageFromScripts ()
	{
		for (ScriptList::Sequencer seq (_setScriptList); !seq.AtEnd (); seq.Advance ())
		{
			ScriptHeader const & hdr = seq.GetHeader ();
			_mainLineage.PushId (hdr.ScriptId ());
		}
	}
private:
	ScriptList	_setScriptList;
	ScriptList	_membershipScriptList;
	Lineage		_mainLineage;
	auto_vector<UnitLineage> _sideLineages;
};

#endif
