#if !defined (SIMPLEMAPIMAILER_H)
#define SIMPLEMAPIMAILER_H
//-------------------------------------
// (c) Reliable Software 2001 -- 2004
// ------------------------------------

#include "EmailTransport.h"
#include "SimpleSession.h"
#include "Addrbook.h"
#include "SimpleStore.h"

class OutgoingMessage;

namespace SimpleMapi
{
	class Mailer : public MailerInterface
	{
	public:
		Mailer (Session & session);

		void Send (OutgoingMessage & msg, std::vector<std::string> const & addrVector);
		void Save (OutgoingMessage & msg, std::vector<std::string> const & addrVector);
		void GetLoggedUser (std::string & name, std::string & emailAddr);
		bool VerifyEmailAddress (std::string & emailAddr);
	private:
		Session	&	_session;
		AddressBook	_addressBook;
		Outbox		_outbox;
	};
}

#endif
