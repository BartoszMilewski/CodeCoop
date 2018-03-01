#if !defined (SCRIPTIO_H)
#define SCRIPTIO_H
//------------------------------------
//  (c) Reliable Software, 2003 - 2007
//------------------------------------

#include "Params.h"
#include "ScriptSerialize.h"
#include "Chunker.h"

#include <Dbg/Assert.h>

namespace Version40
{
	class Script : public ScriptSerializable
	{
	public:
		Script () {}
		
		bool IsSection () const { return true; }
		int  SectionId () const { return 'SCRP'; }
		int  VersionNo () const { return scriptVersion; }
	};
}

class ScriptBuilder: public ScriptChunker
{
public:
	ScriptBuilder (Catalog const * catalog, ScriptHeader * scriptHdr)
		: ScriptChunker (catalog, scriptHdr)
	{}
	void Save (std::string const & toString);
	void Save (std::vector<unsigned> const & chunkList, std::string const & toString);
	void Save (FilePath & toPath, std::string const & fileName) const;
	void Save (Serializer & out) const;
	void SaveChunk (FilePath & toPath, 
					std::string const & fileName, 
					ChunkSeq const & chunks);
};

class ScriptReader
{
public:
	ScriptReader (std::string const & path)
		: _path (path)
	{}
	ScriptReader (char const * path)
		: _path (path)
	{}
	ScriptReader ()
	{}

	void SetInputPath (std::string const & path) { _path = path; }

	std::unique_ptr<TransportHeader> RetrieveTransportHeader (File::Offset offset = File::Offset (0, 0)) const;
	std::unique_ptr<ScriptHeader> RetrieveScriptHeader (File::Offset offset = File::Offset (0, 0)) const;
	std::unique_ptr<ScriptTrailer> RetrieveScriptTrailer (File::Offset offset = File::Offset (0, 0)) const;
	std::unique_ptr<CommandList> RetrieveCommandList (File::Offset offset = File::Offset (0, 0)) const;
	std::unique_ptr<DispatcherScript> RetrieveDispatcherScript (File::Offset offset = File::Offset (0, 0)) const;
	std::unique_ptr<TransportHeader> RetrieveTransportHeader (FileDeserializer & in) const;
	std::unique_ptr<ScriptHeader> RetrieveScriptHeader (FileDeserializer & in) const;
	std::unique_ptr<ScriptTrailer> RetrieveScriptTrailer (FileDeserializer & in) const;
	std::unique_ptr<CommandList> RetrieveCommandList (FileDeserializer & in) const;
	std::unique_ptr<DispatcherScript> RetrieveDispatcherScript (FileDeserializer & in) const;
	std::unique_ptr<CheckOut::List> RetrieveCheckoutNotification (FileDeserializer & in) const;

private:
	class Version40Reader : public Version40::Script
	{
	public:
		Version40Reader (Deserializer & in)
		{
			Read (in);
		}
		void Serialize (Serializer& out) const { Assert (!"Version40Reader::Deserialize -- should never be called!"); }
		void Deserialize (Deserializer& in, int version);

		// May be called only once!
		std::unique_ptr<ScriptHeader> GetHeader () { return std::move(_hdr); }
		std::unique_ptr<CommandList> GetCmdList () { return std::move(_cmdList); }

	private:
		std::unique_ptr<ScriptHeader>	_hdr;
		std::unique_ptr<CommandList>	_cmdList;
	};

private:
	std::string	_path;
};

#endif
