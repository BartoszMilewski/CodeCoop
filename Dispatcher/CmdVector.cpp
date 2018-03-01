// ----------------------------------
// (c) Reliable Software, 2001 - 2007
// ----------------------------------

#include "precompiled.h"
#include "CmdVector.h"
#include "Commander.h"

// Dispatcher Commands

namespace Cmd
{
	const Cmd::Item<Commander> Table [] =
	{
	//	Command Name				Implementation						Predicate implementation				Short help string	
        { "Program_CoopUpdate",	    &Commander::Program_CoopUpdate,	    0,										"Check for Co-op update on the Internet" },
		{ "Program_CoopUpdateOptions",	&Commander::Program_CoopUpdateOptions,	0,								"Check for Co-op update on the Internet" },
		{ "Program_DistributorLicense",	&Commander::Program_DistributorLicense,	&Commander::can_Program_DistributorLicense,	"Transfer Distributor license" },
        { "Program_ConfigWizard",		&Commander::Program_ConfigWizard,		0,								"Step-by-step collaboration settings" },
        { "Program_Collaboration",	&Commander::Program_Collaboration,	0,										"Change collaboration settings" },
		{ "Program_EmailOptions",	&Commander::Program_EmailOptions,	&Commander::can_Program_EmailOptions,   "Change e-mail options" },
		{ "Program_Diagnostics",	&Commander::Program_Diagnostics,	0,										"Test program operation" },
        { "Program_About",			&Commander::Program_About,			0,										"Display About info" },
		{ "Program_Exit",			&Commander::Program_Exit,			0,										"Exit program" },

        { "View_GoUp",				&Commander::View_GoUp,				&Commander::can_View_GoUp,				"Go up" },
		{ "View_Quarantine",		&Commander::View_Quarantine,		&Commander::can_View_Quarantine,		"Switch to Quarantine View" },
		{ "View_AlertLog",			&Commander::View_AlertLog,			&Commander::can_View_AlertLog,			"Switch to Alert Log View" },
		{ "View_PublicInbox",		&Commander::View_PublicInbox,		&Commander::can_View_PublicInbox,		"Switch to Public Inbox View" },
        { "View_Members",			&Commander::View_Members,			&Commander::can_View_Members,			"Switch to Member View" },
        { "View_RemoteHubs",		&Commander::View_RemoteHubs,		&Commander::can_View_RemoteHubs,		"Switch to Remote Hubs View" },
		{ "View_Next",				&Commander::View_Next,				0,										"" },
		{ "View_Previous",			&Commander::View_Previous,			0,										"" },

		{ "All_ReleaseFromQuarantine",&Commander::All_ReleaseFromQuarantine,&Commander::can_All_ReleaseFromQuarantine,"Release all scripts from Quarantine" },
		{ "All_PullFromHub",		&Commander::All_PullFromHub,		&Commander::can_All_PullFromHub,		"Pull scripts from the hub" },
        { "All_DispatchNow",		&Commander::All_DispatchNow,		0,										"Refresh all projects and dispatch local scripts" },
        { "All_SendMail",			&Commander::All_SendMail,			&Commander::can_All_SendMail,			"Email all waiting scripts" },
        { "All_GetMail",			&Commander::All_GetMail,			&Commander::can_All_GetMail,			"Retrieve scripts from mail program" },
        { "All_Purge",				&Commander::All_Purge,				0,										"Purge addresses of local and/or satellite members" },
		{ "All_ViewAlertLog",		&Commander::All_ViewAlertLog,		&Commander::can_All_ViewAlertLog,		"View Alert Log" },
		{ "All_ClearAlertLog",		&Commander::All_ClearAlertLog,		&Commander::can_All_ClearAlertLog,		"Clear Alert Log" },

		{ "Selection_ReleaseFromQuarantine",&Commander::Selection_ReleaseFromQuarantine,&Commander::can_Selection_ReleaseFromQuarantine,"Release selected script(s) from Quarantine" },
        { "Selection_Details",		&Commander::Selection_Details,		&Commander::can_Selection_Details,		"Display selection details" },
		{ "Selection_Delete",		&Commander::Selection_Delete,		&Commander::can_Selection_Delete,		"Delete selected items" },

        { "Help_Contents",			&Commander::Help_Contents,			0,										"Help" },
        { "Window_Show",			&Commander::Window_Show,			0,										"" },
		{ 0,						0,									0,										0 }
	};
}
