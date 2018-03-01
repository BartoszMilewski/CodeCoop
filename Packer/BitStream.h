#if !defined BITSTREAM_H
#define BITSTREAM_H
//-------------------------------------
// (c) Reliable Software 1999 -- 2004
// ------------------------------------

#include "BitCode.h"
#include "PackerGlobal.h"
#include "Crc.h"
#include <deque>

class InBitBuffer;

typedef std::deque<unsigned char> ByteDeque;
typedef std::deque<unsigned char>::const_iterator ByteDequeIter;

class BitBucket
{
public:
	BitBucket ()
		: _bits (0)
	{}

	static const unsigned int Capacity = LongBits;

	void operator |= (unsigned long value) { _bits |= value; }
	void Save (ByteDeque & buf);
	void Flush (ByteDeque & buf, int bitCount);
	unsigned long GetBit (unsigned long mask) const { return ((_bits & mask) != 0 ) ? 1 : 0; }
	unsigned long FillIn (InBitBuffer const & buf, unsigned long bufPos);
	Crc::Type GetCrc () { return _crc.Done (); }
	bool CheckCrc (InBitBuffer const & buf, unsigned long bufPos, unsigned int crcBytesLeft);

private:
	union
	{
		unsigned long	_bits;
		unsigned char	_bytes [Capacity/ByteBits];
	};
	Crc::Calc		_crc;
};

class OutBitBuffer
{
protected:
	BitBucket	_bitBucket;
	ByteDeque	_buf;
};

class InBitBuffer
{
public:
	InBitBuffer (unsigned char const * buf, unsigned int size)
		: _buf (buf),
		  _size (size)
	{}

	unsigned long size () const { return _size; }
	unsigned char at (unsigned long index) const { return _buf [index]; }

protected:
	BitBucket				_bitBucket;
	unsigned long			_size;
	unsigned char const *	_buf;
};

class OutputBitStream : public OutBitBuffer
{
public:
	OutputBitStream ()
		: _bitWritePos (0)
	{}

	void Write (BitCode const & code) { WriteBits (code.GetCode (), code.GetLen ()); }
	void WriteBits (unsigned long long code, int len);
	void End ();

	unsigned int size () const { return _buf.size (); }
	ByteDequeIter begin () const { return _buf.begin (); }
	ByteDequeIter end () const { return _buf.end (); }

private:
	int	_bitWritePos;
};

class InputBitStream : public InBitBuffer
{
public : 
	InputBitStream (unsigned char const * buf, unsigned int size)
		: InBitBuffer (buf, size),
		  _bufReadPos (0),
		  _endOfCompressedData (_size - sizeof (Crc::Type)),
		  _bucketMask (0)
	{}

	unsigned long NextBits (int k);
	unsigned long NextBit ();
	unsigned long GetUlong ();

	bool CheckCrc ();

private :
	unsigned long	_bufReadPos;
	unsigned long	_endOfCompressedData;
	unsigned long	_bucketMask;
};

class PackedULong
{
public:
	PackedULong (unsigned long l)
		: _l (l)
	{}
	PackedULong (InputBitStream & input);

	void Write (OutputBitStream & output);

	operator unsigned long () { return _l; }
	unsigned long & operator-- () { return --_l; }

private:
	unsigned long	_l;
};

#endif
