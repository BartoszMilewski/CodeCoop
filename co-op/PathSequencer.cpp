//------------------------------------
//  (c) Reliable Software, 1998 - 2005
//------------------------------------

#include "precompiled.h"
#include "PathSequencer.h"

FolderContentsSequencer::FolderContentsSequencer (FolderContents const & contents)
	: _contents (contents),
	  _endExtension (contents._dictionary.end ())
{
	// DictionaryOfLists::value_type is pair<std::string, SelectableList>
	// SelectableList is pair<bool, SelStrList>
	// SelStrList is std::vector<SelString>
	_curExtension = std::find_if (_contents._dictionary.begin (), 
		_contents._dictionary.end (), 
		IsExtSelected ());
	if (_curExtension != _endExtension)
	{
		_curFile  = std::find_if (_curExtension->second.second.begin (), 
			_curExtension->second.second.end (), 
			IsSelected ());
	}
}

void FolderContentsSequencer::Advance ()
{
	++_curFile;
	// DictionaryOfLists::value_type is pair<std::string, SelectableList>
	// SelectableList is pair<bool, SelStrList>
	FolderContents::SelStrList const & fileList = _curExtension->second.second;
	_curFile = std::find_if (_curFile, fileList.end (), IsSelected ());
	if (_curFile == fileList.end ())
	{
		// End of current file list -- goto next one
		++_curExtension ;
		_curExtension = std::find_if (_curExtension, _endExtension, IsExtSelected ());
		if (AtEnd ())
			return;
		FolderContents::SelStrList const & fileList = _curExtension->second.second;
		_curFile = std::find_if (fileList.begin (), fileList.end (), IsSelected ());
	}
}
