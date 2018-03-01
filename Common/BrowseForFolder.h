#if !defined (BROWSEFORFOLDER_H)
#define BROWSEFORFOLDER_H
// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------

bool BrowseForLocalFolder (std::string & path,
						   Win::Dow::Handle parentWin,
						   std::string const & hintForTheUser,
						   char const * startupFolder = 0);

bool BrowseForNetworkFolder (std::string & path,
							 Win::Dow::Handle parentWin,
							 std::string const & hintForTheUser,
							 char const * startupFolder = 0);

bool BrowseForAnyFolder (std::string & path,
						 Win::Dow::Handle parentWin,
						 std::string const & hintForTheUser,
						 char const * startupFolder = 0);

bool BrowseForProjectRoot (std::string & path, Win::Dow::Handle owner, bool isCreating = true);
bool BrowseForHubShare (std::string & path, Win::Dow::Handle owner);
bool BrowseForSatelliteShare (std::string & path, Win::Dow::Handle owner);

#endif