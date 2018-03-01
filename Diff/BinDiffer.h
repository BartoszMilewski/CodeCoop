#if !defined BINDIFFER_H
#define BINDIFFER_H
//------------------------------------
//  (c) Reliable Software, 2000 - 2006
//------------------------------------

#include "BlockFinder.h"

#include <File/File.h>

class DiffRecorder;

class BinDiffer
{
public :
	BinDiffer (char const * oldFile, File::Size sizeOldFile,
			   char const * newFile, File::Size sizeNewFile);

	void Record (DiffRecorder & recorder);

private :
	void FindBlocks ();

private:
	char const * const          _oldFile;
	char const * const          _newFile;
	const File::Size            _sizeOldFile;
	const File::Size            _sizeNewFile;
	std::vector<MatchingBlock>  _blocks;
};	
#endif