#if !defined (DISPATCHERCMD_H)
#define DISPATCHERCMD_H
//-------------------------------------
//  (c) Reliable Software, 1999 -- 2007
//-------------------------------------

#include "Serialize.h"
#include "Transport.h"
#include "Invitee.h"
#include <TriState.h>
#include <SerString.h>
#include "SerVector.h"

// REVISIT: make this object the main interface for executing dispatcher commands
class DispatcherCmdExecutor
{
public:
	virtual ~DispatcherCmdExecutor () {}
	virtual void PostForceDispatch () = 0;
	virtual void AddDispatcherLicense (std::string const & licensee,
									   unsigned startNum,
									   unsigned count) = 0;
	virtual Tri::State AddAskInvitedClusterRecipient (Invitee const & invitee) = 0;
	virtual Tri::State InviteToProject (std::string const & adminName,
										std::string const & adminEmailAddress, 
										Invitee const & invitee) = 0;
	virtual void Restart (unsigned timeOutMs) = 0;
};

enum DispatcherCmdType
{
    typeAddressChange = 0,
	typeAddMember = 1,
	typeReplaceTransport = 2,
	typeReplaceRemoteTransport = 3,
	typeChangeHubId = 4,
	typeAddSatelliteRecipients = 5,
	typeForceDispatch = 6,
	typeChunkSize = 7,
	typeDistributorLicense = 8,
	typeInvitation = 9
};

class DispatcherCmd : public Serializable
{
public:
    static std::unique_ptr<DispatcherCmd> DeserializeCmd (Deserializer & in, int version);

    virtual ~DispatcherCmd () {}

    virtual DispatcherCmdType GetType () const = 0;
	virtual std::string GetErrorString () const { return "Script execution failed"; }
	bool IsForSendersCluster () const
	{
		// don't add member outside of the sender's cluster
		return GetType () == typeAddMember;
	}

    void Serialize (Serializer & out) const = 0;
    void Deserialize (Deserializer & in, int version) = 0;
};

class InvitationCmd : public DispatcherCmd
{
public:
	static std::string REQUIRES_ACTION;
public:
	InvitationCmd () {}
	InvitationCmd (std::string const & adminName,
				   std::string const & adminEmailAddress, 
				   Invitee const & invitee,
				   std::string const & defectFilename,
				   std::vector<unsigned char> const & defectScript)
		: _adminName (adminName),
		  _adminEmail (adminEmailAddress),
		  _invitee (invitee),
		  _defectFilename (defectFilename),
		  _defectScript (defectScript)
	{}
	// DispatcherCmd
	DispatcherCmdType GetType () const { return typeInvitation; }
	std::string GetErrorString () const { return InvitationCmd::REQUIRES_ACTION; }

	void Serialize (Serializer & out) const;
	void Deserialize (Deserializer & in, int version);

	// Invitation specific
	std::string const & GetAdminName () const { return _adminName; }
	std::string const & GetAdminEmailAddress () const { return _adminEmail; }
	Invitee const & GetInvitee () const { return _invitee; }
	std::string const & GetDefectFilename () const { return _defectFilename; }
	std::vector<unsigned char> const & GetDefectScript () const { return _defectScript; }
private:
	SerString	_adminName;
	SerString	_adminEmail;
	Invitee		_invitee;
	// defect script data
	SerString					_defectFilename;
	SerVector<unsigned char>	_defectScript;
};

// Updates or removes address from satellite recipient list
class AddressChangeCmd : public DispatcherCmd
{
public:
	AddressChangeCmd () {}
	AddressChangeCmd (std::string const & oldHubId,
					  std::string const & oldUserId,
					  std::string const & newHubId,
					  std::string const & newUserId)
		: _oldHubId (oldHubId),
		  _oldUserId (oldUserId),
		  _newHubId (newHubId),
		  _newUserId (newUserId)
	{}
    // DispatcherCmd
    DispatcherCmdType GetType () const { return typeAddressChange; }
	std::string GetErrorString () const { return "Illegal addressing"; }

    void Serialize (Serializer & out) const;
    void Deserialize (Deserializer & in, int version);

	// Address change specific
	std::string const & OldHubId () const { return _oldHubId; }
	std::string const & OldUserId () const { return _oldUserId; }
	std::string const & NewHubId () const { return _newHubId; }
	std::string const & NewUserId () const { return _newUserId; }

private:
	// Project name in transport header -- no need to repeat here
	SerString	_oldHubId;
	SerString	_oldUserId;
	SerString	_newHubId;
	SerString	_newUserId;
};

// Add new member to satellite recipient list (sender's hub only)
class AddMemberCmd : public DispatcherCmd
{
public:
	AddMemberCmd () {}
	AddMemberCmd (std::string const & hubId,
				  std::string const & userId,
				  Transport const & transport)
		: _hubId (hubId),
		  _userId (userId),
		  _transport (transport)
	{}
    // DispatcherCmd
    DispatcherCmdType GetType () const { return typeAddMember; }
	std::string GetErrorString () const { return "Illegal addressing"; }

    void Serialize (Serializer & out) const;
    void Deserialize (Deserializer & in, int version);

	// AddMember specific
	std::string const & HubId () const { return _hubId; }
	std::string const & GetUserId () const { return _userId; }
	Transport const & TransportMethod () const { return _transport; }

private:
	// Project name in transport header -- no need to repeat here
	SerString	_hubId;
	SerString	_userId;
	Transport	_transport;
};

// Add satellite recipients to hub's satellite recipient list (sender's hub only)
class AddSatelliteRecipientsCmd : public DispatcherCmd
{
public:
	AddSatelliteRecipientsCmd () {}
	AddSatelliteRecipientsCmd (Transport const & transport)
		: _transport (transport)
	{}
	// DispatcherCmd
	DispatcherCmdType GetType () const { return typeAddSatelliteRecipients; }
	std::string GetErrorString () const { return "Illegal addressing"; }

	void Serialize (Serializer & out) const;
	void Deserialize (Deserializer & in, int version);

	// AddMemberList specific
	void AddMember (Address const & address, bool isRemoved) 
	{ 
		if (isRemoved)
			_removedAddresses.push_back (address); 
		else
			_activeAddresses.push_back (address); 
	} 
	Transport const & TransportMethod () const { return _transport; }

	typedef SerVector<Address>::const_iterator const_iterator;
	const_iterator beginActive  () const { return _activeAddresses.begin ();  }
	const_iterator endActive    () const { return _activeAddresses.end ();    }
	const_iterator beginRemoved () const { return _removedAddresses.begin (); }
	const_iterator endRemoved   () const { return _removedAddresses.end ();   }
private:
	Transport			_transport;
	SerVector<Address>	_activeAddresses;
	SerVector<Address>	_removedAddresses;
};

// On a hub, replace transport to all satellite recipients who are using it
// On a satellite, replace transport to hub
class ReplaceTransportCmd : public DispatcherCmd
{
public:
	ReplaceTransportCmd () {}
	ReplaceTransportCmd (Transport const & oldTransport,
					     Transport const & newTransport)
		: _old (oldTransport),
		  _new (newTransport)
	{}
    // DispatcherCmd
    DispatcherCmdType GetType () const { return typeReplaceTransport; }

    void Serialize (Serializer & out) const;
    void Deserialize (Deserializer & in, int version);

	Transport const & OldTransport () const { return _old; }
	Transport const & NewTransport () const { return _new; }

private:
	Transport	_old;
	Transport	_new;
};

class ForceDispatchCmd : public DispatcherCmd
{
public:
	ForceDispatchCmd () {}
    // DispatcherCmd
    DispatcherCmdType GetType () const { return typeForceDispatch; }

    void Serialize (Serializer & out) const;
    void Deserialize (Deserializer & in, int version);
private:
};

class ChunkSizeCmd : public DispatcherCmd
{
public:
	ChunkSizeCmd () : _chunkSize (0) {}
	ChunkSizeCmd (unsigned long chunkSize)
		: _chunkSize (chunkSize) 
	{}
    // DispatcherCmd
    DispatcherCmdType GetType () const { return typeChunkSize; }

	unsigned long GetChunkSize () const { return _chunkSize; }

    void Serialize (Serializer & out) const;
    void Deserialize (Deserializer & in, int version);
private:
	unsigned long _chunkSize;
};

//-----------------
// Remote Hub Table
//-----------------

// Replace transport for this Hub's Email Address in all remote hub tables
class ReplaceRemoteTransportCmd : public DispatcherCmd
{
public:
	ReplaceRemoteTransportCmd () {}
	ReplaceRemoteTransportCmd (std::string const & hubId,
					     Transport const & newTransport)
	 : _transport (newTransport), _hubId (hubId)
	{}
    // DispatcherCmd
    DispatcherCmdType GetType () const { return typeReplaceRemoteTransport; }

	std::string const & HubId () const { return _hubId; }
	Transport const & GetTransport () const { return _transport; }

    void Serialize (Serializer & out) const;
    void Deserialize (Deserializer & in, int version);

private:
	SerString	_hubId;
	Transport	_transport;
};

// Replace this Hub's Email Address in everybody's remote hub list 
class ChangeHubIdCmd : public DispatcherCmd
{
public:
	ChangeHubIdCmd () {}
	ChangeHubIdCmd (std::string const & oldHubId, std::string const & newHubId, Transport const & newTransport)
		: _oldHubId (oldHubId), _newHubId (newHubId), _newTransport (newTransport)
	{}
    // DispatcherCmd
    DispatcherCmdType GetType () const { return typeChangeHubId; }

	std::string const & OldHubId () const { return _oldHubId; }
	std::string const & NewHubId () const { return _newHubId; }
	Transport const & NewTransport () const { return _newTransport; }

    void Serialize (Serializer & out) const;
    void Deserialize (Deserializer & in, int version);
private:
	SerString	_oldHubId;
	SerString	_newHubId;
	Transport	_newTransport;
};

class DistributorLicenseCmd: public DispatcherCmd
{
public:
	DistributorLicenseCmd () {}
	DistributorLicenseCmd (std::string const & licensee, unsigned startNum, unsigned count)
		: _licensee (licensee), _startNum (startNum), _count (count)
	{}
    // DispatcherCmd
    DispatcherCmdType GetType () const { return typeDistributorLicense; }
	std::string const & Licensee () const { return _licensee; }
	unsigned StartNum () const { return _startNum; }
	unsigned Count () const { return _count; }

    void Serialize (Serializer & out) const;
    void Deserialize (Deserializer & in, int version);
private:
	SerString	_licensee;
	unsigned	_startNum;
	unsigned	_count;
};

#endif
