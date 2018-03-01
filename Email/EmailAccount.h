#if !defined (EMAILACCOUNT_H)
#define EMAILACCOUNT_H
// ----------------------------------
// (c) Reliable Software, 2005 - 2008
// ----------------------------------

#include <Bit.h>

namespace RegKey { class Handle; }

namespace Email
{
	class RegConfig;

	class Technology
	{
		enum Kind
		{ 
			bitPop3 = 0,
			bitSmtp,
			bitSimpleMapi,
			bitFullMapi
		};

	public:
		void SetPop3 () { _kind.set (bitPop3, true); }
		void SetSmtp () { _kind.set (bitSmtp, true); }
		void SetSimpleMapi () { _kind.set (bitSimpleMapi, true); }
		void SetFullMapi () { _kind.set (bitFullMapi, true); }

		bool IsPop3 () const { return _kind.test (bitPop3); }
		bool IsSmtp () const { return _kind.test (bitSmtp); }
		bool IsSimpleMapi () const { return _kind.test (bitSimpleMapi); }
		bool IsFullMapi () const { return _kind.test (bitFullMapi); }

		bool IsEqual (Email::Technology const & technology) const
		{
			return _kind.to_ulong () == technology._kind.to_ulong ();
		}

	private:
		BitSet<Technology::Kind>	_kind;
	};

	class Account
	{
	public:
		static const unsigned long MinServerTimeout = 30 * 1000; // 30 sec
		static const unsigned long DefaultServerTimeout = 60 * 1000; // 1 minute
		static const unsigned long MaxServerTimeout = 5 * 60 * 1000; // 5 minutes

	public:
		~Account () {}

		virtual void Save () const {}

		bool IsValid () const { return _isValid; }
		bool IsEqual (Email::Account const & account) const
		{
			return _name == account.GetName () && _technology.IsEqual (account.GetTechnology ());
		}

		std::string const & GetName () const { return _name; }
		Email::Technology const & GetTechnology () const { return _technology; }

		virtual char const * GetImplementationId () const = 0;

	protected:
		Account (std::string const & name = std::string ()) 
		  : _isValid (false),
			_name (name) 
		{}

	protected:
		bool				_isValid;
		std::string	const	_name;
		Email::Technology	_technology;
	};
}

class SimpleMapiAccount : public Email::Account
{
public:
	SimpleMapiAccount ()
		: Email::Account ()
	{
		_technology.SetSimpleMapi ();
		_isValid = true;
	}
	char const * GetImplementationId () const { return "Simple Mapi"; }
};

class FullMapiAccount : public Email::Account
{
public:
	FullMapiAccount ()
		: Email::Account ()
	{
		_technology.SetFullMapi ();
		_isValid = true;
	}
	char const * GetImplementationId () const { return "Full Mapi"; }
};

class ServerAccount : public Email::Account
{
public:
	std::string const & GetServer () const { return _server; }
	std::string const & GetUser () const { return _user; }
	std::string const & GetPassword () const { return _password; }
	void SetServer (std::string const & server) { _server = server; }
	void SetUser (std::string const & user) { _user = user; }
	void SetPassword (std::string const & pass) { _password = pass; }

	short GetPort () const { return _port; }
	void SetPort (short port) { _port = port; }
	virtual bool UseSSL () const = 0;
	virtual void SetUseSSL (bool use) = 0;

	unsigned long GetTimeout () const { return _timeout; }

	bool IsEqual (ServerAccount const & account) const;

	virtual void Dump (std::ostream & out) const = 0;

protected:
	ServerAccount (std::string const & name, short port)
		: Account (name),
		  _timeout (DefaultServerTimeout),
		  _port (port)
	{}

	ServerAccount (std::string const & name,
				   std::string const & server,
				   std::string const & user,
				   std::string const & password,
				   short port)
		: Account (name),
		  _server (server),
		  _user (user),
		  _password (password),
		  _timeout (DefaultServerTimeout),
		  _port (port)
	{}

protected:
	std::string		  _server;
	std::string		  _user;
	std::string	 	  _password;
	unsigned long	  _timeout;
	short			  _port;
};

class Pop3Account : public ServerAccount
{
public:
	Pop3Account (Email::RegConfig const & emailCfg, std::string const & accountName, bool getPasswd = true);
	Pop3Account (std::string const & server,
				 std::string const & user,
				 std::string const & password,
				 bool useSSL,
				 std::string const & accountName);

	std::string Validate (std::string const & passwordVerify) const;
	void Reset (Pop3Account const & account);
	void Save (Email::RegConfig const & emailCfg) const;

	bool UseSSL () const { return _options.test (bitUseSSL); }
	bool IsDeleteNonCoopMsg () const { return _options.test (bitIsDeleteNonCoopMsg); }
	bool IsEqual (Pop3Account const & account) const;

	void SetUseSSL (bool use) { _options.set (bitUseSSL, use); }
	void SetDeleteNonCoopMsg (bool isDel) {	_options.set (bitIsDeleteNonCoopMsg, isDel); }
	char const * GetImplementationId () const { return "POP3"; }

	void Dump (std::ostream & out) const;

	static short GetDefaultPort (bool useSSL) { return useSSL ? DefaultSSLPop3Port : DefaultPop3Port; }

private:
	void Load (RegKey::Handle key, bool getPasswd);
	void Save (RegKey::Handle key) const;

private:
	static const short DefaultPop3Port = 110;
	static const short DefaultSSLPop3Port = 995;
	enum Pop3Options
	{
		bitIsDeleteNonCoopMsg,
		bitUseSSL
	};

private:
	BitSet<Pop3Options> _options;
};

class SmtpAccount : public ServerAccount
{
public:
	SmtpAccount (Email::RegConfig const & emailCfg, bool getPasswd = true);
	SmtpAccount (std::string const & server,
				 std::string const & user,
				 std::string const & password);

	std::string Validate (std::string const & passwordVerify) const;
	void Reset (SmtpAccount const & account);
	void Save (Email::RegConfig const & emailCfg) const;
	std::string const & GetSenderAddress () const { return _senderAddress; }
	std::string const & GetSenderName () const { return _senderName; }

	bool UseSSL () const { return _options.test (bitUseSSL); }
	bool IsAuthenticate () const { return _options.test (bitIsAuthenticate); }
	bool IsEqual (SmtpAccount const & account) const;

	void SetUseSSL (bool use) { _options.set (bitUseSSL, use); }
	void SetSenderAddress (std::string const &  senderAddress) { _senderAddress = senderAddress; }
	void SetSenderName (std::string const &  senderName) { _senderName = senderName; }
	void SetAuthenticate (bool authenticate) { _options.set (bitIsAuthenticate, authenticate); }

	char const * GetImplementationId () const { return "SMTP"; }

	void Dump (std::ostream & out) const;

	static short GetDefaultPort (bool useSSL) { return DefaultSmtpPort; }

private:
	static const short DefaultSmtpPort = 25; // use same default as MS Outlook
	enum SmtpOptions
	{
		bitIsAuthenticate,
		bitUseSSL
	};

private:
	std::string			_senderName;
	std::string			_senderAddress;
	BitSet<SmtpOptions>	_options;
};

#endif
