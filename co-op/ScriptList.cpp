//---------------------------------------
//  (c) Reliable Software, 2003
//---------------------------------------

#include "precompiled.h"
#include "ScriptList.h"
#include "ScriptHeader.h"
#include "ScriptCommandList.h"

void ScriptList::Append (ScriptList & list)
{
	Assert (list._hdrs.size () == list._cmdLists.size ());
	_hdrs.append (list._hdrs);
	_cmdLists.append (list._cmdLists);
	Assert (_hdrs.size () == _cmdLists.size ());
}

void ScriptList::Serialize (Serializer& out) const
{
	Assert (_hdrs.size () == _cmdLists.size ());
	out.PutLong (_hdrs.size ());
	for (Sequencer seq (*this); !seq.AtEnd (); seq.Advance ())
	{
		ScriptHeader const & hdr = seq.GetHeader ();
		hdr.Serialize (out);
		CommandList const & cmdList = seq.GetCmdList ();
		cmdList.Serialize (out);
	}
}

void ScriptList::Deserialize (Deserializer& in, int version)
{
	unsigned long count = in.GetLong ();
	_hdrs.reserve (count);
	_cmdLists.reserve (count);
	for (unsigned long i = 0; i < count; ++i)
	{
		std::unique_ptr<ScriptHeader> hdr (new ScriptHeader);
		hdr->Deserialize (in, version);
		_hdrs.push_back (std::move(hdr));
		std::unique_ptr<CommandList> cmdList (new CommandList);
		cmdList->Deserialize (in, version);
		_cmdLists.push_back (std::move(cmdList));
	}
}

