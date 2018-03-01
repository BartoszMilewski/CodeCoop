//-----------------------------------------
//  FileStateList.cpp
//  (c) Reliable Software, 2000-2003
//-----------------------------------------
#include "precompiled.h"
#include "FileStateList.h"
#include "Serialize.h"

void FileStateList::Init (unsigned char * buf)
{
	// File state request layout in the shared memory buffer
	//
	//    +--------------+
	//    |      N       |  <-- file count
	//    +--------------+
	//    |              |
	//    .              .
	//    .              .  <-- N * sizeof (unsigned int) -- returned file state is placed here
	//    .              .
	//    |              |
	//    +--------------+  <-- lenOff
	//    |              |
	//    .              .
	//    .              .  <-- N * sizeof (unsigned int) -- vector of file path lengths
	//    .              .
	//    |              |
	//    +--------------+ <-- pathOff
	//    |              |
	//    .              .
	//    .              .
	//    .              .  <-- N null terminated file paths 
	//    .              .
	//    .              .
	//    |              |
	//    +--------------+

	MemoryDeserializer in (buf);
	unsigned int fileCount = in.GetLong ();
	// _stateList can be used for storing results
	_stateList = reinterpret_cast<unsigned int *>(in.GetWritePos ());
	// Skip over state array
	in.Advance (fileCount * sizeof (unsigned int));
	unsigned int const * pathLenList = reinterpret_cast<unsigned int const *>(in.GetReadPos ());
	// Skip over path length array
	in.Advance (fileCount * sizeof (unsigned int));
	// Create path vector
	for (unsigned int i = 0; i < fileCount; ++i)
	{
		unsigned char const * path = in.GetReadPos ();
		_pathList.push_back (reinterpret_cast<char const *>(path));
		in.Advance (pathLenList [i] + 1);
	}
}
