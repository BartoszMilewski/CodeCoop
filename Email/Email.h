#if !defined (EMAIL_INTERFACE_H)
#define EMAIL_INTERFACE_H
//----------------------------------
// (c) Reliable Software 2001 - 2007
// ---------------------------------

#include "EmailMan.h"

class SafePaths;
class OutgoingMessage;

class InboxIteratorInterface;

class InboxIterator
{
public:
	InboxIterator (Email::Manager & eMan, std::string const & accountName, bool unreadOnly);
	~InboxIterator ();

	void Advance ();
	bool AtEnd () const;

	void RetrieveAttachements (SafePaths & attPaths);
	std::string const & GetSubject () const;
	bool IsDeleteNonCoopMsg () const;
	bool DeleteMessage () throw ();
	void CleanupMessage () throw ();

private:
	Email::ReceiveLock						_eLock;
	std::unique_ptr<InboxIteratorInterface>	_pImpl;
};

class MailerInterface;

class Mailer
{
public:
	Mailer (Email::Manager & eMan);
	~Mailer ();

	void Send (OutgoingMessage & msg, std::string const & address);
	void Send (OutgoingMessage & msg, std::vector<std::string> const & addrVector);
	void Save (OutgoingMessage & msg, std::string const & address);
	void Save (OutgoingMessage & msg, std::vector<std::string> const & addrVector);
	void GetLoggedUser (std::string & name, std::string & emailAddr);
	bool VerifyEmailAddress (std::string & emailAddr);
private:
	Email::SendLock					_eLock;
	std::unique_ptr<MailerInterface>	_pImpl;
};

#endif
