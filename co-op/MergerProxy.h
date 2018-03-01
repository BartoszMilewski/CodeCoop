#if !defined (MERGERPROXY_H)
#define MERGERPROXY_H
//---------------------------
// (c) Reliable Software 2006
//---------------------------

#include "FileTypes.h"
#include "GlobalId.h"

class Restorer;
class Model;
class ActiveMergerWatcher;
namespace XML { class Tree; }

// Abstract interface to perform merge operations either locally or remotely
class MergerProxy
{
public:
	virtual ~MergerProxy () {}

	void MaterializeFolderPath (std::string const & relativeTargetPath);
	void CopyFile (std::string const & sourceFullPath,
				   std::string const & relativeTargetPath,
				   bool quiet);
	virtual void AddFile (char const * fullTargetPath,
						  GlobalId gid,
						  FileType type) = 0;
	virtual void Delete (char const * fullTargetPath) = 0;
	virtual void ReCreateFile (char const * fullTargetPath, GlobalId gid, FileType type) = 0;
	virtual void MergeAttributes (std::string const & currentPath, std::string const & newPath, FileType newType) = 0;
	virtual void Checkout (char const * fullTargetPath, GlobalId gid) = 0;
};

class LocalMergerProxy: public MergerProxy
{
public:
	LocalMergerProxy (Model & model);

	void AddFile (char const * fullTargetPath, GlobalId gid, FileType type);
	void Delete (char const * fullTargetPath);
	void ReCreateFile (char const * fullTargetPath, GlobalId gid, FileType type);
	void MergeAttributes (std::string const & currentPath, std::string const & newPath, FileType newType);
	void Checkout (char const * fullTargetPath, GlobalId gid);

private:
	Model &	_model;
};

class RemoteMergerProxy: public MergerProxy
{
public:
	RemoteMergerProxy (int targetProjectId);

	void AddFile (char const * fullTargetPath, GlobalId gid, FileType type);
	void Delete (char const * fullTargetPath);
	void ReCreateFile (char const * fullTargetPath, GlobalId gid, FileType type);
	void MergeAttributes (std::string const & currentPath, std::string const & newPath, FileType newType);
	void Checkout (char const * fullTargetPath, GlobalId gid);

private:
	int	_targetProjectId;
};

void ExecuteEditor (XML::Tree & args);
void ExecuteDiffer (XML::Tree & args);
void ExecuteMerger (XML::Tree & args);
// Returns true when auto merge without conflicts
bool ExecuteAutoMerger (XML::Tree & args, 
						ActiveMergerWatcher * mergerWatcher = 0,
						GlobalId fileGid = gidInvalid);

#endif
