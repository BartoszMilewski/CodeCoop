//------------------------------------
//  (c) Reliable Software, 2007 - 2008
//------------------------------------

#include <WinLibBase.h>
#include "FtpUploader.h"
#include "Ftp.h"

::File::Size Ftp::Uploader::GetUploadedFilesize () const
{
	if (_targetFile.get () == 0)
		return ::File::Size (0, 0);

	return _targetFile->GetSize ();
}

void Ftp::Uploader::StartPutFile (std::string const & sourcePath,
								  std::string const & targetPath)
{
	_sourceFile.reset (new LocalFile (sourcePath));
	_targetFile.reset (new Ftp::File (_ftpSessionHandle, targetPath));
	StartCopy (*_sourceFile, *_targetFile);
}
