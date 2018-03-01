//-----------------------------------------------------
//  Statistics.cpp
//  (c) Reliable Software 2001
//-----------------------------------------------------

#include "precompiled.h"
#include "Statistics.h"
#include "Bucketizer.h"

Statistics::Statistics (RepLenBucketizer const & repLenBucketizer, RepDistBucketizer const & repDistBucketizer)
	: _repLenBucketizer (repLenBucketizer),
	  _repDistBucketizer (repDistBucketizer),
	  _mainStats (LiteralRepLenAlphabetSize),
	  _distanceStats(RepDistanceBucketCount)
{}

void Statistics::RecordRepetition (unsigned int repLen, unsigned int repDist)
{
	Assert (repDist <= RepDistBucketizer::GetMaxRepetitionDistance ());
	unsigned int lenBucket = _repLenBucketizer.GetBucket (repLen);
	Assert (MaxLiteralValue < lenBucket && lenBucket < LiteralRepLenAlphabetSize);
	++_mainStats [lenBucket];

	unsigned int distBucket = _repDistBucketizer.GetBucket (repDist);
	Assert (distBucket < RepDistanceBucketCount);
	++_distanceStats [distBucket];
}
