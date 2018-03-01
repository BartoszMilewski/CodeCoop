#if !defined (REGKEYS_H)
#define REGKEYS_H
//----------------------------------
// (c) Reliable Software 2000 - 2008
//----------------------------------

#include "Global.h"
#include "ReliSoftReg.h"

#include <Sys/RegKey.h>
#include <StringOp.h>

//---------------------------------------------
// Warning: this is pure Windows implementation
//---------------------------------------------


namespace Registry
{
	class CoopKeyCheck
	{
	public:
		CoopKeyCheck (bool isUserSetup)
		   :_keyRoot (isUserSetup? HKEY_CURRENT_USER: HKEY_LOCAL_MACHINE),
			_keyMain (_keyRoot, "Software"),
			_keyReliSoft (_keyMain, "Reliable Software"),
			_keyCoop (_keyReliSoft, ApplicationName)
		{}

		bool Exists () const
		{
			return _keyCoop.Exists ();
		}

	private:
		RegKey::Root	_keyRoot;
		RegKey::Check	_keyMain;
		RegKey::Check	_keyReliSoft;
		RegKey::Check	_keyCoop;
	};

	class KeyCoopRo
	{
	public:
		KeyCoopRo (bool isUserSetup)
		   :_keyRoot (isUserSetup? HKEY_CURRENT_USER: HKEY_LOCAL_MACHINE),
			_keyMain (_keyRoot, "Software"),
			_keyReliSoft (_keyMain, "Reliable Software"),
			_keyCoop (_keyReliSoft, ApplicationName)
		{}
		std::string GetProgramPath () { return _keyCoop.GetStringVal ("ProgramPath"); }
		std::string GetCatalogPath () { return _keyCoop.GetStringVal ("CatalogPath"); }
		std::string GetCmdLineToolsPath () { return _keyCoop.GetStringVal ("CmdLineToolsPath"); }
	private:
		RegKey::Root		_keyRoot;
		RegKey::ReadOnly	_keyMain;
		RegKey::ReadOnly	_keyReliSoft;
		RegKey::ReadOnly	_keyCoop;
	};

	// HKEY_LOCAL_MACHINE
	//      Software
	//          SourceCodeControlProvider

	class KeyScc
	{
	public:
		KeyScc ()
		   :_keyRoot (HKEY_LOCAL_MACHINE),
			_keyMain (_keyRoot, "Software"),
			_keyScc (_keyMain, "SourceCodeControlProvider")
		{}

		RegKey::Handle & Key () { return _keyScc; }
		RegKey::Handle const & Key () const { return _keyScc; }
		std::string GetCurrentProvider ()
		{
			return _keyScc.GetStringVal ("ProviderRegKey");
		}
		void SetCurrentProvider (char const * regPath)
		{
			_keyScc.SetValueString ("ProviderRegKey", regPath);
		}
		void AddProvider (char const * name, char const * regPath)
		{
			RegKey::New providers (_keyScc, "InstalledSCCProviders");
			providers.SetValueString (name, regPath);
		}
		void RemoveProvider (char const * name)
		{
			RegKey::Existing providers (_keyScc, "InstalledSCCProviders");
			providers.DeleteValue (name);
		}

	private:
		RegKey::Root		_keyRoot;
		RegKey::Existing	_keyMain;
		RegKey::New			_keyScc;
	};

	// HKEY_LOCAL_MACHINE
	//      Software
	//          SourceCodeControlProvider

	class KeySccCheck
	{
	public:
		KeySccCheck ()
		   :_keyRoot (HKEY_LOCAL_MACHINE),
			_keyMain (_keyRoot, "Software"),
			_keyScc (_keyMain, "SourceCodeControlProvider")
		{}

		bool Exists () const
		{
			return _keyScc.Exists ();
		}

		std::string GetCurrentProvider () const
		{
			return _keyScc.GetStringVal ("ProviderRegKey");
		}

		bool IsCurrentProviderSet () const
		{
			return !GetCurrentProvider ().empty ();
		}

		bool IsCoopCurrentProvider () const
		{
			if (!_keyScc.Exists ())
				return false;

			std::string currentProvider = _keyScc.GetStringVal ("ProviderRegKey");
			return IsNocaseEqual (currentProvider, "Software\\Reliable Software\\Code Co-op");
		}
		bool IsCoopRegisteredProvider () const
		{
			if (!_keyScc.Exists ())
				return false;

			RegKey::New providers (_keyScc, "InstalledSCCProviders");
			std::string coopProvider = providers.GetStringVal ("Reliable Software Code Co-op");
			return !coopProvider.empty ();
		}

	private:
		RegKey::Root	  _keyRoot;
		RegKey::ReadOnly  _keyMain;
		RegKey::Check     _keyScc;
	};

	//
	// User Registry
	//

	// HKEY_LOCAL_USER
	//      Identities

	class OutlookExpressIdentitiesCheck
	{
	public:
		OutlookExpressIdentitiesCheck ()
		   : _keyIdentities (_keyRoot, "Identities")
		{}

		bool Exists () const
		{
			return _keyIdentities.Exists ();
		}

	private:
		RegKey::CurrentUser	_keyRoot;
		RegKey::Check		_keyIdentities;
	};

	class OutlookExpressIdentities
	{
	public:
		OutlookExpressIdentities ()
		   : _keyIdentities (_keyRoot, "Identities")
		{}


		void SetDefaultIdentity (std::string const & identityGuid)
		{
			_keyIdentities.SetValueString ("Default User ID", identityGuid.c_str ());
		}

		RegKey::Handle & Key () { return _keyIdentities; }
		RegKey::Handle const & Key () const { return _keyIdentities; }
		std::string GetDefaultIdentity () const { return _keyIdentities.GetStringVal ("Default User ID"); }
		std::string GetCurrentIdentity () const
		{ 
			std::string current (_keyIdentities.GetStringVal ("Last User ID"));
			if (current == "{00000000-0000-0000-0000-000000000000}")
				return std::string ();
			return current;
		}
		std::string GetStartAsIdentity () const
		{ 
			std::string startAs (_keyIdentities.GetStringVal ("Start As"));
			if (startAs == "{00000000-0000-0000-0000-000000000000}")
				return std::string ();
			return startAs;
		}

	private:
		RegKey::CurrentUser	_keyRoot;
		RegKey::Existing	_keyIdentities;
	};

	class OutlookExpressIdentity : public OutlookExpressIdentities
	{
	public:
		OutlookExpressIdentity (std::string const & identityKeyName)
		   : _keyIdentity (OutlookExpressIdentities::Key (), identityKeyName.c_str ())
		{}

		std::string GetUserName () const { return _keyIdentity.GetStringVal ("Username"); }

	private:
		RegKey::Existing	_keyIdentity;
	};

	class CoopUser: public ReliSoftUser
	{
	public:
		CoopUser (char const *keyName)
		   : ReliSoftUser (ApplicationName),
			 _keyBranch (ReliSoftUser::Key (), keyName)
		{}

		RegKey::Handle & Key () { return _keyBranch; }
		RegKey::Handle const & Key () const { return _keyBranch; }

	private:
		RegKey::New			_keyBranch;
	};

	// HKEY_CURRENT_USER
	//      Software
	//          Reliable Software
	//              Code Co-op
	//                  Branches
	class Branches : public CoopUser
	{
	public:
		Branches ()
			: CoopUser ("Branches")
		{}
	};

	// HKEY_CURRENT_USER
	//      Software
	//          Reliable Software
	//              Code Co-op
	//                  Preferences
	class Preferences : public CoopUser
	{
	public:
		Preferences ()
			: CoopUser ("Preferences")
		{}
	};

	// HKEY_CURRENT_USER
	//      Software
	//          Reliable Software
	//              Code Co-op
	//                  User
	class UserInfo : public CoopUser
	{
	public:
		UserInfo ()
			: CoopUser ("User")
		{}

		void SetUserName (char const * userName) { Key ().SetValueString ("Name", userName); }
		void SetLicense (char const * license) { Key ().SetValueString ("License", license); }

		std::string GetName () { return Key ().GetStringVal ("Name"); }
		std::string GetEmail () { return Key ().GetStringVal ("Email"); }
		std::string GetPhone () { return Key ().GetStringVal ("Phone"); }
		std::string GetLicense () { return Key ().GetStringVal ("License"); }
		std::string GetUserId () { return Key ().GetStringVal ("Location"); }
	};

	// HKEY_CURRENT_USER
	//      Software
	//          Reliable Software
	//              Code Co-op
	//                  Preferences
	//						Editor
	class EditorPrefs : public Preferences
	{
	public:
		EditorPrefs ()
			: _keyEditor (Preferences::Key (), "Editor")
		{}

		RegKey::Handle & Key () { return _keyEditor; }
		RegKey::Handle const & Key () const { return _keyEditor; }

	private:
		RegKey::New	_keyEditor;
	};

	// HKEY_CURRENT_USER
	//      Software
	//          Reliable Software
	//              Code Co-op
	//                  Preferences
	//						Wiki
	class WikiPrefs : public Preferences
	{
	public:
		WikiPrefs ()
			: _keyWiki (Preferences::Key (), "Wiki")
		{}

		RegKey::Handle & Key () { return _keyWiki; }
		RegKey::Handle const & Key () const { return _keyWiki; }

	private:
		RegKey::New	_keyWiki;
	};

	// ------------------
	// Differ keys
	// ------------------

	// HKEY_CURRENT_USER
	//      Software
	//          Reliable Software
	//              Code Co-op
	//                  Preferences
	//						Differ
	class DifferPrefs : public Preferences
	{
	public:
		DifferPrefs ()
			: _keyDiffer (Preferences::Key (), "Differ")
		{}

		RegKey::Handle & Key () { return _keyDiffer; }
		RegKey::Handle const & Key () const { return _keyDiffer; }

	private:
		RegKey::New	_keyDiffer;
	};

	// HKEY_LOCAL_USER
	//      Software
	//          Reliable Software
	//              Code Co-op
	//                  Preferences
	//						Differ
	//							Font
	class DifferFont : public DifferPrefs
	{
	public:
		DifferFont ()
			: _keyFont (DifferPrefs::Key (), "Font")
		{}

		RegKey::Handle & Key () { return _keyFont; }
		RegKey::Handle const & Key () const { return _keyFont; }

	private:
		RegKey::New	_keyFont;
	};
	
	// HKEY_CURRENT_USER
	//      Software
	//          Reliable Software
	//              Code Co-op
	//                  Preferences
	//						Merger
	class MergerPrefs : public Preferences
	{
	public:
		MergerPrefs ()
			: _keyMerger (Preferences::Key (), "Merger")
		{}

		RegKey::Handle & Key () { return _keyMerger; }
		RegKey::Handle const & Key () const { return _keyMerger; }

	private:
		RegKey::New	_keyMerger;
	};

	// HKEY_LOCAL_USER
	//      Software
	//          Reliable Software
	//              Code Co-op
	//                  Preferences
	//						Differ
	//							Alternative
	class DifferAlternative : public DifferPrefs
	{
	public:
		DifferAlternative ()
			: _keyAlternative (DifferPrefs::Key (), "Alternative")
		{}

		RegKey::Handle & Key () { return _keyAlternative; }
		RegKey::Handle const & Key () const { return _keyAlternative; }

	private:
		RegKey::New	_keyAlternative;
	};
	
	// HKEY_LOCAL_USER
	//      Software
	//          Reliable Software
	//              Code Co-op
	//                  Preferences
	//						Merger
	//							Alternative
	class MergerAlternative : public MergerPrefs
	{
	public:
		MergerAlternative ()
			: _keyAlternative (MergerPrefs::Key (), "Alternative")
		{}

		RegKey::Handle & Key () { return _keyAlternative; }
		RegKey::Handle const & Key () const { return _keyAlternative; }

	private:
		RegKey::New	_keyAlternative;
	};
	
	// HKEY_LOCAL_USER
	//      Software
	//          Reliable Software
	//              Code Co-op
	//                  Preferences
	//						Editor
	//							Alternative
	class EditorAlternative : public EditorPrefs
	{
	public:
		EditorAlternative ()
			: _keyAlternative (EditorPrefs::Key (), "Alternative")
		{}

		RegKey::Handle & Key () { return _keyAlternative; }
		RegKey::Handle const & Key () const { return _keyAlternative; }

	private:
		RegKey::New	_keyAlternative;
	};
	
	
	// HKEY_LOCAL_USER
	//      Software
	//          Reliable Software
	//              Code Co-op
	//                  Preferences
	//						Differ
	//							Find
	class DifferFind : public DifferPrefs
	{
	public:
		DifferFind ()
			: _keyFind (DifferPrefs::Key (), "Find")
		{}

		RegKey::Handle & Key () { return _keyFind; }
		RegKey::Handle const & Key () const { return _keyFind; }

	private:
		RegKey::New	_keyFind;
	};
	
	
	// HKEY_LOCAL_USER
	//      Software
	//          Reliable Software
	//              Code Co-op
	//                  Preferences
	//						Differ
	//							Find
	//                             List	
	class DifferFindList : public DifferFind
	{
	public:
		DifferFindList ()
			: _keyList (DifferFind::Key (), "List")
		{}

		RegKey::Handle & Key () { return _keyList; }
		RegKey::Handle const & Key () const { return _keyList; }

	private:
		RegKey::New	_keyList;
	};

	// ---------------------
	// Dispatcher keys
	// ---------------------

	// HKEY_CURRENT_USER
	//      Software
	//          Reliable Software
	//              Dispatcher
	class DispatcherUserRoot : public ReliSoftUser
	{
	public:
		DispatcherUserRoot ()
			: ReliSoftUser (DispatcherSection)
		{}
	};

	// HKEY_CURRENT_USER
	//      Software
	//          Reliable Software
	//              Dispatcher
	//					<email key name>
	class DispatcherEmail : public DispatcherUserRoot
	{
	public:
		DispatcherEmail (std::string const & emailKeyName)
			: _emailKey (ReliSoftUser::Key (), emailKeyName)
		{}

		RegKey::Handle & Key () { return _emailKey; }
		RegKey::Handle const & Key () const { return _emailKey; }

	protected:
		RegKey::New	_emailKey;
	};

	class DispatcherEmailAutoHandle: public RegKey::New
	{
	public:
		DispatcherEmailAutoHandle (std::string const & keyName, std::string const & subKeyName)
			: RegKey::New (RegKey::CurrentUser (), 
			std::string ("Software\\Reliable Software\\Dispatcher\\").append (keyName).append ("\\").append (subKeyName))
		{}
	};

	class DispatcherEmailTest : public DispatcherUserRoot
	{
	public:
		DispatcherEmailTest (std::string const & emailKeyName)
			: _emailKey (ReliSoftUser::Key (), emailKeyName)
		{}

		bool Exists () const { return _emailKey.Exists (); }
		bool ExistsSubKey (std::string const & subKeyName) const 
		{
			if (!Exists ())
				return false;
			RegKey::Check subKey (_emailKey, subKeyName);
			return subKey.Exists ();
		}
	protected:
		RegKey::Check	_emailKey;
	};

	// HKEY_CURRENT_USER
	//      Software
	//          Reliable Software
	//              Dispatcher
	//					Preferences
	class DispatcherPrefs : public DispatcherUserRoot
	{
	public:
		DispatcherPrefs ()
			: _keyBranch (ReliSoftUser::Key (), "Preferences")
		{}

		RegKey::Handle & Key () { return _keyBranch; }
		RegKey::Handle const & Key () const { return _keyBranch; }

		bool GetVersion (unsigned long & major, unsigned long & minor)
		{
			if (!_keyBranch.GetValueLong ("MajorVersionNumber", major))
				return false;
			if (!_keyBranch.GetValueLong ("MinorVersionNumber", minor))
				return false;
			return true;
		}
		void SetVersion (unsigned long major, unsigned long minor)
		{
			_keyBranch.SetValueLong ("MajorVersionNumber", major);
			_keyBranch.SetValueLong ("MinorVersionNumber", minor);
		}

	protected:
		RegKey::New	_keyBranch;
	};

	class DispatcherPrefTest: public ReliSoftUser
	{
	public:
		DispatcherPrefTest () 
			: ReliSoftUser (DispatcherSection),
			  _preferences (ReliSoftUser::Key (), "Preferences")
		{}

		bool Exists () const { return _preferences.Exists (); }
		RegKey::Handle & Key () { return _preferences; }
		RegKey::Handle const & Key () const { return _preferences; }

	private:
		RegKey::Check _preferences;
	};

	//	HKEY_LOCAL_MACHINE
	//		System
	//			CurrentControlSet
	//				Control
	//					Session Manager
	//						Environment

	class EnvironmentVariables
	{
	public:
		EnvironmentVariables ()
			: _keyRoot (HKEY_LOCAL_MACHINE),
			  _keySystem (_keyRoot, "System"),
			  _keyCurrentControlSet (_keySystem, "CurrentControlSet"),
			  _keyControl (_keyCurrentControlSet, "Control"),
			  _keySessionManager (_keyControl, "Session Manager"),
			  _keyEnvironment (_keySessionManager, "Environment")
		{}

		RegKey::Handle & Key () { return _keyEnvironment; }
		RegKey::Handle const & Key () const { return _keyEnvironment; }

	private:
		RegKey::Root		_keyRoot;
		RegKey::Existing	_keySystem;
		RegKey::Existing	_keyCurrentControlSet;
		RegKey::Existing	_keyControl;
		RegKey::Existing	_keySessionManager;
		RegKey::Existing	_keyEnvironment;
	};

	//	HKEY_CURRENT_USER
	//		Environment

	class UserEnvironmentVariables
	{
	public:
		UserEnvironmentVariables ()
			: _keyRoot (HKEY_CURRENT_USER),
			  _keyEnvironment (_keyRoot, "Environment")
		{}

		RegKey::Handle & Key () { return _keyEnvironment; }
		RegKey::Handle const & Key () const { return _keyEnvironment; }

	private:
		RegKey::Root		_keyRoot;
		RegKey::Existing	_keyEnvironment;
	};
}

#endif
