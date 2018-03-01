#if !defined (SMTPSESSION_H)
#define SMTPSESSION_H
// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------

#include "EmailTransport.h"
#include "EmailAccount.h"
#include <Mail/Smtp.h>
#include <Net/Socket.h>

namespace Email
{
	class SmtpSession : public Email::SessionInterface
	{
	public:
		SmtpSession (SmtpAccount const & account, 
					 std::string const & domain)
			: _connection (account.GetServer (),
						   account.GetPort (),
						   account.GetTimeout ()),
			  _session (_connection, domain, account.UseSSL ()),
			  _user (account.GetUser ()),
			  _senderName (account.GetSenderName ()),
			  _senderAddress (account.GetSenderAddress ())
		{
			if (account.IsAuthenticate ())
				_session.Authenticate (account.GetUser (), account.GetPassword ());
		}

		Smtp::Session & GetSession () { return _session; }
		std::string const & GetUser () const { return _user; }
		std::string const & GetSenderName () const { return _senderName; }
		std::string const & GetSenderAddress () const { return _senderAddress; }

	private:
		WinSocks			_useSocks;
		Smtp::Connection	_connection;
		Smtp::Session		_session;
		std::string const	_user;
		std::string	const  	_senderName;
		std::string	const  	_senderAddress;
	};
}

#endif
