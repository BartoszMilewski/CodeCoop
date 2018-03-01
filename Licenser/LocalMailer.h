#if !defined (LOCALMAILER_H)
#define LOCALMAILER_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "EmailSession.h"

class MailerInterface;
class OutgoingMessage;
namespace Email { class Acount; }

class LocalMailer
{
public:
	LocalMailer ();
	~LocalMailer ();
	void Send (OutgoingMessage & msg, std::string const & address);
	void Save (OutgoingMessage & msg, std::string const & address);
private:
	Email::Session	_session;
	std::unique_ptr<Email::Account>	_outgoingAccount;
	std::unique_ptr<MailerInterface>	_pImpl;
};

#endif
