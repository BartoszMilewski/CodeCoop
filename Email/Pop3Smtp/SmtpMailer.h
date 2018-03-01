#if !defined (SMTPMAILER_H)
#define SMTPMAILER_H
//------------------------------
// (c) Reliable Software 2005-06
// -----------------------------
#include "EmailTransport.h"

namespace Smtp { class Session; }

class SmtpMailer : public MailerInterface
{
public:
	SmtpMailer (Smtp::Session & session, 
				std::string const & user,
				std::string const & senderName,
				std::string const & senderAddress);
	// MailerIterface 
	void Send (OutgoingMessage & msg, std::vector<std::string> const & addrVector);
	void Save (OutgoingMessage & msg, std::vector<std::string> const & addrVector);
	void GetLoggedUser (std::string & name, std::string & emailAddr);
	bool VerifyEmailAddress (std::string & emailAddr);

private:
	Smtp::Session		& _session;
	std::string const	& _user;
	std::string const	& _senderName;
	std::string const	& _senderAddress;
	bool				  _isLogging;
};

#endif
