//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include "precompiled.h"
#include "LocalMailer.h"
#include "EmailMessage.h"
#include "EmailRegistry.h"
#include "EmailAccount.h"
#include "MapiMailer.h"
#include "SimpleMapiMailer.h"

LocalMailer::LocalMailer ()
{
	bool canUseFullMapi = Registry::CanUseFullMapi ();
	if (canUseFullMapi)
		_outgoingAccount.reset (new FullMapiAccount ());
	else
		_outgoingAccount.reset (new SimpleMapiAccount ());
	_session.LogonSend (*_outgoingAccount);
	Email::SessionInterface * pSession = _session.GetSession();
	if (canUseFullMapi)
	{
		Mapi::Session & s = reinterpret_cast<Mapi::Session &> (*pSession);
		_pImpl.reset (new Mapi::Mailer (s));
	}
	else
	{
		SimpleMapi::Session & s = reinterpret_cast<SimpleMapi::Session &> (*pSession);
		_pImpl.reset (new SimpleMapi::Mailer (s));
	}
}

LocalMailer::~LocalMailer ()
{
	_session.Logoff ();
}

void LocalMailer::Send (OutgoingMessage & msg, std::string const & address)
{
	std::vector<std::string> addrVector;
	addrVector.push_back (address);
	_pImpl->Send (msg, addrVector);
}

void LocalMailer::Save (OutgoingMessage & msg, std::string const & address)
{
	std::vector<std::string> addrVector;
	addrVector.push_back (address);
	_pImpl->Save (msg, addrVector);
}
