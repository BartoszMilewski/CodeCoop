//------------------------------------
//  (c) Reliable Software, 1999 - 2008
//------------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "SccEx.h"
#include "FileClassifier.h"
#include "CoopCmdLine.h"
#include "CoopExec.h"
#include "SccOptions.h"
#include "IdeOptions.h"
#include "OutputSink.h"
#include "IdeContext.h"
#include "IpcConversation.h"
#include "ProjectVisit.h"
#include "CmdData.h"
#include "PathRegistry.h"
#include "Registry.h"
#include "Catalog.h"
#include "ProjectData.h"
#include "SccDll.h"
#include "GlobalLock.h"

#include <File/Dir.h>
#include <Dbg/Out.h>
#include <File/Path.h>
#include <Sys/Synchro.h>

#include <StringOp.h>
#include <Dbg/Out.h>
#include <Dbg/Log.h>

ServerGate	TheServerGate;

class SccTrace
{
public:
	SccTrace (char const * apiName, LONG nFiles = 0, LPCSTR* lpFileNames = 0)
		: _apiName (apiName)
	{
		dbg << "==> SccDll::" << _apiName.c_str () << std::endl;
#if 0
		if (nFiles <= 16)
		{
			for (int k = 0; k < nFiles; ++k)
				dbg << "    " << lpFileNames [k] << std::endl;
		}
		else
		{
			dbg << "    there are " << std::dec << nFiles << " files on the argument list" << std::endl;
		}
#endif
	};
	~SccTrace ()
	{
		dbg << "<== SccDll::" << _apiName.c_str () << "\n" << std::endl;
	};
private:
	std::string	_apiName;
};

static void ScanCatalog (char const * path, IDEContext * ideContext)
{
	if (path == 0)
		return;
	// Try to match IDE project with Code Co-op project.
	// We scan catalog looking at project root path.
	// If the root path from catalog is prefix of the IDE path
	// then we assume that IDE project maps to this Code Co-op project.
	// The project names can differ.
	// There is no guarantee that we will find match here, because
	// IDE can use really funny project root paths like "c:\".
	FilePath idePath (path);
	for (ProjectSeq it (TheCatSingleton.GetCatalog ()); !it.AtEnd (); it.Advance ())
	{
		Project::Data proj;
		it.FillInProjectData (proj);
		FilePath coopProjectRoot (proj.GetRootDir ());
		if (idePath.HasPrefix (coopProjectRoot))
		{
			// Root path match
			ideContext->SetProjectId (proj.GetProjectId ());
			ideContext->SetProjectName (proj.GetProjectName ());
			ideContext->SetRootPath (coopProjectRoot.GetDir ());
			break;
		}
	}
}

static void Display (std::string const & msg, IDEContext const * ideContext, bool isError = false)
{
	if (ideContext != 0 && ideContext->IsIdeTextOutAvailable ())
	{
		ideContext->Display (msg, isError);
	}
}

static void Display (std::string const & msg, Win::Exception e, IDEContext const * ideContext)
{
	if (ideContext != 0 && ideContext->IsIdeTextOutAvailable ())
	{
		std::string info (msg);
		info += "\n";
		std::string exMsg (Out::Sink::FormatExceptionMsg (e));
		info += exMsg.c_str ();
		ideContext->Display (info, true);
	}
}

static long ExecuteCommand (CmdData const & cmdData, bool keepCheckedOut = false)
{
	SccTrace trace ("ExecuteCommannd");

	IDEContext const * ideContext = cmdData.GetIdeContext ();
	try
	{
		if (cmdData.FileCount () != 0)
		{
			// Go over projects and execute command
			for (FileListClassifier::Iterator iter = cmdData.ProjectBegin ();
				iter != cmdData.ProjectEnd ();
				++iter)
			{
				FileListClassifier::ProjectFiles const * projFiles = *iter;
#if defined (DIAGNOSTIC)
				std::string info ("Executing command ");
				info += cmdData.GetCmdName ();
				info += " in project ";
				info += projFiles->GetProjectName ();
				info += " (";
				info +=projFiles->GetProjectRootPath ();
				info += ") ";
				info += ToString (cmdData.FileCount ());
				info += " file(s)";
				Display (info.c_str (), ideContext);
				dbg << "	" << info.c_str () << std::endl;
#endif
				dbg << "    Locking project" << std::endl;
				ProjectVisit projectVisit (TheServerGate);
				ClientConversation conversation;
				ClientConversation::CmdOptions cmdOptions;
				cmdOptions.SetLastCommand (!keepCheckedOut);
				if (!conversation.ExecuteCommand (projFiles, cmdData.GetCmdName (), cmdOptions))
				{
					dbg << "    FAILURE -- Cannot access Source Code Control provider" << std::endl;
					char const * coopErrMsg = conversation.GetErrorMsg ();
					if (coopErrMsg != 0)
						Display (coopErrMsg, ideContext, true);	// Error
					return SCC_E_NONSPECIFICERROR;
				}

				if (keepCheckedOut)
				{
					if (!conversation.ExecuteCommand (projFiles, "Selection_CheckOut"))
					{
						dbg << "    FAILURE -- Keep checked out -- Cannot access Source Code Control provider" << std::endl;
						char const * coopErrMsg = conversation.GetErrorMsg ();
						if (coopErrMsg != 0)
							Display (coopErrMsg, ideContext, true);	// Error
						return SCC_E_NONSPECIFICERROR;
					}
				}
			}
		}
		else if (ideContext != 0) // no files passed but there is a context
		{
			dbg << "    Locking project" << std::endl;
			dbg << "Executing command " << cmdData.GetCmdName () << " in project " << ideContext->GetProjectName ();
			dbg << " (" << ideContext->GetRootPath () << ") " << std::endl;
			ProjectVisit projectVisit (TheServerGate);
			ClientConversation conversation;
			if (!conversation.ExecuteCommand (cmdData.GetCmdName ()))
			{
				char const * coopErrMsg = conversation.GetErrorMsg ();
				if (coopErrMsg != 0)
					Display (coopErrMsg, ideContext, true);	// Error
				return SCC_E_NONSPECIFICERROR;
			}
		}
	}
	catch (Win::Exception e)
	{
		Display ("Cannot execute Code Co-op command.", e, ideContext);
		dbg << "    FAILURE -- " << e.GetMessage () << std::endl;
		return SCC_E_NONSPECIFICERROR;
	}
	catch ( ... )
	{
		dbg << "    FAILURE -- unknown exception" << std::endl;
		Win::ClearError ();
		Display ("Unknown exception during command execution.", ideContext);
		return SCC_E_NONSPECIFICERROR;
	}
	return SCC_OK;
}

static bool QueryFileStatus (IDEContext * ideContext, 
							 LONG nFiles, 
							 LPCSTR* lpFileNames, 
							 LPLONG lpStatus)
{
	SccTrace trace ("QueryFileStatus");

	FileListClassifier classifier (nFiles, lpFileNames, ideContext);
	for (FileListClassifier::Iterator iter = classifier.begin ();
		 iter != classifier.end ();
		 ++iter)
	{
#if defined (DIAGNOSTIC)
		FileListClassifier::ProjectFiles const * proj = *iter;
		std::string info ("Querying file status in project ");
		info += proj->GetProjectName ().c_str ();
		info += " (";
		info += proj->GetProjectRootPath ();
		info += ") ";
		info += ToString (proj->GetFileCount ());
		info += " file(s)";
		Display (info.c_str (), ideContext);
#endif
		dbg << "    Locking project" << std::endl;
		ProjectVisit projectVisit (TheServerGate);
		ClientConversation conversation;
		if (!conversation.Query (*iter))	// File state request
		{
			dbg << "    FAILURE -- Cannot access Source Code Control provider" << std::endl;
			char const * coopErrMsg = conversation.GetErrorMsg ();
			if (coopErrMsg != 0)
				Display (coopErrMsg, ideContext, true); // Error
			return false;
		}
		// Translate Code Co-op file state to IDE status
		dbg << "    Translating Code Co-op state to IDE status" << std::endl;
		StatusSequencer statusSeq (conversation);
		(*iter)->FillStatus (lpStatus, statusSeq);
	}
	return true;
}

// Common Source Code Control interface implementation.
// Integrates Code Co-op with Microsoft development tools

LONG SccGetVersion (void)
{
	SccTrace trace ("SccGetVersion");
	dbg << "    Version: " << std::hex << SCC_VER_NUM << std::endl;
	return SCC_VER_NUM;
}

/*******************************************************************************
	Call this function once per instance of a provider.
*******************************************************************************/
SCCRTN SccInitialize (LPVOID * context,				// SCC provider contex 
					  HWND hWnd,					// IDE window
					  LPCSTR callerName,			// IDE name
					  LPSTR sccName,				// SCC provider name
					  LPLONG sccCaps,				// SCC provider capabilities
					  LPSTR auxPathLabel,			// Aux path label, used in project open
					  LPLONG checkoutCommentLen,	// Check out comment max length
					  LPLONG commentLen)			// Other comments max length
{
#if defined (DIAGNOSTIC)
	// Uncomment this to turn on logging to the file. You may change the path.
	//FilePath userDesktopPath;
	//ShellMan::VirtualDesktopFolder userDesktop;
	//userDesktop.GetPath (userDesktopPath);
	//if (!Dbg::TheLog.IsOn ()) 
	//	Dbg::TheLog.Open ("SccDllLog.txt", userDesktopPath.GetDir ());

	// By default we log to debug monitor window 
	Dbg::TheLog.DbgMonAttach ("SccDll");
#endif

	SccTrace trace ("SccInitialize");
	dbg << "    Caller name: " << callerName << std::endl;

	try
	{
		TheOutput.SetParent (hWnd);
		// Revisit: do we care about callerName ?
		TheOutput.Init ("Source Code Control Provider (Code Co-op)", Registry::GetLogsFolder ());
		// Check info created during setup
		std::string pgmPath = Registry::GetProgramPath ();
		std::string catPath = Registry::GetCatalogPath ();
		if (pgmPath.empty () || !File::Exists (pgmPath.c_str ()) ||
			catPath.empty () || !File::Exists (catPath.c_str ()))
		{
			TheOutput.Display ("Cannot find Code Co-op program files. Run setup again.", Out::Error);
			return SCC_E_INITIALIZEFAILED;
		}
		*context = new IDEContext (callerName);
		strcpy (sccName, "Code Co-op");
		*sccCaps =	SCC_CAP_REMOVE |				// SccRemove supported
					SCC_CAP_DIFF |					// SccDiff supported
					SCC_CAP_RUNSCC |				// SccRun supported
					SCC_CAP_QUERYINFO |				// SccQueryInfo supported
					SCC_CAP_POPULATELIST |			// SccPopulateList supported
					SCC_CAP_COMMENTCHECKIN |		// Supports comment on check-in
					SCC_CAP_GET_NOUI |				// No user interface for Get command
					SCC_CAP_GETPROJPATH |			// SccGetProjPath supported
					SCC_CAP_TEXTOUT |				// Writes text to an IDE-provided output function
					SCC_CAP_MULTICHECKOUT |			// Supports multiple checkouts on a file
													//   (subject to administrator override)
					//SCC_CAP_HISTORY |				// Show file history in GUI
					SCC_CAP_SCCFILE |				// Provider creates MSSCCPRJ.SCC file
													//   for supported file types
					SCC_CAP_REENTRANT;
		strcpy (auxPathLabel, "AuxPathLabel");
		*checkoutCommentLen = 0;
		*commentLen = 1024;
		dbg << "    SccInitialize: return SCC_OK" << std::endl;
		return SCC_OK;
	}
	catch ( ... )
	{
		Win::ClearError ();
		TheOutput.Display ("Unknown error during initialization.", Out::Error);
		return SCC_E_INITIALIZEFAILED;
	}
}

/*******************************************************************************
Returns extra capabilities for the provider.
*******************************************************************************/
SCCEXTERNC SCCRTN EXTFUN __cdecl SccGetExtendedCapabilities (__inout LPVOID pContext, 
															 __in LONG lSccExCap,
															 __out LPBOOL pbSupported)
{
	SccTrace trace ("SccGetExtendedCapabilities");
	*pbSupported = FALSE;
	if (lSccExCap == SCC_EXCAP_DELETE_CHECKEDOUT ||
		lSccExCap == SCC_EXCAP_RENAME_CHECKEDOUT)
	{
		*pbSupported = TRUE;
	}
	return SCC_OK;
}

/*******************************************************************************
	Call this function once for every instance of a provider, when it is going
	away.  You must call SccInitialize before calling this function, and should
	not call it with any open projects.
*******************************************************************************/
SCCRTN SccUninitialize (LPVOID context)
{
	SccTrace trace ("SccUninitialize");
	IDEContext * ideContext = reinterpret_cast<IDEContext *>(context);
	delete ideContext;
	TheOutput.SetParent (0);
#if defined (DIAGNOSTIC)
	//if (Dbg::TheLog.IsOn ())
	//	Dbg::TheLog.Close ();
	Dbg::TheLog.DbgMonDetach ();
#endif
	return SCC_OK;
}

/*******************************************************************************
	Opens a project.  This function should never be called with an already open
	project on context.  The lpUser, lpProjName, and lpAuxProjPath values
	may be modified by the provider.
*******************************************************************************/
SCCRTN SccOpenProject (LPVOID context,
					   HWND hWnd, 
					   LPSTR lpUser,
					   LPSTR lpProjName,
					   LPCSTR lpLocalProjPath,
					   LPSTR lpAuxProjPath,
					   LPCSTR lpComment,
					   LPTEXTOUTPROC lpTextOutProc,
					   LONG dwFlags)
{
	SccTrace trace ("SccOpenProject");
	dbg << "    lpLocalProjPath: " << lpLocalProjPath << std::endl;
	IDEContext * ideContext = reinterpret_cast<IDEContext *>(context);
	if (ideContext != 0)
		ideContext->SetIdeTextOut (lpTextOutProc);

	if (ideContext == 0 || lpLocalProjPath == 0 || lpLocalProjPath [0] == '\0')
	{
		dbg << "    SccOpenProject with empty path returns SCC_E_OPNOTSUPPORTED" << std::endl;
		return SCC_E_OPNOTSUPPORTED;
	}

	ScanCatalog (lpLocalProjPath, ideContext);
	if (!ideContext->IsProjectLocated ())
	{
		if ((dwFlags & SCC_OP_CREATEIFNEW) != 0)
		{
			// -Project_New project:"Name" root:"Path" email:"e-mail" user:"name" 
			//      filenames:"short" comment:"User comment"
			std::string cmdString ("Project_New project:\"");
			cmdString += lpProjName;
			cmdString += "\" root:\"";
			cmdString += lpLocalProjPath;
			cmdString += "\" email:\"";
			cmdString += TheCatSingleton.GetCatalog ().GetHubId ();
			cmdString += "\" user:\"";
			cmdString += Registry::GetUserName ();
			cmdString += "\" comment:\"No comment\"";
			CmdData cmdData (cmdString, ideContext);
			
			return ExecuteCommand (cmdData);
		}
		else if ((dwFlags & SCC_OP_SILENTOPEN) != 0)
		{
			dbg << "    silent flag set. Returning." << std::endl;
			return SCC_E_UNKNOWNPROJECT;
		}
		else
		{
			dbg << "    silent flag not set" << std::endl;
			// provider may prompt for project name
			dbg << "   Create new project: " << lpProjName;
			dbg << ", path: " << lpLocalProjPath << ", user: " << lpUser << std::endl;
			return SCC_E_UNKNOWNPROJECT;
		}

		dbg << "    SccOpenProject cannot locate project, returns SCC_E_UNKNOWNPROJECT" << std::endl;
		return SCC_E_UNKNOWNPROJECT;
	}

	strcpy (lpProjName, ideContext->GetProjectName ());
	strcpy (lpAuxProjPath, "Code Co-op");

	if (lpUser [0] == 0)
		strcpy (lpUser, "User");

	dbg << "	Return values:" << std::endl;
	dbg << "    lpUser: " << lpUser << std::endl;
	dbg << "    lpProjName: " << lpProjName << std::endl;
	dbg << "    lpAuxProjPath: " << lpAuxProjPath << std::endl;
	return SCC_OK;
}

/*******************************************************************************
	Called to close a project opened by SccOpenProject.
*******************************************************************************/
SCCRTN SccCloseProject (LPVOID context)
{
	SccTrace trace ("SccCloseProject");
	IDEContext * ideContext = reinterpret_cast<IDEContext *>(context);
	if (ideContext != 0)
	{
		ideContext->ResetCurrentProject ();
	}
	return SCC_OK;
}

/*******************************************************************************
	Prompts the user for provider project information.  This may include the
	path to a certain project.  The caller must be prepared to accept changes
	to lpUser, lpProjName, lpLocalPath, and lpAuxProjPath.  lpProjName and
	lpAuxProjPath are then used in a call to SccOpenProject.  They should not
	be modified by the caller upon return.  The caller should avoid displaying
	these two parameters upon return, as the provider might use a formatted
	string that is not ready for view.
*******************************************************************************/
SCCRTN SccGetProjPath (LPVOID context, 
					   HWND hWnd, 
					   LPSTR lpUser,
					   LPSTR lpProjName, 
					   LPSTR lpLocalPath,
					   LPSTR lpAuxProjPath,
					   BOOL bAllowChangePath,
					   LPBOOL pbNew)
{
	SccTrace trace ("SccGetProjectPath");
	dbg << "    lpLocalPath: " << lpLocalPath << ", project: " << lpProjName;
	dbg << ", user: " << lpUser << ", pbNew: " << *pbNew;
	dbg << ", bAllowChangePath: " << bAllowChangePath << std::endl;
	
	IDEContext * ideContext = reinterpret_cast<IDEContext *>(context);
	if (ideContext == 0 || lpLocalPath == 0 || lpLocalPath [0] == 0)
	{
		dbg << "    SccGetProjPath with empty context or path returns SCC_E_OPNOTSUPPORTED" << std::endl;
		return SCC_E_OPNOTSUPPORTED;
	}

	bool isCreated = false;
	if (!ideContext->IsProjectLocated ())
	{
		ScanCatalog (lpLocalPath, ideContext);
		if (!ideContext->IsProjectLocated ())
		{
			if (*pbNew != 0)
			{
				dbg << "    Creating new project" << std::endl;
				isCreated = true;
				ideContext->SetRootPath (lpLocalPath);
				if (lpProjName [0] == 0)
				{
					PathSplitter projPath (lpLocalPath);
					if (File::IsFolder (lpLocalPath) && projPath.HasFileName ())
					{
						ideContext->SetProjectName (projPath.GetFileName ());
					}
					else
					{
						dbg << "    SccGetProjPath cannot locate project, ";
						dbg << "project name empty, cannot retrieve it from project path, ";
						dbg << "returns SCC_E_UNKNOWNPROJECT" << std::endl;
						return SCC_E_UNKNOWNPROJECT;
					}
				}
				else
				{
					ideContext->SetProjectName (lpProjName);
				}
			}
			else
			{
			    dbg << "    SccGetProjPath cannot locate project, ";
				dbg << "not allowed to create it, ";
				dbg << "returns SCC_E_UNKNOWNPROJECT" << std::endl;
					return SCC_E_UNKNOWNPROJECT;
			}
		}
	}
	*pbNew = isCreated ? TRUE : FALSE;
	if (lpUser [0] == 0)
		strcpy (lpUser, "User");

	if (ideContext->IsBorlandIDE () &&
		(lpProjName == 0 || lpProjName [0] == 0))
	{
		std::string projName (ideContext->GetProjectName ());
		std::string projPath (ideContext->GetRootPath ());
		FilePath rootPath (projPath);
		std::string cdpName =  projName + ".cdp";
		if (!File::Exists (rootPath.GetFilePath (cdpName)))
		{
			dbg << "Creating cdp file" << std::endl;
			std::string cdpContents = "[Reliable Software Code Co-op]\nProjName=" + 
									  projName + 
									  "\nAuxProjPath=" + projPath +
									  "\nUserName=" + std::string (lpUser);
			File::Size size (cdpContents.length (), 0);
			MemFileNew cdpFile (rootPath.GetFilePath (cdpName), size);
			memcpy (cdpFile.GetBuf (), cdpContents.c_str (), cdpContents.length ());
		}
	}

	strcpy (lpProjName, ideContext->GetProjectName ());
	strcpy (lpAuxProjPath, "Code Co-op");
	if (bAllowChangePath)
	{
		dbg << "    Changing local path to: " << ideContext->GetRootPath () << std::endl;
		strcpy (lpLocalPath, ideContext->GetRootPath ());
	}
	else
	{
		dbg << "    bAllowChangePath = FALSE\n";
	}

	dbg << "    Return values:" << std::endl;
	dbg << "    *pbNew: " << *pbNew << std::endl;
	dbg << "    lpUser: " << lpUser << std::endl;
	dbg << "    lpProjName: " << lpProjName << std::endl;
	dbg << "    lpLocalPath: " << lpLocalPath << std::endl;
	dbg << "    lpAuxProjPath: " << lpAuxProjPath << std::endl;
	return SCC_OK;
}

/*******************************************************************************
	Retrieves a read only copy of a set of files.  The array is a set of files
	on the local disk.  The paths must be fully qualified.
*******************************************************************************/
SCCRTN SccGet (LPVOID context, 
			   HWND hWnd, 
			   LONG nFiles, 
			   LPCSTR* lpFileNames, 
			   LONG dwFlags,
			   LPCMDOPTS pvOptions)
{
	SccTrace trace ("SccGet");
	return SCC_OK;
}

/*******************************************************************************
	Checks out the array of files.  The array is a set of fully qualified local
	path names.
*******************************************************************************/
SCCRTN SccCheckout (LPVOID context, 
					HWND hWnd, 
					LONG nFiles, 
					LPCSTR* lpFileNames, 
					LPCSTR lpComment, 
					LONG dwFlags,
					LPCMDOPTS pvOptions)
{
	SccTrace trace ("SccCheckout", nFiles, lpFileNames);

	CodeCoop::SccOptions options (pvOptions);
	std::string cmdName;
	if (options.IsAllInProject ())
		cmdName = "All_DeepCheckOut";
	else if (options.IsRecursive ())
		cmdName = "Selection_DeepCheckOut";
	else
		cmdName = "Selection_CheckOut";

	// Special case for VC
	// Scenario: 
	// A file is under Co-op control
	// In VC: a user selects to Add this file To Project
	// VC renames the file and then tries to check out the original file
	// which fails.
	// Solution: Check if an original file is on disk. No? Find files with names
	// in GUID format without an extension. Found one such file? Rename it back to
	// original name and proceed with check-out.
	if (nFiles == 1)
	{
		if (!File::Exists (lpFileNames [0]))
		{
			// find files having names in GUID format
			FilePath folder (lpFileNames [0]);
			folder.DirUp ();
			std::string guidFile;
			for (FileSeq seq (folder.GetFilePath ("{*}"));
				!seq.AtEnd ();
				seq.Advance ())
			{
				if (seq.IsFolder ())
					continue;

				if (HasGuidFormat (seq.GetName ()))
				{
					if (guidFile.empty ())
						guidFile = seq.GetName ();
					else
					{
						guidFile.clear ();
						break; // more than one file with GUID-like name
					}
				}
			}
			if (!guidFile.empty ())
			{
				// we cannot move the file because VC raises an error
				File::CopyNoEx (folder.GetFilePath (guidFile),
								lpFileNames [0]);
			}
		}
	}
	CmdData cmd (cmdName, nFiles, lpFileNames, context);
	return ExecuteCommand (cmd);
}

/*******************************************************************************
	Undo a checkout of an array of files.  The array is a set of fully qualified
	local path names.
*******************************************************************************/
SCCRTN SccUncheckout (LPVOID context, 
					  HWND hWnd, 
					  LONG nFiles, 
					  LPCSTR* lpFileNames, 
					  LONG dwFlags,
					  LPCMDOPTS pvOptions)
{
	SccTrace trace ("SccUncheckout", nFiles, lpFileNames);

	CodeCoop::SccOptions options (pvOptions);
	std::string cmdName;
	if (options.IsAllInProject ())
		cmdName = "All_Uncheckout";
	else
		cmdName = "Selection_Uncheckout";
	CmdData cmd (cmdName, nFiles, lpFileNames, context);
	return ExecuteCommand (cmd);
}

/*******************************************************************************
	Make the modifications the user has made to an array of files permanent. The
	file names must be fully qualified local paths.
*******************************************************************************/
SCCRTN SccCheckin (LPVOID context, 
				   HWND hWnd, 
				   LONG nFiles, 
				   LPCSTR* lpFileNames, 
				   LPCSTR lpComment, 
				   LONG dwFlags,
				   LPCMDOPTS pvOptions)
{
	SccTrace trace ("SccCheckin", nFiles, lpFileNames);

	CodeCoop::SccOptions options (pvOptions);
	std::string cmdName;
	if (options.IsAllInProject ())
		cmdName = "All_CheckIn";
	else
		cmdName = "Selection_CheckIn";
	cmdName += (" comment:\"");
	bool missingComment = (lpComment == 0) ||
						  (lpComment != 0 && strlen (lpComment) == 0);
	if (missingComment)
	{
		cmdName += "No comment\"";
	}
	else
	{
		std::string comment (lpComment);
		size_t i = 0;
		size_t len = comment.length ();
		// replace embedded double quotes with single quotes
		while ((i = comment.find ('"', i)) != std::string::npos)
		{
			comment.replace (i, 1, 1, '\'');
			++i;
			if (i == len)
				break;
		}
		cmdName += comment;
		cmdName += "\"";
	}
	CmdData cmd (cmdName, nFiles, lpFileNames, context);
	return ExecuteCommand (cmd, options.IsKeepCheckedOut ());
}

/*******************************************************************************
	Add an array of fully qualified files to the source control system.  The 
	array of flags describe the type of file.  See the SCC_FILETYPE_xxxx flags.
*******************************************************************************/
SCCRTN SccAdd (LPVOID context, 
			   HWND hWnd, 
			   LONG nFiles, 
			   LPCSTR* lpFileNames, 
			   LPCSTR lpComment, 
			   LONG * pdwFlags,
			   LPCMDOPTS pvOptions)
{
	SccTrace trace ("SccAdd", nFiles, lpFileNames);

	std::string cmdStr ("Selection_Add");
	CodeCoop::SccOptions options (pvOptions);
	if (options.IsTypeHeader ())
		cmdStr += " type:\"Header File\"";
	else if (options.IsTypeSource ())
		cmdStr += " type:\"Source File\"";
	else if (options.IsTypeText ())
		cmdStr += " type:\"Text File\"";
	else if (options.IsTypeBinary ())
		cmdStr += " type:\"Binary File\"";
	else
		cmdStr += " type:\"Auto File\"";
	dbg << cmdStr << std::endl;
	CmdData cmd (cmdStr, nFiles, lpFileNames, context);
	long result = ExecuteCommand (cmd);
	if (result != SCC_OK || options.IsDontCheckIn ())
		return result;

	// Follow it with check-in
	std::string cmdName ("Selection_CheckIn");
	cmdName += (" comment:\"");
	bool missingComment = (lpComment == 0) ||
						  (lpComment != 0 && strlen (lpComment) == 0);
	if (missingComment)
	{
		if (nFiles == 1)
			cmdName += "Added a new file\"";
		else
			cmdName += "Added new files\"";
	}
	else
	{
		cmdName += lpComment;
		cmdName += " [New Files]\"";
	}
	CmdData cmd2 (cmdName, nFiles, lpFileNames, context);
	return ExecuteCommand (cmd2, options.IsKeepCheckedOut ());
}

/*******************************************************************************
	Removes the array of fully qualified files from the source control system.
	The files are not removed from the user's disk, unless advanced options
	are set by the user.  Advaned options are defined by the provider.
*******************************************************************************/
SCCRTN SccRemove (LPVOID context, 
				  HWND hWnd, 
				  LONG nFiles, 
				  LPCSTR* lpFileNames,
				  LPCSTR lpComment,
				  LONG dwFlags,
				  LPCMDOPTS pvOptions)
{
	SccTrace trace ("SccRemove", nFiles, lpFileNames);

	CodeCoop::SccOptions options (pvOptions);
	std::string cmdName;
	if (options.IsDontDeleteFromDisk ())
		cmdName = "Selection_Remove";
	else
		cmdName = "Selection_DeleteFile";

	CmdData cmd (cmdName, nFiles, lpFileNames, context);
	return ExecuteCommand (cmd);
}

/*******************************************************************************
	Renames the given file to a new name in the source control system.  The
	provider should not attempt to access the file on disk.  It is the
	caller's responsibility to rename the file on disk.
*******************************************************************************/
SCCRTN SccRename (LPVOID context, 
				  HWND hWnd, 
				  LPCSTR lpFileName,
				  LPCSTR lpNewName)
{
	SccTrace trace ("SccRename");
	dbg << "    Old: " << lpFileName << " --> " << lpNewName << std::endl;
	return SCC_OK;
}

/*******************************************************************************
	Show the differences between the local users fully qualified file and the
	version under source control.
*******************************************************************************/
SCCRTN SccDiff (LPVOID context, 
				HWND hWnd, 
				LPCSTR lpFileName, 
				LONG dwFlags,
				LPCMDOPTS pvOptions)
{
	SccTrace trace ("SccDiff");
	dbg << "    " << lpFileName << std::endl;
	if ((dwFlags & SCC_DIFF_QUICK_DIFF) != 0)
	{
		// IDE wants to know if this file has changed
		// Revisit: implementation
		// For now always report no differences
		dbg << "    \"quick\", return SCC_OK" << std::endl;
		return SCC_OK;
	}
	else
	{
		CmdData cmd ("Selection_OpenCheckInDiff", 1, &lpFileName, context);
		return ExecuteCommand (cmd);
	}
}

/*******************************************************************************
	Show the history for an array of fully qualified local file names.  The
	provider may not always support an array of files, in which case only the
	first files history will be shown.
*******************************************************************************/
SCCRTN SccHistory (LPVOID context, 
				   HWND hWnd, 
				   LONG nFiles, 
				   LPCSTR* lpFileNames, 
				   LONG dwFlags,
				   LPCMDOPTS pvOptions)
{
	SccTrace trace ("SccHistory", nFiles, lpFileNames);
	IDEContext * ideContext = reinterpret_cast<IDEContext *>(context);
	try
	{
		if (nFiles != 0)
		{
			// Visit the project determined by selected files
			FileListClassifier classifier (nFiles, lpFileNames, ideContext);
			for (FileListClassifier::Iterator iter = classifier.begin ();
				 iter != classifier.end ();
				 ++iter)
			{
				ProjectVisit projectVisit (TheServerGate);
				ClientConversation conversation;
				if (!conversation.ExecuteCommandAndStayInGUI (*iter, "View_History"))
				{
					Display ("Cannot execute Code Co-op.", ideContext);
					return SCC_E_NONSPECIFICERROR;
				}
			}
		}
		else
		{
			CoopExec coop;
			if (!coop.Start ())
				return SCC_E_NONSPECIFICERROR;
		}
		return SCC_OK;
	}
	catch (Win::Exception e)
	{
		dbg << "    " << e.GetMessage () << std::endl;
		Display ("Cannot execute Code Co-op", e, ideContext);
		return SCC_E_NONSPECIFICERROR;
	}
	catch ( ... )
	{
		Win::ClearError ();
		Display ("Unknown exception during Code Co-op execution.", ideContext);
		return SCC_E_NONSPECIFICERROR;
	}

	return SCC_OK;
}

/*******************************************************************************
	Show the properties of a fully qualified file.  The properties are defined
	by the provider and may be different for each one.
*******************************************************************************/
SCCRTN SccProperties (LPVOID context, 
					  HWND hWnd, 
					  LPCSTR lpFileName)
{
	SccTrace trace ("SccProperties");
	dbg << "    " << lpFileName << std::endl;
	return SCC_OK;
}

/*******************************************************************************
	Examine a list of fully qualified files for their current status.  The
	return array will be a bitmask of SCC_STATUS_xxxx bits.  A provider may
	not support all of the bit types.  For example, SCC_STATUS_OUTOFDATE may
	be expensive for some provider to provide.  In this case the bit is simply
	not set.
*******************************************************************************/
SCCRTN SccQueryInfo (LPVOID context, 
					 LONG nFiles, 
					 LPCSTR* lpFileNames, 
					 LPLONG lpStatus)
{
	SccTrace trace ("SccQueryInfo", nFiles, lpFileNames);

	IDEContext * ideContext = reinterpret_cast<IDEContext *>(context);
	try
	{
		if (!QueryFileStatus (ideContext, nFiles, lpFileNames, lpStatus))
			return SCC_E_NONSPECIFICERROR;
	}
	catch (Win::Exception e)
	{
		dbg << e.GetMessage () << std::endl;
		Display ("Cannot query file status.", e, ideContext);
		return SCC_E_NONSPECIFICERROR;
	}
	catch (...)
	{
		Win::ClearError ();
		dbg << "    Unknown exception." << std::endl;
		Display ("Unknown exception during file status query.", ideContext);
		return SCC_E_NONSPECIFICERROR;
	}
	return SCC_OK;
}

/*******************************************************************************
	Like SccQueryInfo, this function will examine the list of files for their
	current status.  In addition, it will use the pfnPopulate function to 
	notify the caller when a file does not match the critera for the nCommand.
	For example, if the command is SCC_COMMAND_CHECKIN, and a file in the list
	is not checked out, then the callback is used to tell the caller this.  
	Finally, the provider may find other files that could be part of the command
	and add them.  This allows a VB user to check out a .bmp file that is used
	by their VB project, but does not appear in the VB makefile.
*******************************************************************************/
SCCRTN SccPopulateList (LPVOID context, 
						enum SCCCOMMAND nCommand, 
						LONG nFiles, 
						LPCSTR* lpFileNames, 
						POPLISTFUNC pfnPopulate, 
						LPVOID pvCallerData,
						LPLONG lpStatus, 
						LONG dwFlags)
{
	SccTrace trace ("SccPopulateList");

	if (pfnPopulate == 0)
		return SCC_E_NONSPECIFICERROR;

	IDEContext * ideContext = reinterpret_cast<IDEContext *>(context);

	try
	{
		// Construct the required file status -- look in 'FileListClassifier::ProjectFiles::FillStatus'
		// for Code Co-op state to IDE status translation details.
		long requiredStatus = 0;
		long relevantBits   = 0;
		dbg << "	command: " << nCommand << std::endl;
		switch (nCommand)
		{
		case SCC_COMMAND_UNCHECKOUT:
			relevantBits = SCC_STATUS_CONTROLLED | SCC_STATUS_CHECKEDOUT;
			requiredStatus = relevantBits;
			break;
		case SCC_COMMAND_CHECKIN:
			relevantBits = SCC_STATUS_CONTROLLED | SCC_STATUS_CHECKEDOUT;
			requiredStatus = relevantBits;
			break;
		case SCC_COMMAND_DIFF:
			relevantBits = SCC_STATUS_CONTROLLED | SCC_STATUS_CHECKEDOUT | SCC_STATUS_MODIFIED;
			requiredStatus = relevantBits;
			break;
		case SCC_COMMAND_ADD:
			relevantBits = SCC_STATUS_CONTROLLED;
			requiredStatus = SCC_STATUS_NOTCONTROLLED;
			break;
		case SCC_COMMAND_CHECKOUT:
			relevantBits = SCC_STATUS_CONTROLLED | SCC_STATUS_CHECKEDOUT;
			requiredStatus = SCC_STATUS_CONTROLLED;
			break;
		case SCC_COMMAND_REMOVE:
			relevantBits = SCC_STATUS_CONTROLLED | SCC_STATUS_DELETED;
			requiredStatus = SCC_STATUS_CONTROLLED;
			break;
		case SCC_COMMAND_GET:
		case SCC_COMMAND_HISTORY:
		case SCC_COMMAND_OPTIONS:
			relevantBits = SCC_STATUS_CONTROLLED;
			requiredStatus = relevantBits;
			break;
		};

		if (nFiles == 0 || (dwFlags & SCC_PL_DIR) != 0)
		{
			FilePath rootPath (ideContext->GetRootPath ());
			std::vector<char const *> foldersQueried;
			if (nFiles == 0)
			{
				// Revisit: temporary.
				// IDE passed no files. 
				// List files from root folder. Project files are usually there.
				dbg << "		IDE passed no files." << std::endl;
				foldersQueried.push_back (rootPath.GetDir ());
			}
			else
			{
				// IDE passed only folders -- list their contents and then query file status
				dbg << "	IDE passed only folders (" << nFiles << " folders)" << std::endl;
				for (int i = 0; i < nFiles; ++i)
				{
					foldersQueried.push_back (lpFileNames [i]);
				}
			}
			std::vector<std::string> filePaths;
			for (unsigned int j = 0; j < foldersQueried.size (); ++j)
			{
				FilePath path (foldersQueried [j]);
				for (FileSeq seq (path.GetAllFilesPath ()); !seq.AtEnd (); seq.Advance ())
				{
					if (seq.IsFolder ())
						continue;
					std::string filePath (path.GetFilePath (seq.GetName ()));
					filePaths.push_back (filePath);
				}
			}
			if (filePaths.empty ())
				return SCC_OK;
			// Build vector of pointers to the file paths
			std::vector<char const *> pathPtrs;
			for (unsigned int i = 0; i < filePaths.size (); ++i)
			{
				pathPtrs.push_back (filePaths [i].c_str ());
			}
			std::vector<long> statusVec;
			statusVec.resize (pathPtrs.size ());
			// Query file status
			if (!QueryFileStatus (ideContext, pathPtrs.size (), &pathPtrs [0], &statusVec [0]))
				return SCC_E_NONSPECIFICERROR;
			// Call back IDE if the file has the required status for a given command.
			BOOL keepGoing = TRUE;
			dbg << "	Files matching state criteria: (out of " << pathPtrs.size () << " files queried)" << std::endl;
			for (unsigned int i = 0; (i < pathPtrs.size ()) && (keepGoing != FALSE); ++i)
			{
				long fileStatus = statusVec [i];
				if ((fileStatus & relevantBits) == requiredStatus)
				{
					// Include file in the operation
					char const * filePath = pathPtrs [i];
					dbg << "		" << filePath << std::endl;
					keepGoing = (*pfnPopulate)(pvCallerData, TRUE, fileStatus, filePath);
				}
			}
		}
		else
		{
			// IDE passed a list of files -- query file status
			dbg << "	IDE passed a list of files:(" << nFiles << " files)" << std::endl;
			if (!QueryFileStatus (ideContext, nFiles, lpFileNames, lpStatus))
				return SCC_E_NONSPECIFICERROR;
			// Call back IDE if the file doesn't have the required status for a given command.
			BOOL keepGoing = TRUE;
			dbg << "	Files NOT matching state criteria:" << std::endl;
			for (long i = 0; (i < nFiles) && (keepGoing != FALSE); ++i)
			{
				long fileStatus = lpStatus [i];
				if ((fileStatus & relevantBits) != requiredStatus)
				{
					// Remove from initial list
					char const * filePath = lpFileNames [i];
					dbg << "		" << filePath << std::endl;
					keepGoing = (*pfnPopulate)(pvCallerData, FALSE, fileStatus, filePath); 
				}
			}
		}
	}
	catch (Win::Exception e)
	{
		dbg << "    " << e.GetMessage () << std::endl;
		Display ("Cannot populate project file list.", e, ideContext);
		return SCC_E_NONSPECIFICERROR;
	}
	catch (...)
	{
		Win::ClearError ();
		dbg << "    Unknown exception." << std::endl;
		Display ("Unknown exception during populating project file list.", ideContext);
		return SCC_E_NONSPECIFICERROR;
	}
	return SCC_OK;
}

/*******************************************************************************
	SccGetEvents runs in the background checking the status of files that the
	caller has asked about (via SccQueryInfo).  When the status changes, it 
	builds a list of those changes that the caller may exhaust on idle.  This
	function must take virtually no time to run, or the performance of the 
	caller will start to degrade.  For this reason, some providers may choose
	not to implement this function.
*******************************************************************************/
SCCRTN SccGetEvents (LPVOID context, 
					 LPSTR lpFileName,
					 LPLONG lpStatus,
					 LPLONG pnEventsRemaining)
{
	SccTrace trace ("SccGetEvents");
	return SCC_OK;
}

/*******************************************************************************
	This function allows a user to access the full range of features of the
	source control system.  This might involve launching the native front end
	to the product.  Optionally, a list of files are given for the call.  This
	allows the provider to immediately select or subset their list.  If the
	provider does not support this feature, it simply ignores the values.
*******************************************************************************/
SCCRTN SccRunScc(LPVOID context, 
				 HWND hWnd, 
				 LONG nFiles, 
				 LPCSTR* lpFileNames)
{
	SccTrace trace ("SccRunScc");

	IDEContext * ideContext = reinterpret_cast<IDEContext *>(context);
	try
	{
		if (nFiles != 0)
		{
			// Visit the project determined by selected files
			FileListClassifier classifier (nFiles, lpFileNames, ideContext);
			for (FileListClassifier::Iterator iter = classifier.begin ();
				 iter != classifier.end ();
				 ++iter)
			{
				ProjectVisit projectVisit (TheServerGate);
				ClientConversation conversation;
				if (!conversation.ExecuteCommandAndStayInGUI (*iter, "View_Files"))
				{
					Display ("Cannot execute Code Co-op.", ideContext);
					return SCC_E_NONSPECIFICERROR;
				}
			}
		}
		else
		{
			CoopExec coop;
			if (!coop.Start ())
				return SCC_E_NONSPECIFICERROR;
		}
		return SCC_OK;
	}
	catch (Win::Exception e)
	{
		dbg << "    " << e.GetMessage () << std::endl;
		Display ("Cannot execute Code Co-op", e, ideContext);
		return SCC_E_NONSPECIFICERROR;
	}
	catch ( ... )
	{
		Win::ClearError ();
		Display ("Unknown exception during Code Co-op execution.", ideContext);
		return SCC_E_NONSPECIFICERROR;
	}
}

/*******************************************************************************
	This function will prompt the user for advaned options for the given
	command.  Call it once with ppvOptions==NULL to see if the provider
	actually supports the feature.  Call it again when the user wants to see
	the advaned options (usually implemented as an Advaned button on a dialog).
	If a valid *ppvOptions is returned from the second call, then this value
	becomes the pvOptions value for the SccGet, SccCheckout, SccCheckin, etc...
	functions.
*******************************************************************************/
SCCRTN SccGetCommandOptions (LPVOID context, 
							 HWND hWnd, 
							 enum SCCCOMMAND nCommand,
							 LPCMDOPTS * ppvOptions)
{
	SccTrace trace ("SccGetCommandOptions");
	return SCC_OK;
}


/*******************************************************************************
	This function allows the user to browse for files that are already in the
	source control system and then make those files part of the current project.
	This is handy, for example, to get a common header file into the current
	project without having to copy the file.  The return array of files
	(lplpFileNames) contains the list of files that the user wants added to
	the current makefile/project.
*******************************************************************************/
SCCRTN SccAddFromScc (LPVOID context, 
					  HWND hWnd, 
					  LPLONG pnFiles,
					  LPCSTR** lplpFileNames)
{
	SccTrace trace ("SccAddFromScc");
	return SCC_OK;
}

/*******************************************************************************
	SccSetOption is a generic function used to set a wide variety of options.
	Each option starts with SCC_OPT_xxx and has its own defined set of values.
*******************************************************************************/
SCCRTN SccSetOption (LPVOID context,
					 LONG nOption,
					 LONG dwVal)
{
	SccTrace trace ("SccSetOption");
	dbg << "    nOption: " << nOption << " value: 0x" << std::hex << dwVal << std::endl;
	return SCC_E_OPNOTSUPPORTED;
}

/*******************************************************************************
Checks if multiple checkouts on a file are allowed.
*******************************************************************************/
SCCRTN SccIsMultiCheckoutEnabled (LPVOID pContext, 
								  LPBOOL pbMultiCheckout)
{
	SccTrace trace ("SccMultiCheckoutEnabled");
	*pbMultiCheckout = TRUE;
	return SCC_OK;
}

/*******************************************************************************
Checks if the provider will create MSSCCPRJ.SCC files in the same
directories as the given files if the given files are placed under source
control. The file paths must be fully qualified.
*******************************************************************************/
SCCRTN SccWillCreateSccFile (LPVOID pContext, 
							 LONG nFiles, 
							 LPCSTR* lpFileNames,
							 LPBOOL pbSccFiles)
{
	SccTrace trace ("SccWillCreateSccFile", nFiles, lpFileNames);
	for (long i = 0; i < nFiles; ++i)
		pbSccFiles [i] = FALSE;	// Code Co-op never creates MSSCCPRJ.SCC file
	return SCC_OK;
}

//
// Code Co-op SccDll extensions
//

static char const * fakeFilePath = "Fake File Path";

// Returns Code Co-op major version.
// Used to detect version mismatch between Code Co-op and command line utilities.
long SccGetCoopVersion (void)
{
	SccTrace trace ("SccGetCoopVersion");
	dbg << "    Version: " << TheCurrentMajorVersion << std::endl;
	return TheCurrentMajorVersion;
}

// Return next version id used by Code Co-op during check-in or
// the current project version
SCCRTN SccVersionIdByPath (void * context, char const ** lpFileNames, unsigned long options, GlobalId * pId)
{
	SccTrace trace ("SccVersionIdByPath");

	IDEContext * ideContext = reinterpret_cast<IDEContext *>(context);
	try
	{
		FileListClassifier classifier (1, lpFileNames, 0);
		FileListClassifier::Iterator iter = classifier.begin ();
		ProjectVisit projectVisit (TheServerGate);
		ClientConversation conversation;
		CodeCoop::SccOptions queryOptions (options);
		ClientConversation::QueryType type;
		type = queryOptions.IsCurrent () ? ClientConversation::CurrentProjectVersion :
										   ClientConversation::NextProjectVersion;
		if (!conversation.Query (*iter, type))
		{
			std::string info ("Cannot query project version.\n");
			char const * errInfo = conversation.GetErrorMsg ();
			if (errInfo != 0)
				info += errInfo;
			Display (info, ideContext);
			return SCC_E_NONSPECIFICERROR;
		}
		StatusSequencer statusSeq (conversation);
		// Wiesiek: What kind of hack is this?
		*pId = statusSeq.GetState ();
	}
	catch (Win::Exception e)
	{
		dbg << "    FAILURE -- " << e.GetMessage () << std::endl;
		Display ("Cannot query project version.", e, ideContext);
		return SCC_E_NONSPECIFICERROR;
	}
	catch ( ... )
	{
		Win::ClearError ();
		dbg << "    FAILURE -- unknown exception" << std::endl;
		Display ("Cannot query project version.", ideContext);
		// TestingOnly TheLog.Write ("<-- SccVersionIdByPath: Unknown exception.");
		return SCC_E_NONSPECIFICERROR;
	}
	return SCC_OK;
}

SCCRTN SccVersionIdByProj (void * context, int projId, unsigned long options, GlobalId * pId)
{
	SccTrace ("SccVersionIdByProj");

	char const ** paths = &fakeFilePath;

	IDEContext * ideContext = reinterpret_cast<IDEContext *>(context);
	try
	{
		FileListClassifier classifier (projId, paths);
		if (classifier.IsEmpty ())
		{
			std::string info ("Cannot find project ");
			info += ToString (projId);
			Display (info.c_str (), ideContext);
			return SCC_E_NONSPECIFICERROR;
		}

		FileListClassifier::Iterator iter = classifier.begin ();
		ProjectVisit projectVisit (TheServerGate);
		ClientConversation conversation;
		CodeCoop::SccOptions queryOptions (options);
		ClientConversation::QueryType type;
		type = queryOptions.IsCurrent () ? ClientConversation::CurrentProjectVersion :
										   ClientConversation::NextProjectVersion;
		if (!conversation.Query (*iter, type))
		{
			std::string info ("Cannot query project version.\n");
			char const * errInfo = conversation.GetErrorMsg ();
			if (errInfo != 0)
				info += errInfo;
			Display (info, ideContext);
			return SCC_E_NONSPECIFICERROR;
		}
		StatusSequencer statusSeq (conversation);
		// Wiesiek: What kind of hack is this?
		*pId = statusSeq.GetState ();
	}
	catch (Win::Exception e)
	{
		Display ("Cannot query project version.", e, ideContext);
		dbg << "    FAILURE -- " << e.GetMessage () << std::endl;
		return SCC_E_NONSPECIFICERROR;
	}
	catch ( ... )
	{
		Win::ClearError ();
		Display ("Cannot query project version.", ideContext);
		dbg << "    FAILURE -- unknown exception" << std::endl;
		return SCC_E_NONSPECIFICERROR;
	}
	return SCC_OK;
}

//
// Return project version description
//
// Buffer for retrieved report -- we have to store report here, because
// in DLL we cannot allocate memory on the client behalf
//
static std::string TheStringBuf;

std::string const & SccGetStringBuf ()
{
	return TheStringBuf;
}

static GidList TheGidListBuf;

GidList const & SccGetGidListBuf ()
{
	return TheGidListBuf;
}

SCCRTN SccReportByPath (void * context, char const ** lpFileNames, GlobalId versionGid)
{
	SccTrace trace ("SccReportByPath");
	IDEContext * ideContext = reinterpret_cast<IDEContext *>(context);
	try
	{
		FileListClassifier classifier (1, lpFileNames, 0);
		FileListClassifier::Iterator iter = classifier.begin ();
		ProjectVisit projectVisit (TheServerGate);
		ClientConversation conversation;
		if (!conversation.Report (*iter, versionGid, TheStringBuf))
		{
			std::string info ("Cannot get project report.\n");
			char const * errInfo = conversation.GetErrorMsg ();
			if (errInfo != 0)
				info += errInfo;
			Display (info, ideContext);
			return SCC_E_NONSPECIFICERROR;
		}
	}
	catch (Win::Exception e)
	{
		Display ("Cannot get project report.", e, ideContext);
		dbg << "    FAILURE -- " << e.GetMessage () << std::endl;
		return SCC_E_NONSPECIFICERROR;
	}
	catch ( ... )
	{
		Win::ClearError ();
		Display ("Cannot get project report.", ideContext);
		dbg << "    FAILURE -- unknown exception" << std::endl;
		return SCC_E_NONSPECIFICERROR;
	}
	return SCC_OK;
}

SCCRTN SccReportByProj (void * context, int projId, GlobalId versionGid)
{
	SccTrace trace ("SccReportByProj");

	IDEContext * ideContext = reinterpret_cast<IDEContext *>(context);
	char const ** paths = &fakeFilePath;
	try
	{
		FileListClassifier classifier (projId, paths);
		if (classifier.IsEmpty ())
		{
			std::string info ("Cannot find project ");
			info += ToString (projId);
			Display (info.c_str (), ideContext);
			return SCC_E_NONSPECIFICERROR;
		}

		FileListClassifier::Iterator iter = classifier.begin ();
		ProjectVisit projectVisit (TheServerGate);
		ClientConversation conversation;
		if (!conversation.Report (*iter, versionGid, TheStringBuf))
		{
			std::string info ("Cannot get project report.\n");
			char const * errInfo = conversation.GetErrorMsg ();
			if (errInfo != 0)
				info += errInfo;
			Display (info, ideContext);
			return SCC_E_NONSPECIFICERROR;
		}
	}
	catch (Win::Exception e)
	{
		Display ("Cannot get project report.", e, ideContext);
		dbg << "    FAILURE -- " << e.GetMessage () << std::endl;
		return SCC_E_NONSPECIFICERROR;
	}
	catch ( ... )
	{
		Win::ClearError ();
		Display ("Cannot get project report.", ideContext);
		dbg << "    FAILURE -- unknown exception" << std::endl;
		return SCC_E_NONSPECIFICERROR;
	}
	return SCC_OK;
}

//
// Execute Code Co-op command
//

SCCRTN SccCoopCmdByPath (void * context, char const ** lpFileNames, char const * cmd)
{
	SccTrace trace ("SccCoopCmdByPath");

	IDEContext * ideContext = reinterpret_cast<IDEContext *>(context);
	try
	{
		FileListClassifier classifier (1, lpFileNames, 0);
		FileListClassifier::Iterator iter = classifier.begin ();
		if (iter != classifier.end ())
		{
			ProjectVisit projectVisit (TheServerGate);
			ClientConversation conversation;
			conversation.SetStayInProject (false);
			if (!conversation.ExecuteCommand (*iter, cmd))
			{
				std::string info ("Cannot execute Code Co-op command.\n");
				char const * coopErrMsg = conversation.GetErrorMsg ();
				if (coopErrMsg != 0)
					info += coopErrMsg;
				Display (info.c_str (), ideContext);
				return SCC_E_NONSPECIFICERROR;
			}
		}
	}
	catch (Win::Exception e)
	{
		Display ("Cannot execute Code Co-op command", e, ideContext);
		dbg << "    FAILURE -- " << e.GetMessage () << std::endl;
		return SCC_E_NONSPECIFICERROR;
	}
	catch ( ... )
	{
		Win::ClearError ();
		Display ("Cannot execute Code Co-op command", ideContext);
		dbg << "    FAILURE -- unknown exception" << std::endl;
		return SCC_E_NONSPECIFICERROR;
	}
	return SCC_OK;
}

SCCRTN SccCoopCmdByProj (void * context, int projId, char const * cmd, bool skipGuiInProject, bool noTimeout)
{
	SccTrace trace ("SccCoopCmdByProj");
	Assert (projId > 0);
	IDEContext * ideContext = reinterpret_cast<IDEContext *>(context);
	try
	{
		// Visit the project determined by project id
		FileListClassifier classifier (projId, 0);
		if (classifier.IsEmpty ())
		{
			std::string info ("Cannot find project ");
			info += ToString (projId);
			Display (info.c_str (), ideContext);
			return SCC_E_NONSPECIFICERROR;
		}

		ProjectVisit projectVisit (TheServerGate);
		ClientConversation conversation;
		conversation.SetStayInProject (false);
		ClientConversation::CmdOptions cmdOptions;
		cmdOptions.SetSkipGUICoopInProject (skipGuiInProject);
		cmdOptions.SetNoCommandTimeout (noTimeout);
		if (!conversation.ExecuteCommand (*classifier.begin (), cmd, cmdOptions))
		{
			std::string info ("Cannot execute Code Co-op command.\n");
			char const * coopErrMsg = conversation.GetErrorMsg ();
			if (coopErrMsg != 0)
				info += coopErrMsg;
			Display (info.c_str (), ideContext);
			return SCC_E_NONSPECIFICERROR;
		}
	}
	catch (Win::Exception e)
	{
		Display ("Cannot execute Code Co-op command", e, ideContext);
		dbg << "    FAILURE -- " << e.GetMessage () << std::endl;
		return SCC_E_NONSPECIFICERROR;
	}
	catch ( ... )
	{
		Win::ClearError ();
		Display ("Unknown exception during Code Co-op command execution.", ideContext);
		dbg << "    FAILURE -- unknown exception" << std::endl;
		return SCC_E_NONSPECIFICERROR;
	}
	return SCC_OK;
}

SCCRTN SccCoopCmd (void * context, char const * cmd)
{
	SccTrace trace ("SccCoopCmd");
	IDEContext * ideContext = reinterpret_cast<IDEContext *>(context);
	try
	{
		ProjectVisit projectVisit (TheServerGate);
		ClientConversation conversation;
		conversation.SetStayInProject (false);
		ClientConversation::CmdOptions cmdOptions;
		cmdOptions.SetNoCommandTimeout (true);
		if (!conversation.ExecuteCommand (cmd, cmdOptions))
		{
			std::string info ("Cannot execute Code Co-op command.\n");
			char const * coopErrMsg = conversation.GetErrorMsg ();
			if (coopErrMsg != 0)
				info += coopErrMsg;
			Display (info.c_str (), ideContext);
			return SCC_E_NONSPECIFICERROR;
		}
	}
	catch (Win::Exception e)
	{
		Display ("Cannot execute Code Co-op command", e, ideContext);
		dbg << "    FAILURE -- " << e.GetMessage () << std::endl;
		return SCC_E_NONSPECIFICERROR;
	}
	catch ( ... )
	{
		Win::ClearError ();
		Display ("Unknown exception during Code Co-op command execution.", ideContext);
		dbg << "    FAILURE -- unknown exception" << std::endl;
		return SCC_E_NONSPECIFICERROR;
	}
	return SCC_OK;
}

// Query file state
SCCRTN SccGetFileState (void * context, 
                        char const * filePath,
						unsigned long & state
                        )
{
	SccTrace trace ("GetFileState");

	IDEContext * ideContext = reinterpret_cast<IDEContext *>(context);
	try
	{
		char const * lpFileNames [1];
		lpFileNames [0] = filePath;
		FileListClassifier classifier (1, lpFileNames, 0);
		FileListClassifier::Iterator iter = classifier.begin ();
		ProjectVisit projectVisit (TheServerGate);
		ClientConversation conversation;
		if (!conversation.Query (*iter))	// File state request
		{
			std::string info ("Cannot get file state: ");
			info += filePath;
			char const * errInfo = conversation.GetErrorMsg ();
			if (errInfo != 0)
			{
				info += "\n";
				info += errInfo;
			}
			Display (info, ideContext);
			return SCC_E_NONSPECIFICERROR;
		}
		StatusSequencer statusSeq (conversation);
		state = statusSeq.GetState ();
	}
	catch (Win::Exception e)
	{
		Display ("Cannot get file state.", e, ideContext);
		dbg << "    FAILURE -- " << e.GetMessage () << std::endl;
		return SCC_E_NONSPECIFICERROR;
	}
	catch ( ... )
	{
		Win::ClearError ();
		Display ("Cannot get file state.", ideContext);
		dbg << "    FAILURE -- unknown exception" << std::endl;
		return SCC_E_NONSPECIFICERROR;
	}
	return SCC_OK;
}

// Project merge support
SCCRTN SccGetForkScriptIds (void * context,
							int projId,
							bool deepForks,
							GidList const & myForkIds,
							GlobalId & youngestFoundScriptId)
{
	SccTrace trace ("SccGetForkScriptIds");

	IDEContext * ideContext = reinterpret_cast<IDEContext *>(context);
	try
	{
		FileListClassifier classifier (projId, 0);
		if (classifier.IsEmpty ())
		{
			std::string info ("Cannot find project ");
			info += ToString (projId);
			Display (info.c_str (), ideContext);
			return SCC_E_NONSPECIFICERROR;
		}

		ProjectVisit projectVisit (TheServerGate);
		ClientConversation conversation;
		conversation.SetStayInProject (false);
		if (!conversation.QueryForkIds (*classifier.begin (),
										deepForks,
										myForkIds,
										youngestFoundScriptId,
										TheGidListBuf))
		{
			std::string info ("Cannot get fork ids for the project ");
			info += ToString (projId);
			char const * errInfo = conversation.GetErrorMsg ();
			if (errInfo != 0)
			{
				info += "\n";
				info += errInfo;
			}
			Display (info, ideContext);
			return SCC_E_NONSPECIFICERROR;
		}
	}
	catch (Win::Exception e)
	{
		Display ("Cannot get fork ids.", e, ideContext);
		dbg << "    FAILURE -- " << e.GetMessage () << std::endl;
		return SCC_E_NONSPECIFICERROR;
	}
	catch ( ... )
	{
		Win::ClearError ();
		Display ("Cannot get fork ids.", ideContext);
		dbg << "    FAILURE -- unknown exception" << std::endl;
		return SCC_E_NONSPECIFICERROR;
	}
	return SCC_OK;
}

SCCRTN SccGetTargetPath (void * context,
						 int projId,
						 GlobalId gid,
						 std::string const & sourceProjectPath,
						 unsigned long & targetType,
						 unsigned long & statusAtTarget)
{
	SccTrace trace ("SccGetTargetPath");

	IDEContext * ideContext = reinterpret_cast<IDEContext *>(context);
	try
	{
		FileListClassifier classifier (projId, 0);
		if (classifier.IsEmpty ())
		{
			std::string info ("Cannot find project ");
			info += ToString (projId);
			Display (info.c_str (), ideContext);
			return SCC_E_NONSPECIFICERROR;
		}

		ProjectVisit projectVisit (TheServerGate);
		ClientConversation conversation;
		conversation.SetStayInProject (true);
		if (!conversation.QueryTargetPath (*classifier.begin (),
										   gid,
										   sourceProjectPath,
										   TheStringBuf,
										   targetType,
										   statusAtTarget))
		{
			std::string info ("Cannot get target paths for the project ");
			info += ToString (projId);
			char const * errInfo = conversation.GetErrorMsg ();
			if (errInfo != 0)
			{
				info += "\n";
				info += errInfo;
			}
			Display (info, ideContext);
			return SCC_E_NONSPECIFICERROR;
		}
	}
	catch (Win::Exception e)
	{
		Display ("Cannot get target paths.", e, ideContext);
		dbg << "    FAILURE -- " << e.GetMessage () << std::endl;
		return SCC_E_NONSPECIFICERROR;
	}
	catch ( ... )
	{
		Win::ClearError ();
		Display ("Cannot get target paths.", ideContext);
		dbg << "    FAILURE -- unknown exception" << std::endl;
		return SCC_E_NONSPECIFICERROR;
	}
	return SCC_OK;
}

void SccHold ()
{
	TheGlobalLock.Set (1);
	dbg << "SccDll LOCKED" << std::endl;	
}

void SccResume ()
{
	TheGlobalLock.Reset ();
	dbg << "SccDll UNLOCKED" << std::endl;
}
