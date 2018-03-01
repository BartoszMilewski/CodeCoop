#if !defined (STATISTICS_H)
#define STATISTICS_H
//-----------------------------------------------------
//  Statistics.h
//  (c) Reliable Software 2001
//-----------------------------------------------------

#include <Dbg/Assert.h>

class RepLenBucketizer;
class RepDistBucketizer;

class Statistics
{
public:
	Statistics (RepLenBucketizer const & repLenBucketizer, RepDistBucketizer const & repDistBucketizer);

	static unsigned int GetLiteralAlphabetSize () { return LiteralRepLenAlphabetSize; }
	static unsigned int GetDistanceAlphabetSize () { return RepDistanceBucketCount; }

	void RecordLiteral (unsigned char value)
	{
		Assert (value <= MaxLiteralValue);
		++_mainStats [value];
	}
	void RecordLenCode (unsigned int value)
	{
		Assert (MaxLiteralValue < value && value < LiteralRepLenAlphabetSize);
		++_mainStats [value];
	}
	void RecordRepetition (unsigned int repLen, unsigned int repDist);

	std::vector<unsigned int> const & GetMainStats () const { return _mainStats; }
	std::vector<unsigned int> const & GetDistanceStats () const { return _distanceStats; }

private:
	enum
	{
		LiteralRepLenAlphabetSize = 288,	// There are 287 'letters' from the range <0; 287>
											// <0; 255> -- literals; 256 -- EOF marker -- original spec;
											// <257; 285> -- repetition length codes;
		RepDistanceBucketCount = 32,		// There are 32 buckets used to specify repetition distance.
		MaxLiteralValue = 255
	};

private:
	RepLenBucketizer const &	_repLenBucketizer;
	RepDistBucketizer const &	_repDistBucketizer;
	std::vector<unsigned int>	_mainStats;			// Literal/Repetition length statistics
	std::vector<unsigned int>	_distanceStats;		// Repetition distance statistics
};

#endif
