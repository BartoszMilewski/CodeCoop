#if !defined (SETUPREGISTRY_H)
#define SETUPREGISTRY_H
//------------------------------------
//  (c) Reliable Software, 1997 - 2008
//------------------------------------

#include "Registry.h"
#include "SetupParams.h"

#include <Sys/RegKey.h>
#include <Dbg/Assert.h>

// Our registry entries

// HKEY_LOCAL_MACHINE
//      Software
//          Reliable Software
//              Code Co-op

namespace Registry
{
	// Co-op keys

	// Top level
	// This is where we keep program path and catalog path
	class KeyCoop
	{
	public:
		KeyCoop (bool isUserSetup)
		   :_keyRoot (isUserSetup? HKEY_CURRENT_USER: HKEY_LOCAL_MACHINE),
			_keyMain (_keyRoot, "Software"),
			_keyReliSoft (_keyMain, "Reliable Software"),
			_keyCoop (_keyReliSoft, "Code Co-op")
		{}
		void SetProgramPath (char const * pgmPath) { _keyCoop.SetValueString ("ProgramPath", pgmPath); }
		std::string GetProgramPath () { return _keyCoop.GetStringVal ("ProgramPath"); }
		void SetCatalogPath (char const * path) { _keyCoop.SetValueString ("CatalogPath", path); }
		std::string GetCatalogPath () { return _keyCoop.GetStringVal ("CatalogPath"); }
		void SetSccServerName (char const * name) { _keyCoop.SetValueString ("SCCServerName", name); }
		void SetSccServerPath (char const * path) { _keyCoop.SetValueString ("SCCServerPath", path); }
		void SetPreviousSccProvider (char const * prev) { _keyCoop.SetValueString ("PreviousSccProvider", prev); }
		void SetCmdLineToolsPath (char const * path) { _keyCoop.SetValueString ("CmdLineToolsPath", path); }
		std::string GetCmdLineToolsPath () { return _keyCoop.GetStringVal ("CmdLineToolsPath"); }

		void DeleteSubKey (char const * subKey) { _keyCoop.DeleteSubKey (subKey); }
		void DeleteSubTree (char const * subKey)
		{
			RegKey::New root (_keyCoop, subKey);
			RegKey::DeleteTree (root);
			DeleteSubKey (subKey); 
		}
	private:
		RegKey::Root		_keyRoot;
		RegKey::Existing	_keyMain;
		RegKey::New			_keyReliSoft;
		RegKey::New			_keyCoop;
	};

	class CoopSubKeyCheck
	{
	public:
		CoopSubKeyCheck (bool isUserSetup, char const * subKeyName)
		   : _keyRoot (isUserSetup ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE),
			 _keyMain (_keyRoot, "Software"),
			 _keyReliSoft (_keyMain, "Reliable Software"),
			 _keyCoop (_keyReliSoft, "Code Co-op"),
			 _keyBranch (_keyCoop, subKeyName)
		{}

		bool Exists () const
		{
			return _keyMain.Exists () && _keyReliSoft.Exists () &&
				   _keyCoop.Exists () && _keyBranch.Exists ();
		}

		RegKey::Handle & Key () { return _keyBranch; }

	private:
		RegKey::Root	_keyRoot;
		RegKey::Check	_keyMain;
		RegKey::Check	_keyReliSoft;
		RegKey::Check	_keyCoop;
		RegKey::Check	_keyBranch;
	};

	class CoopSubKeyRo
	{
	public:
		CoopSubKeyRo (bool isUserSetup, char const * subKeyName)
		   : _keyRoot (isUserSetup ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE),
			 _keyMain (_keyRoot, "Software"),
			 _keyReliSoft (_keyMain, "Reliable Software"),
			 _keyCoop (_keyReliSoft, "Code Co-op"),
			 _keyBranch (_keyCoop, subKeyName)
		{}

		RegKey::Handle & Key () { return _keyBranch; }

	private:
		RegKey::Root		_keyRoot;
		RegKey::ReadOnly	_keyMain;
		RegKey::ReadOnly	_keyReliSoft;
		RegKey::ReadOnly	_keyCoop;
		RegKey::ReadOnly	_keyBranch;
	};

	// Dispatcher keys

	// Top-level
	class KeyDispatcher
	{
	public:
		KeyDispatcher (bool isUserSetup)
		   :_keyRoot (isUserSetup? HKEY_CURRENT_USER: HKEY_LOCAL_MACHINE),
			_keyMain (_keyRoot, "Software"),
			_keyReliSoft (_keyMain, "Reliable Software"),
			_keyDispatcher (_keyReliSoft, "Dispatcher")
		{}
		void DeleteSubKey (char const * subKey) { _keyDispatcher.DeleteSubKey (subKey); }
		void DeleteSubTree (char const * subKey) 
		{
			RegKey::New root (_keyDispatcher, subKey);
			RegKey::DeleteTree (root);
			DeleteSubKey (subKey); 
		}
	private:
		RegKey::Root		_keyRoot;
		RegKey::Existing  _keyMain;
		RegKey::Existing  _keyReliSoft;
		RegKey::New		_keyDispatcher;
	};

	class DispatchSubKeyCheck
	{
	public:
		DispatchSubKeyCheck (bool isUserSetup, char const * subKeyName)
		   :_keyRoot (isUserSetup? HKEY_CURRENT_USER: HKEY_LOCAL_MACHINE),
			_keyMain (_keyRoot, "Software"),
			_keyReliSoft (_keyMain, "Reliable Software"),
			_keyDispatcher (_keyReliSoft, "Dispatcher"),
			_keyBranch (_keyDispatcher, subKeyName)
		{}

		bool Exists () const
		{
			return _keyMain.Exists () && _keyReliSoft.Exists () &&
				   _keyDispatcher.Exists () && _keyBranch.Exists ();
		}

	private:
		RegKey::Root	_keyRoot;
		RegKey::Check _keyMain;
		RegKey::Check _keyReliSoft;
		RegKey::Check _keyDispatcher;
		RegKey::Check _keyBranch;
	};

	class DispatchSubKeyRo
	{
	public:
		DispatchSubKeyRo (bool isUserSetup, char const * subKeyName)
		   :_keyRoot (isUserSetup? HKEY_CURRENT_USER: HKEY_LOCAL_MACHINE),
			_keyMain (_keyRoot, "Software"),
			_keyReliSoft (_keyMain, "Reliable Software"),
			_keyDispatcher (_keyReliSoft, "Dispatcher"),
			_keyBranch (_keyDispatcher, subKeyName)
		{}

		RegKey::Handle & Key () { return _keyBranch; }
	private:
		RegKey::Root			_keyRoot;
		RegKey::ReadOnly	_keyMain;
		RegKey::ReadOnly	_keyReliSoft;
		RegKey::ReadOnly	_keyDispatcher;
		RegKey::ReadOnly	_keyBranch;
	};

	// System registry entries

	// HKEY_LOCAL_MACHINE
	//      Software
	//          Microsoft
	//              Windows
	//					CurrentVersion
	//						AppPaths
	class KeyAppPaths
	{
	public:
		KeyAppPaths ()
		   :_keyRoot (HKEY_LOCAL_MACHINE),
			_keyMain (_keyRoot, "Software"),
			_keyMicrosoft (_keyMain, "Microsoft"),
			_keyWindows (_keyMicrosoft, "Windows"),
			_keyCurVer (_keyWindows, "CurrentVersion"),
			_keyAppPaths (_keyCurVer, "App Paths"),
			_keyCodeCoop (_keyAppPaths, "Co-op.exe")
		{}

		RegKey::Handle & GetParentKey () { return _keyAppPaths; }
		char const * GetCoopAppPathsSubkeyName () const { return "Co-op.exe"; }

		RegKey::Handle & Key () { return _keyCodeCoop; }
		void SetAppPath (char const * pgmPath) { _keyCodeCoop.SetValueString ("", pgmPath); }

	private:
		RegKey::Root		_keyRoot;
		RegKey::Existing  _keyMain;
		RegKey::Existing  _keyMicrosoft;
		RegKey::Existing  _keyWindows;
		RegKey::Existing  _keyCurVer;
		RegKey::Existing  _keyAppPaths;
		RegKey::New       _keyCodeCoop;
	};


	// HKEY_CLASSES_ROOT
	//		classKeyName
	class ClassRootCheck
	{
	public:
		ClassRootCheck (char const * classKeyName)
		   : _keyRoot (HKEY_CLASSES_ROOT), 
			 _keyClass (_keyRoot, classKeyName)
		{}

		bool Exists () const
		{
			return _keyClass.Exists ();
		}

	private:
		RegKey::Root	_keyRoot;
		RegKey::Check _keyClass;
	};

	// HKEY_CLASSES_ROOT
	//		classKeyName
	class ClassRoot
	{
	public:
		ClassRoot (char const * classKeyName)
		   : _keyRoot (HKEY_CLASSES_ROOT), 
			 _keyClass (_keyRoot, classKeyName)
		{}

		RegKey::Handle & Key () { return _keyClass; }

	private:
		RegKey::Root	_keyRoot;
		RegKey::New	_keyClass;
	};

	// HKEY_CLASSES_ROOT
	//		classKeyName
	//			Shell
	//				Open
	//					Command
	class ClassShellOpenCmd
	{
	public:
		ClassShellOpenCmd (char const * classKeyName)
		   : _keyRoot (HKEY_CLASSES_ROOT),
		     _keyClass (_keyRoot, classKeyName),
			 _keyShell (_keyClass, "Shell"),
			 _keyOpen (_keyShell, "Open"),
			 _keyCmd (_keyOpen, "Command")
		{}

		RegKey::Handle & Key () { return _keyCmd; }

	private:
		RegKey::Root		_keyRoot;
		RegKey::Existing	_keyClass;
		RegKey::New       _keyShell;
		RegKey::New       _keyOpen;
		RegKey::New       _keyCmd;
	};

	// HKEY_CLASSES_ROOT
	//		classKeyName
	//			DefaultIcon
	class ClassIcon
	{
	public:
		ClassIcon (char const * classKeyName)
		   : _keyRoot (HKEY_CLASSES_ROOT),
		     _keyClass (_keyRoot, classKeyName),
			 _keyIcon (_keyClass, "DefaultIcon")
		{}

		RegKey::Handle & Key () { return _keyIcon; }

	private:
		RegKey::Root		_keyRoot;
		RegKey::Existing	_keyClass;
		RegKey::New       _keyIcon;
	};

	// HKEY_LOCAL_MACHINE
	//      Software
	//          Microsoft
	//              Windows
	//					CurrentVersion
	//						Uninstall
	//							Code Co-op
	class KeyUninstall
	{
	public:
		KeyUninstall (bool isUserSetup)
		   :_keyRoot (isUserSetup ? HKEY_CURRENT_USER: HKEY_LOCAL_MACHINE),
			_keyMain (_keyRoot, "Software"),
			_keyMicrosoft (_keyMain, "Microsoft"),
			_keyWindows (_keyMicrosoft, "Windows"),
			_keyCurVer (_keyWindows, "CurrentVersion"),
			_keyUninstall (_keyCurVer, "Uninstall"),
			_keyCodeCoop (_keyUninstall, "Code Co-op")
		{}

		RegKey::Handle & GetParentKey () { return _keyUninstall; }
		char const * GetCoopUninstallSubkeyName () const { return "Code Co-op"; }

		RegKey::Handle & Key () { return _keyCodeCoop; }

		void SetProgramDisplayName (char const * info) { _keyCodeCoop.SetValueString ("DisplayName", info); }
		void SetUninstallString (char const * str) { _keyCodeCoop.SetValueString ("UninstallString", str); }
		void SetDisplayIcon (char const * iconPath) { _keyCodeCoop.SetValueString ("DisplayIcon", iconPath); }
		void SetDisplayVersion (char const * version) { _keyCodeCoop.SetValueString ("DisplayVersion", version); }
		void SetCompanyName (char const * company) { _keyCodeCoop.SetValueString ("Publisher", company); }
		void SetSupportLink (char const * link) { _keyCodeCoop.SetValueString ("HelpLink", link); }
		void SetCompanyURL (char const * url) { _keyCodeCoop.SetValueString ("URLInfoAbout", url); }
		void SetProgramURL (char const * url) { _keyCodeCoop.SetValueString ("URLUpdateInfo", url); }
	private:
		RegKey::Root		_keyRoot;
		RegKey::Existing  _keyMain;
		RegKey::Existing  _keyMicrosoft;
		RegKey::Existing  _keyWindows;
		RegKey::Existing  _keyCurVer;
		RegKey::New		  _keyUninstall;
		RegKey::New       _keyCodeCoop;
	};

	class ProjectSeq
	{
	public:
		ProjectSeq (bool userSetup)
			: _allProjects (userSetup, "Projects"),
			  _keySeq (_allProjects.Key ())
		{}

		bool AtEnd () const { return _keySeq.AtEnd (); }
		void Advance () { _keySeq.Advance (); }
		unsigned long Count () const { return _keySeq.Count (); }

		std::string GetProjIdString () const { return _keySeq.GetKeyName (); }
		int  GetProjectId () const { return atoi (_keySeq.GetKeyName ().c_str ()); }
		void GetProjectName (std::string & name);
		void GetUserIdLabel (std::string & location);
		int GetUserId ();
		void GetProjectEmail (std::string & email);
		void GetProjectSourcePath (std::string & path);
		void GetProjectDataPath (std::string & path);
	private:
		CoopSubKeyRo	_allProjects;
		RegKey::Seq		_keySeq;
	};

	inline void ProjectSeq::GetProjectName (std::string & name)
	{
		RegKey::ReadOnly curProject (_allProjects.Key (), _keySeq.GetKeyName ());
		name.assign (curProject.GetStringVal ("Name").c_str ());
	}

	inline void ProjectSeq::GetUserIdLabel (std::string & location)
	{
		RegKey::ReadOnly curProject (_allProjects.Key (), _keySeq.GetKeyName ());
		location.assign (curProject.GetStringVal ("Location").c_str ());
	}

	inline int ProjectSeq::GetUserId ()
	{
		RegKey::ReadOnly curProject (_allProjects.Key (), _keySeq.GetKeyName ());
		unsigned long id;
		curProject.GetValueLong ("User Id", id);
		return int (id);
	}

	inline void ProjectSeq::GetProjectEmail (std::string & email)
	{
		RegKey::ReadOnly curProject (_allProjects.Key (), _keySeq.GetKeyName ());
		email.assign (curProject.GetStringVal ("Email").c_str ());
	}

	inline void ProjectSeq::GetProjectSourcePath (std::string & path)
	{
		RegKey::ReadOnly curProject (_allProjects.Key (), _keySeq.GetKeyName ());
		path.assign (curProject.GetStringVal ("Source Path").c_str ());
	}

	inline void ProjectSeq::GetProjectDataPath (std::string & path)
	{
		RegKey::ReadOnly curProject (_allProjects.Key (), _keySeq.GetKeyName ());
		path.assign (curProject.GetStringVal ("Data Path").c_str ());
	}

	inline std::string GetValidClusterRecipsKey (bool userSetup)
	{
		DispatchSubKeyRo settings (userSetup, "Settings");
		std::string keyName (settings.Key ().GetStringVal ("Valid Recipients Key"));
		if (keyName.empty ())
			return std::string ("Local Recipients 1");
		else
			return keyName;
	}

	class ClusterRecipSeq
	{
	public:
		ClusterRecipSeq (bool userSetup)
			: _clusterRecipRegKey (userSetup, GetValidClusterRecipsKey (userSetup).c_str ()),
			  _emailSeq (_clusterRecipRegKey.Key ()),
			  _atEnd (false)
		{
			FindNonEmptyEmailKey ();
		}
		bool AtEnd () const { return _atEnd; }
		void Advance ()
		{
			Assert (!AtEnd ());
			// structure of cluster recipients registry:
			// HKEY_Local_Machine / Dispatcher / Local Recipients
			//     email 1
			//         project name 1_1
			//             location 1_1_1, path 1_1_1
			//             ...
			//             location 1_1_N1, path 1_1_N1
			//         ...
			//         project name 1_M1
			//     ...
			//     email K

			// simply try advancing location seq
			_locationSeq->Advance ();
			if (!_locationSeq->AtEnd ())
				return;

			// try advancing project seq
			_projectSeq->Advance ();
			if (!FindNonEmptyProjectKey ())
			{
				// try advancing email seq
				_emailSeq.Advance ();
				FindNonEmptyEmailKey ();
			}
		}
		unsigned long Count () const { return _emailSeq.Count (); }

		std::string GetEmail () const { return _emailSeq.GetKeyName (); }
		std::string GetProjectName () const {	return _projectSeq->GetKeyName (); }
		char const * GetLocation () const { return _locationSeq->GetName ().c_str (); }
		char const * GetForwardPath () const { return _locationSeq->GetString ().c_str (); }

	private:
		bool FindNonEmptyProjectKey ()
		{
			while (!_projectSeq->AtEnd ())
			{
				_projectKey.reset (new RegKey::ReadOnly (*_emailKey.get (), 
													   _projectSeq->GetKeyName ()));
				_locationSeq.reset (new RegKey::ValueSeq (*_projectKey.get ()));
				if (!_locationSeq->AtEnd ())
				{
					return true;
				}
				_projectSeq->Advance ();
			}
			return false;
		}
		void FindNonEmptyEmailKey ()
		{
			while (!_emailSeq.AtEnd ())
			{
				_emailKey.reset (new RegKey::ReadOnly (_clusterRecipRegKey.Key (), 
													 _emailSeq.GetKeyName ()));
				_projectSeq.reset (new RegKey::Seq (*_emailKey.get ()));
				if (FindNonEmptyProjectKey ())
					return;

				_emailSeq.Advance ();
			}
			_atEnd = true;
		}

	private:

		DispatchSubKeyRo				_clusterRecipRegKey;
		RegKey::Seq						_emailSeq;
		std::unique_ptr<RegKey::Seq>		_projectSeq;
		std::unique_ptr<RegKey::ValueSeq>	_locationSeq;

		std::unique_ptr<RegKey::ReadOnly> _emailKey;
		std::unique_ptr<RegKey::ReadOnly> _projectKey;

		bool							_atEnd;
	};

}

#endif
