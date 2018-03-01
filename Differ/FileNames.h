#if !defined (FILENAMES_H)
#define FILENAMES_H
//----------------------------------
// (c) Reliable Software 1998 - 2008
//----------------------------------

#include "CommandParams.h"
#include "FileViews.h"
#include <File/File.h>
#include <StringOp.h>

using CmdLine::CommandParams;

//
// In general we have to compare two paths
// Reference -> Original -> Pre-Synch -> Project
// Reference -> Synch -> Project
//
// In a no-synch situation there is only Project and possibly Original.
// If Project is R/W we should open up an edit window

class FileNames
{
public:
	FileNames (XML::Node const * params);

	std::string const & Project () const { return GetPath (FileCurrent); }
	std::string const & GetPath (FileSelection sel) const { return _files [sel]; }
	bool HasPath (FileSelection sel) const { return !_files [sel].empty (); }

	void Clear ();
	void TestBeforeFile ();
	void SetProjectFile (std::string path);
	void VerifyPaths ();
	void SetFileForComparing (std::string path);

	bool IsSingleFile () const { return _count == 1; }
	bool AsViewer () const { return !_isDiff; }
	bool AsDiffer () const { return _isDiff; }
	bool HasFiles () const { return _count > 0; }
	bool IsControlledFile () const { return _sccStatus == Controlled; }
	bool IsCheckedoutFile () const { return _sccStatus == Checkedout; }
	bool IsUnknownFile () const { return _sccStatus == Unknown; }
	void MakeProjectFileControlled () { _sccStatus = Controlled; }
	void MakeProjectFileCheckedout () { _sccStatus = Checkedout; }
	bool IsReadOnlyFile () const { return _isReadOnly; }
	bool ProjectIsPresent () const { return HasPath (FileCurrent) && File::Exists (GetPath (FileCurrent)); }
	bool GetProjectFileTimeStamp ();
	bool GetProjectFileReadOnlyState ();
	std::string GetProjectFileAttribute () const;
	void GetSccStatus (bool force = false);
	void Dump ();
	void GetProjectFileInfo (std::string & time, std::string & size) const;

private:
	enum SccStatus
	{
		Unknown,
		Controlled,
		Checkedout,
		NotControlled
	};

	static void NormalizePath (std::string & path);

private:
	typedef std::map<FileSelection, std::string> FilePaths;

	mutable FilePaths _files;
	int			_count; // only existing files

	// Project file data
	bool		_isDiff;
	bool		_isReadOnly;
	SccStatus	_sccStatus;
	FileTime	_projFileTimeStamp;
};	

#endif
