#if !defined (COOPMEMENTO_H)
#define COOPMEMENTO_H
//
// (c) Reliable Software 1997 -- 2005
//

#include "Memento.h"

#include <Com/ShellRequest.h>

class Catalog;

namespace Project
{
	class Data;
}

class CoopMemento: public Memento
{
public:
	CoopMemento (int oldProjId = -1)
		: _oldProjectId (oldProjId)
	{}

	int GetOldProjectId () const { return _oldProjectId; }

private:
	int	_oldProjectId;
};

class NewProjTransaction
{
public:
	NewProjTransaction (Catalog & catalog, Project::Data const & projData);
	~NewProjTransaction ();
	void Commit () { _commit = true; }
	void AddAbortRequest (ShellMan::CopyRequest const & request)
	{
		_abortRequest = request;
	}
	void AddFolderPath (std::string const & path)
	{
		_createdFolders.push_back (path);
	}
private:
	bool						_commit;
	Catalog &					_catalog;
	Project::Data const &		_projData;
	bool						_rootDirExisted; 
	ShellMan::CopyRequest		_abortRequest;
	std::vector<std::string>	_createdFolders;
};

#endif
