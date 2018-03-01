//----------------------------------
// (c) Reliable Software 1997 - 2006
//----------------------------------
#include "precompiled.h"
#include "WikiDirectory.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "Directory.h"
#include "GlobalFileNames.h"
#include "ExternHtmlSink.h"
#include "WikiConverter.h"
#include "Sql.h"
#include "LinkParser.h"

#include <File/Dir.h>
#include <File/FolderWatcher.h>

WikiDirectory::WikiDirectory (MultiFolderWatcher & watcher,
							  SysPathFinder const & pathFinder, 
							  std::string const & globalWikiDir)
	: _watcher (watcher),
	  _pathFinder (pathFinder), 
	  _globalWikiDir (globalWikiDir), 
	  _topHistory (0)
{
}

WikiDirectory::~WikiDirectory ()
{
}

std::string WikiDirectory::GetRootName () const
{
	return _rootPath;
}

// Return true if wiki site changed
bool WikiDirectory::AfterDirChange (std::string const & newDirectory)
{
	if (_rootPath == newDirectory)
		return false;

	if (!_rootPath.empty ())
		_watcher.StopWatching (_rootPath);
	// new folder already added to watcher by Directory
	_rootPath.assign (newDirectory);
	Clear ();
	return true;
}

void WikiDirectory::ExitWiki ()
{
	if (!_rootPath.empty ())
		_watcher.StopWatching (_rootPath);
	Clear ();
	_rootPath.clear ();
}

void WikiDirectory::GetStartUrl (std::string & url)
{
	if (_topHistory == 0)
	{
		url += "\\index.wiki";
	}
	else
		url = _history [_topHistory - 1]._path;
}

bool WikiDirectory::FolderChange (FilePath const & folder)
{
	if (folder.IsEqualDir (GetRootName ()))
	{
		ExternalNotify ();
		return true;
	}
	return false;
}

// Record set contains the directory for temporary HTML files
// and the global wiki directory
void WikiDirectory::QueryUniqueNames (Restriction const & restrict, std::vector<std::string> & names) const
{
	names.push_back (_pathFinder.GetSysPath ());
	names.push_back (_globalWikiDir);
}

std::string WikiDirectory::GetCaption (Restriction const & restrict) const
{
	std::string list;
	for (unsigned idx = _topHistory; idx != 0; --idx)
	{
		list += _history [idx - 1]._path;
		if (idx != 1)
			list += "\r\n";
	}
	return list;
}

void WikiDirectory::Clear () throw ()
{
	_topHistory = 0;
	_history.clear ();
}

// ---top---------v
// |home|prev|curr|next|
void WikiDirectory::PushPath (std::string const & path, std::string const & url, int scrollPos)
{
	// compare after removing the query string
	bool isNewPath = (_topHistory == 0);
	if (!isNewPath)
	{
		std::string const & oldPath = _history [_topHistory - 1]._path;
		if (oldPath.size () > path.size () && oldPath [path.size ()] == '?')
		{
			FilePath oldFilePath (oldPath);
			isNewPath = !oldFilePath.HasPrefix (path);
		}
		else
			isNewPath = !FilePath::IsEqualDir (path, oldPath);
	}

	// don't push the same path twice
	if (isNewPath)
	{
		// erase history overflow
		while (_topHistory < _history.size ())
			_history.pop_back ();
		// remember scroll position of previous page
		if (_topHistory != 0)
			_history [_topHistory - 1]._scrollPos = scrollPos;
		_history.push_back (HistoryItem (path, url, 0));
		++_topHistory;
	}
	ExternalNotify ("url");
}

std::string WikiDirectory::PopPath (int & scrollPos)
{
	Assert (HasPrevPath ());
	--_topHistory;
	Assert (_topHistory > 0);
	ExternalNotify ("url");
	scrollPos = _history [_topHistory - 1]._scrollPos;
	std::string const & url = _history [_topHistory - 1]._url;
	return url;
}

std::string WikiDirectory::GetCurrentPath (int & scrollPos) const
{
	if (_topHistory > 0)
	{
		scrollPos = _history [_topHistory - 1]._scrollPos;
		return _history [_topHistory - 1]._path;
	}
	else
	{
		scrollPos = 0;
		return std::string ();
	}
}

std::string WikiDirectory::GetHomePath (int & scrollPos) const
{
	Assert (_topHistory > 0);
	scrollPos = _history [0]._scrollPos;
	return _history [0]._path;
}

std::string WikiDirectory::NextPath (int & scrollPos)
{
	Assert (HasNextPath ());
	ExternalNotify ("url");
	scrollPos = _history [_topHistory]._scrollPos;
	return _history [_topHistory++]._url;
}

bool WikiDirectory::HasPrevPath () const
{
	return _topHistory > 1;
}

bool WikiDirectory::HasNextPath () const
{
	return _topHistory < _history.size ();
}

bool WikiDirectory::HasCurrentPath () const
{
	return _topHistory > 0;
}

void WikiDirectory::ExportHtml (FilePath const & targetFolder)
{
	FilePath rootPath (GetRootName ());
	FilePath curPath (GetRootName ());
	FilePath sysPath (_globalWikiDir);
	FilePath targetPath (targetFolder);
	FilePath toRootPath;

	PathFinder::MaterializeFolderPath (targetFolder.GetDir ());

	WikiPaths wikiPaths (curPath, sysPath);

	// Copy CSS file (either local or system)
	File::Copy (wikiPaths.GetCssPath ().c_str (), targetFolder.GetFilePath (CssFileName));

	bool usesSystem = false;
	DoExportHtml (wikiPaths.GetTemplPath (), curPath, targetPath, toRootPath, rootPath, sysPath, usesSystem);
	if (usesSystem)
	{
		// Convert system files as well
		rootPath.Change (_globalWikiDir);
		curPath.Change (_globalWikiDir);
		targetPath.Change (targetFolder);
		targetPath.DirDown ("system");
		PathFinder::MaterializeFolderPath (targetPath.GetDir ());
		toRootPath.Clear ();
		toRootPath.DirDown ("..", true); // use slash
		DoExportHtml (wikiPaths.GetTemplPath (), curPath, targetPath, toRootPath, rootPath, sysPath, usesSystem);
	}
}

// Recursive function
void WikiDirectory::DoExportHtml ( std::string const & templPath,
						FilePath & curPath, 
						FilePath & targetPath, 
						FilePath & toRootPath,
						FilePath const & rootPath,
						FilePath const & sysPath,
						bool & usesSystem)
{
	if (targetPath.HasPrefix (rootPath.GetDir ()))
		throw Win::InternalException ("Attempt to create files inside the currently exported wiki site");

	for (FileSeq seq (curPath.GetAllFilesPath ()); !seq.AtEnd (); seq.Advance ())
	{
		char const * name = seq.GetName ();
		if (seq.IsFolder ())
		{
			curPath.DirDown (name);
			targetPath.DirDown (name);
			toRootPath.DirDown ("..", true); // use slash
			PathFinder::MaterializeFolderPath (targetPath.GetDir ());

			DoExportHtml (templPath, curPath, targetPath, toRootPath, rootPath, sysPath, usesSystem);

			toRootPath.DirUp ();
			targetPath.DirUp ();
			curPath.DirUp ();
		}
		else
		{
			std::string fullSrcPath (curPath.GetFilePath (name));
			PathSplitter splitter (name);
			if (IsNocaseEqual (splitter.GetExtension (), ".wiki"))
			{
				std::string tgtFile = splitter.GetFileName ();
				tgtFile += ".html";

				Sql::Listing::RecordFile recordFile (fullSrcPath);
				Sql::ExistingRecord record (fullSrcPath, recordFile);

				InStream wikiFile (fullSrcPath);
				OutStream htmlFile (targetPath.GetFilePath (tgtFile));
				ExternHtmlSink sink (htmlFile, toRootPath, rootPath, sysPath, record.GetTuples ());
				WikiConverter converter (wikiFile, sink);
				// Parse wiki and create html file
				converter.ParseAndSave (templPath, toRootPath.GetFilePath (CssFileName, true));

				if (sink.UsesSystemLinks ())
					usesSystem = true;
			}
			else
			{
				char const * target = targetPath.GetFilePath (name);
				File::Copy (fullSrcPath.c_str (), target);
				File::MakeReadWrite (target);
			}
		}
	}
}
