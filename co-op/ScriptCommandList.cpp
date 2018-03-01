//------------------------------------
//  (c) Reliable Software, 2003 - 2007
//------------------------------------

#include "precompiled.h"
#include "ScriptCommandList.h"

#include <Ex/Winex.h>

bool CommandList::IsEqual (CommandList const & cmdList) const
{
	// Command list are considered equal if they have the same length
	// and all file commands are equal.
	if (_cmds.size () != cmdList._cmds.size ())
		return false;

	for (unsigned int i = 0; i < _cmds.size (); ++i)
	{
		ScriptCmd const * cmd1 = _cmds [i];
		ScriptCmd const * cmd2 = cmdList._cmds [i];
		if (!cmd1->IsEqual (*cmd2))
			return false;
	}
	return true;
}

void CommandList::Serialize (Serializer& out) const
{
    out.PutLong (_cmds.size ());
    for (unsigned int i = 0; i < _cmds.size (); ++i)
        _cmds [i]->Serialize (out);
}

void CommandList::Deserialize (Deserializer& in, int version)
{
    unsigned int count = in.GetLong ();
    for (unsigned int i = 0; i < count; ++i)
    {
		std::unique_ptr<ScriptCmd> cmd = ScriptCmd::DeserializeCmd (in, version);
		if (!cmd->Verify ())
			throw Win::InternalException ("Error reading script command: possibly corrupt script.");
        push_back (std::move(cmd));
    }
}
