// ----------------------------------
// (c) Reliable Software, 2004 - 2007
// ----------------------------------

#include "precompiled.h"

#include "VersionXml.h"
#include "BuildOptions.h"

#include <File/MemFile.h>
#include <Ex/WinEx.h>
#include <XML/Scanner.h>
#include <XML/XmlParser.h>

#include <StringOp.h>
#include <sstream>

char const RootName					[] = "CodeCoopUpdateInfo";
char const MajorVersionSectionName  [] = "MajorVersion";
char const LatestVersionNodeName    [] = "LatestVersion";
char const LatestBetaNodeName       [] = "LatestBeta";
char const LatestLiteNodeName       [] = "LatestLite";
char const LatestWikiNodeName       [] = "LatestWiki";
char const LatestLiteBetaNodeName   [] = "LatestLiteBeta";
char const LatestWikiBetaNodeName   [] = "LatestWikiBeta";
char const DescriptionNodeName      [] = "Description";
char const BulletinNodeName         [] = "Bulletin";
char const SetupNodeName            [] = "Setup";
char const TextNodeName				[] = "";
char const NumberAttributeName      [] = "Number";
char const LinkAttributeName		[] = "href";
char const TextAttributeName		[] = "Text";

//
// Structure of version info file:
//
// <CodeCoopUpdateInfo>
//	 <MajorVersion Number="3">
// 		<LatestVersion Number="3.5d">
// 			<Description href="http://www.relisoft.com/co_op/ReleaseNotes3x.txt">
// 				 The most up-to-date version of 3.x series
// 			</Description>
// 			<Setup 
// 				href="ftp://ftp.relisoft.com/co-op35.exe" 
// 				Size="254321"
// 			/>
// 		</LatestVersion>
// 		<Bulletin Number="1" href="http://www.relisoft.com/co_op/Bulletin3x.html">
// 	 		  You must read it!
// 		</Bulletin>
//	</MajorVersion>
//
//	<MajorVersion>
//		...
//	</MajorVersion>
//  
// </CodeCoopUpdateInfo>
//
// Note: XML does not need to contain any data concerning a given version

VersionXml::VersionXml (char const * versionInfoFilePath,
						int lastBulletin)
	: _versionNode (0),
	  _bulletinNode (0),
	  _bulletinNumber (0),
	  _lastBulletin (lastBulletin),
	  _installedVersion (COOP_PRODUCT_VERSION)
{
	std::istringstream infoStream;
	{
		MemFileReadOnly	infoFile (versionInfoFilePath);
		if (infoFile.GetBufSize () == 0)
			return;

		infoStream.str (infoFile.GetBuf ());
	}
	XML::Scanner scanner (infoStream);
	XML::TreeMaker treeMaker (_tree);
	XML::Parser parser (scanner, treeMaker);
	parser.Parse ();

	XML::Node const * root = _tree.GetRoot ();
	if (root == 0)
		return;

	// find an info tree for the installed version
	XML::Node const * majorVersionNode = 0;
	for (XML::Node::ConstChildIter it = root->FirstChild ();
		 it != root->LastChild ();
		 it++)
	{
		if ((*it)->GetName () == MajorVersionSectionName)
		{
				std::string versionNumber = (*it)->FindAttribValue (NumberAttributeName);
				if (_installedVersion.IsMajorEqual (versionNumber))
				{
					majorVersionNode = *it;
					break;
				}
		}
	}
	if (majorVersionNode == 0)
		return;

	// select between Beta and Release depending on which of the two is installed
	XML::Node  const * versionTmpNode = 0;
	if (_installedVersion.IsPro ())
	{
		if (_installedVersion.IsBeta ())
			versionTmpNode = majorVersionNode->FindFirstChildNamed (LatestBetaNodeName);
		else
			versionTmpNode = majorVersionNode->FindFirstChildNamed (LatestVersionNodeName);
	}
	else if (_installedVersion.IsLite ())
	{
		if (_installedVersion.IsBeta ())
			versionTmpNode = majorVersionNode->FindFirstChildNamed (LatestLiteBetaNodeName);
		else
			versionTmpNode = majorVersionNode->FindFirstChildNamed (LatestLiteNodeName);
	}
	else if (_installedVersion.IsWiki ())
	{
		if (_installedVersion.IsBeta ())
			versionTmpNode = majorVersionNode->FindFirstChildNamed (LatestWikiBetaNodeName);
		else
			versionTmpNode = majorVersionNode->FindFirstChildNamed (LatestWikiNodeName);
	}

	if (versionTmpNode != 0)
	{
		// analyze contents of the version info tree and
		// check whether it contains minimum required data:
		//    - a version number,
		//	  - an href to a new exe
		_versionNumber = versionTmpNode->FindAttribValue (NumberAttributeName);
		if (!_versionNumber.empty ())
		{
			_exeName = versionTmpNode->FindChildAttribValue (SetupNodeName, LinkAttributeName);
			if (!_exeName.empty ())
			{
				// minimum info available -- accept versionTmpNode as valid
				_versionNode = versionTmpNode;
			}
		}
	}

	// find a bulletin tree
	XML::Node const * bulletinTmpNode = majorVersionNode->FindFirstChildNamed (BulletinNodeName);
	if (bulletinTmpNode != 0)
	{
		// analyze contents of the bulletin info tree and
		// check whether it contains minimum required data:
		//    - a bulletin number,
		//	  - an href to a new bulletin
		std::string numberAttrValue = bulletinTmpNode->FindAttribValue (NumberAttributeName);
		_bulletinNumber = ToInt (numberAttrValue);
		if (_bulletinNumber > 0)
		{
			_bulletinLink = bulletinTmpNode->FindAttribValue (LinkAttributeName);
			if (!_bulletinLink.empty ())
			{
				// minimum info available -- accept bulletinTmpNode as valid
				_bulletinNode = bulletinTmpNode;
			}
		}
	}
}

bool VersionXml::IsNewerExe () const
{
	Assert (_versionNode != 0);
	Assert (!_versionNumber.empty ());
	Assert (!_exeName.empty ());

	return _installedVersion.IsEarlier (_versionNumber);
}

bool VersionXml::IsNewerBulletin () const
{
	Assert (_bulletinNode != 0);
	Assert (_bulletinNumber > 0);
	Assert (!_bulletinLink.empty ());

	return _lastBulletin < _bulletinNumber;
}

std::string const & VersionXml::GetVersionNumber () const
{
	Assert (_versionNode != 0);
	Assert (!_versionNumber.empty ());
	Assert (!_exeName.empty ());

	return _versionNumber;
}

std::string const & VersionXml::GetUpdateExeName () const
{
	Assert (_versionNode != 0);
	Assert (!_versionNumber.empty ());
	Assert (!_exeName.empty ());

	return _exeName;
}

std::string VersionXml::GetVersionHeadline () const
{
	Assert (_versionNode != 0);
	Assert (!_versionNumber.empty ());
	Assert (!_exeName.empty ());

	XML::Node const * descriptionNode = _versionNode->FindFirstChildNamed (DescriptionNodeName);
	if (descriptionNode != 0)
	{
		return descriptionNode->FindChildAttribValue (TextNodeName, TextAttributeName);
	}
	return std::string ();
}

std::string VersionXml::GetReleaseNotesLink () const
{
	Assert (_versionNode != 0);
	Assert (!_versionNumber.empty ());
	Assert (!_exeName.empty ());

	return _versionNode->FindChildAttribValue (DescriptionNodeName, LinkAttributeName);
}

int VersionXml::GetBulletinNumber () const
{
	Assert (_bulletinNode != 0);
	Assert (_bulletinNumber > 0);
	Assert (!_bulletinLink.empty ());

	return _bulletinNumber;
}

std::string const & VersionXml::GetBulletinLink () const
{
	Assert (_bulletinNode != 0);
	Assert (_bulletinNumber > 0);
	Assert (!_bulletinLink.empty ());

	return _bulletinLink;
}

std::string VersionXml::GetBulletinHeadline () const
{
	if (_bulletinNode != 0)
	{
		return _bulletinNode->FindChildAttribValue (TextNodeName, TextAttributeName);
	}
	return std::string ();
}
