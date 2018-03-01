//------------------------------------
//  (c) Reliable Software, 2007 - 2008
//------------------------------------

#include "precompiled.h"
#include "BackupArchive.h"
#include "Catalog.h"
#include "ProjectMarker.h"
#include "ProjectProxy.h"
#include "ProjectData.h"
#include "FileList.h"
#include "PathRegistry.h"
#include "AppInfo.h"
#include "Outputsink.h"

#include <Ctrl/ProgressMeter.h>
#include <Sys/Process.h>
#include <File/Pipe.h>
#include <Com/ShellRequest.h>
#include <Com/Shell.h>
#include <Sys/Active.h>

char BackupArchive::cabArcLogFileName[] = "Co-opBackupLog.txt";
char BackupArchive::mismatchLogFileName[] = "Co-opMismatchedFiles.txt";

// needed by std::set<FileRecord>
bool operator<(BackupArchive::FileRecord const & x, BackupArchive::FileRecord const & y)
{
	return x.relPath < y.relPath;
}

// needed for set comparison
bool operator==(BackupArchive::FileRecord const & x, BackupArchive::FileRecord const & y)
{
	return x.relPath == y.relPath && x.size == y.size;
}

class SafeCoopCatalog
{
public:
	SafeCoopCatalog ();
	~SafeCoopCatalog ();

	void Commit () { _commit = true; }

private:
	FilePath	_originalCatalogPath;
	FilePath	_catalogCopyPath;
	bool		_commit;
};

template<class Input, class Output>
class ActiveFileCopier: public ActiveObject
{
public:
	ActiveFileCopier(Input &in, Output &out)
		: _in(in), _out(out)
	{}
private:
	static const unsigned long BUF_SIZE = 4 * 1024;

	void Run()
	{
		unsigned long sizeRead = BUF_SIZE;
		while (_in.Read(_buf, sizeRead) && sizeRead != 0)
		{
			_out.Write(_buf, sizeRead);
			sizeRead = BUF_SIZE;
		}
	}
	void FlushThread ()
	{}

	char	_buf[BUF_SIZE];
	Input	&_in;
	Output	&_out;
};

class CabArcDriver
{
private:
	std::string _cabArcPath;
	std::string _cabArcCmdLine;
	Win::ChildProcess _cabArcProcess;
public:
	CabArcDriver()
	{
		_cabArcPath = TheAppInfo.GetCabArcPath ();
		if (!File::Exists (_cabArcPath))
		{
			throw Win::InternalException (
				"Missing Code Co-op installation file. Run setup program again.",
				_cabArcPath.c_str());
		}
	}
	~CabArcDriver()
	{
		// Kill CABARC process if it is still running
		if (_cabArcProcess.IsAlive ())
			_cabArcProcess.Terminate ();
	}
	void MakeArchiveCmdLine(std::string const & archiveFilePath,
		SafeTmpFile const & inputList,
		std::vector<std::string> const & stripPatterns)
	{
		_cabArcCmdLine = '"';
		_cabArcCmdLine += _cabArcPath;
		_cabArcCmdLine += "\" -r -p ";
		for (std::vector<std::string>::const_iterator it = stripPatterns.begin(); 
			it != stripPatterns.end(); ++it)
		{
			_cabArcCmdLine += "-P \"";
			_cabArcCmdLine += *it;
			_cabArcCmdLine += "\\\\\" "; // Cabarc expects double backslash here. Go figure!
		}
		_cabArcCmdLine += "n \""; // new archive
		_cabArcCmdLine += archiveFilePath;
		_cabArcCmdLine += "\" @\"";
		_cabArcCmdLine += inputList.GetFilePath ();
		_cabArcCmdLine += '"';
		//TheOutput.Display(_cabArcCmdLine.c_str());
	}
	void MakeExtractCmdLine(std::string const & archiveFilePath, std::string const & targetPath)
	{
		_cabArcCmdLine = '"';
		_cabArcCmdLine += _cabArcPath;
		_cabArcCmdLine += "\" -o -p x \"";
		_cabArcCmdLine += archiveFilePath;
		_cabArcCmdLine += "\" \"";
		_cabArcCmdLine += targetPath;
		_cabArcCmdLine += "\\\\\"";
		// TheOutput.Display(_cabArcCmdLine.c_str());
	}

	void MakeListCmdLine(std::string const & archiveFilePath)
	{
		_cabArcCmdLine = '"';
		_cabArcCmdLine += _cabArcPath;
		_cabArcCmdLine += "\" l \"";
		_cabArcCmdLine += archiveFilePath;
		_cabArcCmdLine += "\"";
		// TheOutput.Display(_cabArcCmdLine.c_str());
	}

	bool Execute(std::string const & currentFolder, 
		SafeTmpFile const & logFilePath, 
		Progress::Meter & meter)
	{
		_cabArcProcess.SetCmdLine (_cabArcCmdLine);
		_cabArcProcess.ShowMinimizedNotActive ();
		_cabArcProcess.SetCurrentFolder (currentFolder);

		// std output redirection
		Pipe pipe(true); // inherit handles
		_cabArcProcess.RedirectOutput(pipe);

		if (!_cabArcProcess.Create (0))
			throw Win::Exception ("Cannot start the archiving program");

		// log std output is a separate thread
		FileIo logFile(logFilePath.GetFilePath(), File::CreateAlwaysMode());
		pipe.CloseWriteEnd(); // otherwise the read loop won't terminate
		auto_active<ActiveFileCopier<Pipe, FileIo> > copier (new ActiveFileCopier<Pipe, FileIo>(pipe, logFile));

		bool success = false;
		try
		{
			do
			{
				_cabArcProcess.WaitForDeath ();	// Default 5 seconds death timeout
				meter.StepAndCheck ();
			} while (_cabArcProcess.IsAlive ());
			success = true;
		}
		catch ( ... )
		{
		}
		return success;
	}
};

// _filesOnDisk will contain absRoot-relative paths and sizes
void BackupArchive::ListTree(FilePath const & absRoot, 
							 FilePath & relCurFolder)
{
	std::vector<std::string> relFolders;
	// full path to cur folder
	FilePath absCurFolder(absRoot.GetFilePath(relCurFolder.GetDir()));
	for (FileSeq seq (absCurFolder.GetAllFilesPath ()); !seq.AtEnd (); seq.Advance ())
	{
		const char * name = seq.GetName();
		if (seq.IsFolder ())
		{
			relCurFolder.DirDown (name);
			ListTree(absRoot, relCurFolder);
			relCurFolder.DirUp ();
		}
		else
		{
			_filesOnDisk.insert(FileRecord(relCurFolder.GetFilePath(name), seq.GetSize()));
		}
	}
}

bool BackupArchive::SaveLogToDesktop()
{
	return ShellMan::CopyToDesktop(_logFilePath.GetFilePath(), cabArcLogFileName);
}

// Returns true when backup archive created successfuly
void BackupArchive::Create (Catalog & catalog, Progress::Meter & meter)
{
	TransactionFileList fileList;
	for (ProjectSeq seq (catalog); !seq.AtEnd (); seq.Advance ())
	{
		meter.StepAndCheck ();
		bool success = true;
		int projectId = seq.GetProjectId ();
		CheckedOutFiles checkoutMarker (catalog, projectId);
		if (!checkoutMarker.Exists ())
			continue;	// Skip projects without checked out files

		SetCurrentActivity (projectId, catalog, meter);
		Project::Proxy projectProxy;
		projectProxy.Visit (projectId, catalog);
		projectProxy.PreserveLocalEdits (fileList);
	}

	// Create BackupMarker.bin file
	SafeTmpFile backupMarker ("BackupMarker.bin");

	{
		OutStream outFile (backupMarker.GetFilePath ());
		if (!outFile.is_open ())
			throw Win::Exception ("Cannot create temporary backup marker.", backupMarker.GetFilePath ());
	}

	FilePath catalogPath (Registry::GetCatalogPath ());
	// Create CABARC input file list
	SafeTmpFile inputList ("inputlist.txt");

	{
		OutStream outFile (inputList.GetFilePath ());
		if (!outFile.is_open ())
			throw Win::Exception ("Cannot create CABARC input file.", inputList.GetFilePath ());
		// Archive 'Database\*.*', "Inbox\*.*' and 'PublicInbox\*.*'
		outFile << '"' << catalogPath.GetFilePath ("Database\\*.*") << '"' << std::endl;
		outFile << '"' << catalogPath.GetFilePath ("Inbox\\*.*") << '"' << std::endl;
		outFile << '"' << catalogPath.GetFilePath ("PublicInbox\\*.*") << '"' << std::endl;

		// Include backup marker
		outFile << '"' << backupMarker.GetFilePath () << '"' << std::endl;
		outFile << std::endl;

		// Create a listing for later verification
		std::list<std::string> folders;
		folders.push_back("Database");
		folders.push_back("Inbox");
		folders.push_back("PublicInbox");
		for (std::list<std::string>::const_iterator it = folders.begin(); it != folders.end(); ++it)
		{
			FilePath folder(*it);
			ListTree(catalogPath, folder);
		}

		FileInfo backupMarkerInfo(backupMarker.GetFilePath ());
		_filesOnDisk.insert(FileRecord("BackupMarker.bin", backupMarkerInfo.GetSize()));
	}

	// Create path stripping pattern (path prefixes to be stripped from file names)
	std::vector<std::string> stripPatterns;
	FilePath databasePathStrippingPattern;
	FullPathSeq pathSeq (catalogPath.GetDir ());
	while (!pathSeq.AtEnd ())
	{
		databasePathStrippingPattern.DirDown (pathSeq.GetSegment ().c_str ());
		pathSeq.Advance ();
	}
	stripPatterns.push_back(databasePathStrippingPattern.GetDir());

	// Create BackupMarker.bin path stripping pattern
	FilePath backupMarkerStrippingPattern;
	FullPathSeq pathSeq1 (backupMarker.GetDirPath ());
	while (!pathSeq1.AtEnd ())
	{
		backupMarkerStrippingPattern.DirDown (pathSeq1.GetSegment ().c_str ());
		pathSeq1.Advance ();
	}
	stripPatterns.push_back(backupMarkerStrippingPattern.GetDir());

	meter.SetActivity ("Archiving project databases.");
	CabArcDriver archiver;
	archiver.MakeArchiveCmdLine(
		_archiveFilePath,
		inputList,
		stripPatterns);

	_success = archiver.Execute(catalogPath.GetDir(), _logFilePath, meter);
}

void BackupArchive::Extract (std::string const & targetPath, Progress::Meter & meter)
{
	std::string activity ("Extracting backup archive to ");
	activity += targetPath;
	meter.SetActivity (activity);
	meter.SetRange (1, 30, 1);	// Estimate - 30 * 5 seconds

	meter.StepAndCheck ();
	SafeCoopCatalog safeCoopCatalog;
	meter.StepAndCheck ();

	FilePath dataBasePath (Registry::GetDatabasePath ());

	CabArcDriver driver;
	driver.MakeExtractCmdLine(_archiveFilePath, targetPath);
	_success = driver.Execute(dataBasePath.GetDir(), _logFilePath, meter);

	safeCoopCatalog.Commit ();
}

class ArchiveVerifier
{
public:
	virtual void OnFile(std::string const & relPath, File::Size size) = 0;
	virtual bool Verify() = 0;
	void SaveLogToDesktop()
	{
		SafeTmpFile logFile(BackupArchive::mismatchLogFileName);
		{
			std::ofstream out(logFile.GetFilePath());
			out << "List of mismatched files "
				"when comparing the backup archive and the Code Co-op database\n";
			out << "Path and Size on Disk\n"; 
			std::vector<BackupArchive::FileRecord>::const_iterator it = _mismatchedFiles.begin();
			while (it != _mismatchedFiles.end())
			{
				out << it->relPath << ", size on disk: " << it->size.ToMath() << "("
					<< it->size.High() << ":" << it->size.Low() << ")" << std::endl;
				++it;
			}
		}
		ShellMan::CopyToDesktop(logFile.GetFilePath(), BackupArchive::mismatchLogFileName);
	}
protected:
	std::vector<BackupArchive::FileRecord> _mismatchedFiles;
};

class NewArchiveVerifier: public ArchiveVerifier
{
public:
	NewArchiveVerifier(BackupArchive::FileRecordSet const & filesOnDisk)
		: _filesOnDisk(filesOnDisk)
	{}
	void OnFile(std::string const & relPath, File::Size size)
	{
		_archivedFiles.insert(BackupArchive::FileRecord(relPath.c_str(), size));
	}
	bool Verify()
	{
		if (_filesOnDisk == _archivedFiles)
			return true;
		std::set_symmetric_difference(
			_filesOnDisk.begin(), _filesOnDisk.end(), // first range
			_archivedFiles.begin(), _archivedFiles.end(), // second range
			std::back_inserter(_mismatchedFiles)); // output
		return false;
	}
private:
	BackupArchive::FileRecordSet const & _filesOnDisk;
	BackupArchive::FileRecordSet _archivedFiles;
};

class RestoreArchiveVerifier: public ArchiveVerifier
{
public:
	RestoreArchiveVerifier(std::string const & rootPath)
		: _rootPath(rootPath)
	{}
	void OnFile(std::string const & relPath, File::Size size)
	{
		const char * absFilePath = _rootPath.GetFilePath(relPath);
		if (!File::Exists(absFilePath))
		{
			_mismatchedFiles.push_back(BackupArchive::FileRecord(absFilePath, File::Size()));
			return;
		}
		FileInfo info(absFilePath);
		if (info.GetSize() != size)
			_mismatchedFiles.push_back(BackupArchive::FileRecord(absFilePath, info.GetSize()));
	}
	bool Verify()
	{
		return _mismatchedFiles.empty();
	}
private:
	FilePath _rootPath;
};

bool BackupArchive::VerifyArchive(ArchiveVerifier & verifier, Progress::Meter & meter)
{
	SafeTmpFile logFile("CoopBackupListing.txt");
	CabArcDriver driver;
	try
	{
		driver.MakeListCmdLine(_archiveFilePath);
		if (!driver.Execute("", logFile, meter))
			return false;

		std::ifstream in(logFile.GetFilePath());
		if (in.fail())
			return false;

		// parse the archive listing file into file record set
		std::string lineBuf;
		// Nine header lines
		for (int i = 0; i < 9; ++i)
			std::getline(in, lineBuf);
		if (in.eof())
			return false;

		FileRecordSet archivedFiles;
		while (std::getline(in, lineBuf) && lineBuf.size() > 0)
		{
			unsigned i = lineBuf.find(' ', 3);
			std::string filePath = lineBuf.substr(3, i - 3);
			i = lineBuf.find_first_not_of(" ", i + 1);
			unsigned j = lineBuf.find(' ', i + 1);
			std::string sizeStr = lineBuf.substr(i, j - i);
			File::Size fileSize(sizeStr);
			verifier.OnFile(filePath, fileSize);
		}

		if (verifier.Verify())
			return true;
	}
	catch(...)
	{}

	verifier.SaveLogToDesktop();
	return false;
}

bool BackupArchive::VerifyNewArchive(Progress::Meter & meter)
{
	NewArchiveVerifier verifier(_filesOnDisk);
	return VerifyArchive(verifier, meter);
}

bool BackupArchive::VerifyRestoreArchive(Progress::Meter & meter)
{
	RestoreArchiveVerifier verifier(Registry::GetCatalogPath());
	return VerifyArchive(verifier, meter);
}

bool BackupArchive::VerifyLog()
{
	// The last line of the log should read: "Completed successfully"
	std::ifstream in(_logFilePath.GetFilePath());
	if (!in.fail())
	{
		unsigned curBuf = 0;
		std::string buf[2];
		while(!in.eof())
		{
			std::getline(in, buf[curBuf]);
			curBuf = (curBuf + 1) % 2;
		}
		if (buf[curBuf] != "Completed successfully" && buf[curBuf] != "Operation successful")
			_success = false;
	}
	else
		_success = false;
	return _success;
}

SafeCoopCatalog::SafeCoopCatalog ()
	: _originalCatalogPath (Registry::GetCatalogPath ()),
	  _commit (false)
{
	_catalogCopyPath.Change (_originalCatalogPath.GetDir ());
	_catalogCopyPath.DirUp ();
	_catalogCopyPath.DirDown ("co-op old");

	ShellMan::Delete (0, _catalogCopyPath.GetDir ());

	ShellMan::CopyRequest copyRequest;
	copyRequest.MakeItQuiet ();
	copyRequest.OverwriteExisting ();
	copyRequest.AddCopyRequest (_originalCatalogPath.GetDir (),
								_catalogCopyPath.GetDir ());
	copyRequest.DoCopy (0, "");
}

SafeCoopCatalog::~SafeCoopCatalog ()
{
	try
	{
		if (_commit)
		{
			ShellMan::Delete (0, _catalogCopyPath.GetDir ());
		}
		else
		{
			ShellMan::Delete (0, _originalCatalogPath.GetDir ());
			File::Move (_catalogCopyPath.GetDir (),
						_originalCatalogPath.GetDir ());
		}
	}
	catch ( ... )
	{
		// Ignore all exceptions
	}
}

void BackupArchive::SetCurrentActivity (int projectId, Catalog & catalog, Progress::Meter & meter)
{
	ProjectSeq seq (catalog);
	seq.SkipTo (projectId);
	Assert (!seq.AtEnd ());
	Project::Data projectData;
	seq.FillInProjectData (projectData);
	std::string activity (projectData.GetProjectName ());
	activity += " (";
	activity += ::ToString (projectId);
	activity += ")";
	meter.SetActivity (activity.c_str ());
}
