//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include "precompiled.h"
#include "AttributeMerge.h"
#include "HistoricalFiles.h"
#include "FileData.h"

AttributeMerge::AttributeMerge (Restorer & restorer, TargetData & targetData)
	: _isConflict (false)
{
	{
		PathSplitter splitter (restorer.GetRootRelativePath ());
		_sourcePath = splitter.GetDir ();
		_sourceName = splitter.GetFileName ();
		_sourceName += splitter.GetExtension ();
		_sourceType = restorer.GetFileTypeAfter ();
	}
	{
		PathSplitter splitter (targetData.GetTargetPath ());
		_targetPath = splitter.GetDir ();
		_targetName = splitter.GetFileName ();
		_targetName += splitter.GetExtension ();
		_targetType = targetData.GetTargetType ();
	}

	PathSplitter splitter (restorer.GetReferenceRootRelativePath ());
	std::string refPath = splitter.GetDir ();
	std::string refName = splitter.GetFileName ();
	refName += splitter.GetExtension ();
	FileType refType = restorer.GetFileTypeBefore ();

	bool isTargetPathModified = !FilePath::IsEqualDir (refPath, _targetPath);
	bool isSourcePathModified = !FilePath::IsEqualDir (refPath, _sourcePath);

	bool isTargetNameModified = (refName != _targetName);
	bool isSourceNameModified = (refName != _sourceName);

	bool isTargetTypeModified = !refType.IsEqual (_targetType);
	bool isSourceTypeModified = !refType.IsEqual (_sourceType);

	if (isSourcePathModified)
	{
		if (isTargetPathModified)
		{
			_isConflict = !FilePath::IsEqualDir (_sourcePath, _targetPath);
			_finalPath = _targetPath;
		}
		else
			_finalPath = _sourcePath;
	}
	else
		_finalPath = _targetPath;

	if (isSourceNameModified)
	{
		if (isTargetNameModified)
		{
			_isConflict = (_sourceName != _targetName);
			_finalName = _targetName;
		}
		else
			_finalName = _sourceName;
	}
	else
		_finalName = _targetName;

	if (isSourceTypeModified)
	{
		if (isTargetTypeModified)
		{
			_isConflict = !_sourceType.IsEqual (_targetType);
			_finalType = _targetType;
		}
		else
			_finalType = _sourceType;
	}
	else
		_finalType = _targetType;
}

char const * AttributeMerge::GetSourceTypeName () const
{
	return _sourceType.GetName ();
}

char const * AttributeMerge::GetTargetTypeName () const
{
	return _targetType.GetName ();
}

char const * AttributeMerge::GetFinalTypeName () const
{
	return _finalType.GetName ();
}

std::string AttributeMerge::GetFinalFilePath () const
{
	FilePath path (_finalPath);
	return std::string (path.GetFilePath (_finalName));
}

bool AttributeMerge::IsAttribMergeNeeded () const
{
	return !_finalType.IsEqual (_targetType) 
		|| _finalName != _targetName
		|| !FilePath::IsEqualDir (_finalPath, _targetPath);
}