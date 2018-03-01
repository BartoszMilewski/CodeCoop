#if !defined (EMAILMAN_H)
#define EMAILMAN_H
// ---------------------------------
// Reliable Software (c) 2004 - 2008
// ---------------------------------

#include "EmailConfig.h"
#include "EmailSession.h"
#include "EmailAccount.h"
#include "ScriptProcessorConfig.h"

#include <Sys/Synchro.h>
#include <Ex/WinEx.h>
#include <auto_vector.h>
#include <StringOp.h>

namespace Email
{
	class Lock;
	class ReceiveLock;
	class SendLock;

	class Manager
	{
		friend class Email::Lock;
		friend class Email::ReceiveLock;
		friend class Email::SendLock;
	public:
		Manager () 
			: _isValidEmail (false), 
			  _isValidProcessor (false), 
			  _isUsingPop3 (false),
			  _isUsingSmtp (false),
			  _passwordDecryptionFailed (false)
		{}

		bool Refresh ();
		void ChangeMaxChunkSize (unsigned long newChunkSize);

		void BeginEdit ();
		void CommitEdit ();
		void AbortEdit ();
		bool IsInTransaction () const { return _emailCfg.IsInTransaction (); }

		void ShutDown ();
		bool IsValid () const { return _isValidEmail && _isValidProcessor; }
		bool PasswordDecryptionFailed () const { return _passwordDecryptionFailed; }

		Email::RegConfig const & GetEmailConfig () const { return _emailCfg; }
		Email::RegConfig & GetEmailConfig () { return _emailCfg; }
		void SetDefaults () { _emailCfg.SetDefaults ();	}
		void SetEmailStatus (Email::Status status) { _emailCfg.SetEmailStatus (status); }
		Email::Status GetEmailStatus () const { return _emailCfg.GetEmailStatus (); }

		void GetIncomingAccountList (std::vector<std::string> & accountList);
		Email::Account const & GetOutgoingAccount () const { return *_outgoingAccount; }
		Email::Account const * FindIncomingAccount (std::string const & name);

		ScriptProcessorConfig GetScriptProcessorConfig () 
		{
			Win::Lock lock (_critSection);
			return _scriptProcessorConfig; 
		}

		bool IsBlacklisted (std::string const & emailAddress)
		{
			Win::Lock lock (_critSection);
			return _emailBlacklist.find (emailAddress) != _emailBlacklist.end ();
		}
		void ClearBlacklist ()
		{
			Win::Lock lock (_critSection);
			_emailBlacklist.clear ();
		}
		void InsertBlacklisted (std::string const & address)
		{
			Win::Lock lock (_critSection);
			_emailBlacklist.insert (address);
		}
	private:
		void ReadAccounts ();
		void LockReceive (std::string const & accountName);
		void LockSend ();
		void Unlock ();
		Email::Session & GetSession () { return _session; }

	private:
		Win::CritSection				_critSection;

		bool							_isValidEmail;
		bool							_isValidProcessor;
		Email::RegConfig				_emailCfg;	// used exclusively by the main thread
		Email::Session					_session;
		auto_vector<Email::Account>		_incomingAccountList;
		std::unique_ptr<Email::Account>	_outgoingAccount;
		bool							_isUsingPop3;
		bool							_isUsingSmtp;
		bool							_passwordDecryptionFailed;

		NocaseSet						_emailBlacklist;		// shared between threads
		ScriptProcessorConfig			_scriptProcessorConfig; // shared between threads
	};

	class Lock: public Win::Lock
	{
	public:
		~Lock ()
		{
			_man.Unlock ();
		}
		Email::Session & GetSession ()
		{
			return _man.GetSession ();
		}
	protected:
		Lock (Email::Manager & man)
			: Win::Lock (man._critSection), _man (man)
		{}
	protected:
		Email::Manager & _man;
	};

	class ReceiveLock : public Email::Lock
	{
	public:
		Email::ReceiveLock (Email::Manager & man, std::string const & accountName)
			: Email::Lock (man)
		{
			_man.LockReceive (accountName);
		}
	};

	class SendLock : public Email::Lock
	{
	public:
		SendLock (Email::Manager & man)
			: Email::Lock (man)
		{
			_man.LockSend ();
		}
	};
}

// Global access point to email
extern Email::Manager TheEmail; 

#endif
