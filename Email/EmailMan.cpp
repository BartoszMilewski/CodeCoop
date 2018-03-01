// ---------------------------------
// Reliable Software (c) 2004 - 2008
// ---------------------------------
#include "precompiled.h"
#include "EmailMan.h"
#include "EmailRegistry.h"
#include "Registry.h"
#include "MapiSession.h"
#include "Validators.h"

#include <Win/Win.h> // Sleep

Email::Manager TheEmail;

bool Email::Manager::Refresh ()
{
	Win::Lock lock (_critSection);

	_emailCfg.ReadScriptProcessorConfig (_scriptProcessorConfig); 
	ScriptProcessorValidator validator (_scriptProcessorConfig);
	_isValidProcessor = validator.IsValid ();

	ReadAccounts ();
	return IsValid ();
}

void Email::Manager::BeginEdit () 
{
	_emailCfg.BeginEdit ();
}

void Email::Manager::CommitEdit ()
{
	_emailCfg.CommitEdit ();
	Win::Lock lock (_critSection);
	ReadAccounts ();
}

void Email::Manager::AbortEdit () 
{
	_emailCfg.AbortEdit ();
}

void Email::Manager::ReadAccounts ()
{
	// _critSection must be already locked

	_isValidEmail = true;	// Assume we have a valid e-mail configuration
	_isUsingPop3 = false;
	_isUsingSmtp = false;
	_passwordDecryptionFailed = false;

	// Select default e-mail implementation
	bool canUseFullMapi = Registry::CanUseFullMapi () && !_emailCfg.IsSimpleMapiForced ();
#if 0
	if (canUseFullMapi &&  (!_emailCfg.IsUsingPop3 () || !_emailCfg.IsUsingSmtp ()))
	{
		// Verify that full MAPI really can be used
		try
		{
			Mapi::Session session;
		}
		catch (...)
		{
			// Mapi initialization fails -- fall back to Simple Mapi.
			canUseFullMapi = false;
		}
	}
#endif
	// Incoming e-mail
	_incomingAccountList.clear ();
	if (_emailCfg.IsUsingPop3 ())
	{
		_isUsingPop3 = true;
		if (_emailCfg.IsPop3RegKey ())
		{
			// main account from POP3 key first (required)
			std::unique_ptr<Email::Account> account;
			try
			{
				// Pass _emailCfg
				account.reset (new Pop3Account (_emailCfg, std::string ())); // Default account has no name
				if (account->IsValid ())
				{
					_incomingAccountList.push_back (std::move(account));
					// Then (named) accounts from sub-keys
					// these are only used by us

					RegKey::AutoHandle pop3Key = _emailCfg.GetPop3RegKey ();
					for (RegKey::Seq seq (pop3Key); !seq.AtEnd (); seq.Advance ())
					{
						account.reset (new Pop3Account (_emailCfg, seq.GetKeyName ()));
						if (account->IsValid ())
							_incomingAccountList.push_back (std::move(account));
					}
				}
			}
			catch (Win::Exception ex)
			{
				if (ex.IsBadDecryptionKey ())
					_passwordDecryptionFailed = true;	
			}
		}

		if (_incomingAccountList.size () == 0)
			_isValidEmail = false;	// Using POP3 and no account defined
	}
	else
	{
		// Use default e-mail implementation
		_isUsingPop3 = false;
		if (canUseFullMapi)
			_incomingAccountList.push_back (std::unique_ptr<Email::Account>(new FullMapiAccount ()));
		else
			_incomingAccountList.push_back (std::unique_ptr<Email::Account>(new SimpleMapiAccount ()));
	}

	// Outgoing e-mail
	_outgoingAccount.reset ();
	if (_emailCfg.IsUsingSmtp ())
	{
		_isUsingSmtp = true;
		try
		{
			_outgoingAccount.reset (new SmtpAccount (_emailCfg));
			if (!_outgoingAccount->IsValid ())
				_outgoingAccount.reset ();
		}
		catch (Win::Exception ex)
		{
			if (ex.IsBadDecryptionKey ())
				_passwordDecryptionFailed = true;	
		}

		if (_outgoingAccount.get () == 0)
			_isValidEmail = false;	// Using SMTP and no account defined
	}
	else
	{
		// Use default e-mail implementation
		_isUsingSmtp = false;
		if (canUseFullMapi)
			_outgoingAccount.reset (new FullMapiAccount ());
		else
			_outgoingAccount.reset (new SimpleMapiAccount ());
	}
}

void Email::Manager::ChangeMaxChunkSize (unsigned long newMaxChunkSize) 
{
	_emailCfg.SetMaxEmailSize (newMaxChunkSize);
	Win::Lock lock (_critSection);
	ReadAccounts ();
}

void Email::Manager::GetIncomingAccountList (std::vector<std::string> & accountList)
{
	Win::Lock lock (_critSection);
	for (auto_vector<Email::Account>::const_iterator it = _incomingAccountList.begin ();
		 it < _incomingAccountList.end ();
		 ++it)
	{
		accountList.push_back ((*it)->GetName ());
	}
}

Email::Account const * Email::Manager::FindIncomingAccount (std::string const & name)
{
	Win::Lock lock (_critSection);
	for (auto_vector<Email::Account>::const_iterator it = _incomingAccountList.begin ();
		it < _incomingAccountList.end ();
		++it)
	{
		if ((*it)->GetName () == name)
			return *it;
	}
	return 0;
}

#if 1
// This version opens a session for every action
void Email::Manager::LockReceive (std::string const & accountName) // Pass Email::RegConfig
{
	// critical section already locked
	if (!_session.IsActive ())
	{
		Email::Account const * account = FindIncomingAccount (accountName);
		if (account != 0)
			_session.LogonReceive (*account);
	}
}

void Email::Manager::LockSend () // Pass Email::RegConfig
{
	// critical section already locked
	if (!_session.IsActive ())
	{
		Assert (_outgoingAccount.get () != 0);
		_session.LogonSend (*_outgoingAccount);
	}
}

void Email::Manager::Unlock ()
{
	// critical section still locked
	_session.Logoff ();
}

void Email::Manager::ShutDown ()
{
}

#else

void Email::Manager::LockReceive (std::string const & accountName)
{
	// critical section already locked
	if (!_session.IsActive ())
	{
		Email::Account const & account = GetIncomingAccount (accountName);
		_session.LogonReceive (account);
		Win::Sleep (1000); // maybe this will help
	}
}

void Email::Manager::LockSend ()
{
	// critical section already locked
	if (!_session.IsActive ())
	{
		_session.LogonSend (*_outgoingAccount);
		Win::Sleep (1000); // maybe this will help
	}
}

void Email::Manager::Unlock ()
{
	// critical section still locked
}

void Email::Manager::ShutDown ()
{
	Win::Lock lock (_critSection);
	_session.Logoff ();
}
#endif
