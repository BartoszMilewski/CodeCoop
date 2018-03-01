#include "precompiled.h"
#include "CreateSetup.h"
#include "PrepareFiles.h"
#include <Catalog.h>
#include <Ctrl/Output.h>
#include <Net/Ftp.h>
#include <iostream>

int main()
{
	FilePath coopRoot;
	FilePath webRoot;
	// Get project paths from catalog
	Catalog catalog;
	ProjectSeq seq(catalog);
	while (!seq.AtEnd())
	{
		std::string const & name = seq.GetProjectName();
		if (name == coopProjName)
		{
			if (!coopRoot.IsDirStrEmpty())
			{
				std::cerr << "There is more than one enlistment in " << coopProjName << std::endl;
				return -1;
			}
			coopRoot = seq.GetProjectSourcePath();
		}
		else if (name == webProjName)
		{
			if (!webRoot.IsDirStrEmpty())
			{
				std::cerr << "There is more than one enlistment in " << webProjName << std::endl;
				return -1;
			}
			webRoot = seq.GetProjectSourcePath();
		}
		seq.Advance();
	}

	if (coopRoot.IsDirStrEmpty())
	{
		std::cerr << "There is no project " << coopProjName << std::endl;
		return -1;
	}
	if (webRoot.IsDirStrEmpty())
	{
		std::cerr << "There is no project " << webProjName << std::endl;
		return -1;
	}

	try
	{
		CmdFileCopier cmdCopier(coopRoot);
		cmdCopier.CopyFiles();
		cmdCopier.ZipFiles();

		ProFileCopier proCopier(coopRoot);
		proCopier.CopyFiles();
		proCopier.ZipFiles();

		LiteFileCopier liteCopier(coopRoot);
		liteCopier.CopyFiles();
		liteCopier.ZipFiles();
		
		Ftp::Login login("ftp.relisoft.com", "relisoft", "UkN3u1gW");
		Uploader uploader(coopRoot, webRoot, login);
		uploader.PrepareFiles();
		uploader.Upload();
	}
	catch (Win::Exception e)
	{
		std::cerr << Out::Sink::FormatExceptionMsg(e) << std::endl;
		return -1;
	}
	catch (...)
	{
		std::cerr << "Unknown error in Release" << std::endl;
		return -1;
	}
	return 0;
}

