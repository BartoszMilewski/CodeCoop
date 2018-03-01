#if !defined (BUCKETIZER_H)
#define BUCKETIZER_H
//-----------------------------------------------------
//  Bucketizer.h
//  (c) Reliable Software 2001
//-----------------------------------------------------

#include "BitStream.h"

#include <Dbg/Assert.h>

class Bucketizer
{
public:
	Bucketizer (unsigned int maxBucket)
		: _bucketExtraBits (maxBucket),
		  _bucketLowerBound (maxBucket)
	{}

	unsigned int GetBucket (unsigned int value) const
	{
		return _bucketFinder.Find (value);
	}
	unsigned int GetExtraBits (unsigned int bucketNo, unsigned int value, unsigned int & extraBitsValue) const
	{
		Assert (bucketNo < _bucketExtraBits.size ());
		extraBitsValue = value - _bucketLowerBound [bucketNo];
		return _bucketExtraBits [bucketNo];
	}
	unsigned int DecodeExtraBits (unsigned int bucketNo, InputBitStream & input) const
	{
		Assert (bucketNo < _bucketExtraBits.size ());
		unsigned int extraBitsValue = input.NextBits (_bucketExtraBits [bucketNo]);
		extraBitsValue += _bucketLowerBound [bucketNo];
		return extraBitsValue;
	}

protected:
	class BucketFinder
	{
		typedef std::pair<unsigned int, unsigned int> Pair;
		class IsAboveLimit 
			: public std::binary_function<Pair const &, 
										  Pair const &, bool>
		{
		public:
			bool operator () (Pair const & bucket, 
							  Pair const & value) const
			{
				return value.first > bucket.first;
			}
		};

	public:
		void Add (unsigned int limit, unsigned int bucket)
		{
			_buckets.push_back (std::make_pair (limit, bucket)); 
		}
		unsigned int Find (unsigned int value) const
		{
			std::vector<std::pair<unsigned int, unsigned int> >::const_iterator iter;
			iter = std::lower_bound (_buckets.begin (), 
									 _buckets.end (), 
									 std::make_pair (value, 0), 
									 IsAboveLimit ());
			Assert (iter != _buckets.end ());
			return iter->second;
		}
	private:
		std::vector<Pair>	_buckets;
	};
protected:
	BucketFinder				_bucketFinder;
	std::vector<unsigned int>	_bucketExtraBits;
	std::vector<unsigned int>	_bucketLowerBound;
};

class RepLenBucketizer : public Bucketizer
{
public:
	RepLenBucketizer ();

	static unsigned int GetMaxRepetitionLength () { return 258; }

	bool IsRepetitionLengthBucket (unsigned int bucketNo) const
	{
		return MinRepLengthBucket <= bucketNo && bucketNo <= MaxRepLengthBucket;
	}
	unsigned int GetExtraBits (unsigned int bucketNo, unsigned int value, unsigned int & extraBitsValue) const
	{
		Assert (IsRepetitionLengthBucket (bucketNo));
		return Bucketizer::GetExtraBits (bucketNo - MinRepLengthBucket, value, extraBitsValue);
	}
	unsigned int DecodeExtraBits (unsigned int bucketNo, InputBitStream & input) const
	{
		Assert (IsRepetitionLengthBucket (bucketNo));
		return Bucketizer::DecodeExtraBits (bucketNo - MinRepLengthBucket, input);
	}

private:
	enum
	{
		MinRepLengthBucket = 257,
		MaxRepLengthBucket = 285,
	};
};

class RepDistBucketizer : public Bucketizer
{
public:
	RepDistBucketizer ();

	static unsigned int GetMaxRepetitionDistance () { return 32768; }

private:
	enum
	{
		MaxRepDistanceBucket = 29
	};
};

#endif
