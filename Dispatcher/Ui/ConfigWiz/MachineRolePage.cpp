// ---------------------------------
// (c) Reliable Software 1999 - 2008
// ---------------------------------

#include "precompiled.h"
#include "MachineRolePage.h"
#include "EmailMan.h"
#include <Ctrl/Static.h>

bool MachineRoleHandler::OnInitDialog () throw (Win::Exception)
{
	_isHub.Init (GetWindow (), IDC_HUB);
	_isSat.Init (GetWindow (), IDC_SAT);

	if (_wizardData.GetNewTopology ().IsHub ())
		_isHub.Check ();
	else
		_isSat.Check ();

	_modified.Set (ConfigData::bitTopologyCfg);

	Win::StaticImage stc;
	stc.Init (GetWindow (), IDC_HUB_IMAGE);
	stc.ReplaceBitmapPixels (Win::Color (0xff, 0xff, 0xff));
	stc.Init (GetWindow (), IDC_SAT_IMAGE);
	stc.ReplaceBitmapPixels (Win::Color (0xff, 0xff, 0xff));
	stc.Init (GetWindow (), IDC_SAT2_IMAGE);
	stc.ReplaceBitmapPixels (Win::Color (0xff, 0xff, 0xff));
	return true;
}

void MachineRoleHandler::RetrieveData (bool acceptPage) throw (Win::Exception)
{
	if (!acceptPage)
		return;

	if (_isHub.IsChecked ())
	{
		if (_wizardData.GetNewTopology ().UsesEmail ())
			_wizardData.GetNewConfig ().MakeHubWithEmail ();
		else
			_wizardData.GetNewConfig ().MakeHubNoEmail ();
	}
	else
		_wizardData.GetNewConfig ().MakeSatellite ();
}

long MachineRoleHandler::ChooseNextPage () const throw ()
{
	if (_wizardData.GetNewTopology ().IsHub ())
	{
		if (_wizardData.IsUsedEmailAndLAN ())
			return IDD_WIZARD_HUB_EMAIL_SELECTION;
		else
			return IDD_WIZARD_HUB_ID; 
	}
	else
		return IDD_WIZARD_HUB_PATH;
}
