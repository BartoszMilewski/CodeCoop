//----------------------------------
// (c) Reliable Software 2004 - 2007
//----------------------------------

#include "precompiled.h"
#include "InputSource.h"
#include "OutputSink.h"
#include "Catalog.h"
#include "PathRegistry.h"

#include <Win/Instance.h>
#include <Win/MsgLoop.h>
#include <Ctrl/ProgressDialog.h>
#include <Sys/Process.h>
#include <Com/Shell.h>

#include <CmdLineScanner.h>
#include <StringOp.h>

#include <fstream>

//
// Code Co-op Dispatcher recognizes the following command lines:
//
// -setup_conversion
//

CmdInputSource::CmdInputSource (char const * str)
	: _scanner (str), _isRestart (false)
{
	while (_scanner.Look () != CmdLineScanner::tokEnd)
	{
		Command ();
	}
}

void CmdInputSource::Command ()
{
	// switch
	if (_scanner.Look () != CmdLineScanner::tokSwitch)
		throw Win::Exception ("Command line error: Command should start with a dash or a slash");
	_scanner.Accept ();
	// command_name
	if (_scanner.Look () != CmdLineScanner::tokString)
		throw Win::Exception ("Command line error: A dash or slash must be followed by command name");

	std::string cmd (_scanner.GetString ());
	_scanner.Accept ();
	_commands.push_back (cmd);
}

static void SetupConversion (Win::Instance hInst);

int MainCmd (CmdInputSource & inputSource, Win::Instance hInst)
{
	int result = -1;
	try
	{
		for (CmdInputSource::Sequencer seq (inputSource); !seq.AtEnd (); seq.Advance ())
		{
			std::string const & cmd = seq.GetCommand ();
			if (IsNocaseEqual (cmd, "setup_conversion"))
			{
				SetupConversion (hInst);
			}
			else if (IsNocaseEqual (cmd, "restart"))
			{
				inputSource.SetRestart ();
			}
			// Else ignore commands that cannot be executed immediately
		}
	}
	catch (Win::Exception ex)
	{
		TheOutput.Display (ex);
		result = 0;
	}
	catch ( ... )
	{
		TheOutput.Display ("Unexpected error during command execution");
		result = 0;
	}
	return result;
}

static void SetupConversion (Win::Instance hInst)
{
	std::vector<std::string> errors;
	std::vector<std::string> logPaths;
	Catalog catalog;
	unsigned int projectCount = 0;
	for (ProjectSeq seq (catalog); !seq.AtEnd (); seq.Advance ())
	{
		projectCount++;
	}
	if (projectCount == 0)
		return;	// Nothing to convert

	FilePath programPath (Registry::GetProgramPath ());
	std::string coopPath (programPath.GetFilePath ("co-op.exe"));
	if (!File::Exists (coopPath.c_str ()))
	{
		Win::ClearError ();
		throw Win::Exception ("Cannot find Code Co-op.  Run Code Co-op intallation again.", coopPath.c_str ());
	}

	Win::MessagePrepro msgPrepro;
	Progress::MeterDialog meterDialog ("Code Co-op Dispatcher",
									   0,
									   msgPrepro,
									   false);	// Cannot cancel operation
	meterDialog.SetCaption ("Converting Code Co-op projects.");
	Progress::Meter & meter = meterDialog.GetProgressMeter ();
	meter.SetRange (0, projectCount, 1);
	// Convert each project by invoking Code Co-op and asking it to visit
	// project. This will permanently convert project database to the new format.
	for (ProjectSeq seq (catalog); !seq.AtEnd (); seq.Advance ())
	{
		std::string info (seq.GetProjectName ());
		info += " (";
		info += seq.GetProjectSourcePath ().GetDir ();
		info += "')";
		meter.SetActivity (info.c_str ());
		meter.StepIt ();
		std::string cmdLine (coopPath);
		cmdLine += " -Project_Visit ";
		cmdLine += ToString (seq.GetProjectId ());
		Win::ChildProcess coop (cmdLine);
		coop.SetCurrentFolder (programPath.GetDir ());
		coop.ShowNormal ();
		Win::ClearError ();
		if (coop.Create (20000)) // Give Code Co-op 20 sec to convert the project
		{
			// Code Co-op finished execution -- check if there were any errors
			FilePath dataBasePath (seq.GetProjectDataPath ());
			char const * logPath = dataBasePath.GetFilePath ("ConversionLog.txt");
			if (File::Exists (logPath))
			{
				// Code Co-op left behind conversion error log -- remember its path
				logPaths.push_back (logPath);
			}
		}
		else
		{
			// Cannot execute Code Co-op in that project
			std::string errInfo ("Cannot execute Code Co-op in the project ");
			errInfo += info;
			errors.push_back (errInfo);
		}
	}
	meter.Close ();

	if (errors.empty () && logPaths.empty ())
	{
		TheOutput.Display ("All project databases have been successfully converted.");
	}
	else
	{
		// Errors detected -- create conversion log
		FilePath userDesktopPath;
		ShellMan::VirtualDesktopFolder userDesktop;
		userDesktop.GetPath (userDesktopPath);
		OutStream out (userDesktopPath.GetFilePath ("ConversionLog.txt"));
		if (out.good ())
		{
			// Write errors
			typedef std::vector<std::string>::const_iterator strIter;
			for (strIter errIter = errors.begin (); errIter != errors.end (); ++errIter)
			{
				out << *errIter << std::endl;
			}

			// Write project conversion logs
			for (unsigned int i = 0; i < logPaths.size (); ++i)
			{
				std::string const & logPath = logPaths [i];
				if (File::Exists (logPath.c_str ()))
				{
					std::ifstream in;
					in.open (logPath.c_str ());
					if (in)
					{
						out << std::endl;
						char c;
						while (in.get (c))
							out.put (c);
					}
					in.close ();
					File::DeleteNoEx (logPath.c_str ());
				}
			}
			out.close ();
			std::string logInfo ("Project database conversion completed with errors. Please, review conversion log created on the Desktop.\n\n");
			logInfo += "(";
			logInfo += userDesktopPath.GetFilePath ("ConversionLog.txt");
			logInfo += ")";
			TheOutput.Display (logInfo.c_str (), Out::Error);
		}
	}
}
