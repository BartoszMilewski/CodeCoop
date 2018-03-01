//------------------------------------
//  (c) Reliable Software, 2005 - 2008
//------------------------------------

#include "precompiled.h"
#include "ReportDlg.h"
#include "OutputSink.h"
#include "Global.h"

#include <Com/Shell.h>
#include <File/File.h>

ReportRequest::ReportRequest (int currentProjectId, Table::Id tableId)
	: _reportFileName (Table::GetName (tableId)),
	  _tableId (tableId)
{
	_reportFileName += " Report";
	if (currentProjectId != -1)
	{
		_reportFileName += ToString (currentProjectId);
	}
	_reportFileName += ".txt";
	ShellMan::VirtualDesktopFolder userDesktop;
	userDesktop.GetPath (_path);
}

ReportCtrl::ReportCtrl (ReportRequest & dlgData)
	: SelectFileCtrl ("Save report file in: ", "Select folder where the report file will be saved"),
	  _dlgData (dlgData)
{}

bool ReportCtrl::OnInitDialog () throw (Win::Exception)
{
	SelectFileCtrl::OnInitDialog ();

	_fileName.SetText (_dlgData.GetTargetFile ());
	_path.SetText (_dlgData.GetTargetPath ());
	Win::Dow::Handle dlgWin = GetWindow ();
	dlgWin.SetText ("Save Report");
	return true;
}

bool ReportCtrl::OnApply () throw ()
{
	if (SelectFileCtrl::OnApply ())
	{
		_dlgData.SetTargetPath (_path.GetString ());
		_dlgData.SetTargetFile (_fileName.GetString ());
		EndOk ();
		return true;
	}
	return false;
}

// command line
// -All_Report | -Selection_Report view:<File | Checkin | Inbox | Sync | History | Project> target:"<diagnostics file path>"
bool ReportCtrl::GetDataFrom (NamedValues const & source)
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
	std::string view (source.GetValue ("view"));
	if (IsNocaseEqual (view, "File"))
		_dlgData.SetTableId (Table::folderTableId);
	else if (IsNocaseEqual (view, "Checkin"))
		_dlgData.SetTableId (Table::checkInTableId);
	else if (IsNocaseEqual (view, "Sync"))
		_dlgData.SetTableId (Table::synchTableId);
	else if (IsNocaseEqual (view, "History"))
		_dlgData.SetTableId (Table::historyTableId);
	else if (IsNocaseEqual (view, "Project"))
		_dlgData.SetTableId (Table::projectTableId);
	else if (IsNocaseEqual (view, "Inbox"))
		_dlgData.SetTableId (Table::mailboxTableId);
	else
		_dlgData.SetTableId (Table::historyTableId);
	return true;
}
