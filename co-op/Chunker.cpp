// ----------------------------------
// (c) Reliable Software, 2004 - 2008
// ----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "Chunker.h"
#include "ScriptHeader.h"
#include "TransportHeader.h"
#include "ScriptTrailer.h"
#include "ScriptCommandList.h"
#include "ScriptList.h"
#include "DispatcherScript.h"
#include "Serialize.h"
#include "Registry.h"
#include "EmailConfig.h"

ScriptChunker::ScriptChunker (Catalog const * catalog, ScriptHeader * scriptHdr)
:	_catalog (catalog),
	_scriptHdr (scriptHdr),
	_txHdr (0),
	_cmdList (0),
	_scriptList (0),
	_scriptTrailer (0),
	_dispatcherScript (0),
	_checkOutNotification (0),
	_chunkNumber (0), // invalid
	_chunkCount (0), // invalid
	_transHdrSize (0),
	_scriptHdrSize (0),
	_cmdListSize (0),
	_scriptListSize (0),
	_trailerSize (0),
	_dispScriptSize (0),
	_cargoChunkSize (0),
	_dontSplit (false)
{ 
	Assert (((_scriptHdr->GetUnitType () == Unit::Set || _scriptHdr->GetUnitType () == Unit::Ignore) && _scriptHdr->GetModifiedUnitId () == gidInvalid) ||
			 (_scriptHdr->GetUnitType () == Unit::Member && (_scriptHdr->GetModifiedUnitId () != gidInvalid || _scriptHdr->IsAck ()))					||
			 (_scriptHdr->IsResendRequest () && (_scriptHdr->GetUnitType () == Unit::Set || _scriptHdr->GetUnitType () == Unit::Member)));
	if (scriptHdr->GetPartCount () != 1) // requesting or resending a chunk
	{
		_cargoChunkSize = scriptHdr->GetMaxChunkSize ();
		_chunkNumber = scriptHdr->GetPartNumber ();
		_chunkCount = scriptHdr->GetPartCount ();
		_dontSplit = _scriptHdr->IsPureControl (); // re-send request
	}
}

// Sizes of headers may change during manual resend, so don't take them
// into account when calculating chunk sizes.
// Because of that, resulting chunks are slightly larger than maxEmailSize
void ScriptChunker::CountChunks ()
{
	dbg << "	ScriptChunker::CountChunks" << std::endl;
	InitSizes ();
	if (IsSingleChunk ())
	{
		dbg << "	Single Chunk!" << std::endl;
		return;
	}
	Email::RegConfig email;
	unsigned long maxEmailSize = email.GetMaxEmailSize ();
	maxEmailSize *= 1024;
	dbg << "	maxEmailSize: " << maxEmailSize << std::endl;

	// each chunk must have these components
	long long overhead = 
		_transHdrSize +
		_scriptHdrSize +
		_trailerSize +
		_dispScriptSize;

	// Variable overhead (overhead - fixedOverhead) may be different
	// when doing manual resend, so don't count it in _cargoChunkSize.
	// Scripts with non-zero fixedOverhead are never resent manually
	long long fixedOverhead = 
		_trailerSize +
		_dispScriptSize;

	// cargo can be split
	long long cargo = 
		_cmdListSize +
		_scriptListSize;

	long long fullCargoChunkSize = maxEmailSize - fixedOverhead;
	// don't split if total is less than max allowed 
	if (fixedOverhead + cargo <= maxEmailSize)
		_chunkCount = 1;
	else
		_chunkCount = static_cast<unsigned>((cargo + fullCargoChunkSize - 1) / fullCargoChunkSize);

	if (_chunkCount > 250)
	{
		fullCargoChunkSize = cargo / 250;
		_chunkCount = static_cast<unsigned>((cargo + fullCargoChunkSize - 1) / fullCargoChunkSize);
	}

	_cargoChunkSize = static_cast<unsigned>(fullCargoChunkSize);
	if (fullCargoChunkSize != _cargoChunkSize)
		throw Win::Exception("Script is too large (too many chunks) to be sent\nPossible fix: Increase maximum chunk size");
	dbg << "	_cargoChunkSize: " << _cargoChunkSize << std::endl;
	dbg << "	_chunkCount: " << _chunkCount << std::endl;
}	

void ScriptChunker::InitSizes ()
{
	dbg << "ScriptChunker::InitSizes" << std::endl;
	Assert (_scriptHdr != 0 && (_cmdList != 0 || _scriptList != 0));
	CountingSerializer out;
	if (_txHdr != 0)
	{
		_txHdr->Save (out);
		_transHdrSize = out.GetSize ();
		dbg << "	_transHdrSize: " << _transHdrSize << std::endl;
		out.ResetSize ();
	}

	_scriptHdr->Save (out);
	_scriptHdrSize = out.GetSize ();
	dbg << "	_scriptHdrSize: " << _scriptHdrSize << std::endl;
	out.ResetSize ();

	if (_cmdList != 0)
	{
		Assert (_scriptList == 0);
		_cmdList->Save (out);
		_cmdListSize = out.GetSize ();
		dbg << "	_cmdListSize: " << _cmdListSize << std::endl;
		out.ResetSize ();
	}
	else
	{
		Assert (_scriptList != 0);
		_scriptList->Save (out);
		_scriptListSize = out.GetSize ();
		dbg << "	_scriptListSize: " << _scriptListSize << std::endl;
		out.ResetSize ();
	}

	if (_scriptTrailer != 0)
	{
		_scriptTrailer->Save (out);
		_trailerSize = out.GetSize ();
		dbg << "	_trailerSize: " << _trailerSize << std::endl;
		out.ResetSize ();
	}
	if (_dispatcherScript != 0)
	{
		_dispatcherScript->Save (out);
		_dispScriptSize = out.GetSize ();
		dbg << "	_dispScriptSize: " << _dispScriptSize << std::endl;
		out.ResetSize ();
	}
}

ChunkSeq::ChunkSeq (ScriptChunker & chunker)
: _chunker (chunker),
  _tmpFilePath(_randomName.GetString() + "FullSync.bin")
{
	dbg << "-->ChunkSeq::ChunkSeq" << std::endl;
	_num = chunker.GetChunkNumber (); // force calculation
	if (!chunker.IsSingleChunk ())
		_num = 1; // start with 1
	_count = chunker.GetChunkCount ();

	try
	{
		FileSerializer out(_tmpFilePath.GetFilePath());
		File::Size size(chunker.CargoSize());
		dbg << "    Cargo Size: " << chunker.CargoSize() << std::endl;
		// Test disk space
		out.Resize(size);
		out.Resize(File::Size());

		if (chunker._cmdList != 0)
			chunker._cmdList->Save (out);
		else if (chunker._scriptList != 0)
			chunker._scriptList->Save (out);
	}
	catch(Win::Exception e)
	{
		throw Win::Exception("Not enough space on disk ", _tmpFilePath.GetDrive(), e.GetError());
	}
	_tmpFileView.reset(new FileViewRo(_tmpFilePath.GetFilePath()));
	dbg << "<--ChunkSeq::ChunkSeq" << std::endl;
}

unsigned char const * ChunkSeq::GetChunk () const
{
	File::Offset chunkOffset(_chunker.GetCargoChunkSize());
	chunkOffset *= (_num - 1);
	unsigned size = (_num != _count)? _chunker.GetCargoChunkSize (): 0; // size 0 means all the way to the end of file
	return reinterpret_cast<unsigned char const *>(_tmpFileView->GetBuf(chunkOffset, size));
}

unsigned ChunkSeq::GetChunkSize () const 
{
	if (_num != _count)
		return _chunker.GetCargoChunkSize ();
	else
	{
		// long long math
		long long chunkSize = _chunker.GetCargoChunkSize ();
		return static_cast<unsigned>(_chunker.CargoSize() - (_num - 1) * chunkSize);
	}
}
