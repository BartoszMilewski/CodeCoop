#include "precompiled.h"
#include "CreateSetup.h"
#include "BuildOptions.h"
#include "SetupParams.h"
#include <Com/Shell.h>
#include <Com/ShellRequest.h>
#include <File/FileIo.h>
#include <Sys/Process.h>
#include <Ctrl/Output.h>
#include <iostream>

ProductNames TheProductNames;
ProductId ProductNames::prodIds[productCount] = {pro, lite, cmd};

ProductNames::ProductNames()
{
	_productMap[pro]  = "Co-op";
	_productMap[lite] = "Co-opLite";
	_productMap[cmd]  = "CmdLine";
	// Remove the dot from version number
	std::string versionSuffix(COOP_PRODUCT_VERSION);
	unsigned i = versionSuffix.find('.');
	versionSuffix.erase(i, 1);
	_productMapV[pro]  = "Co-op" + versionSuffix;
	_productMapV[lite] = "Co-opLite" + versionSuffix;
	_productMapV[cmd]  = "CmdLine" + versionSuffix;
};

void ProFileCopier::CopyFiles()
{
	CoopFileCopier::CopyFiles();
	_srcPath = _rootPath;
	Copy("CAB file SDK\\bin", "cabarc.exe");
}

void CoopFileCopier::CopyFiles()
{
	Copy("Setup","SCC.reg");
	Copy("Setup","znc.ico");
	Copy("Setup","ReleaseNotes.txt");
	Copy("Setup","CoopVersion.xml");
	Copy("Setup\\Beyond Compare","BCMerge.exe");
	Copy("Setup\\Beyond Compare","BCMerge.chm");
	Copy("Setup\\Help","CodeCoop.chm");
	Copy("CmdLine\\Unznc\\Release","Unznc.exe");
	Copy("CmdLine\\Diagnostics\\Release","Diagnostics.exe");
	Copy("CmdLine\\DefectFromAll\\Release","DefectFromAll.exe");

	_srcPath.DirDown("Setup\\Wiki");

	int i = 0;
	char const * filePath = 0;
	while ((filePath = WikiFiles[i]) != 0)
	{
		PathSplitter splitter(filePath);
		if (splitter.HasOnlyFileName())
		{
			Copy(filePath);
		}
		else
		{
			std::string tgtFile(splitter.GetFileName());
			tgtFile += splitter.GetExtension();
			Copy(tgtFile.c_str());
		}
		++i;
	}
}


void CmdFileCopier::CopyFiles()
{
	// Empty file required by cmd line setup
	File cfgFile(_binrPath.GetFilePath(CmdLineToolsMarker), File::CreateAlwaysMode());

	Copy("CmdLine", "ReadMe.txt");
	Copy("CmdLine", "CoopVars.bat");
	Copy("Setup\\Install\\Release", SetupExeName);
	// Cmd line exes are already copied by post-build compiler commands

	int i = 0;
	char const * fileName = 0;
	while ((fileName = CmdLineToolsFiles[i]) != 0)
	{
		if (!File::Exists(_tgtPath.GetFilePath(fileName)))
			throw Win::Exception("Missing file", _tgtPath.GetFilePath(fileName));
		++i;
	}
};

void FileCopier::ZipFiles()
{
	// Find the zippers
	static char const zipperPath[] = "C:\\Program Files (x86)\\WinZip\\wzzip.exe";
	static char const selfExtractorPath[] = "C:\\Program Files (x86)\\WinZip Self-Extractor\\wzipse32.exe";
	if (!File::Exists(zipperPath))
		throw Win::Exception("You don't have the zipper: ", zipperPath);
	if (!File::Exists(selfExtractorPath))
		throw Win::Exception("You don't have the self extractor: ", selfExtractorPath);

	// zip files into the zip directory
	std::string zipCmdLine(" -a ");
	zipCmdLine += _productName;
	zipCmdLine += ".zip ";
	zipCmdLine += _binrPath.GetAllFilesPath();

	Win::ChildProcess zipper(zipCmdLine);
	zipper.SetAppName(zipperPath);
	zipper.ShowMinimizedNotActive ();
	zipper.SetCurrentFolder(_zipFilePath.GetDir());
	zipper.Create ();
	if (!zipper.WaitForDeath (30 * 1000))
		throw Win::Exception("Zipper timed out");
	// Create about and dialog files
	std::string aboutText = CreateAboutText();

	{
		FileIo about(_zipFilePath.GetFilePath("about.txt"), File::CreateAlwaysMode());
		unsigned long len = aboutText.size();
		about.Write(aboutText.data(), len);

		FileIo dialog(_zipFilePath.GetFilePath("dialog.txt"), File::CreateAlwaysMode());
		len = aboutText.size();
		dialog.Write(aboutText.data(), len);
	}

	// Build self-extracting files
	std::string cmdLineStr(" ");
	cmdLineStr += _productName;
	cmdLineStr += ".zip @..\\";
	cmdLineStr += _productName;
	cmdLineStr += ".rsp";

	Win::ChildProcess selfExtractor(cmdLineStr);
	selfExtractor.SetAppName(selfExtractorPath);
	selfExtractor.ShowMinimizedNotActive ();
	selfExtractor.SetCurrentFolder(_zipFilePath.GetDir());
	selfExtractor.Create ();
	if (!selfExtractor.WaitForDeath (30 * 1000))
		throw Win::Exception("SelfExtractor timed out");
}

void FileCopier::Copy(char const * srcDir, char const * file)
{
	FilePath srcPath(_srcPath);
	srcPath.DirDown(srcDir);
	File::Copy(srcPath.GetFilePath(file), _tgtPath.GetFilePath(file));
}
void FileCopier::Copy(char const * file)
{
	File::Copy(_srcPath.GetFilePath(file), _tgtPath.GetFilePath(file));
}

std::string CmdFileCopier::CreateAboutText()
{
	std::string aboutText = "Reliable Software\nCode Co-op ";
	aboutText += "Command Line Tools v. ";
	aboutText += COOP_PRODUCT_VERSION;
	aboutText += "\nInstallation";
	return aboutText;
}

std::string ProFileCopier::CreateAboutText()
{
	std::string aboutText = "Reliable Software\nCode Co-op ";
	aboutText += " Pro v. ";
	aboutText += COOP_PRODUCT_VERSION;
	aboutText += "\nInstallation";
	return aboutText;
}

std::string LiteFileCopier::CreateAboutText()
{
	std::string aboutText = "Reliable Software\nCode Co-op ";
	aboutText += "Lite v. ";
	aboutText += COOP_PRODUCT_VERSION;
	aboutText += "\nInstallation";
	return aboutText;
}