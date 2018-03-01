//------------------------------------
//  (c) Reliable Software, 1996 - 2008
//------------------------------------

#include "precompiled.h"

#include "UninstallCtrl.h"
#include "resource.h"
#include "Uninstaller.h"
#include "UninstallParams.h"
#include "OutputSink.h"
#include "GlobalMessages.h"
#include "SetupRegistry.h"
#include "RegKeys.h"
#include "PathRegistry.h"
#include "Catalog.h"
#include "UninstallDlg.h"
#include "ConfirmDefectDlg.h"
#include "Proxy.h"

#include <Win/Message.h>
#include <Ctrl/ProgressDialog.h>
#include <File/Dir.h>
#include <File/Path.h>
#include <Ex/WinEx.h>

UninstallController::UninstallController (Win::MessagePrepro * msgPrepro, bool isFullUninstall)
	: _msgPrepro (msgPrepro),
	  _timer (TIMER_ID),
	  _retryCount (0),
	  _defectToolFinished (false),
	  _isFullUninstall (isFullUninstall)
{}

UninstallController::~UninstallController ()
{}

bool UninstallController::OnCreate (Win::CreateData const * create, bool & success) throw ()
{
    try
    {
		TheOutput.SetParent (_h);
		_timer.Attach (_h);
		Win::UserMessage msg (UM_START_DEINSTALLATION);
		_h.PostMsg (msg);
		success = true;
    }
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
		success = false;
	}
	catch ( ... )
	{
		Win::ClearError ();
		TheOutput.Display ("Initialization -- Unknown Error", Out::Error, _h); 
		success = false;
	}
	return true;
}

bool UninstallController::OnDestroy () throw ()
{
	Win::Quit ();
	return true;
}

bool UninstallController::OnTimer (int id) throw ()
{
	Assert (id == TIMER_ID);
	_timer.Kill ();
	if (_defectToolFinished)
	{
		if (IsPublicInboxEmpty ())
		{
			_meterDialog->Close ();
			Uninstall ();
			return true;
		}
		else if (_retryCount < 2)
		{
			_retryCount++;
			Progress::Meter & meter = _meterDialog->GetProgressMeter ();
			meter.StepIt ();
			KeepWaiting ();
			return true;
		}
	}

	// External tool is still working or Dispatcher didn't send the defect scripts
	_meterDialog->Close ();
	TheOutput.Display ("Code Co-op uninstaller couldn't send defect scripts to some project members.\n"
		"You still may receive the synchronization scripts for some projects.\n"
		"Please, notify appropriate project administrators, to remove you from the project member list.",
		Out::Information,
		_h);
	Uninstall ();
	return true;
}

bool UninstallController::OnUserMessage (Win::UserMessage & msg) throw ()
{
	if (msg.GetMsg () == UM_START_DEINSTALLATION)
	{
		bool uninstalled = true;
		// Ask the user if he/she really wants to remove us
		if (UserWantsToRemoveUs ())
		{
			if (_isFullUninstall)
			{
				// Remove us completely
				uninstalled = CompleteUninstall ();
			}
			else
			{
				// Restore original Code Co-op version
				RestoreOriginalVersion ();
			}
		}

		if (uninstalled)
			Win::Quit ();
		return true;
	}
	else if (msg.GetMsg () == UM_PROGRESS_TICK)
	{
		// Defecting from all projects - external tool is still working
		Assert (_isFullUninstall);
		Progress::Meter & meter = _meterDialog->GetProgressMeter ();
		meter.StepIt ();
		return true;
	}
	else if (msg.GetMsg () == UM_TOOL_END)
	{
		// Defecting from all projects completed - continue removing Code Co-op
		Assert (_isFullUninstall);
		_timer.Kill ();
		_defectToolFinished = true;
		if (IsPublicInboxEmpty ())
		{
			_meterDialog->Close ();
			Uninstall ();
		}
		else
		{
			KeepWaiting ();
		}
		return true;
	}
	return false;
}

bool UninstallController::UserWantsToRemoveUs () const
{
	if (_isFullUninstall)
	{
		UninstallDlgCtrl ctrl;
		Dialog::Modal dlg (_h, ctrl);
		return dlg.IsOK ();
	}
	else
	{
		Out::Answer userChoice = TheOutput.Prompt (
			"You are about to remove the temporary update version\\n"
			"and restore the original program version.\n\n"
			"Do you want to continue?",
			Out::PromptStyle (Out::YesNo),
			_h);

		if (userChoice == Out::No)
			return false;

		std::string pgmPath = Registry::GetProgramPath ();
		if (pgmPath.empty () || !File::Exists (pgmPath.c_str ()))
		{
			TheOutput.Display ("Your Code Co-op installation is corrupted.\n"
							   "Please, run the Code Co-op Installer to recover.",
							   Out::Information,
							   _h);
			return false;
		}
	}
	return true;
}

// Returns true when Code Co-op uninstalled successfully
bool UninstallController::CompleteUninstall ()
{
	Assert (_isFullUninstall);
	// Check if user has privileges to run uninstaller
	if (UserCanRemoveUs ())
	{
		// Check if user defected from all projects
		if (UserDefectedFromAllProjects ())
		{
			Uninstall ();
		}
		else
		{
			ConfirmDefectDlgCtrl confirmDefectCtrl;
			Dialog::Modal confirDefectDlg (_h, confirmDefectCtrl);
			if (confirDefectDlg.IsOK ())
			{
				if (confirmDefectCtrl.IsDefectFromAllProjects ())
				{
					if (StartDefectTool ())
					{
						// Defect tool started - wait until it is done
						return false;
					}
					else
					{
						TheOutput.Display ("Code Co-op uninstaller could not automatically\n"
										   "remove you from all projects located on this computer.\n\n"
										   "Start Code Co-op, visit all projects and execute Project>Defect.\n"
										   "Once you are removed from all projects, run uninstaller again.",
										   Out::Information,
										   _h);
					}
				}
				else
				{
					// Uninstall without defecting
					Uninstall ();
				}
			}
		}
	}

	return true;
}

void UninstallController::RestoreOriginalVersion ()
{
	Assert (!_isFullUninstall);
	Uninstaller uninstaller;
	try
	{
		uninstaller.UninstallUpdate ();
		TheOutput.Display ("Your original Code Co-op "
			"installation has been successfully restored.",
			Out::Information,
			_h);
	}
	catch (Win::Exception ex)
	{
		TheOutput.Display (ex);
	}
	catch ( ... )
	{
		Win::ClearError ();
		TheOutput.Display ("Unknown error during Code Co-op de-installation.", Out::Error, _h);
	}
}

bool UninstallController::UserCanRemoveUs () const
{
	Assert (_isFullUninstall);
	bool canWriteToCoopMachineReg = Registry::CanWriteToMachineRegKey ();
	if (Registry::IsUserSetup ())
	{
		Registry::KeySccCheck sccKeyCheck;
		if (sccKeyCheck.IsCoopCurrentProvider () || sccKeyCheck.IsCoopRegisteredProvider ())
		{
			if (!canWriteToCoopMachineReg)
			{
				// user setup with SCC Provider registry set
				TheOutput.Display ("You have no write access to the 'SourceCodeControlProvider' branch of the registry.\n"
					"The Uninstaller is not able to remove Code Co-op settings from this branch.\n"
					"After uninstaller completes ask the Administrator to remove the Code Co-op settings manually.",
					Out::Information,
					_h);
				// continue anyway
			}
		}
	}
	else
	{
		// Admin setup
		if (!canWriteToCoopMachineReg)
		{
			TheOutput.Display ("You must be logged on as the Administrator\n"
							   "in order to run the Code Co-op uninstaller.",
							   Out::Information,
							   _h);
			return false;
		}
	}
	return true;
}

bool UninstallController::UserDefectedFromAllProjects ()
{
	Assert (_isFullUninstall);
	std::string catPath = Registry::GetCatalogPath ();
	if (!catPath.empty () && File::Exists (catPath.c_str ()))
	{
		Catalog catalog;
		ProjectSeq seq (catalog);
		return seq.AtEnd ();
	}
	return true;
}

// Returns true when DefectFromAll tool started
bool UninstallController::StartDefectTool ()
{
	Assert (_isFullUninstall);
	FilePath programPath (Registry::GetProgramPath ());
	std::string appletPath (programPath.GetFilePath ("DefectFromAll.exe"));
	char const * dispatcherPath = programPath.GetFilePath (DispatcherExeName);
	if (!File::Exists (dispatcherPath) || !File::Exists (appletPath))
	{
		// Don't bother to defect
		return false;
	}

	// Make sure that Dispatcher is running
	DispatcherProxy proxy;
	if (proxy.GetWin ().IsNull ())
	{
		// Running Dispatcher was not found -- start new instance
		Win::ChildProcess dispatcher (dispatcherPath);
		dispatcher.SetCurrentFolder (programPath.GetDir ());
		dispatcher.ShowNormal ();
		Win::ClearError ();
		dispatcher.Create (5000); // Give the Dispatcher 5 sec to launch
	}

	// Estimate how long it will take to defect from all projects
	Catalog catalog;
	unsigned toolTimeout = 35000;	// Safety margin
	int projectCount = 0;
	for (ProjectSeq seq (catalog); !seq.AtEnd (); seq.Advance ())
		++projectCount;

	toolTimeout += projectCount * 1000;	// 1 second per project
	_meterDialog.reset (new Progress::MeterDialog ("Code Co-op Uninstaller",
													_h,
													*_msgPrepro,
													false)); // Cannot cancel
	_meterDialog->SetCaption ("Defecting from all projects located on this computer.");
	Progress::Meter & meter = _meterDialog->GetProgressMeter ();
	meter.SetRange (0, projectCount + 1);
	meter.StepIt ();
	// Run tool that will defect from all projects
	std::string cmdLine ("\"");
	cmdLine += appletPath;
	cmdLine += "\" -d:\"";
	cmdLine += ToHexString (reinterpret_cast<unsigned>(_h.ToNative ()));
	cmdLine += "\" \"";
	Win::ChildProcess externalTool (cmdLine);
	externalTool.SetNoFeedbackCursor ();
	externalTool.ShowMinimizedNotActive ();
	externalTool.Create (3000); // Give the tool 3 sec to launch
	_timer.Set (toolTimeout);
	return true;
}

void UninstallController::Uninstall ()
{
	Assert (_isFullUninstall);
	Uninstaller uninstaller;
	try
	{
		uninstaller.PerformFullUninstall ();
		TheOutput.Display ("Code Co-op has been successfully removed from your computer.",
							Out::Information,
							_h);
	}
    catch (Win::Exception ex)
    {
        TheOutput.Display (ex);
    }
    catch ( ... )
    {
		Win::ClearError ();
        TheOutput.Display ("Unknown error during Code Co-op de-installation.", Out::Error, _h);
    }
}

bool UninstallController::IsPublicInboxEmpty () const
{
	Assert (_isFullUninstall);
	Assert (_meterDialog.get () != 0);
	FilePath dbPath (Registry::GetCatalogPath ());
	dbPath.DirDown ("PublicInbox");
	FileSeq seq (dbPath.GetFilePath ("*.snc"));
	return seq.AtEnd ();
}

void UninstallController::KeepWaiting ()
{
	Assert (_isFullUninstall);
	_timer.Set (10000);	// Wait another 10 seconds
	Assert (_meterDialog.get () != 0);
	Progress::Meter & meter = _meterDialog->GetProgressMeter ();
	meter.SetActivity ("Waiting for the Dispatcher to complete script dispatching.");
}
