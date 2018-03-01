#if !defined (SCCPROXYEX_H)
#define SCCPROXYEX_H
//------------------------------------
//  (c) Reliable Software, 2003 - 2008
//------------------------------------

#include "SccProxy.h"
#include "GlobalId.h"

class SccProxyEx : public CodeCoop::Proxy
{
public:
	SccProxyEx (CodeCoop::TextOutProc textOutCallback = 0);

	unsigned int VersionId (char const *path, CodeCoop::SccOptions cmdOptions);
	unsigned int VersionId (int projId, CodeCoop::SccOptions cmdOptions);
	bool Report (char const *path, unsigned long versionGid, std::string & versionDescr);
	bool Report (int projId, unsigned long versionGid, std::string & versionDescr);
	bool CoopCmd (char const * path, std::string const & cmd);
	bool CoopCmd (int projId, std::string const & cmd, bool skipGuiInProject, bool noTimeout);
	bool CoopCmd (std::string const & cmd);
	bool GetFileState (char const * filePath, unsigned long & state);
	bool GetForkScriptIds (int targetProjectId,
						   bool deepForks,
						   GidList const & myForkIds,
						   GlobalId & youngestFoundScriptId,
						   GidList & targetForkIds);
	bool GetTargetPath (int targetProjectId,
						GlobalId gid,
						std::string const & sourceProjectPath,
						std::string & targetAbsolutePath,
						unsigned long & targetType,
						unsigned long & statusAtTarget);
	void SccHold ();
	void SccResume ();
};


#endif
