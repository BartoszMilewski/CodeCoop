//-------------------------------------
// (c) Reliable Software 1999 -- 2005
// ------------------------------------

#include "precompiled.h"
#include "BitStream.h"
#include "PackerGlobal.h"

#include <Dbg/Assert.h>
#include <StringOp.h>

void BitBucket::Save (ByteDeque & buf)
{
	buf.push_back (_bytes [3]);
	_crc.PutByte (_bytes [3]);
	buf.push_back (_bytes [2]);
	_crc.PutByte (_bytes [2]);
	buf.push_back (_bytes [1]);
	_crc.PutByte (_bytes [1]);
	buf.push_back (_bytes [0]);
	_crc.PutByte (_bytes [0]);
	_bits = 0;
}

void BitBucket::Flush (ByteDeque & buf, int bitCount)
{
	for (unsigned int i = 3; bitCount > 0; --i)
	{
		buf.push_back (_bytes [i]);
		_crc.PutByte (_bytes [i]);
		bitCount -= ByteBits; 
	}
	_bits = 0;
}

// Returns next read buffer position after filling up the bit bucket
unsigned long BitBucket::FillIn (InBitBuffer const & buf, unsigned long bufPos)
{
	int i = Capacity/ByteBits - 1;
	do
	{
		unsigned char byte = buf.at (bufPos);
		_crc.PutByte (byte);
		_bytes [i] = byte;
		bufPos++;
		--i;
	} while (i >= 0 && bufPos < buf.size ());
	return bufPos;
}

bool BitBucket::CheckCrc (InBitBuffer const & buf, unsigned long bufPos, unsigned int crcBytesLeft)
{
	while (crcBytesLeft != 0)
	{
		unsigned char byte = buf.at (bufPos);
		_crc.PutByte (byte);
		++bufPos;
		--crcBytesLeft;
	}
	return _crc.Done () == 0;
}

void OutputBitStream::WriteBits (unsigned long long dcode, int len)
{
	unsigned long code = static_cast<unsigned long>(dcode);
	//                      _bitWritePos
	//	                         |                      2 3 3
	//	 0 1 2                   V                      9 0 1
	//	+-+-+-+-----------------+-+--------------------+-+-+-+
	//	| | | |                 | |                    | | | |  _bitBucket
	//	+-+-+-+-----------------+-+--------------------+-+-+-+
	//
	//	                        +-+-+-+
	//	                        | | | |<---------------------- shiftLeft
	//	                        +-+-+-+
	//	                                                 +-+-+-+-+-+
	//	                                                 | | | | | |
	//	                                                 +-+-+-+-+-+
	//	                                                     -------> shiftRight
	//
	Assert (len <= BitBucket::Capacity);
	Assert (0 <= _bitWritePos && _bitWritePos < BitBucket::Capacity);
	int shiftLeft = BitBucket::Capacity - _bitWritePos - len;
	if (shiftLeft > 0)
	{
		// Code will completely fit into the bucket
		code <<= shiftLeft;
		_bitBucket |= code;
		_bitWritePos += len;
	}
	else if (shiftLeft < 0)
	{
		// Only code part will fit into the bucket
		unsigned long codeToWrite = code;
		int shiftRight = -shiftLeft;
		codeToWrite >>= shiftRight;
		_bitBucket |= codeToWrite;
		_bitBucket.Save (_buf);
		// len - shiftRight written; shiftRight remaining
		shiftLeft = BitBucket::Capacity - shiftRight;
		code <<= shiftLeft;
		_bitBucket |= code;
		_bitWritePos = shiftRight;
	}
	else
	{
		// Code will completely fit into the bucket
		// and the bucket is full
		Assert (shiftLeft == 0);
		_bitBucket |= code;
		_bitBucket.Save (_buf);
		_bitWritePos = 0;
	}
}

void OutputBitStream::End ()
{
	_bitBucket.Flush (_buf, _bitWritePos);
	_bitWritePos = 0;
	// append CRC
	Crc::Type crc = _bitBucket.GetCrc ();
	_buf.push_back (static_cast<unsigned char>( crc >> 3 * ByteBits));
	_buf.push_back (static_cast<unsigned char>((crc >> 2 * ByteBits) & 0xff));
	_buf.push_back (static_cast<unsigned char>((crc >> 1 * ByteBits) & 0xff));
	_buf.push_back (static_cast<unsigned char>( crc & 0xff));
}

unsigned long InputBitStream::NextBits (int len)
{
	unsigned long result = 0;
	for (int k = 0; k < len; k++)
	{
		result <<= 1;
		result += NextBit ();
	}
	return result;
}

unsigned long InputBitStream::NextBit ()
{		
	if (_bucketMask == 0)
	{
		// All bits from the bucked used -- fill in bucket with new bits from the file
		if (_bufReadPos > _endOfCompressedData)
		{
			Win::ClearError ();
			throw Win::Exception ("Cannot decompress -- truncated compressed data.");
		}
		_bucketMask = 0x80000000;
		_bufReadPos = _bitBucket.FillIn (*this, _bufReadPos);
	}

	unsigned long bit = _bitBucket.GetBit (_bucketMask);
	_bucketMask >>= 1;
	return bit;
}

unsigned long InputBitStream::GetUlong ()
{
	unsigned long result = 0;
    int transl = 0;
	do
	{
		result += (NextBits (3) << transl);
		transl += 3;
	} while (NextBit ());

	return result;
}

bool InputBitStream::CheckCrc ()
{
	// CRC is placed immediately after the compressed data.
	// The last read bucked may contain part of CRC, so
	// keep reading from the _bufReadPos.
	//
	//	_bucketMask		unreadBytesInBucket
	//	0x80000000			4
	//	0x00800000			3
	//	0x00008000			2
	//	0x00000080			1
	//	0x00000000			0

	unsigned int unreadBytesInBucket = 0;
	if (_bucketMask != 0)
	{
		if (_bucketMask == 0x80000000)
			unreadBytesInBucket = 4;
		else if (_bucketMask >= 0x00800000)
			unreadBytesInBucket = 3;
		else if (_bucketMask >= 0x00008000)
			unreadBytesInBucket = 2;
		else if (_bucketMask >= 0x00000080)
			unreadBytesInBucket = 1;
	}

	unsigned int bytesLeftInBuf = size () - _bufReadPos;
	if (unreadBytesInBucket + bytesLeftInBuf < sizeof (Crc::Type))
		throw Win::Exception ("Cannot decompress -- truncated CRC data.");

	unsigned int crcBytesLeft = sizeof (Crc::Type) - unreadBytesInBucket;
	return _bitBucket.CheckCrc (*this, _bufReadPos, crcBytesLeft);
}

PackedULong::PackedULong (InputBitStream & input)
{
	_l = 0;
    unsigned int shiftLeft = 0;
	do
	{
		_l |= (input.NextBits (3) << shiftLeft);
		shiftLeft += 3;
	} while (input.NextBit ());
}

void PackedULong::Write (OutputBitStream & output)
{
	int bitCount = 0;
	unsigned long tmp = _l;
	while (tmp != 0)
	{
		tmp >>= 1;
		++bitCount;
	}
	while (bitCount > 0)
	{
		unsigned int k = (_l & 7);
		bitCount -= 3;
		_l >>= 3;
		output.WriteBits (k, 3);
		k = bitCount > 0 ? 1 : 0;
		output.WriteBits (k, 1);
	}
}

#if !defined (NDEBUG)
// Bit stream unit test
void WriteRandomBitSequence (OutputBitStream & out, unsigned int testBitCount)
{
	unsigned int byteWriteCount = testBitCount / ByteBits;
	unsigned int bitWriteCount = testBitCount - byteWriteCount * ByteBits;
	for (unsigned int i = 0; i < byteWriteCount; ++i)
	{
		unsigned int byte = (rand () & 0xff);
		out.WriteBits (byte, ByteBits);
	}
	for (unsigned int i = 0; i < bitWriteCount; ++i)
	{
		unsigned int bit = ((rand () & 0x100) != 0) ? 1 : 0;
		out.WriteBits (bit, 1);
	}
	out.End ();	// Finalize output bit stream by appending CRC

	if (bitWriteCount != 0)
		++byteWriteCount;
	if (out.size () != (byteWriteCount + sizeof (Crc::Type)))
	{
		std::string info ("Output bit stream length = ");
		info += ToString (out.size ());
		info += "; expected length = ";
		info += ToString (testBitCount + sizeof (Crc::Type));
		throw Win::Exception ("Bit stream unit test failed.", info.c_str ());
	}
}

void ReadTestBitSequence (std::vector<unsigned char> const & buf, unsigned int testBitCount)
{
	InputBitStream in (&buf [0], buf.size ());
	for (unsigned int i = 0; i < testBitCount; ++i)
		in.NextBit ();

	if (!in.CheckCrc ())
	{
		std::string info ("CRC mismatch; test length = ");
		info += ToString (testBitCount);
		throw Win::Exception ("Bit stream unit test failed.", info.c_str ());
	}
}

namespace UnitTest
{
	void BitStream ()
	{
		// Test normal buffer
		for (unsigned int testBitCount = 0; testBitCount < 64; ++testBitCount)
		{
			OutputBitStream out;
			WriteRandomBitSequence (out, testBitCount);

			std::vector<unsigned char> buf (out.begin (), out.end ());
			ReadTestBitSequence (buf, testBitCount);
		}

		// Test longer buffer
		for (unsigned int testBitCount = 0; testBitCount < 64; ++testBitCount)
		{
			// Write testBitCount random byte sequence
			OutputBitStream out;
			WriteRandomBitSequence (out, testBitCount);

			std::vector<unsigned char> buf (out.begin (), out.end ());
			// Add random bytes at buffer end
			unsigned int addCount = rand () & 0xf;
			for (unsigned i = 0; i < addCount; i++)
				buf.push_back (rand () & 0xff);

			ReadTestBitSequence (buf, testBitCount);
		}

	#if 0
		// Test truncated buffer
		for (unsigned int testBitCount = 0; testBitCount < 64; ++testBitCount)
		{
			// Write testBitCount random byte sequence
			OutputBitStream out;
			WriteRandomBitSequence (out, testBitCount);

			std::vector<unsigned char> buf (out.begin (), out.end ());
			// Truncate random bytes at buffer end
			unsigned int truncateCount = rand () & (sizeof(Crc::Type) - 1);
			if (truncateCount == 0)
				truncateCount = 1;
			for (unsigned i = 0; i < truncateCount; i++)
				buf.pop_back ();

			bool readFailed = false;
			try
			{
				ReadTestBitSequence (buf, testBitCount);
			}
			catch (Win::Exception )
			{
				readFailed = true;
			}
			catch ( ... )
			{
				std::string info ("GPF during truncated buffer read; test length = ");
				info += ToString (testBitCount);
				throw Win::Exception ("Bit stream unit test failed.", info.c_str ());
			}
			if (!readFailed)
			{
				std::string info ("Truncated buffer read successfully; test length = ");
				info += ToString (testBitCount);
				throw Win::Exception ("Bit stream unit test failed.", info.c_str ());
			}
		}
	#endif
	}
}

#endif
