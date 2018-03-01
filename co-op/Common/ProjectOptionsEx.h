#if !defined (PROJECTOPTIONSEX_H)
#define PROJECTOPTIONSEX_H
//----------------------------------
// (c) Reliable Software 2005 - 2007
//----------------------------------

#include "ProjectOptions.h"

namespace Encryption { class KeyMan; }
class Catalog;

namespace Project
{
	class Db;

	class OptionsEx : public Options
	{
	public:
		OptionsEx (Project::Db const & projectDb, 
				   Encryption::KeyMan const & keyMan,
				   Catalog & catalog);

		std::string const & GetCaption () const { return _caption; }

		bool MayBecomeDistributor () const { return _mayBecomeDistributor; }
		std::string const & GetDistributorLicensee () const { return _distributorLicensee; }
		unsigned GetSeatTotal () const { return _seatsTotal; }
		unsigned GetSeatsAvailable () const { return _seatsAvailable; }

		void SetEncryptionKey (std::string const & key) { _newKey = key; }

		std::string const & GetEncryptionOriginalKey () const;
		std::string const & GetEncryptionCommonKey () const;
		std::string const & GetEncryptionKey () const { return _newKey; }

		bool IsAutoInvite () const { return _isAutoInvite; }
		std::string const & GetAutoInviteProjectPath () const { return _autoInvitePath; }

		void SetAutoInvite (bool flag) { _isAutoInvite = flag; }
		void SetAutoInviteProjectPath (std::string const & path) { _autoInvitePath = path; }

		bool ValidateAutoInvite (Win::Dow::Handle dlgWin) const;

		static bool ValidateAutoInvite (std::string const & autoInvitePath,
										Catalog & catalog,
										Win::Dow::Handle dlgWin);

	private:
		Catalog	&					_catalog;
		Encryption::KeyMan const &	_keyMan;
		std::string					_caption;
		std::string					_distributorLicensee;
		unsigned					_seatsTotal;
		unsigned					_seatsAvailable;
		bool						_mayBecomeDistributor;
		bool						_isAutoInvite;
		std::string					_newKey;
		std::string					_autoInvitePath;
	};

}

#endif
