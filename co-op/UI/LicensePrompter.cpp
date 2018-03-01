//---------------------------------------------
// (c) Reliable Software 2005
//---------------------------------------------

#include "precompiled.h"
#include "LicensePrompter.h"
#include "Prompter.h"
#include "LicenseDlg.h"
#include "Registry.h"
#include "License.h"

bool LicensePrompter::Query (License const & curLicense, 
							 unsigned int trialDaysLeft,
							 DistributorLicense const & distribLicense)
{
	std::string status = GetLicenseStatus (curLicense, trialDaysLeft, distribLicense);
	// Purchase, register, keep evaluating dialog
	LicenseData  dlgData (curLicense.IsCurrentVersion (), status);
	LicenseCtrl ctrl (&dlgData);
	if (ThePrompter.GetData (ctrl))
	{
		if (dlgData.IsNewLicense ())
			_newLicense = dlgData.GetNewLicense ();
	}
	if (dlgData.IsNoNagging ())
	{
		// User requested to off nagging
		Registry::SetNagging (false);
	}
	else if (!Registry::IsNagging ())
	{
		// User didn't request to turn off nagging and naggign is turned off -- turn it on
		Registry::SetNagging (true);
	}
	return true;
}

std::string LicensePrompter::GetLicenseStatus (License const & curLicense, 
											   unsigned int trialDaysLeft,
											   DistributorLicense const & distribLicense) const
{
	std::string status;
	if (_context == "HasProgramExpired")
	{
		if (trialDaysLeft != 0)
		{
			status = "You have ";
			status += ToString (trialDaysLeft);
			status += " trial day(s) left in your evaluation period.\n\n"
					  "You may purchase a license over the Internet, enter your license information now, "
					  "or keep evaluating our product.";
		}
		else
		{
			status = "The evaluation period of this program has expired and ";
			if (curLicense.GetSeatCount () == 0)
			{
				status += "you do not have a valid license.";
			}
			else
			{
				status += "your current license is only valid for version ";
				status += ToString (curLicense.GetVersion ());
				status += " of Code Co-op.";
			}

			status += "\n\nYou may purchase a license over the Internet, enter your license information now, "
				"or keep evaluating. In that case, your status in the existing "
				"projects will be temporarily changed to \"Observer.\". "
				"You may restart the evaluation period by defecting from all projects, "
				"un-installing Code Co-op using the Control Panel and re-installing it again.";
		}
	}
	else if (_context == "Program_Licensing")
	{
		if (curLicense.IsCurrentVersion ())
		{
			status = "You have a valid license in this project. You may enter a new license now, "
					 "or quit by pressing the \"Continue\" button.";
			if (distribLicense.GetCount () != 0)
			{
				int left = distribLicense.GetCount () - distribLicense.GetNext ();
				status += "\n\nYou have ";
				status += ToString (left);
				status += " distribution licenses left out of total ";
				status += ToString (distribLicense.GetCount ());
				status += " licenses assigned to ";
				status += distribLicense.GetLicensee ();
			}
		}
		else if (curLicense.GetSeatCount () != 0)
		{
			status = "You have an old license for version ";
			status += ToString (curLicense.GetVersion ());
			status += " of Code Co-op.\n\n"
					  "You may purchase an upgrade over the Internet, enter your new license now, "
					  "or keep evaluating our product.";
		}
		else
		{
			status = "You may purchase a license over the Internet, enter your license information now, "
					 "or keep evaluating our product.";
		}
	}
	else if (_context == "Project_New" || _context == "Selection_Branch")
	{
		status = "You cannot create a new project, because your evaluation period has ended and ";
		if (curLicense.GetSeatCount () == 0)
		{
			status += "you do not have a valid license.";
		}
		else
		{
			status += "your current license is only valid for version ";
			status += ToString (curLicense.GetVersion ());
			status += " of Code Co-op.";
		}
		status += "\n\nYou may purchase a license over the Internet, enter your license information now, "
				  "or keep evaluating our product.";
	}
	return status;
}