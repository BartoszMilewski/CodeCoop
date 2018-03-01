// ----------------------------------
// (c) Reliable Software, 1999 - 2008
// ----------------------------------

#include "precompiled.h"
#include "BaseWizardHandler.h"
#include "WizardHelp.h"

void BaseWizardHandler::OnSetActive (long & result) throw (Win::Exception)
{
	SetButtons (_buttons);
	result = 0;
}

bool BaseWizardHandler::GoNext (long & nextPage) throw (Win::Exception)
{
	RetrieveData (true);
	if (Validate ())
	{
		AcceptChanges ();
		nextPage = ChooseNextPage ();
		return true;
	}
	return false;
}

bool BaseWizardHandler::GoPrevious () throw (Win::Exception)
{
	RetrieveData (false);
	DisregardChanges ();
	return true;
}

void BaseWizardHandler::OnCancel (long & result) throw (Win::Exception) 
{ 
	RetrieveData (false); 
	result = FALSE;
}

void BaseWizardHandler::OnHelp () const throw (Win::Exception)
{
	OpenHelp ();
}

void BaseWizardHandler::AcceptChanges ()
{
	_wizardData.AcceptChangesTo (_modified);
}

void BaseWizardHandler::DisregardChanges ()
{
	_wizardData.DisregardChangesTo (_modified);
}
