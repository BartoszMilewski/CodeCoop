#if !defined (CODINGTABLES_H)
#define CODINGTABLES_H
//-----------------------------------------------------
//  CodingTables.h
//  (c) Reliable Software 2001
//-----------------------------------------------------

#include "BitCode.h"
#include "Bucketizer.h"
#include "HuffmanTree.h"

class Statistics;
class OutputBitStream;
class InputBitStream;

#include <Dbg/Assert.h>

class CodingTables
{
public:
	// Create coding tables -- use dynamic Huffman codes
	CodingTables (Statistics const & stats,
				  RepLenBucketizer const & repLenBucketizer,
				  RepDistBucketizer const & repDistBucketizer);
	// Create coding tables -- use static Huffman codes
	CodingTables (RepLenBucketizer const & repLenBucketizer,
				  RepDistBucketizer const & repDistBucketizer);
	// Create coding tables -- read header from input bit stream
	CodingTables (InputBitStream & input,
				  RepLenBucketizer const & repLenBucketizer,
				  RepDistBucketizer const & repDistBucketizer);
	~CodingTables ();

	bool IsEndOfFile (unsigned int symbol) const
	{
		return symbol == EndOfFile;
	}
	bool IsLiteral (unsigned int symbol) const
	{
		return symbol < EndOfFile;
	}
	bool IsRepetitionStart (unsigned int symbol) const
	{
		return _repLenBucketizer.IsRepetitionLengthBucket (symbol);
	}

	BitCode GetLiteralCode (unsigned char symbol) const
	{
		Assert (symbol < _mainCode.size ());
		return _mainCode [symbol];
	}

	BitCode GetEndOfBlockCode () const
	{
		return _mainCode [256];
	}

	BitCode GetLengthCode (unsigned int repLen) const
	{
		unsigned int lenBucket = _repLenBucketizer.GetBucket (repLen);
		BitCode lenCode (_mainCode [lenBucket]);
		unsigned int extraBitsValue;
		unsigned int extraBitsCount = _repLenBucketizer.GetExtraBits (lenBucket, repLen, extraBitsValue);
		lenCode.AppendBits (extraBitsValue, extraBitsCount);
		return lenCode;
	}

	BitCode GetDistanceCode (unsigned int repDist) const
	{
		unsigned int distBucket = _repDistBucketizer.GetBucket (repDist);
		BitCode distCode (_distanceCode [distBucket]);
		unsigned int extraBitsValue;
		unsigned int extraBitsCount = _repDistBucketizer.GetExtraBits (distBucket, repDist, extraBitsValue);
		distCode.AppendBits (extraBitsValue, extraBitsCount);
		return distCode;
	}

	unsigned int DecodeLiteral (InputBitStream & input) const
	{
		return _mainDecoder->DecodeSymbol (input);
	}

	void DecodeRepetition (unsigned int bucketNo,
						   InputBitStream & input,
						   unsigned int & repetitionLength,
						   unsigned int & repetitionDistance) const
	{
		repetitionLength = _repLenBucketizer.DecodeExtraBits (bucketNo, input);
		unsigned int distBucketNo = _distDecoder->DecodeSymbol (input);
		repetitionDistance = _repDistBucketizer.DecodeExtraBits (distBucketNo, input);
	}

	void Write (OutputBitStream & output, bool lastBlock) const;

private:
	// Coding tables header -- written to the output bit stream
	// or read from the input bit stream.
	class Header
	{
	public:
		bool Create (CodeBitLengths const & mainCodeLen,
					 CodeBitLengths const & distCodeLen);
		bool Read (InputBitStream & input,
				   CodeBitLengths & mainCodeLen,
				   CodeBitLengths & distCodeLen);
		void Write (OutputBitStream & output) const;

	private:
		enum
		{
			MinRepetitionLen = 3,
			MaxSmallHeaderEntryCount = 19
		};

		class Layout
		{
		public:
			Layout (unsigned int size, unsigned int bitCount, unsigned int maxSimpleCodeLen,
					unsigned int nonZero, unsigned int shortZero, unsigned int longZero,
					unsigned int const * order, unsigned int minCount)
				: _size (size),
				  _bitCount (bitCount),
				  _maxSimpleCodeLen (maxSimpleCodeLen),
				  _nonZero (nonZero),
				  _shortZero (shortZero),
				  _longZero (longZero),
				  _order (order),
				  _minEntryCount (minCount)
			{}

			unsigned int GetSize () const { return _size; }
			unsigned int GetBitCount () const { return _bitCount; }
			unsigned int GetMaxSimpleCodeLen () const { return _maxSimpleCodeLen; }
			unsigned int GetNonZeroRepetitionMarker () const { return _nonZero; }
			unsigned int GetShortZeroRepetitionMarker () const { return _shortZero; }
			unsigned int GetLongZeroRepetitionMarker () const { return _longZero; }
			unsigned int GetPosition (unsigned int i) const { return _order [i]; }
			unsigned int GetEntryLimit () const { return _minEntryCount; }

		private:
			unsigned int		_size;		// Max header size
			unsigned int		_bitCount;	// How many bits used to read/wrie header entry
			unsigned int		_maxSimpleCodeLen;
			// Repetiton markers in the compressed header
			unsigned int		_nonZero;
			unsigned int		_shortZero;
			unsigned int		_longZero;
			// Header entry read/write order
			unsigned int const *_order;
			unsigned int		_minEntryCount;
		};

		class SmallLayout : public Layout
		{
		public:
			SmallLayout ()
				: Layout (HeaderSize, HeaderBitCount, MaxSimpleCodeLen,
						  NonZeroRepetitionMarker, ShortZeroRepetitionMarker, LongZeroRepetitionMarker,
						  _order, MinEntryCount)
			{}

		private:
			enum
			{
				HeaderSize = 19,
				HeaderBitCount = 3,
				MaxSimpleCodeLen = 15,
				MinRepetitionLen = 3,
				NonZeroRepetitionMarker = 16,
				ShortZeroRepetitionMarker = 17,
				LongZeroRepetitionMarker = 18,
				MinEntryCount = 4
			};

		private:
			static unsigned int	_order [HeaderSize];
		};

		class BigLayout : public Layout
		{
		public:
			BigLayout ()
				: Layout (HeaderSize, HeaderBitCount, MaxSimpleCodeLen,
						  NonZeroRepetitionMarker, ShortZeroRepetitionMarker, LongZeroRepetitionMarker,
						  _order, MinEntryCount)
			{}

		private:
			enum
			{
				HeaderSize = 35,
				HeaderBitCount = 4,
				MaxSimpleCodeLen = 31,
				MinRepetitionLen = 3,
				NonZeroRepetitionMarker = 32,
				ShortZeroRepetitionMarker = 33,
				LongZeroRepetitionMarker = 34,
				MinEntryCount = 20
			};

		private:
			static unsigned int	_order [HeaderSize];
		};

	private:
		void Compress (CodeBitLengths const & mainCodeLen,
					   CodeBitLengths const & distCodeLen);
		void WriteCode (unsigned int codeBitLen, int repetitionLen);
		void CollectStatistics (std::vector<unsigned int> & codeLengthStats);
		void CreateHeaderCodes (CodeBitLengths const & mainCodeLen,
								CodeBitLengths const & distCodeLen);
		void WriteCodeLengths (OutputBitStream & output) const;
		void ReadCodeLengths (InputBitStream & input,
							  CodeTree const & decodingTree,
							  CodeBitLengths & mainCodeBitLen,
							  CodeBitLengths & distCodeBitLen);
#if !defined NDEBUG
		void VerifyCodes () const;
#endif

	private:
		std::unique_ptr<Layout>		_layout;				// Header layout
		unsigned int				_mainCodeLengthCount;	// HLIT from deflate spec
															// This is the number of non-zero
															// code-legths -- the last zero code-leghts
															// are trimmed of.
		unsigned int				_distCodeLengthCount;	// HDIST from deflate spec -- also non-zero
															// code-lengths
		std::vector<unsigned int>	_packedCodeLengths;
		std::vector<BitCode>		_headerCode;
	};

private:
	enum
	{
		StaticBlock = 1,				// Block compressed using static Huffman codes
		DynamicBlock = 2,				// Block compressed using dynamic Huffman codes
		EndOfFile = 256,
		StaticMainCodeRangeCount = 4,
		StaticDistCodeRangeCount = 1
	};

#if !defined NDEBUG
	void VerifyCodes (bool bigHeader) const;
#endif

private:
	RepLenBucketizer const &	_repLenBucketizer;
	RepDistBucketizer const &	_repDistBucketizer;
	bool						_isDynamicHuffmanCode;
	bool						_isLastBlock;
	Header						_header;
	static CodeRange			_staticMainCodeRange [StaticMainCodeRangeCount];
	static CodeRange			_staticDistCodeRange [StaticDistCodeRangeCount];
	std::vector<BitCode>		_mainCode;
	std::vector<BitCode>		_distanceCode;
	std::unique_ptr<CodeTree>		_mainDecoder;
	std::unique_ptr<CodeTree>		_distDecoder;
};

#endif
