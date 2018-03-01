#if !defined (PROJECTDATA_H)
#define PROJECTDATA_H
//----------------------------------
// (c) Reliable Software 1998 - 2009
//----------------------------------

#include "Address.h"

#include <File/Path.h>

namespace Project
{
	class Data
	{
	public:
		Data ()
			: _projectId (-1)
		{}

		void SetProjectName (std::string const & name) { _projectName.Assign (name); }
		void SetProjectId (int projectId) { _projectId = projectId; }
		void SetRootPath (FilePath const & source) { _rootPath.Change (source); }
		void SetRootPath (std::string const & source)
		{
			TrimmedString path (source);
			_rootPath.Change (path);
		}
		void SetDataPath (FilePath const & data) { _dataPath.Change (data); }
		void SetInboxPath (FilePath const & inbox) { _inboxPath.Change (inbox);	}
		void Clear ()
		{
			_projectId = -1;
			_rootPath.Clear ();
			_dataPath.Clear ();
			_inboxPath.Clear ();
			_projectName.Assign ("");
		}

		int GetProjectId () const { return _projectId; }
		std::string GetProjectIdString () const { return ToString (_projectId); }
		std::string const & GetProjectName () const { return _projectName; }
		FilePath const & GetDataPath () const { return _dataPath; }
		char const * GetRootDir () const { return _rootPath.GetDir (); }
		FilePath const & GetRootPath () const { return _rootPath; }
		FilePath const & GetInboxDir () const { return _inboxPath; }
		bool IsValid () const
		{
			return !_projectName.empty () 
				&& !_rootPath.IsDirStrEmpty () 
				&& !_dataPath.IsDirStrEmpty ()
				&& !_inboxPath.IsDirStrEmpty ();
		}

	protected:
		int				_projectId;
		FilePath		_rootPath;
		FilePath		_dataPath;
		FilePath		_inboxPath;
		TrimmedString	_projectName;
	};
}

// Used in catalog only -- no need to trim hubId string
class ProjectUserData : public Project::Data
{
public:
	ProjectUserData ()
		: _isRemoved (false)
	{}

	std::string const & GetUserId () const { return _address.GetUserId (); }
	std::string const & GetHubId () const { return _address.GetHubId (); }
	bool IsRemoved () const { return _isRemoved; }
	void SetUserId (std::string const & userId) { _address.SetUserId (userId); }
	void SetHubId (std::string const & hubId) { _address.SetHubId (hubId); }
	void SetProjectName (std::string const & name)
	{
		Project::Data::SetProjectName (name);
		_address.SetProjectName (_projectName);
	}
	Address const & GetAddress () const
	{
		return _address;
	}
	void MarkRemoved () { _isRemoved = true; }

private:
	Address	_address; // warning: project name is duplicated in the parent
	bool	_isRemoved;
};


#endif
