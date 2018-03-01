// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------
#if !defined (POP3SESSION_H)
#define POP3SESSION_H

#include "EmailTransport.h"
#include "EmailAccount.h"
#include <Mail/Pop3.h>
#include <Net/Socket.h>

namespace Email
{
	class Pop3Session : public Email::SessionInterface
	{
	public:
		Pop3Session (Pop3Account const & account)
			: _connection (account.GetServer (), 
						   account.GetPort (), 
						   account.GetTimeout ()),
			_session (_connection, account.GetUser (), account.GetPassword (), account.UseSSL ()),
			_isDelNonCoopMsg (account.IsDeleteNonCoopMsg ())
		{}
		
		Pop3::Session & GetSession () { return _session; }
		bool IsDeleteNonCoopMsg () const { return _isDelNonCoopMsg; }

	private:
		WinSocks			_useSocks;
		Pop3::Connection	_connection;
		Pop3::Session		_session;
		bool const			_isDelNonCoopMsg;
	};
}

#endif
