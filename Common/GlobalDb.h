#if !defined (GLOBALDB_H)
#define GLOBALDB_H
//----------------------------------
// (c) Reliable Software 2000 - 2006
//----------------------------------

#include "Address.h"
#include "FileTypes.h"
#include "Transactable.h"
#include "Params.h"
#include "XArray.h"
#include "XString.h"
#include "XLong.h"
#include "XFileOffset.h"
#include "SerString.h"
#include "Transport.h"

#include <File/Path.h>
#include <Sys/Synchro.h>
#include <Dbg/Assert.h>

namespace Project
{
	class Data;
}

class ProjectUserData;
class License;

namespace XML { class Node; }

class GlobalDb: public TransactableContainer
{
	friend class CatTransaction;
	friend class FileTypeSeqImpl;
	friend class ProjectSeqImpl;
	friend class UserSeqImpl;
	friend class HubListSeqImpl;
	friend class ClusterRecipSeqImpl;
public:
	// Embedded classes
	class FileTypeNote;
	class FileTypeEntry;
	class ProjectNote;
	class ProjEntry;
	class LogNote;
	class ClusterUserEntry;
	class HubEntry;
	template <class T> 	class NoteSeq;
public:
	enum State
	{
		stateInvalid = -1,
		stateActive = 0,
		stateRemoved = 1,
		stateUnavailable = 2
	};
public:
	GlobalDb (FilePath const & dbPath);

	// all accessing is done AFTER a read transaction
	Topology GetTopology () const
	{
		return Topology (_topology.Get ());
	}
	// all modifying is done UNDER a write transaction
	void XSetTopology (Topology topology)
	{
		_topology.XSet (topology.ToUlong ());
	}
	std::string GetHubId () const { return std::string (_hubId); }
	void XSetHubId (std::string const & hubId) { _hubId.XSet (hubId); }

	Transport GetActiveIntraClusterTransportToMe () const;
	void GetIntraClusterTransportsToMe (std::vector<Transport> & transports, Transport::Method & active) const;
	void XSetIntraClusterTransportsToMe (std::vector<Transport> const & transports, Transport::Method active);
	void GetTransportsToHub (std::vector<Transport> & transports, Transport::Method & active) const;
	void XSetTransportsToHub (std::vector<Transport> const & transports, Transport::Method active);
	void GetInterClusterTransportsToMe (std::vector<Transport> & transports, Transport::Method & active) const;
	void XSetInterClusterTransportsToMe (std::vector<Transport> const & transports, Transport::Method active);
	Transport GetHubRemoteActiveTransport () const;

	// Licensing
	void XSetTrialStart (long trialStart) { _trialStart.XSet (trialStart); }
	long GetTrialStart () const { return _trialStart.Get (); }
	void XSetLicense (std::string const & licensee, std::string const & key);
	std::string const & GetLicensee () const { return _licensee; }
	std::string const & GetKey () const { return _key; }

	std::string const & GetDistributorLicensee () const { return _distributorLicensee; }
	unsigned long GetNextDistributorNumber () const { return _nextDistributorNumber.Get (); }
	unsigned long GetDistributorLicenseCount () const { return _distributorLicenseCount.Get (); }
	void XSetDistributorLicensee (std::string const & licensee) { _distributorLicensee.XSet (licensee); }
	void XSetNextDistributorNumber (unsigned long num) { _nextDistributorNumber.XSet(num); }
	void XRemoveDistributorLicense () { _nextDistributorNumber.XSet (_nextDistributorNumber.XGet () + 1); }
	void XSetDistributorLicenseCount (unsigned long num)  { _distributorLicenseCount.XSet(num); }
	void XClearDistributorLicenses ()
	{
		_distributorLicenseCount.XSet (0);
		_nextDistributorNumber.XSet (0);
	}
	// Projects
	int  XAddProject (std::string const & name, std::string const & path);
	void XRemoveProject (int projId);
	void XMoveProjectTree (int projId, std::string const & newPath);
	void XAddProject (int projId, std::string const & name, std::string const & path);
	void XMarkProjectUnavailable (int projId);
	void XMarkProjectAvailable (int projId);
	// Users
	void XAddUser (Address const & address, int projectId);
	void XRemoveUser (Address const & address, bool permanently);
	void XKillLocalRecipient (Address const & address, int projectId);
	bool XKillRemovedLocalRecipient (Address const & address);
	void XRefreshUserData (Address const & address,
							std::string const & newHubId,
							std::string const & newUserId,
							bool permanently);
	void XLocalHubIdChanged (std::string const & oldHubId, std::string const & newHubId);
	// File types
	void XAddFileType (std::string const & extension, unsigned long type);
	
	// Recipient lists
	bool XPurgeLocalRecipients ();
	bool XPurgeSatelliteRecipients ();

    void XAddClusterRecipient (Address const & address, Transport const & transport);
	void XRemoveClusterRecipient (Address const & address, bool permanently);
	void XActivateClusterRecipient (Address const & address);
	void XChangeTransport (Address const & address, Transport const & transport);
	void XClearClusterRecipients ();
	void XAddRemoteHub (std::string const & hubId, Transport const & transport);
	void XDeleteRemoteHub (std::string const & hubId);

	void Refresh (SysPathFinder & pathFinder);
	void Dump (XML::Node * root, std::ostream & out, bool isDiagnostic) const;
	// Transactable
	void IncrementVersion () { _versionStamp.XSet (_versionStamp.XGet() + 1); }

	// Serializable
	int  VersionNo () const { return catalogVersion; }
	int  SectionId () const { return 'GLOB'; }
	bool IsSection () const { return true; }
	bool ConversionSupported (int versionRead) const { return versionRead > 2; }
	void CheckVersion (int versionRead, int versionExpected) const;
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);

private:
	static const char _fileTypeLogName [];
	static const char _projLogName [];
	static const char _userLogName [];
	static const char _clusterLogName [];
	static const char _hubLogName [];

private:
	void Init (FilePath const & catPath);

	void XConvert (Deserializer & in, int version);
	std::string GetFileExtension (SoftXArray<FileTypeNote>::const_iterator it) const;

	void GetClusterRecipient (SoftXArray<LogNote>::const_iterator it,
		  					  Address & address,
							  Transport & transport) const;

	void GetProjectData (SoftXArray<ProjectNote>::const_iterator it, 
						 std::string & projectName, 
						 std::string & srcPath) const;
	std::string GetProjName (SoftXArray<ProjectNote>::const_iterator it) const;
	std::string GetProjPath (SoftXArray<ProjectNote>::const_iterator it) const;

	void GetUserData  (SoftXArray<ProjectNote>::const_iterator it,
					   Address & address) const;
	std::string GetUserHubId (SoftXArray<ProjectNote>::const_iterator it) const;
	std::string GetUserId (SoftXArray<ProjectNote>::const_iterator it) const;
	void GetHubListEntry (SoftXArray<LogNote>::const_iterator it,
							std::string & hubId,
							Transport & transport) const;
	bool XActivateUser (Address const & address, int projectId);
	static void DumpState (std::ostream & out, unsigned state);

private:
	mutable FilePath	_dbPath;
	Win::Mutex			_mutex;

	// Persistent data
	XLong	_versionStamp;// every update increments it on-disk
	// Licensing
	XLong	_trialStart;
	XString	_licensee;
	XString	_key;
	// Distributor licenses
	XString _distributorLicensee;
	XLong	_nextDistributorNumber;
	XLong	_distributorLicenseCount;

	XLong	_topology;	// hub/satellite, flags, etc...

	XString	_hubId;

	TransactableArray<Transport> _hubTransports;
	XLong						 _hubActiveTransport; // to my hub (on satellites only)
	TransactableArray<Transport> _myTransports;
	XLong						 _myActiveTransport; // hub<->satellite exchanges
	TransactableArray<Transport> _hubRemoteTransports;
	XLong						 _hubRemoteActiveTransport; // from remote hubs to my hub

	// Revisit: not used. Remove when changing catalog version
	XString	_preproCommand;
	XString	_preproResult;
	XString	_postproCommand;
	XString	_postproExtension;
	XLong	_preproNeedsProjName;

	// file types
	TransactableArray<FileTypeNote>	_typeList;
	XFileOffset						_typeValidEnd;
	Log<FileTypeEntry>				_typeLog;
	// local enlistments
	TransactableArray<ProjectNote>	_projList;
	XFileOffset						_projValidEnd;
	Log<ProjEntry>					_projLog;
	// local users
	TransactableArray<ProjectNote>	_userList;
	XFileOffset						_userValidEnd;
	Log<Address>					_userLog;
	// cluster users
	TransactableArray<LogNote>		_clusterList;
	XFileOffset						_clusterValidEnd;
	Log<ClusterUserEntry>			_clusterUserLog;
	// Transport to remote hubs
	TransactableArray<LogNote>		_hubList;
	XFileOffset						_hubListValidEnd;
	Log<HubEntry>					_hubLog;
};

// Used only to read the version stamp off the database
class GlobalDbHeader: public Serializable
{
public:
	GlobalDbHeader (Deserializer & deser)
	{
		Read (deser);
		deser.Rewind ();
	}
	bool IsValid (long versionStamp)
	{
		return _versionStamp == versionStamp;
	}
private:
	int  VersionNo () const { return catalogVersion; }
	int  SectionId () const { return 'GLOB'; }
	bool IsSection () const { return true; }
	void Serialize (Serializer& out) const 
	{
		Assert (!"GlobalDbHeader doesn't support serialization");
	}
	void Deserialize (Deserializer& in, int version);
private:
	long	_versionStamp;
};

class CatTransaction: Win::MutexLock, public Transaction
{
public:
	CatTransaction (GlobalDb & db, SysPathFinder & pathFinder)
		: Win::MutexLock (db._mutex),
		  Transaction (db, pathFinder),
		  _db (db)
	{
		GlobalDbHeader header (GetDeserializer ());
		if (!header.IsValid (db._versionStamp.XGetOriginal ()))
		{
			// read into backup
			db.Read (GetDeserializer ());
		}
	}
	void Commit ()
	{
		_db.IncrementVersion ();
		Transaction::Commit ();
	}
private:
	GlobalDb & _db;
};

//--------------------------------------------
// Definitions of classes embedded in GlobalDb
//--------------------------------------------

class GlobalDb::FileTypeNote: public Serializable
{
public:
	FileTypeNote (Deserializer& in, int version)
	{
		Deserialize (in, version);
	}
	FileTypeNote (unsigned long fileType, unsigned long state, File::Offset logOffset)
		: _fileType (fileType), _logOffset (logOffset), _state (state)
	{}
	unsigned long GetFileType () const { return _fileType; }
	File::Offset GetLogOffset () const { return _logOffset; }
	unsigned long GetState () const { return _state; }
	void SetState (unsigned long state) { _state = state; }
	void SetType (unsigned long type) { _fileType = type; }
	void Serialize (Serializer& out) const
	{
		out.PutLong (_fileType);
		out.PutLong (_state);
		_logOffset.Serialize (out);
	}
	void Deserialize (Deserializer& in, int version)
	{
		_fileType = in.GetLong ();
		_state = in.GetLong ();
		_logOffset.Deserialize (in, version);
	}
	File::Offset Dump (std::ostream & out) const
	{
		out << "Type: " << _fileType << "State: ";
		DumpState (out, _state);
		return _logOffset;
	}
private:
	unsigned long	_fileType;
	unsigned long	_state;
	SerFileOffset	_logOffset;
};

class GlobalDb::FileTypeEntry: public Serializable
{
public:
	FileTypeEntry (std::string const & ext)
		: _ext (ext)
	{}
	FileTypeEntry (Deserializer & in, int version)
	{
		Deserialize (in, version);
	}
	std::string const & GetExt () const { return _ext; }
	void Serialize (Serializer& out) const
	{
		_ext.Serialize (out);
	}
	void Deserialize (Deserializer & in, int version)
	{
		_ext.Deserialize (in, version);
	}
	void Dump (std::ostream & out) const
	{
		out << "Extension: " << _ext << std::endl;
	}
private:
	SerString	_ext;
};

class GlobalDb::ProjectNote: public Serializable
{
public:
	ProjectNote (Deserializer & in, int version)
	{
		Deserialize (in, version);
	}
	ProjectNote (int projId, unsigned long state, File::Offset logOffset)
		: _projId (projId), _logOffset (logOffset), _state (state)
	{}
	long GetProjId () const { return _projId; }
	File::Offset GetLogOffset () const { return _logOffset; }
	long GetState () const { return _state; }
	void SetProjId (long id) { _projId = id; }
	void SetState (long state) { _state = state; }
	void SetLogOffset (File::Offset off) { _logOffset = off; }
	void Serialize (Serializer& out) const
	{
		out.PutLong (_projId);
		out.PutLong (_state);
		_logOffset.Serialize (out);
	}
	void Deserialize (Deserializer& in, int version)
	{
		_projId = in.GetLong ();
		_state = in.GetLong ();
		_logOffset.Deserialize (in, version);
	}
	File::Offset Dump (std::ostream & out) const
	{
		out << _projId << " | ";
		DumpState (out, _state);
		return _logOffset;
	}

private:
	long			_projId;
	long			_state;
	SerFileOffset	_logOffset;
};

class GlobalDb::ProjEntry: public Serializable
{
public:
	ProjEntry (std::string const & name, std::string const & path)
		: _name (name), _path (path)
	{}
	ProjEntry  (Deserializer & in, int version)
	{
		Deserialize (in, version);
	}
	std::string const & GetName () const { return _name; }
	std::string const & GetPath () const { return _path; }
	void Serialize (Serializer& out) const
	{
		_name.Serialize (out);
		_path.Serialize (out);
	}
	void Deserialize (Deserializer& in, int version)
	{
		_name.Deserialize (in, version);
		_path.Deserialize (in, version);
	}
	void Dump (std::ostream & out) const
	{
		out << _name << " | " << _path;
	}
private:
	SerString	_name;
	SerString	_path;
};

class GlobalDb::LogNote: public Serializable
{
public:
	LogNote (Deserializer& in, int version)
	{
		Deserialize (in, version);
	}
	LogNote (long state, File::Offset logOffset)
		: _state (state), _logOffset (logOffset)
	{}
	File::Offset GetLogOffset () const { return _logOffset; }
	void SetOffset (File::Offset offset) { _logOffset = offset; }
	long GetState () const { return _state; }
	void SetState (long state) { _state = state; }
	void Serialize (Serializer& out) const
	{
		out.PutLong (_state);
		_logOffset.Serialize (out);
	}
	void Deserialize (Deserializer& in, int version)
	{
		_state = in.GetLong ();
		_logOffset.Deserialize (in, version);
	}
	File::Offset Dump (std::ostream & out) const
	{
		switch (_state)
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
		return _logOffset;
	}

private:
	long			_state;
	SerFileOffset	_logOffset;
};

class GlobalDb::ClusterUserEntry: public Serializable
{
public:
	ClusterUserEntry (Address const & address, Transport const & transport)
		: _address (address),
		  _transport (transport)
	{}
	ClusterUserEntry  (Deserializer & in, int version)
	{
		Deserialize (in, version);
	}
	Address const & GetAddress () const { return _address; }
	bool IsEqualAddress (Address const & address) const
	{
		return address.IsEqualAddress (_address);
	}
	Transport const & GetTransport () const { return _transport; }

	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);
	void Dump (std::ostream & out) const
	{
		_address.Dump (out);
		out << " | " << _transport;
	}
private:
	Address		_address;
	Transport	_transport;
};

class GlobalDb::HubEntry: public Serializable
{
public:
	HubEntry (std::string const & hubId, Transport const & transport)
		: _hubId (hubId),
			_transport (transport)
	{}
	HubEntry  (Deserializer & in, int version)
	{
		Deserialize (in, version);
	}
	Transport const & GetTransport () const { return _transport; }
	std::string const & GetHubId () const { return _hubId; }
	bool IsEqualHubId (std::string const & hubId) const
	{
		return IsNocaseEqual (hubId, _hubId);
	}
	bool IsEqualTransport (Transport const & transport)
	{
		return transport == _transport;
	}
	void Serialize (Serializer& out) const
	{
		_hubId.Serialize (out);
		_transport.Serialize (out);
	}
	void Deserialize (Deserializer& in, int version)
	{
		_hubId.Deserialize (in, version);
		_transport.Deserialize (in, version);
	}
	void Dump (std::ostream & out) const
	{
		out << ReplaceNullPadding<'.'> (_hubId) << " | " << _transport;
	}
private:
	SerString	_hubId;
	Transport	_transport;
};

//--------------------------
// Sequencer Implementations
//--------------------------

template <class T>
class GlobalDb::NoteSeq
{
public:
	NoteSeq (GlobalDb & globalDb, 
		  		SysPathFinder & pathFinder,
				TransactableArray<T> const & notes)
		: _db (globalDb)
	{
		_db.Refresh (pathFinder);
		_it = notes.begin ();
		_end = notes.end ();
		SkipNonValid ();
	}
	bool AtEnd () const 
	{
		return _it == _end; 
	}
	void Advance ()
	{
		++_it;
		SkipNonValid ();
	}
protected:
	typedef typename TransactableArray<T>::const_iterator const_iterator;
	GlobalDb & _db;
	const_iterator _it;
	const_iterator _end;
private:
	void SkipNonValid ()
	{
		while (!AtEnd () && (*_it)->GetState () == stateInvalid)
			++_it;
	}
};

class ProjectSeqImpl : public GlobalDb::NoteSeq<GlobalDb::ProjectNote>
{
public:
	ProjectSeqImpl (GlobalDb & globalDb, SysPathFinder & pathFinder)
		: GlobalDb::NoteSeq<GlobalDb::ProjectNote> (globalDb, 
													pathFinder, 
													globalDb._projList)
	{}

	bool IsProjectUnavailable () const { return (*_it)->GetState () == GlobalDb::stateUnavailable; }
	int  GetProjectId () const { return (*_it)->GetProjId (); }
	std::string GetProjectName ()
	{
		return _db.GetProjName (_it);
	}
	FilePath GetProjectSourcePath ()
	{
		return _db.GetProjPath (_it);
	}
	void FillInProjectData (Project::Data & projData);
};

class UserSeqImpl : public GlobalDb::NoteSeq<GlobalDb::ProjectNote>
{
public:
	UserSeqImpl (GlobalDb & globalDb, SysPathFinder & pathFinder)
		: GlobalDb::NoteSeq<GlobalDb::ProjectNote> (globalDb, 
													pathFinder, 
													globalDb._userList)
	{}
	std::string GetUserId ();
	int  GetProjectId () const { return (*_it)->GetProjId (); }
	void FillInProjectData (ProjectUserData & projData);
	bool IsRemoved () const { return (*_it)->GetState () == GlobalDb::stateRemoved; }
};

class FileTypeSeqImpl : public GlobalDb::NoteSeq<GlobalDb::FileTypeNote>
{
public:
	FileTypeSeqImpl (GlobalDb & globalDb, SysPathFinder & pathFinder)
		: GlobalDb::NoteSeq<GlobalDb::FileTypeNote> (globalDb, 
													 pathFinder, 
													 globalDb._typeList)
	{}
	std::string GetExt ();
	FileType GetType ();
};

class ClusterRecipSeqImpl : public GlobalDb::NoteSeq<GlobalDb::LogNote>
{
public:
	ClusterRecipSeqImpl (GlobalDb & globalDb, SysPathFinder & pathFinder)
		: GlobalDb::NoteSeq<GlobalDb::LogNote> (globalDb, pathFinder, globalDb._clusterList)
	{}
	bool IsRemoved () const { return (*_it)->GetState () == GlobalDb::stateRemoved; }
	void GetClusterRecipient (Address & address, Transport & transport, bool & isRemoved);
};

class HubListSeqImpl : public GlobalDb::NoteSeq<GlobalDb::LogNote>
{
public:
	HubListSeqImpl (GlobalDb & globalDb, SysPathFinder & pathFinder)
		: GlobalDb::NoteSeq<GlobalDb::LogNote> (globalDb, pathFinder, globalDb._hubList)
	{}
	void GetHubEntry (std::string & hubId, Transport & transport);
};

#endif
