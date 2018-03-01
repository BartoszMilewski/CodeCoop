#if !defined (EMAILSESSION_H)
#define EMAILSESSION_H
// -------------------------------
// Reliable Software (c) 2004 - 05
// -------------------------------

class Pop3Account;
class SmtpAccount;

class InboxIterator;
class Mailer;

namespace Email
{
	class SessionInterface;
	class Account;
	class Technology;

	class Session
	{
	public:
		Session ();
		~Session ();
		void LogonReceive (Email::Account const & account); // Pass Email::RegConfig
		void LogonSend (Email::Account const & account); // Pass Email::RegConfig
		void Logoff ();
		bool IsActive () const { return _pImpl.get () != 0; }
		Email::SessionInterface * GetSession() { return _pImpl.get(); }
	private:
		void Pop3Logon (Pop3Account const & account);
		void SmtpLogon (SmtpAccount const & account);
		void MapiLogon (Email::Technology const & technology);
	private:
		std::unique_ptr<Email::SessionInterface> _pImpl;
	};
}

#endif
