#if !defined (SCCEX_H)
#define SCCEX_H
//-----------------------------------
//  (c) Reliable Software 2001 - 2008
//-----------------------------------

#include <windows.h>
#include "Scc.h"
#include "GlobalId.h"

//
// Code Co-op SccDll extensions
//

//
// Linkage for external functions will be C naming mode.
//

extern "C"
{
// Returns Code Co-op major version.
// Used to detect version mismatch between Code Co-op and command line utilities.
long EXTFUN SccGetCoopVersion (void);

// Return next version id used by Code Co-op during check-in or
// the current project version
SCCRTN EXTFUN SccVersionIdByPath (
						void * context,
                        char const ** lpFileNames,
						unsigned long options,
						GlobalId * pId
                        );

SCCRTN EXTFUN SccVersionIdByProj (
					    void * context,
                        int projId,
						unsigned long options,
						GlobalId * pId
                        );

// Return project version description
SCCRTN EXTFUN SccReportByPath (
					    void * context,
                        char const ** lpFileNames,
						GlobalId versionGid
                        );

SCCRTN EXTFUN SccReportByProj (
					    void * context,
                        int projId,
						GlobalId versionGid
                        );

EXTFUN std::string const & SccGetStringBuf ();
EXTFUN GidList const & SccGetGidListBuf ();

// Execute Code Co-op command
SCCRTN EXTFUN SccCoopCmdByPath (
						void * context,
                        char const ** lpFileNames,
						char const * cmd
                        );

SCCRTN EXTFUN SccCoopCmdByProj (
						void * context,
                        int projId,
						char const * cmd,
						bool skipGuiInProject,
						bool noTimeout
                        );

SCCRTN EXTFUN SccCoopCmd (
    					void * context,
						char const * cmd
                        );
// Query file state
SCCRTN EXTFUN SccGetFileState (
					    void * context,
                        char const * filePath,
						unsigned long & state
                        );

// Project merge support
SCCRTN EXTFUN SccGetForkScriptIds (
					    void * context,
						int projId,
						bool deepForks,
                        GidList const & myForkIds,
						GlobalId & youngestFoundScriptId
                        );

SCCRTN EXTFUN SccGetTargetPath (
					    void * context,
						int projId,
                        GlobalId gid,
						std::string const & sourceProjectPath,
						unsigned long & targeType,
						unsigned long & statusAtTarget
                        );

void EXTFUN SccHold ();
void EXTFUN SccResume ();

}

typedef SCCRTN (*SccVersionIdByPathPtr) (void * context,
										 LPCSTR* lpFileNames,
										 unsigned long options,
										 unsigned int * pId);

typedef SCCRTN (*SccVersionIdByProjPtr) (void * context,
										 int projId,
										 unsigned long options,
										 unsigned int * pId);

typedef SCCRTN (*SccReportByPathPtr) (void * context,
									  LPCSTR* lpFileNames,
									  unsigned long versionGid);

typedef SCCRTN (*SccReportByProjPtr) (void * context,
									  int projId,
									  unsigned long versionGid);

typedef std::string const & (*SccGetStringBufPtr) ();
typedef GidList const & (*SccGetGidListBufPtr) ();

typedef SCCRTN (*SccCoopCmdByPathPtr) (void * context,
									   LPCSTR* path,
									   char const * cmd);

typedef SCCRTN (*SccCoopCmdByProjPtr) (void * context,
									   int projId,
									   char const * cmd,
									   bool skipGuiInProject,
									   bool noTimeout);

typedef SCCRTN (*SccCoopCmdPtr) (void * context, char const * cmd);

typedef SCCRTN (*SccGetFileStatePtr) (void * context,
									  char const * filePath,
									  unsigned long & state);

typedef SCCRTN (*SccGetForkScriptIdsPtr) (void * context,
										  int projId,
										  bool deepForks,
										  GidList const & myForkIds,
										  GlobalId & youngestFoundScriptId);

typedef SCCRTN (*SccGetTargetPathPtr) (void * context,
									   int projId,
									   GlobalId gid,
									   std::string const & sourceProjectPath,
									   unsigned long & targetType,
									   unsigned long & statusAtTarget);

typedef void (*SccHoldPtr) ();
typedef void (*SccResumePtr) ();

#endif
