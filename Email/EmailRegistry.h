#if !defined (EMAILREGISTRY_H)
#define EMAILREGISTRY_H
//----------------------------------
// (c) Reliable Software 2005 - 2007
//----------------------------------

#include <Sys/RegKey.h>

namespace Registry
{
	bool CanUseFullMapi ();
	// machine
	bool IsSimpleMapiAvailable ();
	bool IsFullMapiAvailable ();
	bool IsFullMapiSupported ();

	// HKEY_LOCAL_MACHINE
	//      Software
	//          Microsoft
	//              Windows Messaging Subsystem
	class MAPICheck
	{
	public:
		MAPICheck ()
			: _keyMain (_keyRoot, "Software"),
			_keyMicrosoft (_keyMain, "Microsoft"),
			_keyWMS (_keyMicrosoft, "Windows Messaging Subsystem")
		{}

		bool Exists () const
		{
			return _keyMain.Exists () && _keyMicrosoft.Exists () && _keyWMS.Exists ();
		}

	private:
		RegKey::LocalMachine	_keyRoot;
		RegKey::Check			_keyMain;
		RegKey::Check			_keyMicrosoft;
		RegKey::Check			_keyWMS;
	};

	class MAPI
	{
	public:
		MAPI ()
			: _keyMain (_keyRoot, "Software"),
			_keyMicrosoft (_keyMain, "Microsoft"),
			_keyWMS (_keyMicrosoft, "Windows Messaging Subsystem")
		{}

		bool IsSimpleMapiAvailable ()
		{
			return _keyWMS.GetStringVal ("MAPI") == "1";
		}

		bool IsFullMapiAvailable ()
		{
			return _keyWMS.GetStringVal ("MAPIX") == "1";
		}

	private:
		RegKey::LocalMachine	_keyRoot;
		RegKey::ReadOnly		_keyMain;
		RegKey::ReadOnly		_keyMicrosoft;
		RegKey::ReadOnly		_keyWMS;
	};

	// HKEY_LOCAL_MACHINE
	//      Software
	//          Clients
	//              Mail

	class DefaultEmailClient
	{
	public:
		DefaultEmailClient ()
			: _keyMain (_keyRoot, "Software"),
			_keyClients (_keyMain, "Clients"),
			_keyMail (_keyClients, "Mail")
		{}

		RegKey::Handle & Key () { return _keyMail; }
		bool IsFullMapiSupported ()
		{
			std::string emailClient = GetName ();
			return emailClient == "Microsoft Outlook";
		}

		bool IsMapiEmailClient ()
		{
			std::string emailClient = GetName ();
			return emailClient == "Microsoft Outlook" ||
				   emailClient == "Outlook Express";
		}

		std::string GetName ()
		{
			return _keyMail.GetStringVal ("");
		}

	private:
		RegKey::LocalMachine	_keyRoot;
		RegKey::ReadOnly		_keyMain;
		RegKey::ReadOnly		_keyClients;
		RegKey::ReadOnly		_keyMail;
	};

	// HKEY_LOCAL_MACHINE
	//      Software
	//          Clients
	//              Mail
	//					ClientKey

	class EmailClientCheck
	{
	public:
		EmailClientCheck (std::string const & keyName)
			: _keyMain (_keyRoot, "Software"),
			_keyClients (_keyMain, "Clients"),
			_keyMail (_keyClients, "Mail"),
			_keyClient (_keyMail, keyName.c_str ())
		{}

		bool Exists () const
		{
			return _keyClient.Exists ();
		}

	private:
		RegKey::LocalMachine	_keyRoot;
		RegKey::Check			_keyMain;
		RegKey::Check			_keyClients;
		RegKey::Check			_keyMail;
		RegKey::Check			_keyClient;
	};

	class EmailClient : public DefaultEmailClient
	{
	public:
		EmailClient (std::string const & clientName)
			: _keyClient (DefaultEmailClient::Key (), clientName.c_str ())
		{}

		std::string GetDllPath ()
		{
			return _keyClient.GetStringVal ("DllPath");
		}

	private:
		RegKey::ReadOnly	_keyClient;
	};

}

#endif
