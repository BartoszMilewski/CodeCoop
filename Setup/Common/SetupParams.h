#if !defined (SETUPPARAMS_H)
#define SETUPPARAMS_H
//------------------------------------
//  (c) Reliable Software, 1997 - 2008
//------------------------------------

#include "GlobalFileNames.h"

char const CompanyName []		= "Reliable Software";
char const CoopLink []			= "Code Co-op.lnk";
char const DispatcherLink []	= "Dispatcher.lnk";
char const HlpLink []			= "Code Co-op Help.lnk";
char const UninstallLink []		= "Uninstall.lnk";
char const SccServerName []		= "Reliable Software Code Co-op";
char const SccProvider []		= "Software\\Reliable Software\\Code Co-op";
char const ZncClass []			= "Code Co-op Compression";
char const SncClass []			= "Code Co-op Script";
char const CnkClass []			= "Code Co-op Script Chunk";
char const WikiClass []			= "Code Co-op Wiki";
char const BackupFolder	[]		= "Original";

// used by Installer
char const PermanentUpdateMarker [] = "Permanent.cfg";
char const CmdLineToolsMarker []    = "CmdLineTools.cfg";
char const SetupExeName []          = "CoopSetup.exe";
char const AutoUpdateInfoFile []    = "CoopVersion.xml";
char const * const AutoUpdateFiles [] = 
{
	AutoUpdateInfoFile,
	0
};

char const * const ExeFiles [] =
{
	CoopExeName,
	DifferExeName,
	DispatcherExeName,
#if defined (COOP_PRO)
	CabArcExe,
	FtpAppExe,
#endif
	HelpModuleName,
	UninstallExeName,
	SccDll,
	"SCC.reg",
	"unznc.exe",
	"diagnostics.exe",
	"defectfromall.exe",
	"znc.ico",
	"ReleaseNotes.txt",
	0
};

char const * const WikiFolders [] =
{
	"Image", "ToDo", 0
};

char const * const WikiFiles [] =
{
	CssFileName,
	WikiTemplate,
	"AddRecord.wiki",
	"Forms Help.wiki",
	"Help.wiki",
	"index.wiki",
	"SQWiki Help.wiki",
	"FAQ.wiki",
	"ToDoList.wiki",
	"ErrExists.wiki",
	"Image\\sunset.jpg",
	"ToDo\\1.wiki",
	"ToDo\\2.wiki",
	"ToDo\\template.wiki",
	"ToDo\\Trip.wiki",
	0
};

char const * const IdeIntegrators [] = 
{
	SccDll,
	0
};

char const CoopVarsBatFile [] = "CoopVars.bat";
char const ConfigurationProblems [] = "ConfigurationProblems.txt";

char const * const CmdLineToolsFiles [] =
{
	"allcoopcmd.exe",
	"addfile.exe",
	"checkin.exe",
	"checkout.exe",
	"coopcmd.exe",
	"deliver.exe",
	"export.exe",
	"projectstatus.exe",
	"removefile.exe",
	"report.exe",
	"restore.exe",
	"startcoopbackup.exe",
	"status.exe",
	"synch.exe",
	"uncheckout.exe",
	"unznc.exe",
	"versionid.exe",
	"versionlabel.exe",
	"visitproject.exe",
	"ReadMe.txt",
	CoopVarsBatFile,
	0
};

char const * const BeyondCompareFiles [] = 
{
	BcMergerExe,
	"BCMerge.chm",
	0
};

// 5.0 beta 2
char const * const OldProjectFiles [] =
{
	"BcMerger.exe",
	"BCDiffer.exe",
	"BCDiffer.hlp",
	"BC2.chm",
	"BCMerger.hlp",
	0
};

#endif
