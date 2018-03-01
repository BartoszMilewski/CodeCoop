#if !defined (AGENTS_H)
#define AGENTS_H
//----------------------------------
// (c) Reliable Software 2004 - 2005
//----------------------------------

class Permissions;
class Catalog;
class PathFinder;
namespace Project
{
	class Db;
}

class ThisUserAgent
{
public:
	ThisUserAgent (Permissions & userPermissions)
		: _userPermissions (userPermissions),
		  _descriptionChange (false)
	{}

	void HubIdChange (std::string const & newHubId) { _newHubId = newHubId; }
	void DescriptionChange (bool flag) { _descriptionChange = flag; }
	bool IsMyStateValid () const;
	bool IsReceiver () const;

	void XFinalize (Project::Db const & projectDb, Catalog & catalog, PathFinder & pathFinder, int curProjectId);
	void XRefreshUserData (Project::Db const & projectDb);

private:
	ThisUserAgent (ThisUserAgent const &);
	void operator = (ThisUserAgent const &);

private:
	Permissions	&	_userPermissions;
	std::string		_newHubId;
	bool			_descriptionChange;
};

#endif
