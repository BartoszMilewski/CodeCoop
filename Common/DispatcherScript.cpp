//------------------------------------
//  (c) Reliable Software, 1999 - 2005
//------------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "DispatcherScript.h"
#include "DispatcherCmd.h"
#include "GlobalId.h"
#include "TransportHeader.h"

#include <Dbg/Assert.h>

DispatcherScript::DispatcherScript (Deserializer & in)
: _isInvitation (false)
{
    Read (in);
}

void DispatcherScript::Serialize (Serializer & out) const
{
	dbg << "	DispatcherScript::Serialize " << std::endl;
	dbg << "	size: " << _cmdList.size () << std::endl;
    out.PutLong (_cmdList.size ());
	unsigned int i = 0;
    for (i = 0; i < _cmdList.size (); i++)
        _cmdList [i]->Serialize (out);
    Assert (i == _cmdList.size ());
}

void DispatcherScript::Deserialize (Deserializer & in, int version)
{
    int count = in.GetLong ();
    for (int i = 0; i < count; i++)
    {
		std::unique_ptr<DispatcherCmd> cmd = DispatcherCmd::DeserializeCmd (in, version);
		if (cmd.get () != 0)
			AddCmd (std::move(cmd));
    }
}

void SaveDispatcherScript (std::unique_ptr<DispatcherCmd> cmd, 
						   std::string const & senderId, 
						   std::string const & addresseeId,
						   std::string const & hubId,
						   FilePath const & outDir,
						   std::string & fileName)
{
	DispatcherScript addendum;
	addendum.AddCmd (std::move(cmd));
	GlobalIdPack scriptId (RandomId ());
	TransportHeader txHdr;
	txHdr.AddScriptId (scriptId);
	txHdr.AddSender (Address (hubId.c_str (), "", senderId));

	AddresseeList addresseeList;
	addresseeList.push_back (Addressee (hubId, addresseeId));
	
	txHdr.AddRecipients (addresseeList);
	txHdr.SetDispatcherAddendum (true);

	ScriptSubHeader comment (std::string ("Dispatcher-to-Dispatcher Notification"));
	fileName = scriptId.ToString ();
	fileName.append (".snc");
	FileSerializer out (outDir.GetFilePath (fileName));
	txHdr.Save (out);
	comment.Save (out);
	addendum.Save (out);
}


