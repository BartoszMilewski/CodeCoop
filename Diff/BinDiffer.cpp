//------------------------------------
//  (c) Reliable Software, 2000 - 2006
//------------------------------------

#include "precompiled.h"
#include "BinDiffer.h"
#include "BlockFinder.h"
#include "Cluster.h"
#include "DiffRecord.h"
#include "CluLines.h"

BinDiffer::BinDiffer (char const * oldFile, File::Size sizeOldFile,
					  char const * newFile, File::Size sizeNewFile)
	:_oldFile (oldFile), _sizeOldFile (sizeOldFile),
	 _newFile (newFile), _sizeNewFile (sizeNewFile)
{
	FindBlocks ();	
}

void BinDiffer::FindBlocks ()
{
	BlockFinder blockFinder (_oldFile, _sizeOldFile, _newFile, _sizeNewFile);
	blockFinder.Find (_blocks);
}

void BinDiffer::Record (DiffRecorder & recorder)
{
    // Moved blocks
	std::vector <MatchingBlock>::iterator it;
	for  (it = _blocks.begin (); it != _blocks.end (); ++it)
	{
		int oldL = it->SourceOffset ();
		int newL = it->DestinationOffset ();
		int len = it->Len ();
		Cluster cluster (oldL, newL, len);
		recorder.AddCluster (cluster);
	}
    
	// Added blocks from new file stored in one buf
	std::vector<char> bufAdd;
	GapsFinder gapsFinder (_sizeOldFile, _sizeNewFile, _blocks);
	std::vector<MatchingBlock> blocksAdd;
	
	gapsFinder.GetAddBlocks (blocksAdd);
	for  (it = blocksAdd.begin (); it != blocksAdd.end (); ++it)
	{
		int newL = it->DestinationOffset ();
		int len = it->Len ();
		bufAdd.insert (bufAdd.end (), _newFile + newL, _newFile + newL + len);
	}

	if (bufAdd.size () != 0)
	{
		recorder.AddCluster (Cluster (-1, _sizeNewFile.Low (), bufAdd.size ()));
		SimpleLine blockAdd (& bufAdd [0] , bufAdd.size ());
		recorder.AddNewLine (_sizeNewFile.Low (), blockAdd);
	}
    
	// Deleted blocks from old file stored in one buf
	std::vector<char> bufDelete;
	std::vector<MatchingBlock> blocksDelete;
	gapsFinder.GetDeleteBlocks (blocksDelete);
	for  (it = blocksDelete.begin (); it != blocksDelete.end (); ++it)
	{
		int oldL = it->SourceOffset ();
		int len = it->Len ();
		bufDelete.insert (bufDelete.end (), _oldFile + oldL, _oldFile + oldL + len);
	}

	if (bufDelete.size () != 0)
	{
		recorder.AddCluster (Cluster (_sizeOldFile.Low (), -1, bufDelete.size ()));	
		SimpleLine blockDelete (& bufDelete [0] , bufDelete.size ());
		recorder.AddOldLine (_sizeOldFile.Low (), blockDelete);
	}
}

