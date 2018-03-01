//------------------------------------
//  (c) Reliable Software, 2003 - 2008
//------------------------------------

#include "precompiled.h"
#include "SccProxyEx.h"
#include "SccEx.h"

SccProxyEx::SccProxyEx (CodeCoop::TextOutProc textOutCallback)
	: CodeCoop::Proxy (textOutCallback)
{}

unsigned int SccProxyEx::VersionId (char const *path, CodeCoop::SccOptions cmdOptions)
{
	unsigned int id = -1;
	SccVersionIdByPathPtr versionId;
	_dll.GetFunction ("SccVersionIdByPath", versionId);
	(*versionId) (_context, &path, cmdOptions.GetValue (), &id);
	return id;
}

unsigned int SccProxyEx::VersionId (int projId, CodeCoop::SccOptions cmdOptions)
{
	unsigned int id = -1;
	SccVersionIdByProjPtr versionId;
	_dll.GetFunction ("SccVersionIdByProj", versionId);
	(*versionId) (_context, projId, cmdOptions.GetValue (), &id);
	return id;
}

bool SccProxyEx::Report (char const *path, unsigned long versionGid, std::string & versionDescr)
{
	SccReportByPathPtr report;
	_dll.GetFunction ("SccReportByPath", report);
	SCCRTN result = (*report) (_context, &path, versionGid);
	if (result == SCC_OK)
	{
		SccGetStringBufPtr get;
		_dll.GetFunction ("SccGetStringBuf", get);
		versionDescr.assign ((*get) ());
		return true;
	}
	return false;
}

bool SccProxyEx::Report (int projId, unsigned long versionGid, std::string & versionDescr)
{
	SccReportByProjPtr report;
	_dll.GetFunction ("SccReportByProj", report);
	SCCRTN result = (*report) (_context, projId, versionGid);
	if (result == SCC_OK)
	{
		SccGetStringBufPtr get;
		_dll.GetFunction ("SccGetStringBuf", get);
		versionDescr.assign ((*get) ());
		return true;
	}
	return false;
}

bool SccProxyEx::CoopCmd (char const * path, std::string const & cmd)
{
	SccCoopCmdByPathPtr coopcmd;
	_dll.GetFunction ("SccCoopCmdByPath", coopcmd);
	SCCRTN result = (*coopcmd) (_context, &path, cmd.c_str ());
	return result == SCC_OK;
}

bool SccProxyEx::CoopCmd (int projId, std::string const & cmd, bool skipGuiInProject, bool noTimeout)
{
	Assert (projId > 0);
	SccCoopCmdByProjPtr coopcmd;
	_dll.GetFunction ("SccCoopCmdByProj", coopcmd);
	SCCRTN result = (*coopcmd) (_context, projId, cmd.c_str (), skipGuiInProject, noTimeout);
	return result == SCC_OK;
}

bool SccProxyEx::CoopCmd (std::string const & cmd)
{
	SccCoopCmdPtr coopcmd;
	_dll.GetFunction ("SccCoopCmd", coopcmd);
	SCCRTN result = (*coopcmd) (_context, cmd.c_str ());
	return result == SCC_OK;
}

bool SccProxyEx::GetFileState (char const * filePath, unsigned long & state)
{
	SccGetFileStatePtr getfilestate;
	_dll.GetFunction ("SccGetFileState", getfilestate);
	SCCRTN result = (*getfilestate) (_context, filePath, state);
	return result == SCC_OK;
}

bool SccProxyEx::GetForkScriptIds (int targetProjectId,
								   bool deepForks,
								   GidList const & myForkIds,
								   GlobalId & youngestFoundScriptId,
								   GidList & targetYoungerForkIds)
{
	SccGetForkScriptIdsPtr getforkscriptids;
	_dll.GetFunction ("SccGetForkScriptIds", getforkscriptids);
	SCCRTN result = (*getforkscriptids) (_context,
										 targetProjectId,
										 deepForks,
										 myForkIds,
										 youngestFoundScriptId);
	if (result == SCC_OK)
	{
		SccGetGidListBufPtr get;
		_dll.GetFunction ("SccGetGidListBuf", get);
		GidList const & list = (*get)();
		targetYoungerForkIds.assign (list.begin (), list.end ());
		return true;
	}
	return false;
}

bool SccProxyEx::GetTargetPath (int targetProjectId,
								GlobalId gid,
								std::string const & sourceProjectPath,
								std::string & targetAbsolutePath,
								unsigned long & targetType,
								unsigned long & statusAtTarget)
{
	SccGetTargetPathPtr gettargetpath;
	_dll.GetFunction ("SccGetTargetPath", gettargetpath);
	SCCRTN result = (*gettargetpath) (_context,
									  targetProjectId,
									  gid,
									  sourceProjectPath,
									  targetType,
									  statusAtTarget);
	if (result == SCC_OK)
	{
		SccGetStringBufPtr get;
		_dll.GetFunction ("SccGetStringBuf", get);
		targetAbsolutePath.assign ((*get) ());
		return true;
	}
	return false;
}

void SccProxyEx::SccHold ()
{
	SccHoldPtr sccHold;
	_dll.GetFunction ("SccHold", sccHold);
	(*sccHold)();
}

void SccProxyEx::SccResume ()
{
	SccHoldPtr sccResume;
	_dll.GetFunction ("SccResume", sccResume);
	(*sccResume)();
}
