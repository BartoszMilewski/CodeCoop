#if !defined (BACKUPSUMMARYDLG_H)
#define BACKUPSUMMARYDLG_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include <Win/Dialog.h>
#include <Ctrl/ListView.h>

class Catalog;

class BackupSummary
{
private:
	struct ProjectInfo
	{
		ProjectInfo (std::string const & name,
					 std::string const & id,
					 std::string const & rootPath)
			: _name (name),
			  _id (id),
			  _rootPath (rootPath)
		{}

		std::string	_name;
		std::string	_id;
		std::string	_rootPath;
	};

public:
	BackupSummary (Catalog & catalog,
				  std::vector<int> const & unverifiedProjectIds,
				  std::vector<int> const & notRepairedProjectIds);

	class ProjectSequencer
	{
	public:
		ProjectSequencer (std::vector<ProjectInfo> const & projects)
			: _cur (projects.begin ()),
			  _end (projects.end ())
		{}

		bool AtEnd () const { return _cur == _end; }
		void Advance () { ++_cur; }

		char const * GetProjectName () const { return (*_cur)._name.c_str (); }
		char const * GetProjectId () const { return (*_cur)._id.c_str (); }
		char const * GetRootPath () const { return (*_cur)._rootPath.c_str (); }

	private:
		std::vector<ProjectInfo>::const_iterator	_cur;
		std::vector<ProjectInfo>::const_iterator	_end;
	};

	ProjectSequencer GetUnverifiedProjects () const
	{
		return ProjectSequencer (_unverifiedProjects);
	}
	ProjectSequencer GetUnrepairedProjects () const
	{
		return ProjectSequencer (_notRepairedProjects);
	}

private:
	std::vector<ProjectInfo>	_unverifiedProjects;
	std::vector<ProjectInfo>	_notRepairedProjects;
};

class BackupSummaryCtrl : public Dialog::ControlHandler
{
public:
	BackupSummaryCtrl (BackupSummary const & summary);

	bool OnInitDialog () throw (Win::Exception);

private:
	Win::ReportListing		_projectList;
	BackupSummary const &	_summary;
};

#endif
