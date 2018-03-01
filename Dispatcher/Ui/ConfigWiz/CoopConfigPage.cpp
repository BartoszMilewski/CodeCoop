// ----------------------------------
// (c) Reliable Software, 1999 - 2008
// ----------------------------------

#include "precompiled.h"
#include "CoopConfigPage.h"
#include "EmailMan.h"
#include "resource.h"

bool CoopConfigHandler::OnInitDialog () throw (Win::Exception)
{
	_standalone.Init (GetWindow (), IDC_STANDALONE);
	_onlyLAN.Init (GetWindow (), IDC_LAN);
	_onlyEmail.Init (GetWindow (), IDC_EMAIL);
	_emailAndLAN.Init (GetWindow (), IDC_BOTH);

	_modified.Set (ConfigData::bitTopologyCfg);

	if (_wizardData.IsUsedOnlyEmail ())
		_onlyEmail.Check ();
	else if (_wizardData.IsUsedOnlyOnLAN ())
		_onlyLAN.Check ();
	else if (_wizardData.IsUsedEmailAndLAN ())
		_emailAndLAN.Check ();
	else if (_wizardData.IsUsedStandalone ())
		_standalone.Check ();
	else
		_onlyEmail.Check ();	// Default is e-mail peer

	return true;
}

void CoopConfigHandler::RetrieveData (bool acceptPage) throw (Win::Exception)
{
	if (acceptPage)
	{
		if (_standalone.IsChecked () && _standalone.IsVisible ())
			_wizardData.UseStandalone ();
		else if (_onlyLAN.IsChecked () && _onlyLAN.IsVisible ())
			_wizardData.UseOnlyOnLAN ();
		else if (_onlyEmail.IsChecked () && _onlyEmail.IsVisible ())
			_wizardData.UseOnlyEmail ();
		else if (_emailAndLAN.IsVisible ())
			_wizardData.UseEmailAndLAN ();
	}
}

long CoopConfigHandler::ChooseNextPage () const throw (Win::Exception)
{
	if (_wizardData.IsUsedStandalone ())
		return IDD_WIZARD_FINISH;
	else if (_wizardData.IsUsedOnlyOnLAN ())
		return IDD_WIZARD_ROLE_LAN;
	else if (_wizardData.IsUsedOnlyEmail ())
		return IDD_WIZARD_EMAIL_SELECTION;
	else
		return IDD_WIZARD_ROLE_EMAIL_LAN;
}
