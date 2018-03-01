#if !defined (FILEFILTER_H)
#define FILEFILTER_H
//----------------------------------
// (c) Reliable Software 1998 - 2008
//----------------------------------

#include "GlobalId.h"

#include <auto_vector.h>

class SelectionSeq;
class Directory;
class FileTag;

class FileFilter
{
private:
	typedef std::map<GlobalId, std::string>::const_iterator FilterIter;

public:
	FileFilter ()
	{}
	FileFilter (SelectionSeq & iter, Directory & folder);
	FileFilter (FileFilter const & filter);

	void AddFile (GlobalId gid, std::string const & projectPath = std::string ());
	void AddFiles (auto_vector<FileTag> const & files);
	void AddScripts (GidList const & scripts)
	{
		_scripts.insert (scripts.begin (), scripts.end ());
	}
	void SetFilterPattern (std::string const & pattern) { _filterPattern = pattern; }

	std::string const & GetFilterPattern () const { return _filterPattern; }

	bool IsFileFilterOn () const { return !_filter.empty (); }
	unsigned int FileCount () const { return _filter.size (); }
	bool IsFilteringOn () const;
	bool IsIncluded (GlobalId gid) const;
	bool IsScriptIncluded (GlobalId gid) const;
	bool IsEqual (FileFilter const & ff) const;
	bool IsScriptFilterOn () const { return !_scripts.empty (); }
	bool IsFilterPattern () const { return !_filterPattern.empty (); }

	class Sequencer
	{
	public:
		Sequencer (FileFilter const & filter)
			: _cur (filter.begin ()),
			  _end (filter.end ())
		{}
		void Advance () { ++_cur; }
		bool AtEnd () const { return _cur == _end; }

		GlobalId GetGid () const { return _cur->first; }
		std::string GetPath () const { return _cur->second; }

	private:
		FilterIter	_cur;
		FilterIter	_end;
	};

	class Inserter: public std::iterator<std::output_iterator_tag, void, void, void, void>
	{
	public:
		explicit Inserter (FileFilter & filter)
			: _filter (filter._filter)
		{}
		Inserter& operator= (GlobalId gid)
		{
			_filter.insert (std::make_pair (gid, std::string ()));
			return *this;
		}
		Inserter & operator++ () { return *this; }
		Inserter operator++ (int) { return *this; }
		Inserter & operator * () { return *this; }
	private:
		std::map<GlobalId, std::string> & _filter;
	};

	friend class FileFilter::Sequencer;
	friend class FileFilter::Inserter;

private:
	FilterIter begin () const { return _filter.begin (); }
	FilterIter end () const { return _filter.end (); }

private:
	std::map<GlobalId, std::string>	_filter;
	std::string						_filterPattern;
	GidSet							_scripts;
};

#endif
