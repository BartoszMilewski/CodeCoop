#if !defined (RESTORER_H)
#define RESTORER_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "GlobalId.h"
#include "HistoryPath.h"
#include "TmpProjectArea.h"
#include "FileTypes.h"
#include "UniqueName.h"

class PathFinder;
class FileCmd;
namespace History
{
	class Db;
	class Range;
}
namespace Workspace
{
	class HistorySelection;
	class HistoryItem;
}
namespace XML { class Tree; }

// One file restore or merge information
class Restorer
{
public:
	Restorer (History::Db const & history,
			  PathFinder & pathFinder,
			  History::Range const & scriptRange,
			  GlobalId forkId,
			  GlobalId fileGid);
	~Restorer ();

	void SetReconstructed (bool flag) { _isReconstructed = flag; }
	void CreateDifferArgs (XML::Tree & args) const;
	bool IsReconstructed () const { return _isReconstructed; }
	bool IsCurrentVersionSelected () const { return _currentVersionSelected; }
	static bool IsBackwardDiff (History::Db const & history, History::Range const & range); 
	bool IsFolder () const;
	bool IsTextual () const;
	bool IsBinary () const;
	FileType GetFileTypeAfter () const;
	FileType GetFileTypeBefore () const;
	bool IsCreatedByBranch () const { return _createdByBranch; }
	bool EditsItem () const;
	bool DeletesItem () const;
	bool IsAbsent () const;
	std::string const & GetBinaryFilePath ();
	std::string GetRootRelativePath () const;
	std::string GetReferenceRootRelativePath ();
	std::string const & GetFileName () const;
	std::string GetReferencePath ();
	std::string GetRestoredPath ();
	FileData const & GetEarlierFileData () const;
	FileData const & GetLaterFileData () const;
	GlobalId GetFileGid () const { return _fileGid; }
	History::Path const & GetBasePath () const { return _basePath; }
	History::Path const & GetDiffPath () const { return _diffPath; }
	TmpProjectArea & GetBaseArea () { return _baseArea; }
	TmpProjectArea & GetDiffArea () { return _diffArea; }
	TmpProjectArea const & GetAfterArea () const;
	TmpProjectArea const & GetBeforeArea () const;
	TmpProjectArea & GetBeforeArea ();
	UniqueName const & GetAfterUniqueName () const;
	UniqueName const & GetBeforeUniqueName () const;
	FileCmd const & GetFileCmd () const;

private:
	Restorer (Restorer const &);
	Restorer & operator= (Restorer const &);

private:
	bool CreatesItem () const;
	Workspace::HistoryItem const & GetEarlierItem () const;
	Workspace::HistoryItem const & GetLaterItem () const;
	void CreateTmpFilePath ();

private:
	GlobalId			_fileGid;
	PathFinder &		_pathFinder;
	std::string			_binaryFileTmpPath;
	History::Path		_basePath;
	History::Path		_diffPath;
	TmpProjectArea		_baseArea;	// Holds result of base path execution
	TmpProjectArea		_diffArea;	// Holds result of diff path execution
	bool				_createdByBranch;
	bool				_isReconstructed;
	bool				_diffBackwards;
	bool				_currentVersionSelected;
};


#endif
