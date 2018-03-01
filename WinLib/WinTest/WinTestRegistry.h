#if !defined (TOOLREGISTRY_H)
#define TOOLREGISTRY_H
//----------------------------
// (c) Reliable Software, 2004
//----------------------------
#include "ReliSoftReg.h"

namespace Registry
{
	class WinTestUser: public ReliSoftUser
	{
	public:
		WinTestUser (std::string const & appName, std::string const & keyName)
			: ReliSoftUser (appName.c_str ()),
			_branch (ReliSoftUser::Key (), keyName.c_str ())
		{}
		RegKey::Handle const & Key () const { return _branch; }
	private:
		RegKey::New	_branch;
	};
}

class WinTestRegistry
{
public:
	WinTestRegistry (std::string const & appName)
		:_appName (appName)
	{}
	void ReadPlacement (Win::Placement & placement, 
		std::string const & windowName = std::string ("MainWindow"))
	{
		Registry::WinTestUser userWin (_appName, windowName);
		RegKey::ReadWinPlacement (placement, userWin.Key ());
	}
	void SavePlacement (Win::Placement const & placement, 
		std::string const & windowName = std::string ("MainWindow"))
	{
		Registry::WinTestUser userWin (_appName, windowName);
		RegKey::Handle key = userWin.Key ();
		RegKey::SaveWinPlacement (placement,key );
	}
private:
	std::string _appName;
};

extern WinTestRegistry TheRegistry;

#endif

