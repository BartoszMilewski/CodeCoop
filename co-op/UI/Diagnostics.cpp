//----------------------------------
// (c) Reliable Software 2005 - 2007
//----------------------------------

#include "precompiled.h"
#include "Diagnostics.h"
#include "OutputSink.h"
#include "Resource.h"

#include <Com/Shell.h>
#include <File/File.h>

DiagnosticsRequest::DiagnosticsRequest (int currentProjectId)
	: _diagFileName ("CoopDiagnostics")
{
	SetVersion (true);
	SetOverwriteExisting (true);
	SetCatalog (true);
	if (currentProjectId != -1)
	{
		_diagFileName += ToString (currentProjectId);
		SetMembership (true);
		SetHistory (true);
	}
	else
	{
		SetMembership (false);
		SetHistory (false);
	}
	_diagFileName += ".txt";
	ShellMan::VirtualDesktopFolder userDesktop;
	userDesktop.GetPath (_path);
}

bool DiagnosticsRequest::IsXMLDump () const
{
	std::string::size_type dotPos = _diagFileName.rfind ('.');
	if (dotPos != std::string::npos)
	{
		std::string extension (_diagFileName.substr (dotPos));
		return IsNocaseEqual (extension, ".xml");
	}
	return false;
}

DiagnosticsCtrl::DiagnosticsCtrl (DiagnosticsRequest & dlgData)
	: SelectFileCtrl ("Save diagnostics file in: ", "Select folder where the diagnostic file will be saved"),
	  _dlgData (dlgData)
{}

bool DiagnosticsCtrl::OnInitDialog () throw (Win::Exception)
{
	SelectFileCtrl::OnInitDialog ();

	_fileName.SetText (_dlgData.GetTargetFile ());
	_path.SetText (_dlgData.GetTargetPath ());
	_overwriteExisting.Check ();

	Win::Dow::Handle dlgWin = GetWindow ();
	dlgWin.SetText ("Save Diagnostics");
	return true;
}

bool DiagnosticsCtrl::OnApply () throw ()
{
	if (SelectFileCtrl::OnApply ())
	{
		_dlgData.SetTargetPath (_path.GetString ());
		_dlgData.SetTargetFile (_fileName.GetString ());
		_dlgData.SetOverwriteExisting (_overwriteExisting.IsChecked ());
		return Dialog::ControlHandler::OnApply ();
	}
	return false;
}
// command line
// -Help_SaveDiagnostics catalog:"yes | no" membership:"yes | no" history:"yes | no" overwrite:"yes | no" target:"<diagnostics file path>"
bool DiagnosticsCtrl::GetDataFrom (NamedValues const & source)
{
	std::string target (source.GetValue ("target"));
	if (target.empty ())
		return false;

	PathSplitter splitter (target);
	std::string path (splitter.GetDrive ());
	path += splitter.GetDir ();
	_dlgData.SetTargetPath (path);
	std::string filename (splitter.GetFileName ());
	filename += splitter.GetExtension ();
	_dlgData.SetTargetFile (filename);
	std::string version (source.GetValue ("version"));
	std::string catalog (source.GetValue ("catalog"));
	std::string membership (source.GetValue ("membership"));
	std::string history (source.GetValue ("history"));
	std::string overwrite (source.GetValue ("overwrite"));
	_dlgData.SetVersion (version == "yes");
	_dlgData.SetCatalog (catalog == "yes");
	_dlgData.SetMembership (membership == "yes");
	_dlgData.SetHistory (history == "yes");
	_dlgData.SetOverwriteExisting (overwrite == "yes");
	return true;
}

