#if !defined (RUNDIAGPAGE_H)
#define RUNDIAGPAGE_H
// ---------------------------------
// (c) Reliable Software 1999 - 2008
// ---------------------------------

#include "BaseWizardHandler.h"
#include "resource.h"

#include <Ctrl/Button.h>
#include <Ctrl/Edit.h>

// assume that this dialog is executed when 
// a computer is to be configured as E-mail Peer or Hub
class RunDiagHandler : public BaseWizardHandler
{
public:
	RunDiagHandler (ConfigDlgData & configDlgData,
				 unsigned pageId)
		: BaseWizardHandler (configDlgData, pageId),
		  _previousHubId (configDlgData.GetOldHubId ()),
		  _isDiagPerformed (false),
		  _isDontAsk (false)
	{}

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	void OnSetActive (long & result);

protected:
	void RetrieveData (bool acceptPage) throw (Win::Exception);
	bool Validate () const;
	long ChooseNextPage () const throw () { return IDD_WIZARD_AUTOMATIC; }

private:
	bool ValidateEmailAddress (std::string & emailAddress) const;

private:
	std::string		_previousHubId;
	bool			_isDiagPerformed;
	bool			_isDontAsk;
	Win::CheckBox	_dontAsk;
	Win::Edit		_emailAddr;
	Win::Edit		_status;
	Win::Button		_diag;

};

#endif
