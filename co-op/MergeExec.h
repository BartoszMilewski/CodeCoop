#if !defined (MERGEEXEC_H)
#define MERGEEXEC_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include "FileTypes.h"
#include <GlobalId.h>

class HistoricalFiles;
class FileIndex;
class Directory;
class AttributeMerge;
class Restorer;
class MergerProxy;
class TargetData;

class MergeExec
{
public:
	MergeExec (HistoricalFiles & historicalFiles, 
		GlobalId fileGid, 
		bool autoMerge, 
		bool localMerge);
	// Returns true if we can execute merge operation
	virtual bool VerifyMerge (FileIndex const & fileIndex, Directory const & directory) = 0;
	// Returns true when trivial merge executed
	virtual bool DoMerge (MergerProxy & merger) = 0;

protected:
	//Returns true when content identical or auto merge without conflicts
	bool MergeContent (MergerProxy & merger);
	//Returns true when target path is recorded in the project database
	bool IsTargetPathRecorded (FileIndex const & fileIndex, Directory const & directory) const;

protected:
	HistoricalFiles &	_historicalFiles;
	GlobalId			_fileGid;
	Restorer &			_restorer;
	TargetData &		_targetData;
	bool				_autoMerge;
	bool				_localMerge;
};

class MergeCreateExec : public MergeExec
{
public:
	MergeCreateExec (HistoricalFiles & historicalFiles, 
		GlobalId fileGid, 
		bool autoMerge, 
		bool localMerge)
		: MergeExec (historicalFiles, fileGid, autoMerge, localMerge)
	{}

	bool VerifyMerge (FileIndex const & fileIndex, Directory const & directory);
	bool DoMerge (MergerProxy & merger);
};

class MergeReCreateExec : public MergeExec
{
public:
	MergeReCreateExec (HistoricalFiles & historicalFiles, 
		GlobalId fileGid, 
		bool autoMerge, 
		bool localMerge)
		: MergeExec (historicalFiles, fileGid, autoMerge, localMerge)
	{}

	bool VerifyMerge (FileIndex const & fileIndex, Directory const & directory);
	bool DoMerge (MergerProxy & merger);
};

class MergeDeleteExec : public MergeExec
{
public:
	MergeDeleteExec (HistoricalFiles & historicalFiles, 
		GlobalId fileGid, 
		bool autoMerge, 
		bool localMerge)
		: MergeExec (historicalFiles, fileGid, autoMerge, localMerge)
	{}

	bool VerifyMerge (FileIndex const & fileIndex, Directory const & directory);
	bool DoMerge (MergerProxy & merger);
};

class MergeContentsExec : public MergeExec
{
public:
	MergeContentsExec (HistoricalFiles & historicalFiles, 
		GlobalId fileGid, 
		bool autoMerge, 
		bool localMerge)
		: MergeExec (historicalFiles, fileGid, autoMerge, localMerge),
		  _attributeMergeNeeded (false)
	{}

	bool VerifyMerge (FileIndex const & fileIndex, Directory const & directory);
	bool DoMerge (MergerProxy & merger);

private:
	void SetAttributeMerge (AttributeMerge const & attrib);

private:
	bool		_attributeMergeNeeded;
	std::string	_newTargetPath;
	FileType	_newTargetType;
};

#endif
