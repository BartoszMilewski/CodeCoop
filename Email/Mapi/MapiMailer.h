#if !defined (MAPIMAILER_H)
#define MAPIMAILER_H
//-------------------------------------
// (c) Reliable Software 2001 -- 2004
// ------------------------------------

#include "EmailTransport.h"
#include "MapiDefDir.h"
#include "MapiAddrBook.h"
#include "MapiStore.h"

namespace Mapi
{
	class Mailer : public MailerInterface
	{
	public:
		Mailer (Session & session)
			: _session (session),
			  _defaultDir (session),
			  _addressBook (_defaultDir),
			  _outbox (_defaultDir.GetMsgStore ())
		{}

		void Send (OutgoingMessage & msg, std::vector<std::string> const & addressVector);
		void Save (OutgoingMessage & msg, std::vector<std::string> const & addressVector);
		void GetLoggedUser (std::string & name, std::string & emailAddr);
		bool VerifyEmailAddress (std::string & emailAddr);
	private:
		Session	&			_session;
		DefaultDirectory	_defaultDir;
		AddressBook			_addressBook;
		Outbox				_outbox;
	};
}

#endif
