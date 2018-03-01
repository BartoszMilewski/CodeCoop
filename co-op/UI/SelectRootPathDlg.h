#if !defined (SELECTROOTPATHDLG_H)
#define SELECTROOTPATHDLG_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "Resource.h"
#include "ProjectPathClassifier.h"

#include <Win/Dialog.h>
#include <Ctrl/ListView.h>
#include <Ctrl/Combobox.h>
#include <Ctrl/Button.h>

class SelectRootData
{
public:
	SelectRootData (Project::PathClassifier::ProjectSequencer projects)
		: _projects (projects)
	{}

	void SetNewPrefix (std::string const & prefix) { _newPrefix = prefix; }
	std::string const & GetNewPrefix () const { return _newPrefix; }

	Project::PathClassifier::ProjectSequencer GetProjectSequencer () const { return _projects; }

private:
	Project::PathClassifier::ProjectSequencer	_projects;
	std::string									_newPrefix;
};

class SelectRootCtrl : public Dialog::ControlHandler
{
public:
	SelectRootCtrl (SelectRootData & data)
		: Dialog::ControlHandler (IDD_SELECT_ROOT_PATH),
		  _dlgData (data)
	{}

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

private:
	void RefreshPathList (std::string const & prefix);

private:
	Win::ReportListing		_pathList;
	Win::ComboBox			_driveSelector;
	Win::Button				_browseNetwork;
	SelectRootData &		_dlgData;
};

#endif
