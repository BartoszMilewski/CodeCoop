#if !defined (ADDRESSDB_H)
#define ADDRESSDB_H
// ----------------------------------
// (c) Reliable Software, 1999 - 2006
// ----------------------------------

#include "Table.h"
#include "LocalRecipient.h"
#include "ClusterRecipient.h"
#include "RemoteHub.h"

class Catalog;
class Recipient;
class FilePath;
class RecipientKey;

class AddressDatabase : public Table
{
	class EmptyRecipient: public Recipient
	{
	public:
		EmptyRecipient () : Recipient ("nowhere", "any", "any")
		{}
		bool AcceptScript (ScriptTicket * script, MailTruck & truck, int recipIdx);
	};
public:
	AddressDatabase (Catalog & catalog, std::string const & hubId, bool isLoadCluster);
	void LoadLocalRecips (std::string const & currentHubId = std::string ());
    void ClearLocalRecips () { _localRecipientList.Clear (); }
	unsigned LoadClusterRecips (); // returns count
	void ClearClusterRecips () { _clusterRecipientList.Clear (); }
	
	LocalRecipientList const & GetLocalRecipients () const { return _localRecipientList; }
	ClusterRecipientList const & GetClusterRecips () const { return _clusterRecipientList; }
	void GetInviteeList (std::vector<Address> & inviteeList) const;
	void GetActiveSatelliteTransports (std::set<Transport> & transports) const
	{
		_clusterRecipientList.GetActiveSatelliteTransports (transports);
	}
	RemoteHubList & GetRemoteHubList () { return _remoteHubList; }
	RemoteHubList const & GetRemoteHubList () const { return _remoteHubList; }
	void GetActiveProjectList (NocaseSet & projects) const
	{
		_localRecipientList.GetActiveProjectList (projects);
		_clusterRecipientList.GetActiveProjectList (projects);
	}
	void GetActiveOffsiteSatList (NocaseSet & addresses) const
	{ 
		_clusterRecipientList.GetActiveOffsiteSatList (addresses); 
	}
	void ListLocalHubIdsByImportance (std::vector<std::string> & idList) const;
	void ListClusterHubIdsByImportance (std::vector<std::string> & idList) const;

	Recipient * Find (Address const & address);
	Recipient * FindLocal (Address const & address);
	Recipient * FindWildcard (std::string const & hubId, 
							  std::string const & projName, 
							  std::string const & senderHubId, 
							  std::string const & senderLocation);
	std::string FindSatellitePath (std::string const & computerName) const;
	// Remote Hub List
	bool IsRemoteHub (std::string const & hubId) const { return _remoteHubList.IsKnown (hubId); }
	void AddInterClusterTransport (std::string const & hubId, Transport const & transport);
	void GetAskRemoteTransport (std::string const & hubId, Transport & transport);
	Transport const & GetInterClusterTransport (std::string const & hubId) const;
	void UpdateInterClusterTransport (std::string const & hubId, Transport const & newTransport);
	bool VerifyAddRemoteHub (std::string const & hubId);
	void ChangeRemoteHubId (std::string const & oldHubId, 
							std::string const & newHubId, 
							Transport const & newTransport);
	void DeleteRemoteHub (std::string const & hubId);
	
	bool HasAnyClusterRecipients () const { return _clusterRecipientList.GetCount () > 0; }
	int CountTransportUsers (Transport const & transport) const;
											  
	bool KillRemovedLocalRecipient (Address const & address);
	void AddTempLocalRecipient (Address const & address, bool isRemoved);
	void KillTempLocalRecipient (Address const & address);

	Recipient * AddClusterRecipient (Address const & address, Transport const & transport);
	void AddRemovedClusterRecipient (Address const & address);
	ClusterRecipient const * FindClusterRecipient (Address const & address) const;
	void KillClusterRecipient (Address const & address);

    void ForgetClusterRecips ();
	bool Purge (bool purgeLocal, bool purgeSat);
	bool UpdateAddress (std::string const & project, 
						std::string const & oldHubId, std::string const & oldLocation,
                        std::string const & newHubId, std::string const & newLocation);
	void RemoveAddress (Address const & address);
	void RemoveSatelliteAddress (Address const & address);
    bool UpdateTransport (Address const & address, Transport const & transport);
	void ReplaceTransport (Transport const & oldTransport, Transport const & newTransport);

	// Table interface
    void QueryUniqueNames (std::vector<std::string> & unames,
						   Restriction const * restrict = 0);
    int	GetNumericField (Column col, std::string const & uname) const;

    void QueryUniqueTripleKeys (std::vector<TripleKey> & ukeys,
								Restriction const * restrict = 0);
    std::string	GetStringField (Column col, TripleKey const & ukey) const;
    int	GetNumericField (Column col, TripleKey const & ukey) const;
	std::string QueryCaption (Restriction const & r) const;
	void HubIdChanged (std::string const & oldHubId, std::string const & newHubId);
	bool IsProjectLocal (std::string const & project) 
	{
		return _localRecipientList.FindProject (project);
	}

private:
	bool KillRemovedLocalOrWarnUser (Address const & address);
	bool IsEqualNameUnderRestrict (Recipient const & recip) const;
	bool IsEqualAddressUnderRestrict (Recipient const & recip, 
									  RecipientKey const & ukey) const;

	Catalog &	_catalog;
	// Revisit: the recipient lists should be soft-transactable
    LocalRecipientList			_localRecipientList;
    ClusterRecipientList		_clusterRecipientList;
	RemoteHubList				_remoteHubList;

	EmptyRecipient _nobody; // has special address "nowhere"
};

#endif
