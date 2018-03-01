//------------------------------------
//  (c) Reliable Software, 1998 - 2007
//------------------------------------

#include "precompiled.h"
#include "FolderContents.h"
#include "DirTraversal.h"

#include <Ctrl/ProgressMeter.h>
#include <File/File.h>
#include <File/Dir.h>

//	keep it upper case in alphabetical order
char const * const FolderContents::_specialExtensions [] =
{
	".APS",
    ".BAK",
	".BSC",
	".CDB",
	".CLW",
	".DPD",
	".ERR",
	".EXP",
	".IDB",
	".ILK",
	".LNK",
	".NCB",
	".OBJ",
	".OPT",
	".PCH",
	".PDB",
	".PLG",
	".RES",
	".SBR",
	".SUO",
	".TLOG",
	".USER",
	0
};


bool FolderContents::IsSpecialExtension (std::string const &ext)
{
	std::string extension (ext);
	TheUpCaseTable.UpCaseString (extension);
	for (unsigned iExt = 0; _specialExtensions [iExt] != 0; ++iExt)
	{
		if (extension < _specialExtensions [iExt])
			return false;
		if (extension == _specialExtensions [iExt])
			return true;
		//	keep going...
	}

	return false;
}

FolderContents::FolderContents (std::string const & folder,
								NocaseSet const & filter,
								Progress::Meter & progressMeter,
								bool excludeSpecial)
	: _root (folder),
	  _filter (filter),
	  _excludeSpecial (excludeSpecial)
{
	int fileCount = 0;
	int folderCount = 0;
	// Count files and folders at root level
	for (::FileSeq seq (_root.GetAllFilesPath ()); !seq.AtEnd (); seq.Advance ())
	{
		if (File::IsFolder (_root.GetFilePath (seq.GetName ())))
			folderCount++;
		else
			fileCount++;
	}
	if (folderCount + fileCount > 0)
	{
		progressMeter.SetRange (0, folderCount, 1);
		Traversal collecting (_root, *this, progressMeter);
	}
	_isEmpty = _dictionary.empty ();
}

bool FolderContents::Visit (char const * path)
{
	if (File::IsFolder (path))
		return true; // folders are not taken into account

	if (_filter.find (path) != _filter.end ())
		return false; // exclude filtered out files

	if (_excludeSpecial)
	{
		File::Attributes attrib (path);
		if (attrib.IsHidden ())
			return false; // exclude hidden files
	}

	PathSplitter splitter (path);
	std::string extension (splitter.GetExtension ());
	if (!_excludeSpecial || !IsSpecialExtension (extension))
	{
		// add not excluded files
		SelectableList & fileList = _dictionary [extension];
		SelString projectPath (path, false);
		fileList.second.push_back (projectPath);
	}
	return false;
}

class SortFileList : public std::unary_function<FolderContents::DictionaryOfLists::value_type &, void>
{
public:
	SortFileList (Progress::Meter & progressMeter)
		: _meter (progressMeter)
	{}

	void operator () (FolderContents::DictionaryOfLists::value_type & dictEntry)
	{
		// DictionaryOfLists::value_type is pair<std::string, SelectableList>
		// SelectableList is pair<bool, SelStrList>
		FolderContents::SelStrList & fileList = dictEntry.second.second;
		std::sort (fileList.begin (), fileList.end (), FolderContents::SSFileCmp ());
		_meter.StepIt ();
	}

private:
	Progress::Meter & _meter;
};

void FolderContents::Sort (Progress::Meter & progressMeter)
{
	SortFileList sortFileList (progressMeter);
	progressMeter.SetRange (0, _dictionary.size (), 1);
	progressMeter.SetActivity ("Sorting files.");
	std::for_each (_dictionary.begin (), _dictionary.end (), sortFileList);
}

class SetFSelection : public std::unary_function<FolderContents::SelString &, void>
{
public:
	SetFSelection (bool flag)
		: _flag (flag)
	{}
	void operator () (FolderContents::SelString & file)
	{
		file.second = _flag;
	}

private:
	bool	_flag;
};

class SetListSelection : public std::unary_function<FolderContents::DictionaryOfLists::value_type &, void>
{
public:
	SetListSelection (bool flag)
		: _flag (flag)
	{}
	void operator () (FolderContents::DictionaryOfLists::value_type & dictEntry)
	{
		// DictionaryOfLists::value_type is pair<std::string, SelectableList>
		// SelectableList is pair<bool, SelStrList>
		// Set selection for the whole list
		dictEntry.second.first = _flag;
		// Now set selection for the each list element
		FolderContents::SelStrList & fileList = dictEntry.second.second;
		std::for_each (fileList.begin (), fileList.end (), SetFSelection (_flag));
	}

private:
	bool	_flag;
};

void FolderContents::SelectAll ()
{
	std::for_each (_dictionary.begin (), _dictionary.end (), SetListSelection (true));
}

void FolderContents::DeSelectAll ()
{
	std::for_each (_dictionary.begin (), _dictionary.end (), SetListSelection (false));
}

void FolderContents::SetExtensionSelection (std::string extension, bool flag)
{
	SelectableList & selList = _dictionary [extension];
	selList.first = flag;
	// Now set selection for the each list element
	FolderContents::SelStrList & fileList = selList.second;
	std::for_each (fileList.begin (), fileList.end (), SetFSelection (flag));
}

void FolderContents::SetFileSelection (std::string extension, int fileIdx, bool flag)
{
	SelectableList & selList = _dictionary [extension];
	selList.second [fileIdx].second = flag;
}

void FolderContents::MarkExtensionSelection (std::string extension, bool flag)
{
	SelectableList & selList = _dictionary [extension];
	selList.first = flag;
}

bool FolderContents::IsExtensionSelected (std::string extension)
{
	SelectableList const & selList = _dictionary [extension];
	return selList.first;
}

bool FolderContents::IsFileSelected (std::string extension, int fileIdx)
{
	SelectableList const & selList = _dictionary [extension];
	return selList.second [fileIdx].second;
}

int FolderContents::GetSelectedCount () const
{
	int count = 0;
	DictionaryOfLists::const_iterator iter = std::find_if (_dictionary.begin (), 
											 _dictionary.end (), 
											 IsExtSelected ());
	while (iter != _dictionary.end ())
	{
		// DictionaryOfLists::value_type is pair<std::string, SelectableList>
		// SelectableList is pair<bool, SelStrList>
		SelStrList const & fileList = iter->second.second;
		count += std::count_if (fileList.begin (), fileList.end (), IsSelected ());
		++iter;
		iter = std::find_if (iter, _dictionary.end (), IsExtSelected ());
	}
	return count;
}

int FolderContents::GetSelectedFileCount (std::string const & extension)
{
	SelStrList const & fileList = _dictionary [extension].second;
	return std::count_if (fileList.begin (), fileList.end (), IsSelected ());
}

void FolderContents::GetExtensionList (std::vector<std::string> & list) const
{
	for (DictionaryOfLists::const_iterator iter = _dictionary.begin (); iter != _dictionary.end (); ++iter)
	{
		// DictionaryOfLists::value_type is pair<std::string, SelectableList>
		list.push_back (iter->first);
	}
}

class ExtractRelativePath : public std::unary_function<FolderContents::SelString const &, void>
{
public:
	ExtractRelativePath (std::vector<std::string> & list, int rootLen)
		: _list (list),
		  _rootLen (rootLen)
	{}
	void operator () (FolderContents::SelString const & file) const
	{
		FilePath path (file.first.c_str ());
		_list.push_back (path.GetRelativePath (_rootLen));
	}

private:
	std::vector<std::string> &	_list;
	int							_rootLen;
};

void FolderContents::SelectFiles (std::string const & extension, std::vector<int> & selection)
{
	SelectableList & selList = _dictionary [extension];
	SelStrList & strList = selList.second;

	// Clear current selection
	selList.first = false;
	typedef std::vector<SelString>::iterator iter;
	for (iter it = strList.begin (); it != strList.end (); ++it)
		it->second = false;

	if (!selection.empty ())
	{
		// Set new selection
		selList.first = true;
		typedef std::vector<int>::iterator intIter;
		for (intIter iit = selection.begin (); iit != selection.end (); ++iit)
			strList [*iit].second = true;
	}
}
