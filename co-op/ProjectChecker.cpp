//------------------------------------
//  (c) Reliable Software, 1999 - 2008
//------------------------------------

#include "precompiled.h"
#include "ProjectChecker.h"
#include "Model.h"
#include "VerifyReportDlg.h"
#include "Prompter.h"

#include <Ctrl/ProgressDialog.h>

void ProjectChecker::Verify (Progress::Meter & overallMeter, Progress::Meter & specificMeter)
{
	overallMeter.SetRange (0, 4);
	overallMeter.SetActivity ("Verifying project folders.");
	overallMeter.StepAndCheck ();
	_missingFolders = _model.FindMissingFolders (_report, specificMeter);

	overallMeter.SetActivity ("Verifying project files.");
	overallMeter.StepAndCheck ();
	_model.FindCorruptedFiles (_report,
							   _extraRepairNeeded,
							   specificMeter);
	overallMeter.SetActivity ("Checking project database consistency.");
	overallMeter.StepAndCheck ();
	_model.CheckConsistency (_report, specificMeter);

	// Revisit: get some verification data to be passed to DoProjectRepair
	//_model.VerifyHistory (specific);
	//_extraRepairNeeded = true; // <- temporary
	_hasBeenVerified = true;
}

bool ProjectChecker::IsFileRepairNeeded () const
{
	return !_report.IsEmpty () || _extraRepairNeeded;
}

bool ProjectChecker::Repair ()
{
	Assume(_hasBeenVerified, "Project repair called without prior verification");
	Project::Path relativePath (_model.GetFileIndex ());
	VerifyReportCtrl dlgCtrl (_report, _model.GetProjectName (), relativePath);
	ThePrompter.GetData (dlgCtrl);
	return _model.RecoverFiles (_report);
}
