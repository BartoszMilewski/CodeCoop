#if !defined (SCRIPTTRAILER_H)
#define SCRIPTTRAILER_H
//---------------------------------------------
// (c) Reliable Software 2004
//---------------------------------------------

#include "Params.h"
#include "ScriptSerialize.h"
#include "ScriptCommandList.h"

class ScriptTrailer : public ScriptSerializable
{
public:
	ScriptTrailer ()
	{}
	ScriptTrailer (Deserializer & in)
	{
		Read (in);
	}

	void push_back (std::unique_ptr<ScriptCmd> cmd) { _cmdList.push_back (std::move(cmd)); }
	bool empty () const { return _cmdList.size () == 0; }
	CommandList const & GetCmdList () const { return _cmdList; }

	bool IsSection () const { return true; }
    int  SectionId () const { return 'SCTR'; }
    int  VersionNo () const { return scriptVersion; }
	void Serialize (Serializer& out) const
	{
		dbg << "	ScriptTailer::Serialize " << std::endl;
		_cmdList.Serialize (out);
	}
	void Deserialize (Deserializer& in, int version) { _cmdList.Deserialize (in, version); }

private:
	CommandList	_cmdList;	
};

#endif
