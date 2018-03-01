#if !defined (FTPPROXY_H)
#define FTPPROXY_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

namespace Ftp
{
	class Login;
}

void ExecuteFtpUpload (std::string const & sourcePath,
					   std::string const & targetPath,
					   Ftp::Login const & login);

#endif
