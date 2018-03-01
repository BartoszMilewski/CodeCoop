//-------------------------------------
// (c) Reliable Software 1999-2003
// ------------------------------------

#include "precompiled.h"
#include "Compressor.h"
#include "PackerGlobal.h"
#include "NamedBlock.h"
#include "Bucketizer.h"
#include "Statistics.h"
#include "CodingTables.h"
#include "ForgetfulHashTable.h"

void Compressor::Compress ()
{
	std::vector<InNamedBlock>::const_iterator iter;
	File::Size totalSize (0, 0);
	for (iter = _sources.begin (); iter != _sources.end (); iter++)
	    totalSize += iter->GetSize ();

	RepLenBucketizer repLenBucketizer;
	RepDistBucketizer repDistBucketizer;
	Statistics stats (repLenBucketizer, repDistBucketizer);
	Read (stats);
	if (!totalSize.IsLarge () && totalSize.Low () <= 256)
	{
		// Compress using static Huffman codes -- statistics not used
		// for building coding tables
		CodingTables codingTables (repLenBucketizer, repDistBucketizer);
		Write (codingTables);	
	}
	else
	{
		// Compress using dynamic Huffman codes
		CodingTables codingTables (stats, repLenBucketizer, repDistBucketizer);
		Write (codingTables);	
	}
}

void Compressor::Read (Statistics & stats)
{
	// To increase performance we create hash table here and pass it
	// to each block coder. In this way we can make BlockCoder constructor
	// very fast.
	ForgetfulHashTable hashTable;
    std::vector<InNamedBlock>::const_iterator iter;
	for (iter = _sources.begin (); iter != _sources.end (); ++iter)
	{
		std::unique_ptr<BlockCoder> curCoder (new BlockCoder (*iter));
		curCoder->Read (hashTable, stats);
		_blockCoders.push_back (std::move(curCoder));				
	}
}

void Compressor::Write (CodingTables const & codingTables)
{
	_out.WriteBits (Version, VersionBits);
	PackedULong fileCount (_blockCoders.size ());
	fileCount.Write (_out);

	codingTables.Write (_out, true);	// Last compressed block

	auto_vector<BlockCoder >::iterator iter;
	for (iter = _blockCoders.begin (); iter != _blockCoders.end (); iter++)
		(*iter)->Write (_out, codingTables);

	_out.End ();
}


