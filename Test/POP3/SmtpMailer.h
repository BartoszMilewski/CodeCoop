#if !defined (SMTP_MAILER_H)
#define SMTP_MAILER_H
// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------
#include <Net/Socket.h>
#include <Mail/Smtp.h>

class OutgoingMessage;

namespace Smtp
{
	class Mailer
	{
	public:
		Mailer (std::string const & server, 
				std::string const & user, 
				std::string const & password,
				std::string const & senderAddress,
				std::string const & domain);
		void Send (OutgoingMessage const & msg,
				   std::vector<std::string> const & addrVector,
				   std::string const & toFieldLabel = std::string ()); // "To:"
	private:
		WinSocks			_socks;
		Smtp::Connection	_connection;
		Smtp::Session		_session;
		std::string const	_senderAddress;
	};
}

#endif
