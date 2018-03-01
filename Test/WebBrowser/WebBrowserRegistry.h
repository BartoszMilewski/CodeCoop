#if !defined (WEBBROWSERREGISTRY_H)
#define WEBBROWSERREGISTRY_H
//----------------------------
// (c) Reliable Software, 2004
//----------------------------
#include "ReliSoftReg.h"

namespace Registry
{
	class WebBrowserUser: public ReliSoftUser
	{
	public:
		WebBrowserUser (std::string const & appName, std::string const & keyName)
			: ReliSoftUser (appName.c_str ()),
			_branch (ReliSoftUser::Key (), keyName.c_str ())
		{}
		RegKey::Handle const & Key () const { return _branch; }
	private:
		RegKey::New	_branch;
	};
}

class WebBrowserRegistry
{
public:
	WebBrowserRegistry (std::string const & appName)
		:_appName (appName)
	{}
	void ReadPlacement (Win::Placement & placement, 
		std::string const & windowName = std::string ("MainWindow"))
	{
		Registry::WebBrowserUser userWin (_appName, windowName);
		RegKey::ReadWinPlacement (placement, userWin.Key ());
	}
	void SavePlacement (Win::Placement const & placement, 
		std::string const & windowName = std::string ("MainWindow"))
	{
		Registry::WebBrowserUser userWin (_appName, windowName);
		RegKey::Handle key = userWin.Key ();
		RegKey::SaveWinPlacement (placement,key );
	}
private:
	std::string _appName;
};

extern WebBrowserRegistry TheRegistry;

#endif

