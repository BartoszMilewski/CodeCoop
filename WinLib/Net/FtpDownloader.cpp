// ----------------------------------
// (c) Reliable Software, 2003 - 2007
// ----------------------------------

#include <WinLibBase.h>
#include "FtpDownloader.h"
#include "Ftp.h"
#include <File/FileIo.h>
#include <Sys/GlobalUniqueName.h>

namespace Ftp
{
	void Downloader::StartGetFile (std::string const & sourcePath,
				                   std::string const & targetPath)
	{
		_srcFile.reset (new Ftp::File (_ftpSessionHandle, sourcePath));
		_destFile.reset (new LocalFile (targetPath));
		StartCopy (*_srcFile, *_destFile);
	}
}
