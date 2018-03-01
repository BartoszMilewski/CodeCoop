#if !defined (PREPAREFILES_H)
#define PREPAREFILES_H
//---------------------------
// (c) Reliable Software 2010
//---------------------------
#include "CreateSetup.h"
#include <File/Path.h>
namespace Ftp { class Login; }

class Uploader
{
public:
	Uploader(FilePath & coopRoot, FilePath & webRoot, Ftp::Login const & login);
	void PrepareFiles();
	void Upload();
private:
	void UploadFile(std::string const & sourcePath, std::string const & targetPath);
private:
	FilePath _coopRoot;
	FilePath _webRoot;
	FilePath _setupPath;
	FilePath _uploadPath;
	Ftp::Login const & _login;
	std::string _ftpAppPath;
};

#endif
