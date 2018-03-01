//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "CheckInArea.h"
#include "DataBase.h"
#include "FileState.h"
#include "PhysicalFile.h"
#include "PathFind.h"
#include "Catalog.h"
#include "ProjectTable.h"
#include "ProjectMarker.h"
#include "CheckoutNotifications.h"

#include <Ex/WinEx.h>
#include <File/File.h>
#include <File/MemFile.h>
#include <File/Path.h>

bool CheckInArea::IsEmpty () const
{
	GidList files;
	_dataBase.ListCheckedOutFiles (files);
	return files.empty ();
}

void CheckInArea::QueryUniqueIds (Restriction const & restrict, GidList & ids) const
{
	if (restrict.HasFiles ())
	{
		// Special restriction pre-filled with unique names
		Restriction::UnameIter it;
		for (it = restrict.BeginFiles (); it != restrict.EndFiles (); ++it)
		{
			UniqueName const & uname = *it;
			FileData const * fd = _dataBase.FindProjectFileByName (uname);
			if (fd != 0)
				ids.push_back (fd->GetGlobalId ());
		}
	}
	else
	{
		_dataBase.ListCheckedOutFiles (ids);
	}
}

bool CheckInArea::IsValid () const
{
	return _dataBase.GetMyId () != gidInvalid;
}

std::string CheckInArea::GetStringField (Column col, GlobalId gid) const
{
	FileData const * file = _dataBase.GetFileDataByGid (gid);
	if (col == colName)
	{
		return file->GetName ();
	}
	else if (col == colStateName)
	{
		FileState state = file->GetState ();
		return state.GetName ();
	}
	else if (col == colParentPath)
	{
		Project::Path parent (_dataBase);
		char const * path = parent.MakePath (gid);
		if (path == 0)
			return std::string ();

		return std::string (path);
	}
	Assert (!"Invalid column");
	return std::string ();
}

bool CheckInArea::GetBinaryField (Column col, void * buf, unsigned int bufLen, GlobalId gid) const
{
	if (col == colTimeStamp && bufLen >= sizeof (FileTime))
	{
		FileData const * fileData = _dataBase.GetFileDataByGid (gid);
		Assert (fileData != 0);
		char const * fullPath = 0;
		if (fileData->GetState ().IsPresentIn (Area::Project))
		{
			fullPath = _pathFinder.GetFullPath (fileData->GetUniqueName ());
		}
		else if (fileData->GetState ().IsPresentIn (Area::Original))
		{
			fullPath = _pathFinder.GetFullPath (gid, Area::Original);
		}

		if (fullPath != 0 && File::Exists (fullPath) && !File::IsFolder (fullPath))
		{
			FileInfo file (fullPath);
			FileTime fTime (file);
			memcpy (buf, &fTime, sizeof (FileTime));
		}
		else
		{
			// use current time if nothing else
			PackedTime time;
			memcpy (buf, &time, sizeof (FileTime));
		}
		return true;
	}
	return false;
}

GlobalId CheckInArea::GetIdField (Column col, GlobalId gid) const
{
	Assert (col == colParentId);
	FileData const * data = _dataBase.GetFileDataByGid (gid);
	return data->GetUniqueName ().GetParentId ();
}

std::string CheckInArea::GetStringField (Column col, UniqueName const & uname) const
{
	return std::string ();
}

GlobalId CheckInArea::GetIdField (Column col, UniqueName const & uname) const
{
	return gidInvalid;
}

unsigned long CheckInArea::GetNumericField (Column col, GlobalId gid) const
{
	FileData const * file = _dataBase.GetFileDataByGid (gid);
	FileType type = file->GetType ();
	if (col == colState)
	{
		FileState state = file->GetState ();
		if (!type.IsFolder () && !type.IsRoot ())
		{
			bool diff = true;
			try
			{
				PhysicalFile physFile (*file, _pathFinder);
				if (state.IsPresentIn (Area::Reference))
					diff = physFile.IsContentsDifferent (Area::Project, Area::Reference);
				else
					diff = physFile.IsContentsDifferent (Area::Project, Area::Original);
			}
			catch (...) 
			{
				Win::ClearError ();
			}
			state.SetCoDiff (diff);
			state.SetCheckedOutByOthers (_checkOutDb.IsCheckedOut (gid));
			if (file->IsRenamedIn (Area::Original))
			{
				UniqueName const & alias = file->GetUnameIn (Area::Original);
				UniqueName const & uname = file->GetUniqueName ();
				if (uname.GetParentId () != alias.GetParentId ())
					state.SetMoved (true);
				else
					state.SetRenamed (true);
			}
			else if (file->IsRenamedIn (Area::Reference))
			{
				UniqueName const & alias = file->GetUnameIn (Area::Reference);
				UniqueName const & uname = file->GetUniqueName ();
				if (uname.GetParentId () != alias.GetParentId ())
					state.SetMoved (true);
				else
					state.SetRenamed (true);
			}
			if (file->IsTypeChangeIn (Area::Original)
				||file->IsTypeChangeIn (Area::Reference))
			{
				state.SetTypeChanged (true);
			}
		}
		return state.GetValue ();
	}
	else if (col == colType)
	{
		return type.GetValue ();
	}
	Assert (!"Invalid numeric column");
	return 0;
}

std::string CheckInArea::GetCaption (Restriction const & restrict) const
{
	std::string caption;
	GlobalId thisUser = _dataBase.GetMyId ();
	if (thisUser != gidInvalid)
	{
		MemberState thisUserState = _dataBase.GetMemberState (thisUser);
		if (thisUserState.IsObserver ())
		{
			caption = "Project observer -- check-in not available";
		}
		else
		{
			RecoveryMarker recovery (_catalog, _project.GetCurProjectId ());
			if (recovery.Exists ())
			{
				caption = "Project waiting for the verification report -- check-in not ";
				BlockedCheckinMarker blockedCheckin (_catalog, _project.GetCurProjectId ());
				if (blockedCheckin.Exists ())
					caption += "available";
				else
					caption += "recommended";
			}
		}
	}
	return caption;
}
