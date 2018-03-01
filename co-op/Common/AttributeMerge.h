#if !defined (ATTRIBUTEMERGE_H)
#define ATTRIBUTEMERGE_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include "FileTypes.h"
class Restorer;
class TargetData;

class AttributeMerge
{
public:
	AttributeMerge (Restorer & restorer, TargetData & targetData);

	std::string const & GetSourceName () const { return _sourceName; }
	std::string const & GetSourcePath () const { return _sourcePath; }
	char const * GetSourceTypeName () const;

	std::string const & GetTargetName () const { return _targetName; }
	std::string const & GetTargetPath () const { return _targetPath; }
	char const * GetTargetTypeName () const;

	std::string const & GetFinalName () const { return _finalName; }
	std::string const & GetFinalPath () const { return _finalPath; }
	char const * GetFinalTypeName () const;

	bool IsConflict () const { return _isConflict; }
	bool IsAttribMergeNeeded () const;

	void SetFinalPath (char const * path) { _finalPath = path; }
	void SetFinalName (char const * name) { _finalName = name; }
	void SetFinalType (FileType type)     { _finalType = type; }

	std::string GetFinalFilePath () const;
	FileType GetFinalType () const { return _finalType; }

private:
	std::string		_sourcePath;
	std::string		_sourceName;
	FileType		_sourceType;

	std::string		_targetPath;
	std::string		_targetName;
	FileType		_targetType;

	std::string		_finalPath;
	std::string		_finalName;
	FileType		_finalType;

	bool			_isConflict;
};

#endif
