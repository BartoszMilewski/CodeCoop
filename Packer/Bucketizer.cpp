//-----------------------------------------------------
//  Bucketizer.cpp
//  (c) Reliable Software 2001
//-----------------------------------------------------

#include "precompiled.h"
#include "Bucketizer.h"

RepLenBucketizer::RepLenBucketizer ()
	: Bucketizer (MaxRepLengthBucket - MinRepLengthBucket + 1)
{
	// See section 3.2.5 section of the 'Deflate' spec

	// Repetition length buckets as pairs (max repetition lenght, bucket)
	_bucketFinder.Add (3, 257);
	_bucketFinder.Add (4, 258);
	_bucketFinder.Add (5, 259);
	_bucketFinder.Add (6, 260);
	_bucketFinder.Add (7, 261);
	_bucketFinder.Add (8, 262);
	_bucketFinder.Add (9, 263);
	_bucketFinder.Add (10, 264);
	_bucketFinder.Add (12, 265);	// <11, 12>
	_bucketFinder.Add (14, 266);	// <13, 14>
	_bucketFinder.Add (16, 267);	// <15, 16>
	_bucketFinder.Add (18, 268);	// <17, 18>
	_bucketFinder.Add (22, 269);	// <19 - 22>
	_bucketFinder.Add (26, 270);	// <23 - 26>
	_bucketFinder.Add (30, 271);	// <27 - 30>
	_bucketFinder.Add (34, 272);	// <31 - 34>
	_bucketFinder.Add (42, 273);	// <35 - 42>
	_bucketFinder.Add (50, 274);	// <43 - 50>
	_bucketFinder.Add (58, 275);	// <51 - 58>
	_bucketFinder.Add (66, 276);	// <59 - 66>
	_bucketFinder.Add (82, 277);	// <67 - 82>
	_bucketFinder.Add (98, 278);	// <83 - 98>
	_bucketFinder.Add (114, 279);	// <99 - 114>
	_bucketFinder.Add (130, 280);	// <115 - 130>
	_bucketFinder.Add (162, 281);	// <131 - 162>
	_bucketFinder.Add (194, 282);	// <163 - 194>
	_bucketFinder.Add (226, 283);	// <195 - 226>
	_bucketFinder.Add (257, 284);	// <227 - 257>
	_bucketFinder.Add (258, 285);	// <258 - 258>

	_bucketExtraBits [8] = 1;
	_bucketExtraBits [9] = 1;
	_bucketExtraBits [10] = 1;
	_bucketExtraBits [11] = 1;
	_bucketExtraBits [12] = 2;
	_bucketExtraBits [13] = 2;
	_bucketExtraBits [14] = 2;
	_bucketExtraBits [15] = 2;
	_bucketExtraBits [16] = 3;
	_bucketExtraBits [17] = 3;
	_bucketExtraBits [18] = 3;
	_bucketExtraBits [19] = 3;
	_bucketExtraBits [20] = 4;
	_bucketExtraBits [21] = 4;
	_bucketExtraBits [22] = 4;
	_bucketExtraBits [23] = 4;
	_bucketExtraBits [24] = 5;
	_bucketExtraBits [25] = 5;
	_bucketExtraBits [26] = 5;
	_bucketExtraBits [27] = 5;
	_bucketExtraBits [28] = 6;

	_bucketLowerBound [0] = 3;
	_bucketLowerBound [1] = 4;
	_bucketLowerBound [2] = 5;
	_bucketLowerBound [3] = 6;
	_bucketLowerBound [4] = 7;
	_bucketLowerBound [5] = 8;
	_bucketLowerBound [6] = 9;
	_bucketLowerBound [7] = 10;
	_bucketLowerBound [8] = 11;
	_bucketLowerBound [9] = 13;
	_bucketLowerBound [10] = 15;
	_bucketLowerBound [11] = 17;
	_bucketLowerBound [12] = 19;
	_bucketLowerBound [13] = 23;
	_bucketLowerBound [14] = 27;
	_bucketLowerBound [15] = 31;
	_bucketLowerBound [16] = 35;
	_bucketLowerBound [17] = 43;
	_bucketLowerBound [18] = 51;
	_bucketLowerBound [19] = 59;
	_bucketLowerBound [20] = 67;
	_bucketLowerBound [21] = 83;
	_bucketLowerBound [22] = 99;
	_bucketLowerBound [23] = 115;
	_bucketLowerBound [24] = 131;
	_bucketLowerBound [25] = 163;
	_bucketLowerBound [26] = 195;
	_bucketLowerBound [27] = 227;
	_bucketLowerBound [28] = 258;
}

RepDistBucketizer::RepDistBucketizer ()
	: Bucketizer (MaxRepDistanceBucket + 1)
{
	// See section 3.2.5 section of the 'Deflate' spec

	// Repetition distance buckets as pairs (max repetition distance, bucket)
	_bucketFinder.Add (1, 0);
	_bucketFinder.Add (2, 1);
	_bucketFinder.Add (3, 2);
	_bucketFinder.Add (4, 3);
	_bucketFinder.Add (6, 4);		// <5, 6>
	_bucketFinder.Add (8, 5);		// <7, 8>
	_bucketFinder.Add (12, 6);		// <9 - 12>
	_bucketFinder.Add (16, 7);		// <13 - 16>
	_bucketFinder.Add (24, 8);		// <17 - 24>
	_bucketFinder.Add (32, 9);		// <25 - 32>
	_bucketFinder.Add (48, 10);		// <33 - 48>
	_bucketFinder.Add (64, 11);		// <49 - 64>
	_bucketFinder.Add (96, 12);		// <65 - 96>
	_bucketFinder.Add (128, 13);	// <97 - 128>
	_bucketFinder.Add (192, 14);	// <129 - 192>
	_bucketFinder.Add (256, 15);	// <193 - 256>
	_bucketFinder.Add (384, 16);	// <257 - 384>
	_bucketFinder.Add (512, 17);	// <385 - 512>
	_bucketFinder.Add (768, 18);	// <513 - 768>
	_bucketFinder.Add (1024, 19);	// <769 - 1024>
	_bucketFinder.Add (1536, 20);	// <1025 - 1536>
	_bucketFinder.Add (2048, 21);	// <1537 - 2048>
	_bucketFinder.Add (3072, 22);	// <2049 - 3072>
	_bucketFinder.Add (4096, 23);	// <3073 - 4096>
	_bucketFinder.Add (6144, 24);	// <4097 - 6144>
	_bucketFinder.Add (8192, 25);	// <6145 - 8192>
	_bucketFinder.Add (12288, 26);	// <8193 - 12288>
	_bucketFinder.Add (16384, 27);	// <12289 - 16384>
	_bucketFinder.Add (24576, 28);	// <16385 - 24576>
	_bucketFinder.Add (32768, 29);	// <24577 - 32768>

	_bucketExtraBits.resize (MaxRepDistanceBucket + 1, 0);
	_bucketExtraBits [4] = 1;
	_bucketExtraBits [5] = 1;
	_bucketExtraBits [6] = 2;
	_bucketExtraBits [7] = 2;
	_bucketExtraBits [8] = 3;
	_bucketExtraBits [9] = 3;
	_bucketExtraBits [10] = 4;
	_bucketExtraBits [11] = 4;
	_bucketExtraBits [12] = 5;
	_bucketExtraBits [13] = 5;
	_bucketExtraBits [14] = 6;
	_bucketExtraBits [15] = 6;
	_bucketExtraBits [16] = 7;
	_bucketExtraBits [17] = 7;
	_bucketExtraBits [18] = 8;
	_bucketExtraBits [19] = 8;
	_bucketExtraBits [20] = 9;
	_bucketExtraBits [21] = 9;
	_bucketExtraBits [22] = 10;
	_bucketExtraBits [23] = 10;
	_bucketExtraBits [24] = 11;
	_bucketExtraBits [25] = 11;
	_bucketExtraBits [26] = 12;
	_bucketExtraBits [27] = 12;
	_bucketExtraBits [28] = 13;
	_bucketExtraBits [29] = 13;

	_bucketLowerBound.resize (MaxRepDistanceBucket + 1, 0);
	_bucketLowerBound [0] = 1;
	_bucketLowerBound [1] = 2;
	_bucketLowerBound [2] = 3;
	_bucketLowerBound [3] = 4;
	_bucketLowerBound [4] = 5;
	_bucketLowerBound [5] = 7;
	_bucketLowerBound [6] = 9;
	_bucketLowerBound [7] = 13;
	_bucketLowerBound [8] = 17;
	_bucketLowerBound [9] = 25;
	_bucketLowerBound [10] = 33;
	_bucketLowerBound [11] = 49;
	_bucketLowerBound [12] = 65;
	_bucketLowerBound [13] = 97;
	_bucketLowerBound [14] = 129;
	_bucketLowerBound [15] = 193;
	_bucketLowerBound [16] = 257;
	_bucketLowerBound [17] = 385;
	_bucketLowerBound [18] = 513;
	_bucketLowerBound [19] = 769;
	_bucketLowerBound [20] = 1025;
	_bucketLowerBound [21] = 1537;
	_bucketLowerBound [22] = 2049;
	_bucketLowerBound [23] = 3073;
	_bucketLowerBound [24] = 4097;
	_bucketLowerBound [25] = 6145;
	_bucketLowerBound [26] = 8193;
	_bucketLowerBound [27] = 12289;
	_bucketLowerBound [28] = 16385;
	_bucketLowerBound [29] = 24577;
}