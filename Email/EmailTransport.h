#if !defined (EMAILTRANSPORT_H)
#define EMAILTRANSPORT_H
//-------------------------------------
// (c) Reliable Software 2001 -- 2004
// ------------------------------------

class SafePaths;
class OutgoingMessage;

namespace Email
{
	class SessionInterface
	{
	public:
		virtual ~SessionInterface () {}
	};
}

class InboxIteratorInterface
{
public:
	virtual ~InboxIteratorInterface () {}
	virtual void Advance () = 0;
	virtual bool AtEnd () const = 0;

	virtual void RetrieveAttachements (SafePaths & attPaths) = 0;
	virtual std::string const & GetSubject () const = 0;
	virtual bool IsDeleteNonCoopMsg () const { return false; }
	virtual bool DeleteMessage () throw () = 0; // return true if client can continue iteration
	virtual void CleanupMessage () throw () = 0;
};

class MailerInterface
{
public:
	virtual ~MailerInterface () {}
	virtual void Send (OutgoingMessage & msg, std::vector<std::string> const & addrVector) = 0;
	virtual void Save (OutgoingMessage & msg, std::vector<std::string> const & addrVector) = 0;
	virtual void GetLoggedUser (std::string & name, std::string & emailAddr) = 0;
	virtual bool VerifyEmailAddress (std::string & emailAddr) = 0;
};

#endif
