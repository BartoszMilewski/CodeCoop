#if !defined (BASEWIZARDHANDLER_H)
#define BASEWIZARDHANDLER_H
// ----------------------------------
// (c) Reliable Software, 1999 - 2008
// ----------------------------------

#include "ConfigDlgData.h"

#include <Ctrl/PropertySheet.h>

// Base controller for all configuration wizard page controllers

class BaseWizardHandler : public PropPage::WizardHandler
{
public:
	// Called always before the current wizard page is shown
	// regardles of the navigation direction (next or previous).
	// NOTE: OnInitDialog for each wizard page controller is called only once when
	// the page dialog is displayed for the very first time.
	void OnSetActive (long & result) throw (Win::Exception);
	void OnCancel (long & result) throw (Win::Exception);
	void OnHelp () const throw (Win::Exception);

protected:
	BaseWizardHandler (ConfigDlgData & cfg, 
					unsigned pageId,
					PropPage::Wiz::Buttons buttons = PropPage::Wiz::NextBack)
		: PropPage::WizardHandler (pageId, true), // supports help
		  _wizardData (cfg),
	      _buttons (buttons)
	{}

private:
	bool GoNext (long & result);
	bool GoPrevious ();

protected:
	virtual long ChooseNextPage () const throw (Win::Exception) { return -1; }

	// data manipulation procedures
	virtual void RetrieveData (bool acceptPage) {}
	virtual bool Validate () const { return true; }

protected:
	ConfigDlgData &					_wizardData;
	BitFieldMask<ConfigData::Field>	_modified;

private:
	void AcceptChanges ();
	void DisregardChanges ();

private:
	PropPage::Wiz::Buttons _buttons;
};

#endif
