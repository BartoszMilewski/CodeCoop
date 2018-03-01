// ----------------------------------
// (c) Reliable Software, 1999 - 2008
// ----------------------------------

#include "precompiled.h"
#include "ConfigWiz.h"
#include "ConfigDlgData.h"
#include "RestorePage.h"
#include "CoopConfigPage.h"
#include "MachineRolePage.h"
#include "HubPathPage.h"
#include "EmailTechnology.h"
#include "RunDiagPage.h"
#include "EmailOptionsPage.h"
#include "FinishPage.h"
#include "EmailConfig.h"
#include "EmailConfigData.h"
#include "EmailMan.h"
#include "Email.h"
#include "Prompter.h"
#include "AppInfo.h"
#include "Registry.h"

#include <Ctrl/PropertySheet.h>
#include <Win/Win.h>

bool ConfigWizard::Execute ()
{
	PropPage::HandlerSet hndlrSet ("Code Co-op Dispatcher Step-by-step Setup");

	RestoreHandler restoreHandler (_config, IDD_WIZARD_RESTORE);
	if (Registry::IsRestoredConfiguration ())
		hndlrSet.AddHandler (restoreHandler);

	CoopConfigHandler configHandler (_config,
									 IDD_WIZARD_INTRO,
									 !Registry::IsRestoredConfiguration ());
	hndlrSet.AddHandler (configHandler);

	MachineRoleHandler roleHandler (_config, IDD_WIZARD_ROLE_LAN);
	hndlrSet.AddHandler (roleHandler);

	MachineRoleHandler role2Handler (_config, IDD_WIZARD_ROLE_EMAIL_LAN);
	hndlrSet.AddHandler (role2Handler);

	SatelliteTransportsHandler satTransHandler (_config, IDD_WIZARD_HUB_PATH);
	hndlrSet.AddHandler (satTransHandler);

	HubTransportsHandler hubTransHandler (_config, IDD_WIZARD_HUB_ID);
	hndlrSet.AddHandler (hubTransHandler);

	EmailSelectionHandler emailSelectionHandler (_config, IDD_WIZARD_EMAIL_SELECTION);
	hndlrSet.AddHandler (emailSelectionHandler);

	HubEmailSelectionHandler hubEmailSelectionHandler (_config, IDD_WIZARD_HUB_EMAIL_SELECTION);
	hndlrSet.AddHandler (hubEmailSelectionHandler);

	EmailPop3SmtpHandler emailPop3SmtpHandler (_config, IDD_WIZARD_EMAIL_POP3SMTP);
	hndlrSet.AddHandler (emailPop3SmtpHandler);
	
	EmailGmailHandler emailGmailHandler (_config, IDD_WIZARD_EMAIL_GMAIL);
	hndlrSet.AddHandler (emailGmailHandler);

	RunDiagHandler runDiagHandler (_config, IDD_WIZARD_DIAGNOSTICS);
	hndlrSet.AddHandler (runDiagHandler);

	EmailOptionsHandler modeHandler (_config, IDD_WIZARD_AUTOMATIC);
	hndlrSet.AddHandler (modeHandler);

	FinishHandler finishHandler (_config, IDD_WIZARD_FINISH);
	hndlrSet.AddHandler (finishHandler);
	
	if (ThePrompter.GetWizardData (hndlrSet))
		return _config.AnalyzeChanges ();
	else
		return false;
}

bool EmailConfigWizard::Execute ()
{
	PropPage::HandlerSet hndlrSet ("Code Co-op Dispatcher E-mail Setup");

	EmailSelectionHandler emailSelectionHandler (_config, IDD_WIZARD_EMAIL_SELECTION, true); // is first page
	hndlrSet.AddHandler (emailSelectionHandler);

	EmailPop3SmtpHandler emailPop3SmtpHandler (_config, IDD_WIZARD_EMAIL_POP3SMTP);
	hndlrSet.AddHandler (emailPop3SmtpHandler);

	EmailGmailHandler emailGmailHandler (_config, IDD_WIZARD_EMAIL_GMAIL);
	hndlrSet.AddHandler (emailGmailHandler);

	RunDiagHandler runDiagHandler (_config, IDD_WIZARD_DIAGNOSTICS);
	hndlrSet.AddHandler (runDiagHandler);

	EmailOptionsHandler modeHandler (_config, IDD_WIZARD_AUTOMATIC);
	hndlrSet.AddHandler (modeHandler);

	FinishHandler finishHandler (_config, IDD_WIZARD_FINISH);
	hndlrSet.AddHandler (finishHandler);
	
	return ThePrompter.GetWizardData (hndlrSet);
}
