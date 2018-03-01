#if !defined (SMTP_H)
#define SMTP_H
// --------------------------------
// (c) Reliable Software, 2005 - 06
// --------------------------------
#include <Net/SimpleSocket.h>
#include <Net/SecureSocket.h>
#include <Net/Connection.h>

// SMTP: based on RFC #2821

namespace Smtp
{
	class Response
	{
	public:
		enum Status
		{
			ConnectSucceeded = 220,
			Ok               = 250,
			AuthSucceeded    = 235,
			AuthChallenge    = 334,
			DataCmdOk        = 354,
			TooManyRecipients = 452,
			BadEmailAddress   = 550,
			TooMuchMailData   = 552
		};
	public:
		Response (Socket & socket);
		unsigned int GetStatus () const { return _code; }
		std::string const & GetComment () const { return _comment; }
		char const * c_str () const { return _comment.c_str (); }
	private:
		unsigned int	_code;
		std::string		_comment;
	};

	class Connection : public ::Connection
	{
	public:
		Connection (std::string const & server, short port, int timeout = 0);
		~Connection ();
	};

	class Session
	{
	public:
		Session (Connection & connection, std::string const & domain, bool useSSL);
		void Authenticate (std::string const & user, std::string const & password);

		Socket & GetSocket () { return _connection.GetSocket (); }
	private:
		void SayHello ();
		void ReceiveConnectResponse ();
		void SendStartTlsCmd ();
	private:
		Connection	& _connection;
		std::string const _domain;
	};

	class MailTransaction
	{
	public:
		MailTransaction (Socket & socket)
			: _socket (socket),
			  _isCommit (false)
		{
		}
		~MailTransaction ()
		{
			if (!_isCommit)
			{
				try
				{
					_socket.SendLine ("RSET");
					Smtp::Response rsetResponse (_socket);
				}
				catch (...)
				{
				}
			}
		}
		void Commit() { _isCommit = true; }
	private:
		Socket & _socket;
		bool	 _isCommit;
	};
}

#endif
