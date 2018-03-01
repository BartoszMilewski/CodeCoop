//----------------------------------
// (c) Reliable Software 2003 - 2006
//----------------------------------

#include "precompiled.h"
#include "Commander.h"
#include "RichDumpWin.h"
#include "Serialize.h"

#include <Com/Shell.h>
#include <Ctrl/FileGet.h>
#include <File/Path.h>

Commander::Commander (Win::Dow::Handle hwnd, RichDumpWindow & display)
	: _hwnd (hwnd),
	  _display (display)
{}

void Commander::Monitor_Save ()
{
	FilePath userDesktopPath;
	ShellMan::VirtualDesktopFolder userDesktop;
	userDesktop.GetPath (userDesktopPath);
	FileGetter fileDlg;
	fileDlg.SetInitDir (userDesktopPath.GetDir ());
	fileDlg.SetFilter ("Plain Text (*.txt)\0*.txt\0\0", "*.txt");
	fileDlg.SetFileName ("DebugLog.txt");
	if (fileDlg.GetNewFile (_hwnd, "Save debug output as"))
	{
		FileSerializer out (fileDlg.GetPath ());
		std::string log = _display.GetString ();
		out.PutBytes (&log [0], log.length ());
	}
}

void Commander::Monitor_ClearAll ()
{
	_display.Select (0, Win::RichEdit::EndPos ());
	_display.ReplaceSelection ("");
}

void Commander::Monitor_Exit ()
{
	throw Win::ExitException (0);
}
