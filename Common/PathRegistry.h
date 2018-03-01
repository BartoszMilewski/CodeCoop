#if !defined (PATHREGISTRY_H)
#define PATHREGISTRY_H
//----------------------------------
// (c) Reliable Software 2000 - 2007
//----------------------------------

namespace Registry
{
	bool IsUserSetup ();
	std::string GetProgramPath ();
	std::string GetCatalogPath ();
	std::string GetDatabasePath ();
	std::string	GetLogsFolder ();
	std::string GetCmdLineToolsPath ();
};

#endif
