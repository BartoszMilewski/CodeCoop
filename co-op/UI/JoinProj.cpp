//------------------------------------
//  (c) Reliable Software, 1997 - 2009
//------------------------------------

#include "precompiled.h"

#include "JoinProj.h"
#include "resource.h"
#include "OutputSink.h"
#include "Registry.h"
#include "PathRegistry.h"
#include "JoinProjectData.h"

#include <Dbg/Out.h>

// command line
// -Project_Join project:"Name" root:"Local\Path" recipient:"recipHubId" user:"My Name"
//      email:"myHubId" comment:"my comment" state:"observer"
//      autosynch:"yes" autofullsynch:"yes" keepcheckedout:"yes" script:"full\synch\path"

bool JoinProjectCtrl::GetDataFrom (NamedValues const & source)
{
	_joinData.ReadNamedValues (source);
	Project::Data & project = _joinData.GetProject ();
	MemberDescription const & joinee = _joinData.GetThisUser ();
	if (!project.GetProjectName ().empty () 
		&& !joinee.GetName ().empty () 
		&& !joinee.GetHubId ().empty () 
		&& project.GetRootPath ().IsDirStrEmpty ())
	{
		// no root path was given
		PathSplitter catPath (Registry::GetCatalogPath ());
		FilePath projPath (catPath.GetDrive ());
		projPath.DirDown ("Projects");
		projPath.DirDown (project.GetProjectName ().c_str ());
		JoinPathData data (projPath.GetDir ());
		JoinPathCtrl ctrl (&data);
		Dialog::Modal dlg (0, ctrl);
		if (dlg.IsOK ())
			project.SetRootPath (data.GetPath ());
		else
			return false;
	}

	bool validJoinData = _joinData.IsValid ();
	// don't nag observers who use command line join
	if (validJoinData && _joinData.IsObserver ())
		Registry::SetNagging (false);

	return validJoinData;
}

JoinPathCtrl::JoinPathCtrl (JoinPathData * data)
	: Dialog::ControlHandler (IDD_JOIN_PATH),
	  _dlgData (data)
{}

bool JoinPathCtrl::OnInitDialog () throw (Win::Exception)
{
	GetWindow ().CenterOverScreen ();
	_path.Init (GetWindow (), IDC_EDIT);
	_path.SetText (_dlgData->GetPath ().c_str ());
	return true;
}

bool JoinPathCtrl::OnApply () throw ()
{
	_dlgData->SetPath (_path.GetString ());
	if (_dlgData->IsValid ())
		EndOk ();
	else
		_dlgData->DisplayErrors (GetWindow ());

	return true;
}

bool JoinPathData::IsValid () const
{
	return !_path.empty () && FilePath::IsValid (_path.c_str ())
		&& FilePath::IsAbsolute (_path.c_str ());
}

void JoinPathData::DisplayErrors (Win::Dow::Handle winOwner) const
{
    TheOutput.Display ("Please specify a valid absolute path.", Out::Error, winOwner);
}
