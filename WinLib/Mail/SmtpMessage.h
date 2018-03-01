#if !defined (SMTP_MESSAGE_H)
#define SMTP_MESSAGE_H
// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------
#include <Net/Socket.h>
#include <Mail/MsgTree.h>

namespace Smtp
{
	class Message
	{
	public:
		Message (std::string const & nameFrom, std::string const & addrFrom, std::string const & subject);
		void SetToField (std::vector<std::string> const & addrVector);
		void AddBody (std::unique_ptr<MessagePart> body)
		{
			Assert (_body.get () == 0);
			_body = std::move(body);
		}

		void Send (Socket & socket);
	private:
		// Header
		std::string		_from;	  // required
		std::string		_to;	  // optional
		std::string		_subject; // optional

		static const char MimeVersion [];

		// Body
		std::unique_ptr<MessagePart>	_body;
	};
}

#endif
