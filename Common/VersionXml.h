#if !defined (VERSIONXML_H)
#define VERSIONXML_H
// ----------------------------------
// (c) Reliable Software, 2004 - 2007
// ----------------------------------

#include "CoopVersion.h"

#include <XML/XmlTree.h>

class VersionXml
{
public:
	VersionXml (char const * versionInfoFilePath,
				int lastBulletin);
	bool IsVersionInfo () const  { return _versionNode  != 0; }
	bool IsBulletinInfo () const { return _bulletinNode != 0; }

	bool IsNewerExe () const;
	bool IsNewerBulletin () const;
	std::string const & GetVersionNumber () const;
	std::string const & GetUpdateExeName () const;
	std::string GetVersionHeadline () const;
	std::string GetReleaseNotesLink () const;
	int GetBulletinNumber () const;
	std::string const & GetBulletinLink () const;
	std::string GetBulletinHeadline () const;

private:
	// information about the installed version
	CoopVersion		   _installedVersion;
	int const		   _lastBulletin;

	// information about the latest available version found in the update information file
	XML::Tree		   _tree;
	// version info
	XML::Node const *  _versionNode;	
	std::string		   _versionNumber;
	std::string		   _exeName;
	// bulletin info
	XML::Node const *  _bulletinNode;   
	int				   _bulletinNumber;
	std::string		   _bulletinLink;
};

#endif
