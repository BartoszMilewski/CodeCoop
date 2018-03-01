#if !defined (PATHSEQUENCER_H)
#define PATHSEQUENCER_H
//------------------------------------
//  (c) Reliable Software, 1998 - 2005
//------------------------------------

#include "FolderContents.h"

#include <File/Path.h>

class FolderContentsSequencer : public PathSequencer
{
public:
	FolderContentsSequencer (FolderContents const & contents);

	bool AtEnd () const { return _curExtension == _endExtension; }
    void Advance ();

	char const * GetFilePath () const { return _curFile->first.c_str(); }

private:
	FolderContents const &								_contents;
	FolderContents::DictionaryOfLists::const_iterator	_curExtension;
	FolderContents::DictionaryOfLists::const_iterator	_endExtension;
	FolderContents::SelStrList::const_iterator			_curFile;
};

#endif
