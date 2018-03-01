#if !defined (CHUNKER_H)
#define CHUNKER_H
//-----------------------------------
// (c) Reliable Software, 2004 - 2007
//-----------------------------------

#include "RandomUniqueName.h"
#include "ScriptSerialize.h"
#include "Params.h"
#include <File/SafePaths.h>
#include <File/MemFile.h>

class TransportHeader;
class ScriptHeader;
class CommandList;
class ScriptList;
class ScriptTrailer;
class DispatcherScript;
class FileSerializer;
class Catalog;
namespace CheckOut
{
	class List;
}

class ScriptChunk : public ScriptSerializable
{
public:
	ScriptChunk (unsigned char const * buf, unsigned size)
		: _buf (buf), _size (size), _ownedBuf (0)
	{}
	ScriptChunk (Deserializer & in)
		: _ownedBuf (0)
	{
		Read (in);
	}
	~ScriptChunk ()
	{
		delete [] _ownedBuf;
	}
	unsigned const char * GetCargo () const { return _ownedBuf; }
	unsigned GetSize () const { return _size; }

	bool IsSection () const { return true; }
    int  SectionId () const { return 'CHUN'; }
    int  VersionNo () const { return scriptVersion; }
	void Serialize (Serializer& out) const 
	{
		dbg << "	ScriptChunk::Serialize " << std::endl;
		out.PutLong (_size);
		dbg << "	size: " << _size << std::endl;
		out.PutBytes (_buf, _size);
	}
	void Deserialize (Deserializer& in, int version)
	{
		_size = in.GetLong ();
		_ownedBuf = new unsigned char [_size];
		in.GetBytes (_ownedBuf, _size);
	}
private:
	unsigned _size;
	unsigned char const * _buf; // when writing, don't own buffer
	unsigned char * _ownedBuf;  // when reading, we allocate this
};


// Note: the chunker is used in three different contexts
// 1. Splitting one large script into multiple chunks
// 2. Creating a chunk request (a control script)
// 3. Re-sending a single chunk
class ScriptChunker
{
	friend class ChunkSeq;
public:
	ScriptChunker (Catalog const * catalog, ScriptHeader * scriptHdr);
	void AddTransportHeader (TransportHeader const * txHdr) { _txHdr = txHdr; }
	void AddCommandList (CommandList const * cmdList) { _cmdList = cmdList; }
	void AddScriptList (ScriptList const * scriptList) { _scriptList = scriptList; }
	void AddScriptTrailer (ScriptTrailer const * scriptTrailer) { _scriptTrailer = scriptTrailer; }
	void AddDispatcherScript (DispatcherScript const * dispatcherScript) { _dispatcherScript = dispatcherScript; }
	void AddCheckoutNotification (CheckOut::List const * notification)
	{
		_checkOutNotification = notification;
	}

	bool DontSplit () const { return _dontSplit; }
	unsigned GetChunkCount ()
	{
		if (CargoSize () == 0)
			CountChunks ();
		return _chunkCount;
	}
protected:
	long long CargoSize () const { return _cmdListSize + _scriptListSize; }
	unsigned GetChunkNumber () const { return _chunkNumber; }
	bool IsSingleChunk () const { return _chunkNumber != 0; }
	unsigned GetCargoChunkSize () const { return _cargoChunkSize; }

	void InitSizes ();
	void CountChunks ();
protected:
	Catalog	const *			_catalog;

	TransportHeader const *	_txHdr;
	ScriptHeader *			_scriptHdr;
	CommandList const *		_cmdList;
	ScriptList const *		_scriptList;
	ScriptTrailer const *	_scriptTrailer;
	DispatcherScript const *_dispatcherScript;
	CheckOut::List const *	_checkOutNotification;

	long long _transHdrSize;
	long long _scriptHdrSize;
	long long _cmdListSize;
	long long _scriptListSize;
	long long _trailerSize;
	long long _dispScriptSize;

	unsigned _chunkNumber; // when requesting or re-sending single chunk
	unsigned _chunkCount;
	bool	 _dontSplit;
	unsigned _cargoChunkSize;
};

// Note: the chunk sequencer is used in three different contexts
// 1. Splitting one large script into multiple chunks
// 2. Re-sending a single chunk
class ChunkSeq
{
public:
	ChunkSeq (ScriptChunker & chunker);
	bool AtEnd () const { return _num > _count; }
	void Advance ()
	{
		if (_chunker.IsSingleChunk ())
			_num = _count + 1;
		else
			++_num;
	}
	void Seek (unsigned n) { _num = n; }
	unsigned char const * GetChunk () const;
	unsigned GetChunkSize () const;
	// ChunkNumber goes from 1 to ChunkCount ()
	unsigned ChunkNumber () const { return _num; }
	unsigned ChunkCount () const { return _count; }
	unsigned MaxChunkSize () const { return _chunker.GetCargoChunkSize (); }
private:
	ScriptChunker & _chunker;
	unsigned _num; // from 1 to _count
	unsigned _count;
	
	// Order of these is important
	RandomUniqueName _randomName;
	SafeTmpFile	_tmpFilePath;
	std::unique_ptr<FileViewRo> _tmpFileView;
};
#endif
