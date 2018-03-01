#if !defined (PROJECTMARKER_H)
#define PROJECTMARKER_H
//------------------------------------
//  (c) Reliable Software, 2003 - 2007
//------------------------------------

#include <string>

class Catalog;

class FileMarker
{
public:
	void SetMarker (bool create);
	bool Exists () const;

protected:
	std::string	_markerPath;
};

class ProjectMarker : public FileMarker
{
protected:
	ProjectMarker (Catalog const & catalog, int projectId, std::string const & markerName);
};

class GlobalMarker : public FileMarker
{
protected:
	GlobalMarker (std::string const & markerName);
};

class CheckedOutFiles : public ProjectMarker
{
public:
	CheckedOutFiles (Catalog const & catalog, int projectId)
		: ProjectMarker (catalog, projectId, "CheckedOut.bin")
	{}
};

class IncomingScripts : public ProjectMarker
{
public:
	IncomingScripts (Catalog const & catalog, int projectId)
		: ProjectMarker (catalog, projectId, "IncomingScripts.bin")
	{}
};

class MissingScripts : public ProjectMarker
{
public:
	MissingScripts (Catalog const & catalog, int projectId)
		: ProjectMarker (catalog, projectId, "MissingScripts.bin")
	{}
};

class AwaitingFullSync : public ProjectMarker
{
public:
	AwaitingFullSync (Catalog const & catalog, int projectId)
		: ProjectMarker (catalog, projectId, "AwaitingFullSync.bin")
	{}
};

class RecoveryMarker : public ProjectMarker
{
public:
	RecoveryMarker (Catalog const & catalog, int projectId)
		: ProjectMarker (catalog, projectId, "UnderVerification.bin")
	{}
};

class BlockedCheckinMarker : public ProjectMarker
{
public:
	BlockedCheckinMarker (Catalog const & catalog, int projectId)
		: ProjectMarker (catalog, projectId, "BlockedCheckin.bin")
	{}
};

class BackupMarker : public GlobalMarker
{
public:
	BackupMarker ()
		: GlobalMarker ("BackupMarker.bin")
	{}
};

class RepairList
{
public:
	RepairList ();
	RepairList (Catalog & catalog);

	unsigned ProjectCount () const { return _projectIds.size (); }
	bool Exists () const;
	std::vector<int> GetProjectIds () const;

	void Save ();
	void Remove (int projectId);
	void Delete ();

private:
	void InitializeFilePath ();
	void SaveTo (char const * path);

private:
	static char const *	_fileName;
	static char const *	_delimiter;
	std::string			_filePath;
	std::vector<int>	_projectIds;
};

#endif
