#if !defined (BACKUPARCHIVE_H)
#define BACKUPARCHIVE_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------
#include <File/SafePaths.h>
#include <File/File.h>
class Catalog;
namespace Progress { class Meter; }

class BackupArchive
{
	friend class ArchiveVerifier;
	friend class NewArchiveVerifier;
	friend class RestoreArchiveVerifier;

	struct FileRecord
	{
	public:
		FileRecord(char const * path, File::Size sz)
			: relPath(path), size(sz)
		{}
		std::string relPath;
		File::Size  size;
	};
	friend bool operator<(BackupArchive::FileRecord const & x, BackupArchive::FileRecord const & y);
	friend bool operator==(BackupArchive::FileRecord const & x, BackupArchive::FileRecord const & y);
	typedef std::set<FileRecord> FileRecordSet;

	void ListTree(FilePath const & absRoot, FilePath & relCurFolder);
public:
	static char cabArcLogFileName[];
	static char mismatchLogFileName[];
public:
	BackupArchive (std::string const & path)
		: _archiveFilePath (path),
		  _logFilePath(cabArcLogFileName),
		  _success(false)
	{}

	void Create (Catalog & catalog, Progress::Meter & meter);
	void Extract (std::string const & targetPath, Progress::Meter & meter);
	bool VerifyLog();
	bool SaveLogToDesktop();
	bool VerifyNewArchive(Progress::Meter & meter);
	bool VerifyRestoreArchive(Progress::Meter & meter);

private:
	void BackupArchive::SetCurrentActivity (int projectId, Catalog & catalog, Progress::Meter & meter);
	bool VerifyArchive(ArchiveVerifier & verifier, Progress::Meter & meter);

private:
	std::string		_archiveFilePath;
	SafeTmpFile		_logFilePath;
	FileRecordSet	_filesOnDisk;
	bool			_success;
};

#endif
