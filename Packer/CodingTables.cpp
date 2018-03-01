//-----------------------------------------------------
//  CodingTables.cpp
//  (c) Reliable Software 2001
//-----------------------------------------------------

#include "precompiled.h"
#include "CodingTables.h"
#include "Statistics.h"
#include "BitStream.h"

#include <Dbg/Assert.h>
#include <Ex/winex.h>
#include <bitset>

#if !defined NDEBUG
#include <LightString.h>

#if defined (PACKER_TEST)
std::unique_ptr<CodeBitLengths> senderMainCodeLengths;
std::unique_ptr<CodeBitLengths> senderDistanceCodeLengths;
#endif

#endif

// Compression with fixed Huffman codes uses predefined Huffman codes
// that are not present in the compressed block. The Huffman code lengths for
// the Literal/Length alphabet are:
//
//	Literal Value	Bits	Codes
//	-------------	----	-----
//	  0 - 143		  8		00110000 (0x30) -- 10111111 (0xBF)
//	144 - 255		  9		110010000 (0x190) -- 111111111 (0x1FF)
//	256 - 279		  7		0000000 (0x0) -- 0010111 (0x17)
//	280 - 287		  8		11000000 (0xC0) -- 11000111 (0xC7)
//
// The Huffman code lengths for the distance alphabet are:
//
//	Literal Value	Bits	Codes
//	-------------	----	-----
//	  0 - 31		  5		00000 (0x0) -- 11111 (0x1F)

CodeRange CodingTables::_staticMainCodeRange [] =
{
	CodeRange (256, 279, 7, 0),
	CodeRange (  0, 143, 8, 0x30),
	CodeRange (280, 287, 8, 0xc0),
	CodeRange (144, 255, 9, 0x190)
};

CodeRange CodingTables::_staticDistCodeRange [] =
{
	CodeRange (0, 31, 5, 0)
};

CodingTables::CodingTables (Statistics const & stats,
							RepLenBucketizer const & repLenBucketizer,
							RepDistBucketizer const & repDistBucketizer)
	: _repLenBucketizer (repLenBucketizer),
	  _repDistBucketizer (repDistBucketizer),
	  _isDynamicHuffmanCode (true)
{
	// Build Huffman trees and collect Huffman code bit lenght statistics
	std::vector<unsigned int> const & mainStats = stats.GetMainStats ();
	HuffmanTree mainTree (mainStats);
	CodeBitLengths mainCodeBitLen (mainTree, mainStats.size ());

	std::vector<unsigned int> const & distanceStats = stats.GetDistanceStats ();
	HuffmanTree distTree (distanceStats);
	CodeBitLengths distanceCodeBitLen (distTree, distanceStats.size ());

#if defined (PACKER_TEST)
	senderMainCodeLengths.reset (new CodeBitLengths (mainCodeBitLen));
	senderDistanceCodeLengths.reset (new CodeBitLengths (distanceCodeBitLen));
#endif

	// Rebuild Huffman trees in such a way that they define new codes having
	// two additional properties:
	//		1. All codes of a given bit length have lexicographically
	//		   consecutive values, in the same order as the symbols
	//		   they represent.
	//		2. Shorter codes lexicographically precede longer codes.
	CodeTree mainCodeTree (mainCodeBitLen);
	_mainCode.resize (mainStats.size (), BitCode (0, 0));
	mainCodeTree.CalcCodes (_mainCode);
	CodeTree distCodeTree (distanceCodeBitLen);
	_distanceCode.resize (distanceStats.size (), BitCode (0, 0));
	distCodeTree.CalcCodes (_distanceCode);

	// Create coding tables header
	bool bigHeader = _header.Create (mainCodeBitLen, distanceCodeBitLen);

#if !defined NDEBUG
	VerifyCodes (bigHeader);
#endif
}

CodingTables::CodingTables (RepLenBucketizer const & repLenBucketizer,
							RepDistBucketizer const & repDistBucketizer)
	: _repLenBucketizer (repLenBucketizer),
	  _repDistBucketizer (repDistBucketizer),
	  _isDynamicHuffmanCode (false),
	  _mainCode (Statistics::GetLiteralAlphabetSize ()),
	  _distanceCode (Statistics::GetDistanceAlphabetSize ())
{
	for (unsigned int i = 0; i < StaticMainCodeRangeCount; ++i)
	{
		CodeRange const & range = _staticMainCodeRange [i];
		unsigned int firstSymbol = range.GetFirstSymbol ();
		unsigned int lastSymbol = range.GetLastSymbol ();
		unsigned int bitLength = range.GetBitLength ();
		unsigned int code = range.GetFirstCode ();
		for (unsigned int symbol = firstSymbol; symbol <= lastSymbol; ++symbol, ++code)
		{
			_mainCode [symbol] = BitCode (code, bitLength);
		}
	}

	for (unsigned int j = 0; j < StaticDistCodeRangeCount; ++j)
	{
		CodeRange const & range = _staticDistCodeRange [j];
		unsigned int firstSymbol = range.GetFirstSymbol ();
		unsigned int lastSymbol = range.GetLastSymbol ();
		unsigned int bitLength = range.GetBitLength ();
		unsigned int code = range.GetFirstCode ();
		for (unsigned int symbol = firstSymbol; symbol <= lastSymbol; ++symbol, ++code)
		{
			_distanceCode [symbol] = BitCode (code, bitLength);
		}
	}

#if !defined NDEBUG
	VerifyCodes (false);
#endif
}

CodingTables::CodingTables (InputBitStream & input,
							RepLenBucketizer const & repLenBucketizer,
							RepDistBucketizer const & repDistBucketizer)
	: _repLenBucketizer (repLenBucketizer),
	  _repDistBucketizer (repDistBucketizer)
{
	_isLastBlock = input.NextBit () != 0;
	unsigned int blockType = input.NextBits (2);
	_isDynamicHuffmanCode = blockType == DynamicBlock;
	bool bigHeader = false;
	if (_isDynamicHuffmanCode)
	{
		CodeBitLengths mainCodeBitLen (Statistics::GetLiteralAlphabetSize ());
		CodeBitLengths distanceCodeBitLen (Statistics::GetDistanceAlphabetSize ());
		bigHeader = _header.Read (input, mainCodeBitLen, distanceCodeBitLen);
#if defined (PACKER_TEST)
		if (senderMainCodeLengths.get () != 0 && !mainCodeBitLen.IsEqual (*senderMainCodeLengths))
			throw Win::Exception ("Received main code code-lengths don't match");
		if (senderDistanceCodeLengths.get () != 0 && !distanceCodeBitLen.IsEqual (*senderDistanceCodeLengths))
			throw Win::Exception ("Received distance code code-lengths don't match");
#endif
		_mainDecoder.reset (new CodeTree (mainCodeBitLen));
		_distDecoder.reset (new CodeTree (distanceCodeBitLen));
	}
	else
	{
		_mainDecoder.reset (new CodeTree (_staticMainCodeRange, StaticMainCodeRangeCount));
		_distDecoder.reset (new CodeTree (_staticDistCodeRange, StaticDistCodeRangeCount));
#if !defined NDEBUG
		_mainCode.resize (Statistics::GetLiteralAlphabetSize (), BitCode (0, 0));
		std::vector<BitCode> mainStaticCode (Statistics::GetLiteralAlphabetSize ());
		for (unsigned i = 0; i < StaticMainCodeRangeCount; ++i)
		{
			CodeRange const & range = _staticMainCodeRange [i];
			unsigned int firstSymbol = range.GetFirstSymbol ();
			unsigned int lastSymbol = range.GetLastSymbol ();
			unsigned int bitLength = range.GetBitLength ();
			unsigned int code = range.GetFirstCode ();
			for (unsigned int symbol = firstSymbol; symbol <= lastSymbol; ++symbol, ++code)
			{
				mainStaticCode [symbol] = BitCode (code, bitLength);
			}
		}
		_mainDecoder->CalcCodes (_mainCode);
		for (unsigned i = 0; i < Statistics::GetLiteralAlphabetSize (); ++i)
		{
			if (mainStaticCode [i].GetCode () != _mainCode [i].GetCode ())
			{
				Msg info;
				info << "Recipient main static code doesn't match sender code\n";
				std::bitset<16> senderCode (mainStaticCode [i].GetCode ());
				info << "Sender code: '" << senderCode << "'; length " << mainStaticCode [i].GetLen () << "\n";
				std::bitset<16> recipientCode (_mainCode [i].GetCode ());
				info << "Recipient code: '" << recipientCode << "'; length " << _mainCode [i].GetLen ();
				throw Win::Exception ("Internal error -- static codes don't match", info.c_str ());
			}
		}
		_distanceCode.resize (Statistics::GetDistanceAlphabetSize (), BitCode (0, 0));
		std::vector<BitCode> distStaticCode (Statistics::GetDistanceAlphabetSize ());
		for (unsigned j = 0; j < StaticDistCodeRangeCount; ++j)
		{
			CodeRange const & range = _staticDistCodeRange [j];
			unsigned int firstSymbol = range.GetFirstSymbol ();
			unsigned int lastSymbol = range.GetLastSymbol ();
			unsigned int bitLength = range.GetBitLength ();
			unsigned int code = range.GetFirstCode ();
			for (unsigned int symbol = firstSymbol; symbol <= lastSymbol; ++symbol, ++code)
			{
				distStaticCode [symbol] = BitCode (code, bitLength);
			}
		}
		_distDecoder->CalcCodes (_distanceCode);
		for (unsigned i = 0; i < Statistics::GetDistanceAlphabetSize (); ++i)
		{
			if (distStaticCode [i].GetCode () != _distanceCode [i].GetCode ())
			{
				Msg info;
				info << "Recipient distance static code doesn't match sender code\n";
				std::bitset<16> senderCode (distStaticCode [i].GetCode ());
				info << "Sender code: '" << senderCode << "'; length " << distStaticCode [i].GetLen () << "\n";
				std::bitset<16> recipientCode (_distanceCode [i].GetCode ());
				info << "Recipient code: '" << recipientCode << "'; length " << _distanceCode [i].GetLen ();
				throw Win::Exception ("Internal error -- static codes don't match", info.c_str ());
			}
		}
#endif
	}

#if !defined NDEBUG
	VerifyCodes (bigHeader);
#endif
}

CodingTables::~CodingTables ()
{
	_mainDecoder.reset (0);
	_distDecoder.reset (0);	
}

void CodingTables::Write (OutputBitStream & output, bool lastBlock) const
{
	// Header format is as follows:
	//
	// 1 bit: 1 - last block;
	// 2 bits: 01 -- block compressed with fixed Huffman codes
	//         10 -- block compressed with dynamic Huffnam codes

	output.WriteBits (lastBlock, 1);
	output.WriteBits (_isDynamicHuffmanCode ? DynamicBlock : StaticBlock, 2);
	if (_isDynamicHuffmanCode)
		_header.Write (output);
}

#if !defined NDEBUG
void CodingTables::VerifyCodes (bool bigHeader) const
{
	unsigned int maxCodeBitLen = bigHeader ? 31 : 15;
	std::vector<unsigned int> previousCode (maxCodeBitLen + 1);
	// Check if all codes preserve lexicographical order of symbols
	std::vector<BitCode>::const_iterator iter;
	for (iter = _mainCode.begin (); iter != _mainCode.end (); ++iter)
	{
		unsigned code = static_cast<unsigned>(iter->GetCode ());
		if (code == 0)
			continue;
		unsigned int len = iter->GetLen ();
		if (len > maxCodeBitLen)
		{
			Msg info;
			info << "Code for symbol " << iter - _mainCode.begin () << " exceeds maximal code bit len - " << len;
			throw Win::Exception ("Internal error -- illegal Literal/Length Huffman code", info.c_str ());
		}
		if (code <= previousCode [len])
		{
			Msg info;
			info << "Code for symbol " << iter - _mainCode.begin () << " breaks lexicographical order\n";
			info << "Code bit len = " << len << "; previous code in this length group = " << previousCode [len] << "; this code = " << code;
			throw Win::Exception ("Internal error -- illegal Literal/Length Huffman code", info.c_str ());
		}
		else
		{
			previousCode [len] = code;
		}
	}
	std::fill (previousCode.begin (), previousCode.end (), 0);
	for (iter = _distanceCode.begin (); iter != _distanceCode.end (); ++iter)
	{
		unsigned int code = static_cast<unsigned int>(iter->GetCode ());
		if (code == 0)
			continue;
		unsigned int len = iter->GetLen ();
		if (len > maxCodeBitLen)
		{
			Msg info;
			info << "Code for symbol " << iter - _distanceCode.begin () << " exceeds maximal code bit len - " << len;
			throw Win::Exception ("Internal error -- illegal Distance Huffman code", info.c_str ());
		}
		if (code <= previousCode [len])
		{
			Msg info;
			info << "Code for symbol " << iter - _distanceCode.begin () << " breaks lexicographical order\n";
			info << "Code bit len = " << len << "; previous code in this length group = " << previousCode [len] << "; this code = " << code;
			throw Win::Exception ("Internal error -- illegal Distance Huffman code", info.c_str ());
		}
		else
		{
			previousCode [len] = code;
		}
	}
}
#endif

//
// Coding tables header
//

unsigned int CodingTables::Header::SmallLayout::_order [] =
{
	16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

unsigned int CodingTables::Header::BigLayout::_order [] =
{
	32, 33, 34, 0, 16, 15, 17, 14, 18, 13, 19, 12, 20, 11, 21, 10, 22, 9, 23, 8, 24, 7, 25, 6, 26, 5, 27, 4, 28, 3, 29, 2, 30, 1, 31
};

// Returns true if big header layout used
bool CodingTables::Header::Create (CodeBitLengths const & mainCodeLen,
								   CodeBitLengths const & distCodeLen)
{
	// The code-length sequences for literal/repetition length and repetition
	// distance alphabets themselves are compressed using a code-length Huffman code.
	// The alphabet for code-lengths is as follows:
	//
	//	0 - 15	- Represents code lengths of 0 - 15 or
	//	0 - 31  - Represents code lengths of 0 - 31 (big header)
	//	    16	- Copy the previous code lengths 3 - 6 times or
	//		32	- Copy the previous code lengths 3 - 6 timer (big header).
	//			  The next 2 bits indicate repeat count: 0 = 3 times, ... , 3 = 6 times.
	//		17	- Repeat a code length of 0 for 3 - 10 times or
	//		33	- Repeat a code length of 0 for 3 - 10 times (big header)
	//			  The next 3 bits indicate repeat count: 0 = 3 times, ... , 7 = 10 times.
	//		18	- Repeat a code length of 0 for 11 - 138 times or
	//		33	- Repeat a code length of 0 for 11 - 138 times (big header).
	//			  The next 7 bits indicate repeat count: 0 = 11 times, ... , 127 = 138 times.
	//
	// The code-length repeat count can cross from literal/repetition length list
	// to the distance list. In other words code-lengths codes are created from
	// concatenated sequences of literal/repetition length and repetition distance
	// code lengths.

	// Check if any code is longer then 15 bits
	bool bigHeader = false;
	CodeBitLengths::Iterator iter = std::find_if (mainCodeLen.begin (),
												  mainCodeLen.end (),
												  std::bind2nd (std::greater<unsigned int>(), 15));
	bigHeader = iter != mainCodeLen.end ();
	if (!bigHeader)
	{
		iter = std::find_if (distCodeLen.begin (),
							 distCodeLen.end (),
							 std::bind2nd (std::greater<unsigned int>(), 15));
		bigHeader = iter != distCodeLen.end ();
	}
	if (bigHeader)
		_layout.reset (new BigLayout ());
	else
		_layout.reset (new SmallLayout ());

	// Trimm off zero code-lengths at the end
	_mainCodeLengthCount = mainCodeLen.size ();
	while (_mainCodeLengthCount >= 257 && mainCodeLen.at (_mainCodeLengthCount - 1) == 0)
		--_mainCodeLengthCount;
	_distCodeLengthCount = distCodeLen.size ();
	while (_distCodeLengthCount >= 1 && distCodeLen.at (_distCodeLengthCount - 1) == 0)
		--_distCodeLengthCount;

	CreateHeaderCodes (mainCodeLen, distCodeLen);

	if (!bigHeader)
	{
		// Check if header code bit lengths are longer then 7 bits.
		// If this is the case we have to switch to big header layout.
		for (unsigned int i = 0; i < _headerCode.size (); ++i)
		{
			if (_headerCode [i].GetLen () > 7)
			{
				_layout.reset (new BigLayout ());
				_packedCodeLengths.clear ();
				_headerCode.clear ();
				CreateHeaderCodes (mainCodeLen, distCodeLen);
				bigHeader = true;
				break;
			}
		}
	}

#if !defined NDEBUG
	VerifyCodes ();
#endif

	return bigHeader;
}

void CodingTables::Header::Write (OutputBitStream & output) const
{
	// Every compressed block starts with header.
	// Header format is as follows:
	//
	// 5 bits: HLIT, number of Literal/Length codes - 257		(257 - 286)
	// 5 bits: HDIST, number of Distance codes - 1				(1 - 32)
	// 5 bits: HCLEN, number of Code Length codes - 4			(4 - 19) small header
	// 5 bits: HCLEN, number of Code Length codes - 4			(20 - 35) big header
	//
	// Small header:
	// (HCLEN + 4) x 3 bits: code-lengths for the code length alphabet
	// in the order: 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15.
	// These code-lengths are interpreted as 3-bit integers (0-7). A code length of
	// 0 means that corresponding symbol (literal/length or distance code-length)
	// is not used.
	//
	// Big header:
	// (HCLEN + 4) x 4 bits: code-lengths for the code length alphabet
	// in the order: 32, 33, 34, 0, 16, 15, 17, 14, 18, 13, 19, 12, 20, 11, 21, 10, 22,
	// 9, 23, 8, 24, 7, 25, 6, 26, 5, 27, 4, 28, 3, 29, 2, 30, 1, 31.
	// These code-lengths are interpreted as 4-bit integers (0-15). A code length of
	// 0 means that corresponding symbol (literal/length or distance code-length)
	// is not used.
	//
	// HLIT + 257 code lenghts for the literal/length alphabet, encoded using
	// the code-length Huffman code.
	//
	// HDIST + 1 code lengths for the distance alphabet, encoded using
	// the code-length Huffman code.

	Assert (_mainCodeLengthCount >= 257);
	output.WriteBits (_mainCodeLengthCount - 257, 5);
	Assert (_distCodeLengthCount >= 1);
	output.WriteBits (_distCodeLengthCount - 1, 5);
	// Trimm off zero code-lengths from the header
	unsigned int headerCodeLengthCount = _layout->GetSize ();
	unsigned int i = headerCodeLengthCount - 1;
	unsigned int countLimit = _layout->GetEntryLimit ();
	while (headerCodeLengthCount > countLimit && _headerCode [_layout->GetPosition (i)].GetLen () == 0)
	{
		--i;
		--headerCodeLengthCount;
	}
	Assert (headerCodeLengthCount >= countLimit);
	output.WriteBits (headerCodeLengthCount - 4, 5);

	unsigned int bits = _layout->GetBitCount ();
	for (i = 0; i < headerCodeLengthCount; ++i)
	{
		unsigned int j = _layout->GetPosition (i);
		output.WriteBits (_headerCode [j].GetLen (), bits);
	}

	WriteCodeLengths (output);
}

// Returns true if big header layout read
bool CodingTables::Header::Read (InputBitStream & input,
								 CodeBitLengths & mainCodeLen,
								 CodeBitLengths & distCodeLen)
{
	_mainCodeLengthCount = input.NextBits (5) + 257;
	_distCodeLengthCount = input.NextBits (5) + 1;
	unsigned int headerCodeLengthCount = input.NextBits (5) + 4;
	bool bigLayout = headerCodeLengthCount > MaxSmallHeaderEntryCount;
	if (bigLayout)
		_layout.reset (new BigLayout ());
	else
		_layout.reset (new SmallLayout ());

	CodeBitLengths headerCodeBitLen (_layout->GetSize ());
	unsigned int bits = _layout->GetBitCount ();
	for (unsigned int i = 0; i < headerCodeLengthCount; ++i)
	{
		unsigned int j = _layout->GetPosition (i);
		headerCodeBitLen [j] = input.NextBits (bits);
	}

	CodeTree headerCodeTree (headerCodeBitLen);
	_headerCode.resize (_layout->GetSize ());
	headerCodeTree.CalcCodes (_headerCode);

#if !defined NDEBUG
	VerifyCodes ();
#endif

	ReadCodeLengths (input, headerCodeTree, mainCodeLen, distCodeLen);
	return bigLayout;
}

void CodingTables::Header::Compress (CodeBitLengths const & mainCodeBitLen,
									 CodeBitLengths const & distCodeBitLen)
{
	// Start with main code lengths
	unsigned int i = 1;
	unsigned int previousCodeBitLen = mainCodeBitLen.at (0);
	int repetitionLen = 1;
	while (i < _mainCodeLengthCount)
	{
		if (previousCodeBitLen == mainCodeBitLen.at (i))
		{
			++repetitionLen;
		}
		else
		{
			WriteCode (previousCodeBitLen, repetitionLen);
			previousCodeBitLen = mainCodeBitLen.at (i);
			repetitionLen = 1;
		}
		++i;
	}
	// Continue with distance code lengths
	if (previousCodeBitLen != distCodeBitLen.at (0))
	{
		// Write last main code bit length
		WriteCode (previousCodeBitLen, repetitionLen);
		previousCodeBitLen = distCodeBitLen.at (0);
		repetitionLen = 1;
		i = 1;
	}
	else
	{
		// Repetition continues from main codes to distance codes
		i = 0;
	}
	while (i < _distCodeLengthCount)
	{
		if (previousCodeBitLen == distCodeBitLen.at (i))
		{
			++repetitionLen;
		}
		else
		{
			WriteCode (previousCodeBitLen, repetitionLen);
			previousCodeBitLen = distCodeBitLen.at (i);
			repetitionLen = 1;
		}
		++i;
	}
	WriteCode (previousCodeBitLen, repetitionLen);
}

void CodingTables::Header::WriteCode (unsigned int codeBitLen, int repetitionLen)
{
	if (codeBitLen == 0)
	{
		// Repetition of zero code length
		if (repetitionLen < MinRepetitionLen)
		{
			// Repetition too short -- expand in place
			while (repetitionLen > 0)
			{
				_packedCodeLengths.push_back (0);
				repetitionLen--;
			}
		}
		else if (repetitionLen <= 10)
		{
			_packedCodeLengths.push_back (_layout->GetShortZeroRepetitionMarker ());
			_packedCodeLengths.push_back (repetitionLen - 3);
		}
		else if (repetitionLen <= 138)
		{
			_packedCodeLengths.push_back (_layout->GetLongZeroRepetitionMarker ());
			_packedCodeLengths.push_back (repetitionLen - 11);
		}
		else
		{
			Assert (repetitionLen > 138);
			while (repetitionLen > 138)
			{
				_packedCodeLengths.push_back (_layout->GetLongZeroRepetitionMarker ());
				_packedCodeLengths.push_back (127);
				repetitionLen -= 138;
			}
			while (repetitionLen > 10)
			{
				_packedCodeLengths.push_back (_layout->GetShortZeroRepetitionMarker ());
				_packedCodeLengths.push_back (7);
				repetitionLen -= 10;
			}
			Assert (repetitionLen <= 10);
			if (repetitionLen < MinRepetitionLen)
			{
				// Repetition too short -- expand in place
				while (repetitionLen > 0)
				{
					_packedCodeLengths.push_back (0);
					repetitionLen--;
				}
			}
			else
			{
				_packedCodeLengths.push_back (_layout->GetShortZeroRepetitionMarker ());
				_packedCodeLengths.push_back (repetitionLen - 3);
			}
		}
	}
	else
	{
		// Repetition of non zero code lenght
		_packedCodeLengths.push_back (codeBitLen);
		repetitionLen--;
		if (repetitionLen == 0)
			return;

		if (repetitionLen < MinRepetitionLen)
		{
			// Repetition too short -- expand in place
			while (repetitionLen > 0)
			{
				_packedCodeLengths.push_back (codeBitLen);
				repetitionLen--;
			}
		}
		else
		{
			while (repetitionLen > 6)
			{
				_packedCodeLengths.push_back (_layout->GetNonZeroRepetitionMarker ());
				_packedCodeLengths.push_back (3);
				repetitionLen -= 6;
			}
			Assert (repetitionLen <= 6);
			if (repetitionLen < MinRepetitionLen)
			{
				// Repetition too short -- expand in place
				while (repetitionLen > 0)
				{
					_packedCodeLengths.push_back (codeBitLen);
					repetitionLen--;
				}
			}
			else
			{
				_packedCodeLengths.push_back (_layout->GetNonZeroRepetitionMarker ());
				_packedCodeLengths.push_back (repetitionLen - 3);
			}
		}
	}
}

void CodingTables::Header::CollectStatistics (std::vector<unsigned int>  & codeLengthStats)
{
	Assert (codeLengthStats.size () == _layout->GetSize ());
	for (unsigned int i = 0; i < _packedCodeLengths.size (); ++i)
	{
		unsigned int codeLen = _packedCodeLengths [i];
		Assert (codeLen < _layout->GetSize ());
		codeLengthStats [codeLen] ++;
		if (codeLen > _layout->GetMaxSimpleCodeLen ())
		{
			// Skip repetition count
			++i;
		}
	}
}

void CodingTables::Header::CreateHeaderCodes (CodeBitLengths const & mainCodeLen,
											  CodeBitLengths const & distCodeLen)
{
	Compress (mainCodeLen, distCodeLen);
	std::vector<unsigned int> codeLengthStats (_layout->GetSize ());
	CollectStatistics (codeLengthStats);

	HuffmanTree headerTree (codeLengthStats);
	CodeBitLengths headerCodeBitLen (headerTree, _layout->GetSize ());
	CodeTree headerCodeTree (headerCodeBitLen);
	_headerCode.resize (_layout->GetSize ());
	headerCodeTree.CalcCodes (_headerCode);
}

void CodingTables::Header::WriteCodeLengths (OutputBitStream & output) const
{
	for (unsigned int i = 0; i < _packedCodeLengths.size (); ++i)
	{
		unsigned int lengthCode = _packedCodeLengths [i];
		output.Write (_headerCode [lengthCode]);
		if (lengthCode > _layout->GetMaxSimpleCodeLen ())
		{
			// Write repetition count
			++i;
			Assert (i < _packedCodeLengths.size ());
			unsigned int repetitionCountBitLen;
			if (lengthCode == _layout->GetNonZeroRepetitionMarker ())
			{
				repetitionCountBitLen = 2;
			}
			else if (lengthCode == _layout->GetShortZeroRepetitionMarker ())
			{
				repetitionCountBitLen = 3;
			}
			else
			{
				Assert (lengthCode == _layout->GetLongZeroRepetitionMarker ());
				repetitionCountBitLen = 7;
			}
			output.WriteBits (_packedCodeLengths [i], repetitionCountBitLen);
		}
	}
}

void CodingTables::Header::ReadCodeLengths (InputBitStream & input,
											CodeTree const & decodingTree,
											CodeBitLengths & mainCodeBitLen,
											CodeBitLengths & distCodeBitLen)
{
	Assert (mainCodeBitLen.size () >= _mainCodeLengthCount);
	Assert (distCodeBitLen.size () >= _distCodeLengthCount);
	unsigned int i = 0;
	unsigned int symbol = 0;
	int repetitionLen = 0;
	unsigned int codeLengthToRepeat = 0;
	// Read main (literal/repetition lenght) code lengths
	while (i < _mainCodeLengthCount)
	{
		symbol = decodingTree.DecodeSymbol (input);
		if (symbol <= _layout->GetMaxSimpleCodeLen ())
		{
			mainCodeBitLen [i] = symbol;
			++i;
		}
		else
		{
			if (symbol == _layout->GetNonZeroRepetitionMarker ())
			{
				repetitionLen = input.NextBits (2) + 3;
				Assert (i > 0);
				codeLengthToRepeat = mainCodeBitLen.at (i - 1);
			}
			else if (symbol == _layout->GetShortZeroRepetitionMarker ())
			{
				repetitionLen = input.NextBits (3) + 3;
				codeLengthToRepeat = 0;
			}
			else
			{
				Assert (symbol == _layout->GetLongZeroRepetitionMarker ());
				repetitionLen = input.NextBits (7) + 11;
				codeLengthToRepeat = 0;
			}
			Assert (repetitionLen != 0);
			while (i < _mainCodeLengthCount && repetitionLen > 0)
			{
				mainCodeBitLen [i] = codeLengthToRepeat;
				++i;
				--repetitionLen;
			}
		}
	}
	// Continue reading distance code lengths
	i = 0;
	while (i < _distCodeLengthCount && repetitionLen > 0)
	{
		distCodeBitLen [i] = codeLengthToRepeat;
		++i;
		--repetitionLen;
	}
	while (i < _distCodeLengthCount)
	{
		symbol = decodingTree.DecodeSymbol (input);
		if (symbol <= _layout->GetMaxSimpleCodeLen ())
		{
			distCodeBitLen [i] = symbol;
			++i;
		}
		else
		{
			if (symbol == _layout->GetNonZeroRepetitionMarker ())
			{
				repetitionLen = input.NextBits (2) + 3;
				if (i == 0)
				{
					codeLengthToRepeat = mainCodeBitLen.at (_mainCodeLengthCount - 1);
				}
				else
				{
					Assert (i > 0);
					codeLengthToRepeat = distCodeBitLen.at (i - 1);
				}
			}
			else if (symbol == _layout->GetShortZeroRepetitionMarker ())
			{
				repetitionLen = input.NextBits (3) + 3;
				codeLengthToRepeat = 0;
			}
			else
			{
				Assert (symbol == _layout->GetLongZeroRepetitionMarker ());
				repetitionLen = input.NextBits (7) + 11;
				codeLengthToRepeat = 0;
			}
			Assert (repetitionLen != 0);
			while (i < _distCodeLengthCount && repetitionLen > 0)
			{
				distCodeBitLen [i] = codeLengthToRepeat;
				++i;
				--repetitionLen;
			}
		}
	}
}

#if !defined NDEBUG
void CodingTables::Header::VerifyCodes () const
{
	unsigned int maxHeaderCodeBitLen = _layout->GetBitCount () == 3 ? 7 : 15;
	// All header code-lengths codes have to have length less or equal to macHeaderCodeBitLen
	for (unsigned int i = 0; i < _headerCode.size (); ++i)
	{
		unsigned int len = _headerCode [i].GetLen ();
		if (len > maxHeaderCodeBitLen)
		{
			Msg info;
			info << "Header code-length code for code length " << i << " exceeds maximum lenght\n";
			std::bitset<8> code (_headerCode [i].GetCode ());
			info << "Code-lenght code: '" << code << "'; length: " << _headerCode [i].GetLen ();
			throw Win::Exception ("Internal error -- bad compressed block header.", info.c_str ());
		}
	}
}
#endif
