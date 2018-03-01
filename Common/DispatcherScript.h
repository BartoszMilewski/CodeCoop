#if !defined (DISPATCHERSCRIPT_H)
#define DISPATCHERSCRIPT_H
//-------------------------------------
//  DispatcherScript.h
//  (c) Reliable Software, 1999 -- 2002
//-------------------------------------

#include "ScriptSerialize.h"
#include "DispatcherCmd.h"
#include "Params.h"

#include <auto_vector.h>

//
// Dispatcher script -- piggybacked on top of Code Co-op synch script
//

class DispatcherScript : public ScriptSerializable
{
public:
	DispatcherScript (bool isInvitation = false)
	  : _isInvitation (isInvitation) 
	{}
    DispatcherScript (Deserializer & in);

    void AddCmd (std::unique_ptr<DispatcherCmd> cmd) { _cmdList.push_back (std::move(cmd)); }
	int  CmdCount () const { return _cmdList.size (); }

    bool IsSection () const { return true; }
    int  SectionId () const { return 'DSPC'; }
    int  VersionNo () const { return scriptVersion; }

    void Serialize (Serializer& out) const;
    void Deserialize (Deserializer& in, int version);

    // typedef iterators
    typedef auto_vector<DispatcherCmd>::iterator CommandIter;
    typedef auto_vector<DispatcherCmd>::const_iterator ConstCommandIter;
    ConstCommandIter begin () const { return _cmdList.begin (); }
    ConstCommandIter end () const { return _cmdList.end ();   }
    CommandIter begin () { return _cmdList.begin (); }
    CommandIter end () { return _cmdList.end ();   }

	CommandIter erase (CommandIter it) { return _cmdList.erase (it); }

	bool IsInvitation () const { return _isInvitation; }
private:
    auto_vector<DispatcherCmd>	_cmdList;
	bool				  const _isInvitation; // volatile
};

// Create and save dispatcher script with one command

void SaveDispatcherScript (std::unique_ptr<DispatcherCmd> cmd, 
						   std::string const & senderId, 
						   std::string const & addresseeId, // satellite or hub dispatcher
						   std::string const & hubId,
						   FilePath const & outDir,
						   std::string & fileName);


#endif
