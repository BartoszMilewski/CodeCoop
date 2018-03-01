#if !defined (STANDALONEJOIN_H)
#define STANDALONEJOIN_H
// ----------------------------------
// (c) Reliable Software, 2002 - 2005
// ----------------------------------

#include "resource.h"
#include "JoinProjectData.h"
#include "StringOp.h"
#include "WizardHelp.h"

#include <Win/Win.h>
#include <Ctrl/PropertySheet.h>
#include <Ctrl/Button.h>
#include <Ctrl/Edit.h>
#include <Ctrl/ComboBox.h>

class Catalog;

class StandaloneJoinTemplateCtrl : public PropPage::WizardHandler
{
public:
	StandaloneJoinTemplateCtrl (JoinProjectData & joinData, unsigned pageId)
		: PropPage::WizardHandler (pageId, false),
		  _joinData (joinData)
	{}

	void OnHelp () const throw (Win::Exception)	{ OpenHelp (); }

protected:
		bool GoNext (long & nextPage)
		{
			Assert (!"Must be overwritten");
			return false;
		}
		bool GoPrevious () { return true; }

protected:
	JoinProjectData &	_joinData;
};

class StandaloneJoinCtrl : public StandaloneJoinTemplateCtrl
{
public:
	StandaloneJoinCtrl (JoinProjectData & joinData, NocaseSet const & projects, unsigned pageId)
		: StandaloneJoinTemplateCtrl (joinData, pageId),
		  _projectList (projects)
	{}
    bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	void OnSetActive (long & result) throw (Win::Exception);
	bool GoNext (long & result);
	void OnFinish (long & result) throw (Win::Exception);
private:
	Win::RadioButton	_local;
	Win::RadioButton	_nonLocal;
	Win::ComboBox		_projects;
	NocaseSet const &	_projectList;
};

class StandaloneJoinSourceCtrl : public StandaloneJoinTemplateCtrl
{
public:
	StandaloneJoinSourceCtrl (JoinProjectData & joinData, unsigned pageId)
		: StandaloneJoinTemplateCtrl (joinData, pageId)
	{}
	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	void OnSetActive (long & result) throw (Win::Exception);
protected:
	bool GoNext (long & result);
private:
	Win::Edit   _source;
	Win::Button	_browse;
};

class StandaloneJoinUserCtrl : public StandaloneJoinTemplateCtrl
{
public:
	StandaloneJoinUserCtrl (JoinProjectData & joinData, unsigned pageId)
		: StandaloneJoinTemplateCtrl (joinData, pageId)
	{}
	bool OnInitDialog () throw (Win::Exception);
	void OnSetActive (long & result) throw (Win::Exception);
	void OnFinish (long & result) throw (Win::Exception);
private:
	Win::Edit	  _name;
	Win::Edit	  _comment;
	Win::CheckBox _observer;
};

// wizard
class StandaloneJoinHandlerSet : public PropPage::HandlerSet
{
public:
	StandaloneJoinHandlerSet (JoinProjectData & joinData, NocaseSet const & projects);

private:
	StandaloneJoinCtrl		 _joinCtrl;
	StandaloneJoinSourceCtrl _sourceCtrl;
	StandaloneJoinUserCtrl	 _userCtrl;
};

#endif
