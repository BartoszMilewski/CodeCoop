#if !defined (PROJECTBLUEPRINT_H)
#define PROJECTBLUEPRINT_H
//----------------------------------
// (c) Reliable Software 2005 - 2008
//----------------------------------

#include "ProjectData.h"
#include "ProjectOptions.h"
#include "MemberDescription.h"

class Catalog;
class NamedValues;

namespace Project
{
	class Blueprint
	{
	public:
		Blueprint (Catalog & catalog);
		~Blueprint ();

		bool IsValid () const;
		bool IsRootPathOk () const;
		void DisplayErrors (Win::Dow::Handle winOwner) const;

		Project::Data & GetProject () { return _project; }
		Project::Options & GetOptions () { return _options; }
		MemberDescription & GetThisUser () { return _thisUser; }

		std::string GetNamedValues ();
		void ReadNamedValues (NamedValues const & input);

		void Clear ();

		static bool IsRootPathWellFormed (FilePath const & rootPath);
		static bool DisplayPathErrors (FilePath const & rootPath, Win::Dow::Handle winOwner);

	private:
		bool IsProjectDefined () const;

	private:
		Catalog &			_catalog;
		Data				_project;
		Options				_options;
		MemberDescription	_thisUser;
	};
}

#endif
