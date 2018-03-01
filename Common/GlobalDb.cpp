//----------------------------------
// (c) Reliable Software 2000 - 2008
//----------------------------------
#include "precompiled.h"
#include "GlobalDb.h"
#include "OutputSink.h"
#include "ProjectData.h"
#include "Crypt.h"

#include <TimeStamp.h>
#include <XML/XmlTree.h>
#include <auto_Vector.h>

const char GlobalDb::_projLogName [] = "ProjLog.bin";
const char GlobalDb::_userLogName [] = "UserLog.bin";
const char GlobalDb::_clusterLogName [] = "SatUserLog.bin";
const char GlobalDb::_fileTypeLogName [] = "FileTypeLog.bin";
const char GlobalDb::_hubLogName [] = "HubLog.bin";

GlobalDb::GlobalDb (FilePath const & dbPath)
: _mutex ("Global\\CodeCoopMutex"), 
  _versionStamp (-1),
  _dbPath (dbPath)
{
	AddTransactableMember (_versionStamp);

	AddTransactableMember (_topology);
	AddTransactableMember (_trialStart);
	AddTransactableMember (_licensee);
	AddTransactableMember (_key);
	AddTransactableMember (_distributorLicensee);
	AddTransactableMember (_nextDistributorNumber);
	AddTransactableMember (_distributorLicenseCount);

	AddTransactableMember (_hubId);
	AddTransactableMember (_hubTransports);
	AddTransactableMember (_hubActiveTransport);
	AddTransactableMember (_myTransports);
	AddTransactableMember (_myActiveTransport);
	AddTransactableMember (_hubRemoteTransports);
	AddTransactableMember (_hubRemoteActiveTransport);

	AddTransactableMember (_preproCommand);
	AddTransactableMember (_preproResult);
	AddTransactableMember (_postproCommand);
	AddTransactableMember (_postproExtension);
	AddTransactableMember (_preproNeedsProjName);

	AddTransactableMember (_typeList);
	AddTransactableMember (_typeValidEnd);
	AddTransactableMember (_projList);
	AddTransactableMember (_projValidEnd);
	AddTransactableMember (_userList);
	AddTransactableMember (_userValidEnd);
	AddTransactableMember (_clusterList);
	AddTransactableMember (_clusterValidEnd);
	AddTransactableMember (_hubList);
	AddTransactableMember (_hubListValidEnd);

	Init (dbPath);
}

void GlobalDb::Init (FilePath const & dbPath)
{
	_typeLog.Init (_dbPath.GetFilePath (_fileTypeLogName));
	_projLog.Init (_dbPath.GetFilePath (_projLogName));
	_userLog.Init (_dbPath.GetFilePath (_userLogName));
	_clusterUserLog.Init (_dbPath.GetFilePath (_clusterLogName));
	_hubLog.Init (_dbPath.GetFilePath (_hubLogName));
}

void GlobalDb::XSetLicense (std::string const & licensee, std::string const & key)
{
	_licensee.XSet (licensee);
	_key.XSet (key);
}

int GlobalDb::XAddProject (std::string const & name, std::string const & path)
{
	File::Offset logOffset = _projValidEnd.XGet ();
	_projValidEnd.XSet (_projLog.Append (_projValidEnd.XGet (), ProjEntry (name, path)));
	
	// accumulate project ids in use
	std::set<int> projIds;
	for (unsigned int i = 0; i < _projList.XCount (); ++i)
	{
		ProjectNote const * note = _projList.XGet (i);
		if (note && note->GetState () != stateInvalid)
			projIds.insert (note->GetProjId ());
	}
	// find smallest unused project id
	int id = 1;
	for (std::set<int>::iterator it = projIds.begin (); it != projIds.end (); ++it)
	{
		if (*it != id)
			break;
		++id;
	}

	_projList.XAppend (std::unique_ptr<ProjectNote> (
		new ProjectNote (id, stateActive, logOffset)));
	return id;
}

void GlobalDb::XAddProject (int projId, std::string const & name, std::string const & path)
{
	File::Offset logOffset = _projValidEnd.XGet ();
	_projValidEnd.XSet (_projLog.Append (_projValidEnd.XGet (), ProjEntry (name, path)));
	
	_projList.XAppend (std::unique_ptr<ProjectNote> (
		new ProjectNote (projId, stateActive, logOffset)));
}

void GlobalDb::XRemoveProject (int projId)
{
	Assert (projId > 0);
	for (unsigned int i = 0; i < _projList.XCount (); ++i)
	{
		ProjectNote const * note = _projList.XGet (i);
		if (note && (note->GetProjId () == projId || note->GetState () == stateInvalid))
			_projList.XMarkDeleted (i);
	}
	for (unsigned int j = 0; j < _userList.XCount (); ++j)
	{
		if (_userList.XGet (j) && _userList.XGet (j)->GetProjId () == projId)
		{
			ProjectNote * note = _userList.XGetEdit (j);
			note->SetState (stateRemoved);
		}
	}
}

void GlobalDb::XMarkProjectUnavailable (int projId)
{
	Assert (projId > 0);
	for (unsigned int i = 0; i < _projList.XCount (); ++i)
	{
		ProjectNote const * note = _projList.XGet (i);
		if (note != 0 && note->GetProjId () == projId && note->GetState () == stateActive)
		{
			ProjectNote * editedProject = _projList.XGetEdit (i);
			editedProject->SetState (stateUnavailable);
			break;
		}
	}
}

void GlobalDb::XMarkProjectAvailable (int projId)
{
	for (unsigned int i = 0; i < _projList.XCount (); ++i)
	{
		ProjectNote const * note = _projList.XGet (i);
		if (note != 0 && note->GetProjId () == projId && note->GetState () == stateUnavailable)
		{
			ProjectNote * editedProject = _projList.XGetEdit (i);
			editedProject->SetState (stateActive);
			break;
		}
	}
}

void GlobalDb::XMoveProjectTree (int projId, std::string const & newPath)
{
	Assert (projId > 0);
	unsigned int i;
	for (i = 0; i < _projList.XCount (); ++i)
	{
		ProjectNote const * note = _projList.XGet (i);
		if (note && note->GetProjId () == projId)
			break;
	}
	Assert (i < _projList.XCount ());
	ProjectNote * note = _projList.XGetEdit (i);
	std::unique_ptr<ProjEntry> p = _projLog.Retrieve (note->GetLogOffset (), VersionNo ());
	File::Offset logOffset = _projValidEnd.XGet ();
	_projValidEnd.XSet (_projLog.Append (_projValidEnd.XGet (), ProjEntry (p->GetName (), newPath)));
	note->SetLogOffset (logOffset);
}

bool GlobalDb::XActivateUser (Address const & address, int projectId)
{
	int count = _userList.XCount ();
	for (int i = 0; i < count; ++i)
	{
		ProjectNote const * note = _userList.XGet (i);
		if (note != 0)
		{
			File::Offset off = note->GetLogOffset ();
			std::unique_ptr<Address> p = _userLog.Retrieve (off, VersionNo ());

			if (address.IsEqualAddress (*p))
			{
				ProjectNote * note = _userList.XGetEdit (i);
				note->SetProjId (projectId);
				note->SetState (stateActive);
				return true;
			}
		}
	}
	return false;
}

void GlobalDb::XAddUser (Address const & address, int projectId)
{
	if (!XActivateUser (address, projectId))
	{
		File::Offset logOffset = _userValidEnd.XGet ();
		_userValidEnd.XSet (_userLog.Append (_userValidEnd.XGet (), address));
		_userList.XAppend (std::unique_ptr<ProjectNote> (new ProjectNote (projectId,
																		stateActive, 
																		logOffset)));
	}
}

void GlobalDb::XRemoveUser (Address const & address, bool permanently)
{
	int count = _userList.XCount ();
	for (int i = 0; i < count; ++i)
	{
		if (_userList.XGet (i) == 0)
			continue;
		File::Offset off = _userList.XGet (i)->GetLogOffset ();
		std::unique_ptr<Address> p = _userLog.Retrieve (off, VersionNo ());
		if (address.IsEqualAddress (*p))
		{
			if (permanently)
			{
				_userList.XMarkDeleted (i);
			}
			else
			{
				ProjectNote * note = _userList.XGetEdit (i);
				note->SetState (stateRemoved);
			}
		}
	}
}

void GlobalDb::XKillLocalRecipient (Address const & address, int projectId)
{
	int count = _userList.XCount ();
	for (int i = 0; i < count; ++i)
	{
		ProjectNote const * note = _userList.XGet (i);
		if (note == 0)
			continue;
		File::Offset off = note->GetLogOffset ();
		std::unique_ptr<Address> p = _userLog.Retrieve (off, VersionNo ());
		if (address.IsEqualAddress (*p) && (projectId == note->GetProjId ()))
		{
			_userList.XMarkDeleted (i);
			break;
		}
	}
}

// Delete removed local recipient from database
// returns false if active recipient with the address is found
// in that case recipient is not deleted
bool GlobalDb::XKillRemovedLocalRecipient (Address const & address)
{
	int count = _userList.XCount ();
	for (int i = 0; i < count; ++i)
	{
		ProjectNote const * note = _userList.XGet (i);
		if (note == 0)
			continue;
		File::Offset off = note->GetLogOffset ();
		std::unique_ptr<Address> p = _userLog.Retrieve (off, VersionNo ());
		if (address.IsEqualAddress (*p))
		{
			if (stateActive == note->GetState ())
			{
				return false;
			}
			else
			{
				_userList.XMarkDeleted (i);
				break;
			}
		}
	}
	return true;
}

void GlobalDb::XRefreshUserData (Address const & address,
								std::string const & newHubId,
								std::string const & newUserId,
								bool permanently)
{
	Assert (!newHubId.empty ());
	Assert (!newUserId.empty ());

	int count = _userList.XCount ();
	// remove old entries and find duplicates of new entry
	int projId = -1;
	bool duplicate = false;
	int idxNewAddr = -1;
	for (int i = 0; i < count; ++i)
	{
		if (_userList.XGet (i) == 0)
			continue;
		File::Offset off = _userList.XGet (i)->GetLogOffset ();
		std::unique_ptr<Address> p = _userLog.Retrieve (off, VersionNo ());
		if (address.IsEqualAddress (*p))
		{
			// remove old address
			ProjectNote * note = _userList.XGetEdit (i);

			if (note->GetState () == stateActive)
				projId = note->GetProjId ();

			if (permanently)
				_userList.XMarkDeleted (i);
			else
				note->SetState (stateRemoved);
		}
		else if (IsNocaseEqual (address.GetProjectName (), p->GetProjectName ())
			&& IsNocaseEqual (newHubId, p->GetHubId ())
			&& IsNocaseEqual (newUserId, p->GetUserId ()))
		{
			// found new address: is it inactive?
			if (_userList.XGet (i)->GetState () != stateActive)
			{
				idxNewAddr = i;
			}
			duplicate = true;
		}
	}

	if (projId == -1)
		return; // don't know the old address!

	if (duplicate)
	{
		// found new address
		if (idxNewAddr != -1)
		{
			// it was inactive. Activate it!
			ProjectNote * note = _userList.XGetEdit (idxNewAddr);
			note->SetState (stateActive);
			note->SetProjId (projId);
		}
	}
	else
	{
		// didn't find new address
		Address newAddress (newHubId, address.GetProjectName (), newUserId);
		File::Offset logOffset = _userValidEnd.XGet ();
		_userValidEnd.XSet (_userLog.Append (_userValidEnd.XGet (), newAddress));
		_userList.XAppend (std::unique_ptr<ProjectNote> (new ProjectNote (projId,
																		stateActive, 
																		logOffset)));
	}
}

void GlobalDb::XLocalHubIdChanged (std::string const & oldHubId, std::string const & newHubId)
{
	// Find all entries with old Hub's Email Address and add corresponding entries with new Hub's Email Address
	// Remove old entries
	// Don't add duplicate entries
	std::map<Address, int> projIds;
	bool doActivate = false;
	
	int count = _userList.XCount ();
	for (int i = 0; i < count; ++i)
	{
		ProjectNote const * note = _userList.XGet (i);
		if (note != 0)
		{
			File::Offset off = note->GetLogOffset ();
			std::unique_ptr<Address> p = _userLog.Retrieve (off, VersionNo ());

			if (IsNocaseEqual (p->GetHubId (), oldHubId) && note->GetState () == stateActive)
			{
				Address addr (newHubId, p->GetProjectName (), p->GetUserId ());
				if (projIds.find (addr) == projIds.end ())
					projIds [addr] = note->GetProjId (); // to be added
				// if it's already there, don't touch it

				_userList.XGetEdit (i)->SetState (stateRemoved);
			}
			else if (IsNocaseEqual (p->GetHubId (), newHubId))
			{
				if (note->GetState () == stateActive)
				{
					// mark it as already present, to avoid duplicating it
					projIds [*p] = -1;
				}
				else
				{
					doActivate = true;
					// we'll have to work harder
				}
			}
		}
	}

	if (doActivate) // rare case
	{
		std::vector<std::pair<Address, int> > activate;
		// scan again for inactive entries with the new address
		for (int i = 0; i < count; ++i)
		{
			ProjectNote const * note = _userList.XGet (i);
			if (note != 0)
			{
				File::Offset off = note->GetLogOffset ();
				std::unique_ptr<Address> p = _userLog.Retrieve (off, VersionNo ());
				if (IsNocaseEqual (p->GetHubId (), newHubId) && note->GetState () == stateRemoved)
				{
					std::map<Address, int>::iterator it = projIds.find (*p);
					if (it != projIds.end () && it->second == note->GetProjId ())
					{
						it->second = -1;
						activate.push_back (std::make_pair (*p, note->GetProjId ()));
					}
				}
			}
		}
		for (std::vector<std::pair<Address, int> >::iterator it = activate.begin ();
			it != activate.end (); ++it)
		{
			XActivateUser (it->first, it->second);
		}
	}
	// Add modified entries (with new hubId)
	for (std::map<Address, int>::const_iterator it = projIds.begin ();
		it != projIds.end (); ++it)
	{
		int projId = it->second;
		if (projId != -1)
		{
			File::Offset logOffset = _userValidEnd.XGet ();
			_userValidEnd.XSet (_userLog.Append (_userValidEnd.XGet (), it->first));
			_userList.XAppend (std::unique_ptr<ProjectNote> (new ProjectNote (it->second,
																			stateActive, 
																			logOffset)));
		}
	}
}

void GlobalDb::XAddFileType (std::string const & extension, unsigned long type)
{
	int count = _typeList.XCount ();
	for (int i = 0; i < count; ++i)
	{
		if (_typeList.XGet (i) == 0)
			continue;
		File::Offset off = _typeList.XGet (i)->GetLogOffset ();
		std::unique_ptr<FileTypeEntry> p = _typeLog.Retrieve (off, VersionNo ());
		if (IsFileNameEqual (extension, p->GetExt ()))
		{
			_typeList.XGetEdit (i)->SetType (type);
			return;
		}
	}
	File::Offset logOffset = _typeValidEnd.XGet ();
	_typeValidEnd.XSet (_typeLog.Append (_typeValidEnd.XGet (), FileTypeEntry (extension)));
	_typeList.XAppend (std::unique_ptr<FileTypeNote> (new FileTypeNote (type, 0, logOffset)));
}

// ---------------
// Recipient lists
// ---------------
bool GlobalDb::XPurgeLocalRecipients ()
{
	bool anyPurged = false;
	int count = _userList.XCount ();
	for (int i = 0; i < count; ++i)
	{
		ProjectNote const * note = _userList.XGet (i);
		if (note == 0)
			continue;
		File::Offset off = note->GetLogOffset ();
		std::unique_ptr<Address> p = _userLog.Retrieve (off, VersionNo ());
		{
			if (note->GetState () != stateActive)
			{
				_userList.XMarkDeleted (i);
				anyPurged = true;
			}
		}
	}
	return anyPurged;
}

bool GlobalDb::XPurgeSatelliteRecipients ()
{
	bool anyPurged = false;
	int count = _clusterList.XCount ();
	for (int i = 0; i < count; ++i)
	{
		LogNote const * note = _clusterList.XGet (i);
		if (note == 0)
			continue;
		File::Offset off = note->GetLogOffset ();
		std::unique_ptr<ClusterUserEntry> p = _clusterUserLog.Retrieve (off, VersionNo ());
		{
			if (note->GetState () != stateActive)
			{
				_clusterList.XMarkDeleted (i);
				anyPurged = true;
			}
		}
	}
	return anyPurged;
}

void GlobalDb::XAddClusterRecipient (Address const & address, Transport const & transport)
{
	// first check for duplicate entry
	int count = _clusterList.XCount ();
	for (int i = 0; i < count; ++i)
	{
		LogNote const * note = _clusterList.XGet (i);
		if (note == 0)
			continue;
		File::Offset off = note->GetLogOffset ();
		std::unique_ptr<ClusterUserEntry> p = _clusterUserLog.Retrieve (off, VersionNo ());
		if (p->IsEqualAddress (address))
		{
			if (transport == p->GetTransport ())
			{
				if (note->GetState () != stateActive)
				{
					// Revisit: more states
					LogNote * noteEdit = _clusterList.XGetEdit (i);
					noteEdit->SetState (stateActive);
				}
				return;
			}
			else
			{
				_clusterList.XMarkDeleted (i);
				break;
			}
		}
	}
	ClusterUserEntry entry (address, transport);
	File::Offset logOffset = _clusterValidEnd.XGet ();
	_clusterValidEnd.XSet (_clusterUserLog.Append (_clusterValidEnd.XGet (), entry));
	_clusterList.XAppend (std::unique_ptr<LogNote> (new LogNote (stateActive, logOffset)));
}

void GlobalDb::XRemoveClusterRecipient (Address const & address, bool permanently)
{
	int count = _clusterList.XCount ();
	for (int i = 0; i < count; ++i)
	{
		if (_clusterList.XGet (i) == 0)
			continue;
		File::Offset off = _clusterList.XGet (i)->GetLogOffset ();
		std::unique_ptr<ClusterUserEntry> p = _clusterUserLog.Retrieve (off, VersionNo ());
		if (p->IsEqualAddress (address))
		{
			// revisit: more states
			if (permanently)
			{
				_clusterList.XMarkDeleted (i);
			}
			else
			{
				LogNote * note = _clusterList.XGetEdit (i);
				note->SetState (stateRemoved);
			}
		}
	}
}

void GlobalDb::XActivateClusterRecipient (Address const & address)
{
	int count = _clusterList.XCount ();
	for (int i = 0; i < count; ++i)
	{
		if (_clusterList.XGet (i) == 0)
			continue;
		File::Offset off = _clusterList.XGet (i)->GetLogOffset ();
		std::unique_ptr<ClusterUserEntry> p = _clusterUserLog.Retrieve (off, VersionNo ());
		if (p->IsEqualAddress (address))
		{
			LogNote * note = _clusterList.XGetEdit (i);
			note->SetState (stateActive);
		}
	}
}

void GlobalDb::XAddRemoteHub (std::string const & hubId, Transport const & transport)
{
	Assert (!IsNocaseEqual (hubId, _hubId));
	Assert (!transport.IsUnknown ());
	int count = _hubList.XCount ();
	for (int i = 0; i < count; ++i)
	{
		LogNote const * note = _hubList.XGet (i);
		if (note == 0)
			continue;
		File::Offset off = note->GetLogOffset ();
		std::unique_ptr<HubEntry> p = _hubLog.Retrieve (off, VersionNo ());
		if (p->IsEqualHubId (hubId))
		{
			if (p->IsEqualTransport (transport))
				return; // it's already there

			_hubList.XMarkDeleted (i);
		}
	}
	HubEntry entry (hubId, transport);
	File::Offset logOffset = _hubListValidEnd.XGet ();
	_hubListValidEnd.XSet (_hubLog.Append (_hubListValidEnd.XGet (), entry));
	_hubList.XAppend (std::unique_ptr<LogNote> (new LogNote (stateActive, logOffset)));
}

void GlobalDb::XDeleteRemoteHub (std::string const & hubId)
{
	int count = _hubList.XCount ();
	for (int i = 0; i < count; ++i)
	{
		LogNote const * note = _hubList.XGet (i);
		if (note == 0)
			continue;
		File::Offset off = note->GetLogOffset ();
		std::unique_ptr<HubEntry> p = _hubLog.Retrieve (off, VersionNo ());
		if (p->IsEqualHubId (hubId))
		{
			_hubList.XMarkDeleted (i);
			return;
		}
	}
}

Transport GlobalDb::GetActiveIntraClusterTransportToMe () const
{
	if (_myActiveTransport.Get () == Transport::Unknown)
		return Transport ();

	for (unsigned int i = 0; i < _myTransports.Count (); ++i)
	{
		if (_myTransports.Get (i)->GetMethod () == _myActiveTransport.Get ())
			return *_myTransports.Get (i);
	}
	Assert (!"Active transport not found in transport vector");
	return Transport ();
}

void GlobalDb::GetIntraClusterTransportsToMe (std::vector<Transport> & transports, Transport::Method & active) const
{
	active = static_cast<Transport::Method>(_myActiveTransport.Get ());
	std::map<Transport::Method, std::string> map;
	for (unsigned int i = 0; i < _myTransports.Count (); ++i)
	{
		Transport const * tr = _myTransports.Get (i);
		map [tr->GetMethod ()] = tr->GetRoute ();
	}
	std::for_each (map.begin (), map.end (), [&transports](std::pair<Transport::Method, std::string> const & p)
    {
        transports.push_back(Transport (p.second, p.first));
    });
}

void GlobalDb::XSetIntraClusterTransportsToMe (std::vector<Transport> const & transports, Transport::Method active)
{
	_myActiveTransport.XSet (active);
	_myTransports.XClear ();
	for (unsigned int i = 0; i < transports.size (); ++i)
	{
		_myTransports.XAppend (std::unique_ptr<Transport> (new Transport (transports [i])));
	}
}

void GlobalDb::GetTransportsToHub (std::vector<Transport> & transports, Transport::Method & active) const
{
	active = static_cast<Transport::Method> (_hubActiveTransport.Get ());
	std::map<Transport::Method, std::string> map;
	for (unsigned int i = 0; i < _hubTransports.Count (); ++i)
	{
		Transport const * tr = _hubTransports.Get (i);
		map [tr->GetMethod ()] = tr->GetRoute ();
	}
	std::for_each (map.begin (), map.end (), [&transports](std::pair<Transport::Method, std::string> const & p)
    {
        transports.push_back(Transport (p.second, p.first));
    });
}

void GlobalDb::XSetTransportsToHub (std::vector<Transport> const & transports, Transport::Method active)
{
	_hubActiveTransport.XSet (active);
	_hubTransports.XClear ();
	for (unsigned int i = 0; i < transports.size (); ++i)
	{
		_hubTransports.XAppend (std::unique_ptr<Transport> (new Transport (transports [i])));
	}
}

void GlobalDb::GetInterClusterTransportsToMe (std::vector<Transport> & transports, Transport::Method & active) const
{
	active = static_cast<Transport::Method> (_hubRemoteActiveTransport.Get ());
	std::map<Transport::Method, std::string> map;
	for (unsigned int i = 0; i < _hubRemoteTransports.Count (); ++i)
	{
		Transport const * tr = _hubRemoteTransports.Get (i);
		map [tr->GetMethod ()] = tr->GetRoute ();
	}
	std::for_each (map.begin (), map.end (), [&transports](std::pair<Transport::Method, std::string> const & p)
    {
        transports.push_back(Transport (p.second, p.first));
    });
}

void GlobalDb::XSetInterClusterTransportsToMe (std::vector<Transport> const & transports, Transport::Method active)
{
	_hubRemoteActiveTransport.XSet (active);
	_hubRemoteTransports.XClear ();
    // for (Transport & tr; transports)
    std::for_each(transports.cbegin(), transports.cend(), [this](Transport const & tr)
    {
		_hubRemoteTransports.XAppend (std::unique_ptr<Transport> (new Transport (tr)));
    });
}

Transport GlobalDb::GetHubRemoteActiveTransport () const
{
	if (_hubRemoteActiveTransport.Get () == Transport::Unknown)
		return Transport ();

	for (unsigned int i = 0; i < _hubRemoteTransports.Count (); ++i)
	{
		if (_hubRemoteTransports.Get (i)->GetMethod () == _hubRemoteActiveTransport.Get ())
			return *_hubRemoteTransports.Get (i);
	}
	Assert (!"Active hub remote transport not found in transport vector");
	return Transport ();
}

void GlobalDb::XChangeTransport (Address const & address, Transport const & newTransport)
{
	int count = _clusterList.XCount ();
	for (int i = 0; i < count; ++i)
	{
		LogNote const * note = _clusterList.XGet (i);
		if (note == 0)
			continue;
		File::Offset off = note->GetLogOffset ();
		std::unique_ptr<ClusterUserEntry> p = _clusterUserLog.Retrieve (off, VersionNo ());
		if (p->IsEqualAddress (address))
		{
			if (newTransport != p->GetTransport ())
			{
				ClusterUserEntry entry (address, newTransport);
				File::Offset logOffset = _clusterValidEnd.XGet ();
				_clusterValidEnd.XSet (_clusterUserLog.Append (_clusterValidEnd.XGet (), entry));
				LogNote * noteEdit = _clusterList.XGetEdit (i);
				noteEdit->SetOffset (logOffset);
			}
			if (note->GetState () != stateActive)
			{
				LogNote * noteEdit = _clusterList.XGetEdit (i);
				noteEdit->SetState (stateActive);
			}
			return;
		}
	}
}

void GlobalDb::XClearClusterRecipients ()
{
	int count = _clusterList.XCount ();
	for (int i = 0; i < count; ++i)
	{
		_clusterList.XMarkDeleted (i);
	}
	_clusterValidEnd.XSet (File::Offset (0, 0));
	File::Delete (_clusterUserLog.GetPath ());
}

// iteration support
std::string GlobalDb::GetFileExtension (SoftXArray<FileTypeNote>::const_iterator it) const
{
	File::Offset off = (*it)->GetLogOffset ();
	std::unique_ptr<FileTypeEntry> p = _typeLog.Retrieve (off, VersionNo ());
	return p->GetExt ();
}

void GlobalDb::GetClusterRecipient (SoftXArray<LogNote>::const_iterator it,
							        Address & address,
									Transport & transport) const
{
	File::Offset off = (*it)->GetLogOffset ();
	std::unique_ptr<ClusterUserEntry> p = _clusterUserLog.Retrieve (off, VersionNo ());
	address.Set (p->GetAddress ());
	transport = p->GetTransport ();
}

void GlobalDb::GetHubListEntry (SoftXArray<LogNote>::const_iterator it,
								std::string & hubId,
								Transport & transport) const
{
	File::Offset off = (*it)->GetLogOffset ();
	std::unique_ptr<HubEntry> p = _hubLog.Retrieve (off, VersionNo ());
	hubId.assign (p->GetHubId ());
	transport = p->GetTransport ();
}

void GlobalDb::GetProjectData (SoftXArray<ProjectNote>::const_iterator it, 
							   std::string & projectName, 
							   std::string & srcPath) const
{
	File::Offset off = (*it)->GetLogOffset ();
	std::unique_ptr<ProjEntry> p = _projLog.Retrieve (off, VersionNo ());
	projectName = p->GetName ();
	srcPath     = p->GetPath ();
}

std::string GlobalDb::GetProjName (SoftXArray<ProjectNote>::const_iterator it) const
{
	File::Offset off = (*it)->GetLogOffset ();
	std::unique_ptr<ProjEntry> p = _projLog.Retrieve (off, VersionNo ());
	return p->GetName ();
}

std::string GlobalDb::GetProjPath (SoftXArray<ProjectNote>::const_iterator it) const
{
	File::Offset off = (*it)->GetLogOffset ();
	std::unique_ptr<ProjEntry> p = _projLog.Retrieve (off, VersionNo ());
	return p->GetPath ();
}

void GlobalDb::GetUserData (SoftXArray<ProjectNote>::const_iterator it,
							Address & address) const
{
	File::Offset off = (*it)->GetLogOffset ();
	std::unique_ptr<Address> p = _userLog.Retrieve (off, VersionNo ());
	address.Set (p->GetHubId (), p->GetProjectName (), p->GetUserId ());
}

std::string GlobalDb::GetUserHubId (SoftXArray<ProjectNote>::const_iterator it) const
{
	File::Offset off = (*it)->GetLogOffset ();
	std::unique_ptr<Address> p = _userLog.Retrieve (off, VersionNo ());
	return p->GetHubId ();
}

std::string GlobalDb::GetUserId (SoftXArray<ProjectNote>::const_iterator it) const
{
	File::Offset off = (*it)->GetLogOffset ();
	std::unique_ptr<Address> p = _userLog.Retrieve (off, VersionNo ());
	return p->GetUserId ();
}

void GlobalDb::Dump (XML::Node * root, std::ostream & out, bool isDiagnostic) const
{
	XML::Node * catalog = root->AddChild ("Catalog");
	StrTime trial (_trialStart.Get ());
	out << "*Trial start: " << trial.GetString () << std::endl;
	out << "*Global license -- ";
	std::string global (_licensee);
	global += '\n';
	global += _key;
	int version;
	int seats;
	char product;
	if (::DecodeLicense (global, version, seats, product))
	{
		out << "licensee: " << _licensee.c_str () << "; seats: " << seats 
			<< "; version: " << version << "; product: " << product;
	}
	else
	{
		out << "NOT PRESENT";
	}
	out << std::endl;
	out << "*Distribution license -- ";
	if (_distributorLicenseCount.Get () != 0)
	{
		out << "licensee: " << _distributorLicensee.c_str () << "; seats: " << _distributorLicenseCount.Get () << "; used: ";
		unsigned i = _nextDistributorNumber.Get () == 0 ? 0 : _nextDistributorNumber.Get () - 1;
		out << i;
	}
	else
	{
		out << "NOT PRESENT";
	}
	out << std::endl;

	out << "*Topology: " << Topology (_topology.Get ()) << std::endl;
	out << "*Hub Id: " << _hubId.c_str () << std::endl; 
	out << "*Transports to my hub (active: " 
		<< Transport::GetMethodName (_hubActiveTransport.Get ()) 
		<< "):" << std::endl;
	unsigned int i = 0;
	for (i = 0; i < _hubTransports.Count (); ++i)
	{
		out << "**" << *_hubTransports.Get (i) << std::endl;
	}
	out << "*Transports to my computer used within my cluster (active: "
		<< Transport::GetMethodName (_myActiveTransport.Get ())
		<< "):" << std::endl;
	for (i = 0; i < _myTransports.Count (); ++i)
	{
		out << "**" << *_myTransports.Get (i) << std::endl;
	}
	out << "*Transports from other hubs to my hub (active: "
		<< Transport::GetMethodName (_hubRemoteActiveTransport.Get ())
		<< "):" << std::endl;
	for (i = 0; i < _hubRemoteTransports.Count (); ++i)
	{
		out << "**" << *_hubRemoteTransports.Get (i) << std::endl;
	}

	unsigned int count;
	if (!isDiagnostic)
	{
		out << std::endl << "==Type associations:" << std::endl;
		count = _typeList.Count ();
		for (i = 0; i < count; ++i)
		{
			out << "| ";
			File::Offset off = _typeList.Get (i)->Dump (out);
			std::unique_ptr<FileTypeEntry> p = _typeLog.Retrieve (off, VersionNo ());
			out << " | ";
			p->Dump (out);
			out << " |" << std::endl;
		}
	}

	out << std::endl << "===Local projects:" << std::endl;
	out << "|! Project id |! State |! Name |! Path |" << std::endl;
	count = _projList.Count ();
	for (i = 0; i < count; ++i)
	{
		out << "| ";
		File::Offset off = _projList.Get (i)->Dump (out);
		out << " | ";
		std::unique_ptr<ProjEntry> p = _projLog.Retrieve (off, VersionNo ());
		p->Dump (out);
		out << " |" << std::endl;
	}
	out << std::endl << "===Local users:" << std::endl;
	out << "|! Project id |! User state |! Project name |! User hub id |! User project id |" << std::endl;
	count = _userList.Count ();
	for (i = 0; i < count; ++i)
	{
		out << "| ";
		File::Offset off = _userList.Get (i)->Dump (out);
		out << " | ";
		std::unique_ptr<Address> p = _userLog.Retrieve (off, VersionNo ());
		p->Dump (out);
		out << " |" << std::endl;
	}
	out << std::endl << "===Satellite users:" << std::endl;
	out << "|! State |! Project name |! User hub id |! User project id |! Transport |" <<std::endl;
	count = _clusterList.Count ();
	for (i = 0; i < count; ++i)
	{
		out << "| ";
		File::Offset off = _clusterList.Get (i)->Dump (out);
		out << " | ";
		std::unique_ptr<ClusterUserEntry> p = _clusterUserLog.Retrieve (off, VersionNo ());
		p->Dump (out);
		out << " |" << std::endl;
	}
	out << std::endl << "===Remote hubs:" << std::endl;
	out << "|! State |! Hub id |! Transport |" << std::endl;
	count = _hubList.Count ();
	for (i = 0; i < count; ++i)
	{
		out << "| ";
		File::Offset off = _hubList.Get (i)->Dump (out);
		out << " | ";
		std::unique_ptr<HubEntry> p = _hubLog.Retrieve (off, VersionNo ());
		p->Dump (out);
		out << " |" << std::endl;
	}
}

// Sequencers

void ProjectSeqImpl::FillInProjectData (Project::Data & projData)
{
	std::string projectName;
	std::string rootPath;
	_db.GetProjectData (_it, projectName, rootPath);
	projData.SetProjectId ((*_it)->GetProjId ());
	projData.SetProjectName (projectName);
	projData.SetRootPath (rootPath);
}

std::string UserSeqImpl::GetUserId ()
{
	return _db.GetUserId (_it);
}

void UserSeqImpl::FillInProjectData (ProjectUserData & projData)
{
	Address address;
	_db.GetUserData (_it, address);
	projData.SetProjectName (address.GetProjectName ());
	projData.SetHubId (address.GetHubId ());
	projData.SetUserId (address.GetUserId ());
	projData.SetProjectId ((*_it)->GetProjId ());
	if (IsRemoved ())
		projData.MarkRemoved ();

	const_iterator pit;
	for (pit = _db._projList.begin ();
		 pit != _db._projList.end ();
		 ++pit)
	{
		if ((*pit)->GetProjId () == (*_it)->GetProjId ())
			break;
	}
	if (pit != _db._projList.end ())
	{
		std::string foundProjName;
		std::string rootPath;
		_db.GetProjectData (pit, foundProjName, rootPath);
		if (IsNocaseEqual (foundProjName, address.GetProjectName ()))
			projData.SetRootPath (rootPath);
	}
}

std::string FileTypeSeqImpl::GetExt ()
{
	return _db.GetFileExtension (_it); 
}

FileType FileTypeSeqImpl::GetType ()
{
	return FileType ((*_it)->GetFileType ());
}

void ClusterRecipSeqImpl::GetClusterRecipient ( Address & address,
												Transport & transport,
												bool & isRemoved)
{
	_db.GetClusterRecipient (_it, address, transport);
	isRemoved = IsRemoved ();
}

void HubListSeqImpl::GetHubEntry (std::string & hubId, Transport & transport)
{
	_db.GetHubListEntry (_it, hubId, transport);
}

// must be called before any reading operation

void GlobalDb::Refresh (SysPathFinder & pathFinder)
{
	Win::MutexLock lock (_mutex);
	ReadTransaction xact (*this, pathFinder);
	GlobalDbHeader header (xact.GetDeserializer ());
	if (!header.IsValid (_versionStamp.XGetOriginal ()))
	{
		// read into backup
		Read (xact.GetDeserializer ());
		// copy backup to original
		xact.Commit ();
	}
};

// Transactable

void GlobalDb::Serialize (Serializer & out) const
{
	out.PutLong (_versionStamp.XGet ());

	out.PutLong (_topology.XGet ());

	out.PutLong (_trialStart.XGet ());
	_licensee.Serialize (out);
	_key.Serialize (out);
	_distributorLicensee.Serialize (out);
	_nextDistributorNumber.Serialize (out);
	_distributorLicenseCount.Serialize (out);

	_hubId.Serialize (out);

	_hubTransports.Serialize (out);
	_hubActiveTransport.Serialize (out);
	_myTransports.Serialize (out);
	_myActiveTransport.Serialize (out);
	_hubRemoteTransports.Serialize (out);
	_hubRemoteActiveTransport.Serialize (out);

	_preproCommand.Serialize (out);
	_preproResult.Serialize (out);
	_postproCommand.Serialize (out);
	_postproExtension.Serialize (out);
	_preproNeedsProjName.Serialize (out);

	_typeList.Serialize (out);
	_typeValidEnd.Serialize (out);
	_projList.Serialize (out);
	_projValidEnd.Serialize (out);
	_userList.Serialize (out);
	_userValidEnd.Serialize (out);
	_clusterList.Serialize (out);
	_clusterValidEnd.Serialize (out);

	_hubList.Serialize (out);
	_hubListValidEnd.Serialize (out);
}

// Important: these two Deserialize methods must be in synch

void GlobalDbHeader::Deserialize (Deserializer & in, int version)
{
	_versionStamp = in.GetLong ();
}

void GlobalDb::Deserialize (Deserializer& in, int version)
{
    CheckVersion (version, VersionNo ());

	// must be first!
	_versionStamp.XSet (in.GetLong ());
	
	if (version < 4)
	{
		XConvert (in, version);
		return;
	}

	// current format
	_topology.XSet (in.GetLong ());
	_trialStart.XSet (in.GetLong ());
	if (version >= 53)
	{
		_licensee.Deserialize (in, version);
		_key.Deserialize (in, version);
	}
	if (version >= 54)
	{
		_distributorLicensee.Deserialize (in, version);
		_nextDistributorNumber.Deserialize (in, version);
		_distributorLicenseCount.Deserialize (in, version);
	}

	_hubId.Deserialize (in, version);

	_hubTransports.Deserialize (in, version);
	_hubActiveTransport.Deserialize (in, version);
	_myTransports.Deserialize (in, version);
	_myActiveTransport.Deserialize (in, version);
	if (version >= 45)
	{
		_hubRemoteTransports.Deserialize (in, version);
		_hubRemoteActiveTransport.Deserialize (in, version);
	}
	
	_preproCommand.Deserialize (in, version);
	_preproResult.Deserialize (in, version);
	_postproCommand.Deserialize (in, version);
	_postproExtension.Deserialize (in, version);
	if (version < 46)
		_preproNeedsProjName.XSet (0);
	else
		_preproNeedsProjName.Deserialize (in, version);

	_typeList.Deserialize (in, version);
	_typeValidEnd.Deserialize (in, version);
	_projList.Deserialize (in, version);
	_projValidEnd.Deserialize (in, version);
	_userList.Deserialize (in, version);
	_userValidEnd.Deserialize (in, version);
	_clusterList.Deserialize (in, version);
	_clusterValidEnd.Deserialize (in, version);
	
	_hubList.Deserialize (in, version);
	_hubListValidEnd.Deserialize (in, version);
	if (version < 45)
	{
		// Find our own hub on the list of remote hubs
		std::unique_ptr<Transport> transport;
		unsigned i = 0;
		unsigned count = _hubList.XCount ();
		while (i < count)
		{
			LogNote const * note = _hubList.XGet (i);
			if (note->GetState () != stateInvalid)
			{
				File::Offset off = note->GetLogOffset ();
				std::unique_ptr<HubEntry> p = _hubLog.Retrieve (off, VersionNo ());
				if (p->IsEqualHubId (_hubId.XGet ()))
				{
					transport.reset (new Transport (p->GetTransport ()));
					break;
				}
			}
			++i;
		}
		if (i != count)
		{
			Assert (transport.get () != 0);
			_hubRemoteActiveTransport.XSet (transport->GetMethod ());
			_hubRemoteTransports.XAppend (std::move(transport));
			// remove our own hub from the list of remote hubs
			_hubList.XMarkDeleted (i);
		}
	}
}

// Conversion to version 4 format helpers
bool CheckIsEmail (std::string & route)
{
	// examine syntax for 'mailto:' prefix
	static std::string mailtoPrefix ("mailto:");
	std::string prefix (route.substr (0, 7));
	if (IsNocaseEqual (prefix, mailtoPrefix))
	{
		route = route.substr (7);
		return true;
	}
	return false;
}

void Convert2Transport (SerString const & str, Transport & transport)
{
	if (str.empty ())
	{
		transport.Set (str, Transport::Unknown);
	}
	else
	{
		std::string route = str;
		if (CheckIsEmail (route))
			transport.Set (route, Transport::Email);
		else
			transport.Set (route, Transport::Network);
	}
}

class OldClusterUserEntry: public Serializable
{
public:
	OldClusterUserEntry  (Deserializer & in, int version)
	{
		Deserialize (in, version);
	}
	Address const & GetAddress () const { return _address; }
	Transport const & GetTransport () const { return _transport; }

	void Serialize (Serializer& out) const
	{
		Assert (!"Not meant for serialization");
	}
	void Deserialize (Deserializer& in, int version)
	{
		SerString	hubId;
		SerString	userId;
		SerString	project;
		SerString   fwdPath;
		// mind the order of address triplet:
		// deserialize : hubId, userId, project
		// Address takes : hubId, project, userId
		hubId.Deserialize (in, version);
		userId.Deserialize (in, version);
		project.Deserialize (in, version);
		fwdPath.Deserialize (in, version);
        _address.Set (hubId, project, userId);
		Convert2Transport (fwdPath, _transport);
	}
private:
	Address		_address;
	Transport	_transport;
};

void GlobalDb::XConvert (Deserializer & in, int version)
{
	Topology topology;
	enum { isConfigured, isHubSetting, useEmailSetting, isProxySetting, isRemoteSatSetting, numSettings };
	std::bitset<numSettings> bits (in.GetLong ());
	if (!bits.test (isConfigured))
		topology.MakeStandalone ();
	else if (bits.test (isProxySetting))
		topology.MakeTemporaryHub ();
	else if (bits.test (isRemoteSatSetting))
		topology.MakeRemoteSatellite ();
	else if (bits.test (isHubSetting))
		topology.MakeHub ();
	else
		topology.MakeSatellite ();

	topology.SetUseEmail (bits.test (useEmailSetting));

	_topology.XSet (topology.ToUlong ());

	// Reset trial start when converting catalog
	_trialStart.XSet (CurrentTime ());
	in.GetLong ();	// Eat previous trial start
	_hubId.Deserialize (in, version); // always empty
	if (!bits.test (isConfigured))
	{
		// standalone
		_hubId.XSet ("Unknown");
	}

	SerString hubPath (in, version);
	std::unique_ptr<Transport> hubTransport (new Transport);
	Convert2Transport (hubPath, *hubTransport);
	if (!hubTransport->IsUnknown ())
	{
		_hubActiveTransport.XSet (hubTransport->GetMethod ());
		_hubTransports.XAppend (std::move(hubTransport));
	}
	SerString piShare (in, version);
	if (!piShare.empty ())
	{
		_myActiveTransport.XSet (Transport::Network);
		_myTransports.XAppend (std::unique_ptr<Transport>(new Transport (piShare,
																	   Transport::Network)));
	}
	_preproCommand.Deserialize (in, version);
	_preproResult.Deserialize (in, version);
	_postproCommand.Deserialize (in, version);
	_postproExtension.Deserialize (in, version);
	_preproNeedsProjName.XSet (0);

	_typeList.Deserialize (in, version);
	_typeValidEnd.Deserialize (in, version);
	_projList.Deserialize (in, version);
	_projValidEnd.Deserialize (in, version);
	_userList.Deserialize (in, version);
	_userValidEnd.Deserialize (in, version);
	_clusterList.Deserialize (in, version);
	_clusterValidEnd.Deserialize (in, version);

	// Convert ClusterLog.bin
	// prepare old cluster recipient log for reading
	const char oldClusterLogName [] = "ClusterLog.bin";
	Log<OldClusterUserEntry> oldClusterUserLog;
	oldClusterUserLog.Init (_dbPath.GetFilePath (oldClusterLogName));
	if (!oldClusterUserLog.Exists ())
		return;

	int count = _clusterList.XCount ();
	if (count == 0)
		return;

	// create new log
	_clusterUserLog.CreateFile ();

	// retrieve all offsets
	std::vector<File::Offset> offsets;
	offsets.reserve (count);
	for (int i = 0; i < count; ++i)
	{
		if (_clusterList.XGet (i) != 0)
			offsets.push_back (_clusterList.XGet (i)->GetLogOffset ());
	}
	// retrieve all entries
	auto_vector<OldClusterUserEntry> entries;
	oldClusterUserLog.Retrieve (offsets, entries, VersionNo ());
	// create all new entries
	auto_vector<ClusterUserEntry> newEntries;
	for (int i = 0; i < count; ++i)
	{
		newEntries.push_back (std::unique_ptr<ClusterUserEntry> (
			new ClusterUserEntry (entries [i]->GetAddress (), entries [i]->GetTransport ())));
	}
	// Serialize all new entries
	// append them to log
	offsets.clear ();
	File::Offset endOffset = _clusterUserLog.Append (File::Offset (0, 0), newEntries, offsets);
	_clusterValidEnd.XSet (endOffset);
	// add all the log notes
	for (int i = 0; i < count; ++i)
	{
		if (_clusterList.XGet (i) == 0)
			continue;
		State state = static_cast<State> (_clusterList.XGet (i)->GetState ());
		// delete old entry
		_clusterList.XMarkDeleted (i);
		_clusterList.XAppend (std::unique_ptr<LogNote> (new LogNote (state, offsets [i])));
	}
}

void GlobalDb::CheckVersion (int versionRead, int versionExpected) const
{
    if (versionRead > versionExpected)
    {
        TheOutput.Display ("Code Co-op cannot read the global database, because it is of newer version\n"
                           "You have to have the latest version of Code Co-op");
        throw Win::Exception ();
    }
    if (!ConversionSupported (versionRead))
    {
        TheOutput.Display ("Code Co-op cannot read the global database, because it was created using a very old version\n"
                           "You have to use an older version of Code Co-op to convert this database");
        throw Win::Exception ();
    }
}

void GlobalDb::ClusterUserEntry::Serialize (Serializer& out) const
{
	_address.Serialize (out);
	_transport.Serialize (out);
}

void GlobalDb::ClusterUserEntry::Deserialize (Deserializer& in, int version)
{
	_address.Deserialize (in, version);
	_transport.Deserialize (in, version);
}

void GlobalDb::DumpState (std::ostream & out, unsigned state)
{
	switch (state)
	{
	case stateInvalid:
		out << "invalid";
		break;
	case stateActive:
		out << "active";
		break;
	case stateRemoved:
		out << "removed";
		break;
	case stateUnavailable:
		out << "unavailable";
		break;
	default:
		out << "unknown";
		break;
	}
}
