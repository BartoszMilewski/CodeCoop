//----------------------------------
// (c) Reliable Software 1998 - 2007
//----------------------------------

#include "precompiled.h"
#include "BlockFinder.h"

#include <Ex/WinEx.h>

BlockFinder::BlockFinder (char const * const oldFile, File::Size sizeOldFile,
						  char const * const newFile, File::Size sizeNewFile)
	: _oldFile (oldFile),
	  _sizeOldFile (sizeOldFile),
	  _oldBufSize (sizeOldFile.Low ()),
	  _newFile (newFile),
	  _sizeNewFile (sizeNewFile),
	  _newBufSize (sizeNewFile.Low ()),
	  _hashTable (_oldBufSize / 100 + 1000),
	  _freeSpace (_oldBufSize, _newBufSize)
{
	if (sizeOldFile.IsLarge ())
		throw Win::InternalException ("File size exceeds 4GB", oldFile);
	if (sizeNewFile.IsLarge ())
		throw Win::InternalException ("File size exceeds 4GB", newFile);
}

void BlockFinder::Find (std::vector<MatchingBlock> & blocks)
{
	FindEasyClusters ();
	std::list<char>  markers;
	ChooseMarkers (markers);
	std::list<char>::iterator it;
	for (it = markers.begin (); it != markers.end (); ++it)
	{	
		_currentMarkerChar = *it;
		RefineClusters ();
		_hashTable.Reset ();
	}
	blocks.swap (_recorder);
}

void BlockFinder::RefineClusters ()
{
	HashOldFile ();
	ScanNewFile ();
	FindMatchingBlocks ();	
}

// Find all markers and hash the 4 characters following them
void BlockFinder::HashOldFile ()
{
	GapsFinder delGaps (_sizeOldFile, _sizeNewFile, _recorder);
	std::vector<MatchingBlock> deletedBlocks;
	delGaps.GetDeleteBlocks (deletedBlocks);
	std::vector<MatchingBlock>::iterator it;
	//hash only parts old file are being Deleted at the moment
	for (it = deletedBlocks.begin (); it != deletedBlocks.end (); ++it)
	{
		int start = it->SourceOffset ();
		int end   = start + it->Len ();
		for (int k = start; k < end; ++k)
			if (_oldFile [k]  == _currentMarkerChar)
			{				
				unsigned long hash = OldHashAround (k + 1);
				_hashTable.AddLink (k, hash);
				int duplicationLen = DuplicationLen (k);
				// skip markers
				if (duplicationLen != 0)
					while (k < end && OldHashAround (k + duplicationLen + 1) == hash) 
						k += duplicationLen;			
			}
	}
}

// Find markers in the new file. 
// For each marker find the longest segment in the old file maching the segment
// in the new file around the marker
void BlockFinder::ScanNewFile ()
{
	GapsFinder addGaps (_sizeOldFile, _sizeNewFile, _recorder);
	std::vector<MatchingBlock> addedBlocks;
	addGaps.GetAddBlocks (addedBlocks);
	std::vector<MatchingBlock>::iterator it;
	int start = 0;
	// finding MatchingBlock only in parts of new file wich are being Added at the moment
	for (it = addedBlocks.begin (); it != addedBlocks.end (); ++it)
	{
	    int k = std::max (it->DestinationOffset (), start);
		int end = it->DestinationOffset () + it->Len ();	
		for ( ; k < end; ++k)
		{			
			if (_newFile [k] == _currentMarkerChar)
			{
				int source;
				int destination;
				int repetition = minBlockSize - 1;
				// find the one with longest match between old and new
				for (ListIt seq = _hashTable.GetList (NewHashAround (k + 1));
					!seq.AtEnd ();
					seq.Advance ())
				{
					int currentSource = seq.GetValue();
					// extend repetition left and right
					int leftMatchLen = LeftMatch (currentSource, k);
					int matchLen = leftMatchLen + RightMatch (currentSource, k) - 1;
					// remember only if longer
					if (matchLen > repetition)
					{
						 source = currentSource - leftMatchLen + 1;
						 repetition = matchLen;
						 destination = k - leftMatchLen + 1;
					 }
					 if (repetition > 2000)
						 break;
				}
				if (repetition >= minBlockSize)
				{
					_recorder.push_back (MatchingBlock (source, destination, repetition));
				    k = destination + repetition - 1;
				}
			}
		}			
		start = std::max (it->DestinationOffset () + it->Len (), k);
	}
}
                          
// Verify each block, starting with the longest.
// After accepting a given span, parts of other blocks
// are invalidated by incrementing the area of the span

void BlockFinder::FindMatchingBlocks ()
{	
	// sort blocks by length
    std::sort (_recorder.begin (), _recorder.end (), MatchingBlock::GreaterLen ());
	std::list<MatchingBlock> tmpList (_recorder.begin (), _recorder.end ());
	_recorder.clear ();
    
	std::list<MatchingBlock>::iterator it;
   	for (it = tmpList.begin (); it != tmpList.end (); )
	{
		std::list<MatchingBlock>::iterator itCur = it;
		++it;
		_freeSpace.CutBlock (*itCur);

		if (itCur->Len () >= minBlockSize)
		{			
			if (it != tmpList.end () && it->Len () > itCur->Len ())
			{ // if next block was longer than current , put current back on the sorted list
				std::list<MatchingBlock>::iterator itFind 
					= std::find_if (it, tmpList.end (), MatchingBlock::LessThan (*itCur));
				tmpList.insert (itFind, *itCur);
			}
			else
			{
				_recorder.push_back (*itCur);
				_freeSpace.SetSpaceInUse (*itCur);
			}			
		}						
	}
	_freeSpace.Reset ();
}

int BlockFinder::LeftMatch (int indexOld, int indexNew) const
{
	int matchLen = 0;
	while (indexOld >= 0 && indexNew >= 0)
	{
		if (_oldFile [indexOld] != _newFile [indexNew])
			break;
		--indexOld;
		--indexNew;
		++matchLen;
	}
	return matchLen;
}

int BlockFinder::RightMatch (int indexOld, int indexNew) const
{
	int matchLen = 0;
	while (indexOld < _oldBufSize && indexNew < _newBufSize)
	{
		if (_oldFile [indexOld] != _newFile [indexNew])
			break;
		++indexOld;
		++indexNew;
		++matchLen;
	}
	return matchLen;
}

unsigned long BlockFinder::OldHashAround (int indexOld) const 
{
	if (indexOld >= _oldBufSize - int(sizeof(unsigned long)))
		return 0;
	return * reinterpret_cast <const  unsigned long *> (_oldFile + indexOld);
}

unsigned long BlockFinder::NewHashAround (int indexNew) const 
{
	if (indexNew >= _newBufSize - int(sizeof(unsigned long)))
		return 0;
	return * reinterpret_cast <const  unsigned long *> (_newFile + indexNew);
}

class StatRecord
{
public:
	StatRecord ()
		:_freq (0)
	{}
	int Freq () const { return _freq;}
	void EraseFreq () { _freq = 0;}
	char Char () const { return _c;}
	void IncrementFreq (){ ++_freq;}
	void SetChar (char c) { _c = c;}
	bool operator < (const StatRecord & rec) const
	{
		return rec.Freq () < _freq;
	}
private:
	char _c;
	int  _freq;
};
	
void BlockFinder::ChooseMarkers (std::list<char> & markers)
{
	if (_sizeNewFile.IsZero ())
		return;
    //table frequence chars in file
    int frequenceTable  [256];
	//table hit chars in sequence that have len 7 
	std::vector<StatRecord> occurrenceTable (256);
	int k = 0;
	for ( k = 0; k < 256; ++k)
	{
		frequenceTable[k] = 0;
		occurrenceTable[k].SetChar (static_cast <char>(k));
	}
	
	
	for (k = 0; k < _newBufSize; k += 71)
	{
		std::set<char> setChar;
		for (int i = 0; (i + k < _newBufSize) && (i < 7);  ++i)
		{
			char c = _newFile [k + i];
			setChar.insert (c);				
			frequenceTable [static_cast <unsigned char> (c)]++;			
		}
		for(std::set<char>::iterator it = setChar.begin (); it != setChar.end (); ++it)
			occurrenceTable [static_cast <unsigned char> (*it)].IncrementFreq ();
		
	}
	//copy only for exepctional event
	std::vector<StatRecord> copyOccurrenceTable = occurrenceTable;

    //erase occurrence if  frequence > 3%
	for (int i = 0; i < 256; ++i)
		if ((frequenceTable[i] * 1000) /_newBufSize > 3)
			occurrenceTable[i].EraseFreq ();

	// sort by occurrence
	std::sort (occurrenceTable.begin (), occurrenceTable.end ());

	if (occurrenceTable [0].Freq () != 0)
	{   //first, second, third, fifth, eighth, twelfth, eighteenth (if occurrence != 0)
		for (int k = 0; k < 20 ; k = k + 1 + k/2)
			if(occurrenceTable [k].Freq () != 0)
				markers.push_front (occurrenceTable [k].Char ());
	}
	else
	{//only for exepctionnal event
		std::sort (copyOccurrenceTable.begin (), copyOccurrenceTable.end ());
		for (int k = 0; k < 3; ++k)
			if (copyOccurrenceTable [k].Freq () != 0)
				markers.push_front (copyOccurrenceTable [k].Char ());
		
	}
}

int BlockFinder::DuplicationLen (int index) const
{
	if (index >= _oldBufSize - 8)
		return 0;
	if (_oldFile [index + 4] == _currentMarkerChar)
	{
		if (memcmp (_oldFile + index, _oldFile + index + 4, 4) == 0)
			return 4;
	}
	if (_oldFile [index + 3] == _currentMarkerChar)
	{
		if (memcmp (_oldFile + index, _oldFile + index + 3, 3) == 0)
			return 3;
	}
	if (_oldFile [index + 2] == _currentMarkerChar)
	{
		if (memcmp (_oldFile + index, _oldFile + index + 2, 2) == 0)
			return 2;
	}
	if (_oldFile [index + 1] == _currentMarkerChar)
		return 1;
	       
	return 0;
}
// natural cluster : Cluster (i ,i ,len)
void BlockFinder::FindEasyClusters ()
{
	int lenLastCluster = LeftMatch (_oldBufSize - 1, _newBufSize - 1);
	for (int k = 0; k < _oldBufSize - lenLastCluster; ++k)
	{		
		if ((k < _newBufSize) && (_oldFile [k] == _newFile [k]))
		{			
			 int repetition =  RightMatch (k, k);
			 if (repetition >= minBlockSize)
			 {
				 MatchingBlock block(k, k, repetition) ;
				 _recorder.push_back (block);				
			 }
			 k += repetition;
		}
	}
	if (!_recorder.empty ())
	{
		MatchingBlock & block = _recorder.back ();
		int freeSpaceInNew = _newBufSize - (block.DestinationOffset () + block.Len ());
		lenLastCluster = std::min (lenLastCluster, freeSpaceInNew);
	}
	if (lenLastCluster>= minBlockSize)
		_recorder.push_back (MatchingBlock (_oldBufSize - lenLastCluster, _newBufSize - lenLastCluster, lenLastCluster));
}

void GapsFinder::GetDeleteBlocks (std::vector<MatchingBlock> & gaps)
{
	// sort matching blocks by position in the old file
	// gaps between these are deleted blocks
	int lastEndSource = 0;
	std::sort (_blocks.begin (), _blocks.end (), MatchingBlock::LessSource ());
	std::vector <MatchingBlock>::iterator it;
	for (it = _blocks.begin (); it != _blocks.end (); ++it)
	{
		int source = it->SourceOffset ();
		if (source != lastEndSource)
			gaps.push_back (MatchingBlock (lastEndSource, -1, source - lastEndSource));
		lastEndSource = source + it->Len ();
	}
	if (lastEndSource != _oldBufSize)
		gaps.push_back (MatchingBlock (lastEndSource, -1, _oldBufSize - lastEndSource));	
}

void GapsFinder::GetAddBlocks (std::vector<MatchingBlock> & gaps)
{	
	// sort matching blocks by position in the new file
	// gaps between these are added blocks
	int lastEndDestination = 0;
	std::sort (_blocks.begin (), _blocks.end (), MatchingBlock::LessDestination ());
	std::vector <MatchingBlock>::iterator it;		
	for (it = _blocks.begin (); it != _blocks.end (); ++it)
	{
		int destination = it->DestinationOffset ();
		if (destination != lastEndDestination)
			gaps.push_back (MatchingBlock (-1, lastEndDestination, destination - lastEndDestination));
		lastEndDestination =  destination + it->Len ();
	}
	if (lastEndDestination != _newBufSize)
		gaps.push_back (MatchingBlock (-1, lastEndDestination, _newBufSize - lastEndDestination));
}

void FreeSpaceRecorder::CutBlock (MatchingBlock & block)
{
	int source = block.SourceOffset ();
	int destination = block.DestinationOffset ();
	int len = block.Len ();
	int endSource = source + len;
	while (source < endSource && (! _isOldFree [source] || ! _isNewFree [destination]))
	{
		++source;
		++destination;
	}

	len = 0;
	while (source + len < endSource && _isOldFree [source + len] 
		   && _isNewFree [destination + len])
	{
		++len;
	}
	block.SetNewParametrs (source, destination, len);
}

void FreeSpaceRecorder::SetSpaceInUse (const MatchingBlock & block)
{
	std::fill_n (_isOldFree.begin () + block.SourceOffset (), block.Len (), false);
	std::fill_n (_isNewFree.begin () + block.DestinationOffset (), block.Len (), false);
}

void FreeSpaceRecorder::Reset ()
{
	std::fill (_isOldFree.begin (), _isOldFree.end (), true);
	std::fill (_isNewFree.begin (), _isNewFree.end (), true);
}

	
