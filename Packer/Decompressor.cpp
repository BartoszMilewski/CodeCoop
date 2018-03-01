//-------------------------------------
// (c) Reliable Software 1999 -- 2004
// ------------------------------------

#include "precompiled.h"
#include "Decompressor.h"
#include "NamedBlock.h"
#include "PackerGlobal.h"
#include "CodingTables.h"
#include "Bucketizer.h"
#include "BitStream.h"

#include <Ex/Winex.h>
#include <Dbg/Assert.h>
#include <StringOp.h>

// return true if CRC succeeds

bool Decompressor::Decompress (unsigned char const * buf, File::Size size, OutBlockVector & outBlocks)
{
	if (size.IsLarge ())
	{
		Win::ClearError ();
		throw Win::Exception ("Cannot decompress --  file size exceeds 4GB.");
	}
	InputBitStream input (buf, size.Low ());
	RepLenBucketizer repLenBucketizer;
	RepDistBucketizer repDistBucketizer;

	unsigned int version = input.NextBits (VersionBits);
	if (version != Version)
	{
		Win::ClearError ();
		throw Win::Exception ("Cannot decompress -- version mismatch or corrupted script.");
	}
	PackedULong fileCount (input);

	CodingTables codingTables (input, repLenBucketizer, repDistBucketizer);

    outBlocks.resize (fileCount);
	for (unsigned int k = 0; k < fileCount; k++)
	{
		// Decompress file name length
		PackedULong fileNameLen (input);
		if (fileNameLen == 0)
		{
			Win::ClearError ();
			throw Win::Exception ("Cannot decompress -- corrupted script.");
		}
		// Decompress file name
		std::string fileName;
		while (fileNameLen > 0)
		{
			unsigned int tmp = codingTables.DecodeLiteral (input);
			fileName.push_back (static_cast<char>(tmp));
			--fileNameLen;
		}
		OutNamedBlock & curBlock = outBlocks [k];
		curBlock.Init (fileName);
		// Decompress file contents
		Buffer buffer (curBlock);
		unsigned int symbol = 0;
		while (!codingTables.IsEndOfFile (symbol))
		{
			symbol = codingTables.DecodeLiteral (input);
			if (codingTables.IsRepetitionStart (symbol))
			{
				unsigned int repetitionLength;
				unsigned int repetitionDistance;
				codingTables.DecodeRepetition (symbol, input, repetitionLength, repetitionDistance);
				buffer.Copy (repetitionDistance, repetitionLength);
			}
			else if (codingTables.IsLiteral (symbol))
			{
				buffer.Write (symbol);
			}
		}
		buffer.Flush ();
	}
	return input.CheckCrc ();
}

Decompressor::Buffer::Buffer (OutNamedBlock & block)
	: _block (block),
	  _bufWritePos (0)
{
	Assert (RepLenBucketizer::GetMaxRepetitionLength () < RepDistBucketizer::GetMaxRepetitionDistance ());
	_buf.resize (RepDistBucketizer::GetMaxRepetitionDistance ());
}

void Decompressor::Buffer::Write (unsigned char byte)
{
	if (_bufWritePos == _buf.size ())
	{
		_block.Write (&_buf [0], _buf.size ());
		_bufWritePos = 0;
	}
	_buf [_bufWritePos] = byte;
	++_bufWritePos;
}

void Decompressor::Buffer::Copy (unsigned int distance, unsigned int count)
{
	// Auto copy in the circular buffer
	const unsigned int bufSize = _buf.size ();
	Assert (count <= bufSize);
	Assert (distance <= bufSize);
	if (_bufWritePos == bufSize)
	{
		_block.Write (&_buf [0], _buf.size ());
		_bufWritePos = 0;
	}
	Assert (_bufWritePos < bufSize);
	if (distance <= _bufWritePos)
	{
		unsigned int freeSpace = bufSize - _bufWritePos;
		unsigned int begin = _bufWritePos - distance;
		if (count <= freeSpace)
		{
			//		       count
			//		0    <------->                        buf size
			//		+-------------------------------------+
			//		|                                     |
			//		+-------------------------------------+
			//		    ^               ^   free space
			//		    |<------------->|<---------------->
			//		begin   distance   _bufWritePos
			//
			unsigned char * from = &_buf [begin];
			unsigned char * to = &_buf [_bufWritePos];
			overlapped_copy (from, from + count, to);
			_bufWritePos += count;
		}
		else
		{
			//		       count
			//		0    <----------------------------->  buf size
			//		+-------------------------------------+
			//		|                                     |
			//		+-------------------------------------+
			//		    ^               ^   free space
			//		    |<------------->|<---------------->
			//		begin   distance   _bufWritePos
			//
			unsigned char * from = &_buf [begin];
			unsigned char * to = &_buf [_bufWritePos];
			overlapped_copy (from, from + freeSpace, to);
			// Write full buffer to the out block
			Assert (_bufWritePos  + freeSpace == bufSize);
			_block.Write (&_buf [0], bufSize);
			_bufWritePos = 0;
			unsigned int bytesLeft2Copy = count - freeSpace;
			begin += freeSpace;
			Assert (begin < bufSize);
			freeSpace = bufSize - begin;
			from = &_buf [begin];
			to = &_buf [_bufWritePos];
			if (bytesLeft2Copy <= freeSpace)
			{
				overlapped_copy (from, from + bytesLeft2Copy, to);
				_bufWritePos += bytesLeft2Copy;
			}
			else
			{
				// Copy start index wraps
				overlapped_copy (from, from + freeSpace, to);
				_bufWritePos += freeSpace;
				bytesLeft2Copy -= freeSpace;
				begin = 0;
				from = &_buf [begin];
				to = &_buf [_bufWritePos];
				Assert (bytesLeft2Copy < bufSize - _bufWritePos);
				overlapped_copy (from, from + bytesLeft2Copy, to);
				_bufWritePos += bytesLeft2Copy;
			}
		}
	}
	else
	{
		unsigned int begin = bufSize - (distance - _bufWritePos);
		unsigned int freeSpace = bufSize - begin;
		if (count <= freeSpace)
		{
			//	count
			//	<--->   0                                     buf size
			//			+-------------------------------------+
			//			|                                     |
			//			+-------------------------------------+
			//		          ^                       ^
			//	|<----------->|                       |<------>   
			//	    distance _bufWritePos         begin  free space
			//
			unsigned char * from = &_buf [begin];
			unsigned char * to = &_buf [_bufWritePos];
			overlapped_copy (from, from + count, to);
			_bufWritePos += count;
		}
		else
		{
			//	count
			//	<------------------------->                   buf size
			//			+-------------------------------------+
			//			|                                     |
			//			+-------------------------------------+
			//		          ^                       ^
			//	|<----------->|                       |<------>   
			//	    distance _bufWritePos         begin  free space
			//
			unsigned char * from = &_buf [begin];
			unsigned char * to = &_buf [_bufWritePos];
			overlapped_copy (from, from + freeSpace, to);
			_bufWritePos += freeSpace;
			if (_bufWritePos == bufSize)
			{
				// Write full buffer to the out block
				_block.Write (&_buf [0], bufSize);
				_bufWritePos = 0;
			}
			unsigned int bytesLeft2Copy = count - freeSpace;
			begin = 0;
			freeSpace = bufSize - _bufWritePos;
			from = &_buf [begin];
			to = &_buf [_bufWritePos];
			if (bytesLeft2Copy <= freeSpace)
			{
				overlapped_copy (from, from + bytesLeft2Copy, to);
				_bufWritePos += bytesLeft2Copy;
			}
			else
			{
				// Write position index wraps
				overlapped_copy (from, from + freeSpace, to);
				Assert (_bufWritePos + freeSpace == bufSize);
				// Write full buffer to the out block
				_block.Write (&_buf [0], bufSize);
				_bufWritePos = 0;
				bytesLeft2Copy -= freeSpace;
				begin += freeSpace;
				Assert (bytesLeft2Copy < bufSize);
				Assert (begin > _bufWritePos);
				Assert (begin < bufSize);
				from = &_buf [begin];
				to = &_buf [_bufWritePos];
				overlapped_copy (from, from + bytesLeft2Copy, to);
				_bufWritePos += bytesLeft2Copy;
			}
		}
	}
}

void Decompressor::Buffer::Flush ()
{
	_block.Write (&_buf [0], _bufWritePos);
}
