#if !defined (FOLDERCONTENTS_H)
#define FOLDERCONTENTS_H
//------------------------------------
//  (c) Reliable Software, 1998 - 2007
//------------------------------------

#include "Visitor.h"
#include "GlobalId.h"

#include <StringOp.h>
#include <File/Path.h>

namespace Progress { class Meter; }

class FolderContents : public Visitor
{
	friend class SetListSelection;
	friend class SortFileList;
	friend class ExtractRelativePath;
	friend class FolderContentsSequencer;

public:

	// Selectable String - first string, second flag - true selected
	typedef std::pair<std::string, bool> SelString;	
	
	class FileSeq
	{
	public:
		FileSeq (std::vector<SelString> const & strList)
			: _cur (strList.begin ()),
			  _end (strList.end ())
		{}
		bool AtEnd () const { return _cur == _end; }
		void Advance () { ++_cur; }
		bool IsSelected () const { return _cur->second; }
		std::string const & GetPath () const { return _cur->first; }
	private:
		std::vector<SelString>::const_iterator _cur;
		std::vector<SelString>::const_iterator _end;
	};

public:

	FolderContents (std::string const & folder,
					NocaseSet const & filter,
					Progress::Meter & progressMeter,
					bool excludeSpecial = false);
	bool IsEmpty () const { return _isEmpty; }

	bool Visit (char const * path);
	void CancelVisit () { _dictionary.clear (); }
	void Sort (Progress::Meter & progressMeter);
	int  GetSelectedCount () const;

	void SelectAll ();
	void DeSelectAll ();
	bool IsExtensionSelected (std::string extension);
	bool IsFileSelected (std::string extension, int fileIdx);
	void MarkExtensionSelection (std::string extension, bool flag);
	void SetExtensionSelection (std::string extension, bool flag);
	void SetFileSelection (std::string extension, int fileIdx, bool flag);

	void GetExtensionList (std::vector<std::string> & list) const;

	FileSeq GetFileList (std::string const & extension)
	{
		SelStrList const & strList = _dictionary [extension].second;
		return FileSeq (strList);
	}

	int GetFileCount (std::string const & extension)
	{
		SelStrList const & strList = _dictionary [extension].second;
		return strList.size ();
	}

	int GetSelectedFileCount (std::string const & extension);
	void SelectFiles (std::string const & extension, std::vector<int> & selection);

private:
	static bool IsSpecialExtension (std::string const& extension);

	// Selectable String List
	typedef std::vector<SelString> SelStrList;
	// Selectable List Of Selectable Strings - first flag - true the whole list selected
	typedef std::pair<bool, SelStrList> SelectableList;	
	
	class SSFileCmp
	{
	public:
		bool operator () (SelString const & str1, SelString const & str2) const
		{
			return (IsFileNameLess (str1.first.c_str (), str2.first.c_str ()));
		}
	};

	// Map - std::string key to Selectable List Of Selectable Strings
	// used for mapping extensions into selectable list of file names
public:
	typedef std::map<std::string, SelectableList, FileNameLess> DictionaryOfLists;

private:
	FilePath			_root;
	GlobalId			_rootId;
	NocaseSet const &	_filter;	// Don't include files from filter
	bool				_excludeSpecial; // files to be excluded: 
										 // 1. hidden files 
										 // 2. files with special extensions
	bool				_isEmpty;
	// Project contents dictionary: for every file extension
	// found in the project lists -- file paths with this extension
	DictionaryOfLists	_dictionary;
	static char const * const _specialExtensions [];
};

class IsExtSelected 
	: public std::unary_function<FolderContents::DictionaryOfLists::value_type const &, bool>
{
public:
	bool operator () (FolderContents::DictionaryOfLists::value_type const & dictEntry) const
	{
		// DictionaryOfLists::value_type is pair<std::string, SelectableList>
		// SelectableList is pair<bool, SelStrList>
		return dictEntry.second.first;
	}
};

class IsSelected 
	: public std::unary_function<FolderContents::SelString const &, bool>
{
public:
	bool operator () (FolderContents::SelString const & file) const
	{
		return file.second;
	}
};

#endif
