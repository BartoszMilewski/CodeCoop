//-----------------------------------
// (c) Reliable Software 2001 -- 2007
// ----------------------------------

#include "precompiled.h"
#include "Email.h"
#include "EmailAccount.h"
#include "EmailTransport.h"
#include "MailReader.h"
#include "SimpleMapiMailer.h"
#include "MapiMailReader.h"
#include "MapiMailer.h"
#include "EmailRegistry.h"
#include "Registry.h"
#include "RegFunc.h"
#include "Pop3Session.h"
#include "SmtpSession.h"
#include "Pop3InboxIterator.h"
#include "SmtpMailer.h"

#include <Mail/EmailAddress.h>
#include <Ex/Winex.h>
#include <Dbg/Assert.h>

Email::Session::Session ()
{
}

Email::Session::~Session ()
{
}
 // Pass Email::RegConfig
void Email::Session::LogonReceive (Email::Account const & account)
{
	_pImpl.reset ();
	// Select implementation
	Email::Technology const & technology = account.GetTechnology ();
	if (technology.IsPop3 ())
	{
		Pop3Logon (reinterpret_cast<Pop3Account const &>(account));
	}
	else
		MapiLogon (technology);
}
 // Pass Email::RegConfig
void Email::Session::LogonSend (Email::Account const & account)
{
	_pImpl.reset ();
	// Select implementation
	Email::Technology const & technology = account.GetTechnology ();
	if (technology.IsSmtp ())
	{
		SmtpLogon (reinterpret_cast<SmtpAccount const &>(account));
	}
	else
		MapiLogon (technology);
}

void Email::Session::Pop3Logon (Pop3Account const & account)
{
	_pImpl.reset (new Email::Pop3Session (account));
}

void Email::Session::SmtpLogon (SmtpAccount const & account)
{
	std::string domain = Registry::GetComputerName ();
	if (domain.empty ())
		domain = "Code Co-op";

	_pImpl.reset (new Email::SmtpSession (account, domain));
}

void Email::Session::MapiLogon (Email::Technology const & technology)
{
	if (technology.IsFullMapi ())
	{
		_pImpl.reset (new Mapi::Session ());
	}
	else
	{
		_pImpl.reset (new SimpleMapi::Session ());
	}
}

void Email::Session::Logoff ()
{
	_pImpl.reset ();
}

InboxIterator::InboxIterator (Email::Manager & eMan, std::string const & accountName, bool unreadOnly)
: _eLock (eMan, accountName)
{
	Assert (eMan.IsValid ());
	Email::Session & session = _eLock.GetSession ();
	if (!session.IsActive ())
		return;

	// Select implementation
	Email::Account const * account = eMan.FindIncomingAccount (accountName);
	if (account == 0)
		throw Win::InternalException ("Dispatcher e-mail subsystem failure.\n"
								  	  "Please contact support@relisoft.com");

	Email::Technology const & technology = account->GetTechnology ();
	Email::SessionInterface * pSession = session.GetSession();
	if (technology.IsPop3 ())
	{
		Email::Pop3Session & s = reinterpret_cast<Email::Pop3Session &> (*pSession);
		_pImpl.reset (new Pop3InboxIterator (s.GetSession (), s.IsDeleteNonCoopMsg ()));
	}
	else if (technology.IsFullMapi ())
	{
		Mapi::Session & s = reinterpret_cast<Mapi::Session &> (*pSession);
		_pImpl.reset (new Mapi::MailboxIterator (s, unreadOnly));
	}
	else
	{
		// always try our default -- Simple MAPI
		SimpleMapi::Session & s = reinterpret_cast<SimpleMapi::Session &> (*pSession);
		_pImpl.reset (new SimpleMapi::MailboxIterator (s, unreadOnly));
	}
}

InboxIterator::~InboxIterator ()
{}

//
// Inbox iterator implementation forwarders
//

void InboxIterator::Advance ()
{
	Assert (_pImpl.get () != 0);
	_pImpl->Advance ();
}

bool InboxIterator::AtEnd () const
{
	return _pImpl.get () == 0 || _pImpl->AtEnd ();
}

void InboxIterator::RetrieveAttachements (SafePaths & attPaths)
{
	Assert (_pImpl.get () != 0);
	_pImpl->RetrieveAttachements (attPaths);
}

std::string const & InboxIterator::GetSubject () const
{
	Assert (_pImpl.get () != 0);
	return _pImpl->GetSubject ();
}

bool InboxIterator::IsDeleteNonCoopMsg () const
{
	return _pImpl->IsDeleteNonCoopMsg ();
}

bool InboxIterator::DeleteMessage () throw ()
{
	Assert (_pImpl.get () != 0);
	return _pImpl->DeleteMessage ();
}

void InboxIterator::CleanupMessage () throw ()
{
	Assert (_pImpl.get () != 0);
	return _pImpl->CleanupMessage ();
}

Mailer::Mailer (Email::Manager & eMan)
: _eLock (eMan)
{
	Assert (eMan.IsValid ());
	Email::Session & session = _eLock.GetSession ();
	if (!session.IsActive ())
		return;

	// Select implementation
	Email::Account const & account = eMan.GetOutgoingAccount ();
	Email::Technology const & technology = account.GetTechnology ();
	Email::SessionInterface * pSession = session.GetSession();
	if (technology.IsSmtp ())
	{
		Email::SmtpSession & s = reinterpret_cast<Email::SmtpSession &> (*pSession);
		_pImpl.reset (new SmtpMailer (s.GetSession (), 
									  s.GetUser (), 
									  s.GetSenderName (), 
									  s.GetSenderAddress ()));
	}
	else if (technology.IsFullMapi ())
	{
		Mapi::Session & s = reinterpret_cast<Mapi::Session &> (*pSession);
		_pImpl.reset (new Mapi::Mailer (s));
	}
	else
	{
		// always try our default -- Simple MAPI
		SimpleMapi::Session & s = reinterpret_cast<SimpleMapi::Session &> (*pSession);
		_pImpl.reset (new SimpleMapi::Mailer (s));
	}
}

Mailer::~Mailer ()
{
}

//
// Mailer implementation forwarders
//

void Mailer::Send (OutgoingMessage & msg, std::string const & address)
{
	std::vector<std::string> addrVector;
	addrVector.push_back (address);
	Send (msg, addrVector);
}

void Mailer::Send (OutgoingMessage & msg, std::vector<std::string> const & addrVector)
{
	Assert (_pImpl.get () != 0);
	_pImpl->Send (msg, addrVector);
}

void Mailer::Save (OutgoingMessage & msg, std::string const & address)
{
	std::vector<std::string> addrVector;
	addrVector.push_back (address);
	Save (msg, addrVector);
}

void Mailer::Save (OutgoingMessage & msg, std::vector<std::string> const & addrVector)
{
	Assert (_pImpl.get () != 0);
	_pImpl->Save (msg, addrVector);
}

void Mailer::GetLoggedUser (std::string & name, std::string & emailAddr)
{
	Assert (_pImpl.get () != 0);
	_pImpl->GetLoggedUser (name, emailAddr);
}

bool Mailer::VerifyEmailAddress (std::string & emailAddr)
{
	Assert (_pImpl.get () != 0);
	return _pImpl->VerifyEmailAddress (emailAddr);
}
