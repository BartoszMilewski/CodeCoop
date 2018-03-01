#if !defined (WIKIDIRECTORY_H)
#define WIKIDIRECTORY_H
//----------------------------------
// (c) Reliable Software 1997 - 2006
//----------------------------------
#include "Table.h"
#include "GlobalId.h"
#include "PathFind.h"
#include "Global.h"

#include <StringOp.h>

class Directory;
class MultiFolderWatcher;

class WikiDirectory: public Table
{
public:
	WikiDirectory (MultiFolderWatcher & watcher, 
				  SysPathFinder const & pathFinder, 
				  std::string const & globalWikiDir);
	~WikiDirectory ();
	bool AfterDirChange (std::string const & newDirectory);
	void ExitWiki ();
	std::string GetCaption (Restriction const & restrict) const;
	void Clear () throw ();
	std::string GetRootName () const;
	void PushPath (std::string const & path, std::string const & url, int scrollPos);
	std::string PopPath (int & scrollPos);
	std::string GetCurrentPath (int & scrollPos) const;
	std::string GetHomePath (int & scrollPos) const;
	std::string NextPath (int & scrollPos);
	bool HasPrevPath () const;
	bool HasNextPath () const;
	bool HasCurrentPath () const;
	void GetStartUrl (std::string & url); // in: wiki root, out: path to navigate to
	void ExportHtml (FilePath const & targetFolder);
	bool FolderChange (FilePath const & folder);
	// Table interface
	Table::Id GetId () const { return Table::WikiDirectoryId; }
	bool IsValid () const { return true; }
	void QueryUniqueNames (Restriction const & restrict, std::vector<std::string> & names) const;
	std::string	GetStringField (Column col, GlobalId gid) const { return std::string (); }
	std::string	GetStringField (Column col, UniqueName const & uname) const { return std::string (); }
    GlobalId GetIdField (Column col, UniqueName const & uname) const { return gidInvalid; }
    GlobalId GetIdField (Column col, GlobalId gid) const { return gidInvalid; }
	unsigned long GetNumericField (Column col, GlobalId gid) const 
	{ Assert (!"GetNumericField called on WikiDirectory"); return 0; }
private:
	static void DoExportHtml ( std::string const & templPath,
						FilePath & curPath, 
						FilePath & targetPath, 
						FilePath & toRootPath,
						FilePath const & rootPath,
						FilePath const & sysPath,
						bool & usesSystem);
private:
	struct HistoryItem
	{
		HistoryItem (std::string const & path, std::string const & url, int scrollPos)
			: _path (path), _url (url), _scrollPos (scrollPos)
		{}
		std::string _path;
		std::string _url;
		int			_scrollPos;
	};
private:
	MultiFolderWatcher &	_watcher;
	std::string				_rootPath;
	SysPathFinder const &	_pathFinder;
	std::string				_globalWikiDir; // where the css is
	std::vector<HistoryItem> _history;
	unsigned				_topHistory;
};
	
#endif
