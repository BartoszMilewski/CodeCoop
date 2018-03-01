#if !defined (FILEDROPINFOCOLLECTOR_H)
#define FILEDROPINFOCOLLECTOR_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "GlobalId.h"
#include "FileCtrlState.h"

#include <Com/ShellRequest.h>
#include <auto_vector.h>

class PathSequencer;
class FileIndex;
class Directory;
class DropPreferences;

class FileDropInfoCollector
{
public:
	FileDropInfoCollector (Win::Dow::Handle parentWin,
						   std::string const & projectName,
						   bool canMakeChanges,
						   bool isPaste,
						   Directory & currentDir,
						   FileIndex const & fileIndex)
	: _parentWin (parentWin),
	  _projectName (projectName),
	  _canMakeChanges (canMakeChanges),
	  _isPaste (isPaste),
	  _currentDir (currentDir),
	  _fileIndex (fileIndex)
	{
		_copyRequest.OverwriteExisting ();
	}

	bool GatherFileInfo (PathSequencer & pathSeq, bool allControlledFileOverride, bool allControlledFolderOverride);

	bool HasFilesToCheckout () const { return _filesToCheckout.size () != 0; }
	bool HasFilesToAdd () const { return _filesToAdd.size () != 0; }

	ShellMan::CopyRequest & GetCopyRequest () { return _copyRequest; }
	GidList & GetFilesToCheckout () { return _filesToCheckout; }
	std::vector<std::string> const & GetFilesToAdd () const { return _filesToAdd; }
	char const * GetCurrentPath () const;

	void RememberFileToCheckout (GlobalId gid) { _filesToCheckout.push_back (gid); }
	void RememberFileToAdd (std::string const & path) { _filesToAdd.push_back (path); }
	void RememberCopyRequest (std::string const & from, std::string const & to)
	{
		_copyRequest.AddCopyRequest (from.c_str (), to.c_str ());
	}
	void RememberCreatedFolder (std::string const & path) { _createdFolders.push_back (path); }
	void FolderDown (std::string const & folderName);
	void FolderUp ();
	void Cleanup ();

private:
	void InspectFiles (PathSequencer & pathSeq,
					   auto_vector<DropInfo> & files,
					   DropPreferences & userPrefs);
	std::unique_ptr<DropInfo> CreateDropInfo (std::string const & sourcePath);

private:
	Win::Dow::Handle			_parentWin;
	std::string					_projectName;
	bool						_canMakeChanges;
	bool						_isPaste;
	Directory &					_currentDir;
	FileIndex const &			_fileIndex;
	GidList						_filesToCheckout;
	ShellMan::CopyRequest		_copyRequest;
	std::vector<std::string>	_filesToAdd;
	std::vector<std::string>	_createdFolders;
};

#endif
