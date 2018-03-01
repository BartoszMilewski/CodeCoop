#if !defined (TESTGLOBAL_H)
#define TESTGLOBAL_H
// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------

// global functions declarations

bool GetSMTPInfoFromUser (WinOut & out,
						  std::string & server, 
						  std::string & user, 
						  std::string & password, 
						  std::string & senderAddress,
						  unsigned long & options);
bool GetPOP3InfoFromUser (WinOut & out,
						  std::string & server, 
						  std::string & user, 
						  std::string & password, 
						  unsigned long & options);

bool GetAccountInfoFromUser (WinOut & out, 
							 std::string & senderAddress, 
							 std::string & pop3Server, 
							 std::string & pop3User,
							 std::string & pop3Pass,
							 std::string & smtpServer, 
							 std::string & smtpUser, 
							 std::string & smtpPass, 
							 unsigned long & options);

void SendTestMessage (WinOut & out,
					  std::vector<std::string> const & addrVector,
					  std::string const & server, 
					  std::string const & user, 
					  std::string const & password, 
					  std::string const & senderAddress,
					  std::string const & subject,
					  std::string const & text,
					  std::string const & att);

void RunConfigTool (WinOut & out);


#endif
