#if !defined (CLUSTERRECIPIENT_H)
#define CLUSTERRECIPIENT_H
// ----------------------------------
// (c) Reliable Software, 1999 - 2006
// ----------------------------------

#include "Recipient.h"
#include "Transport.h"
#include <File/Path.h>

class ClusterRecipient : public Recipient
{
public:
	ClusterRecipient () {}
	ClusterRecipient (Address const & address, Transport const & transport)
		: Recipient (address), _transport (transport)
	{}
    ClusterRecipient (std::string const & hubId, std::string const & project, 
                      std::string const & userId, Transport const & transport)
        : Recipient (hubId, project, userId), _transport (transport)
    {}
	Transport const & GetTransport () const { return _transport; }
	void ChangeTransport (Transport const & transport)
	{
		_transport = transport;
	}

    // Recipient interface
    bool AcceptScript (ScriptTicket & script, MailTruck & truck, int recipIdx);

private:
    Transport _transport;
};

class Catalog;

class ClusterRecipientList
{
public:
	ClusterRecipientList (Catalog & catalog) : _catalog (catalog) {}
    unsigned Refresh ();
    void Clear () { _recips.clear (); }
	void ClearPersistent ();
	void PurgeInMemory ();

	ClusterRecipient * Add (ClusterRecipient const & newRecip);
	void AddRemoved (ClusterRecipient const & newRecip);
	void RemoveRecipient (Address const & address);
	void KillRecipient (Address const & address);
	void UpdateAddress (Address const & currentAddress,
		std::string const & newHubId,
		std::string const & newUserId);
	void UpdateTransport (Address const & address, Transport const & transport);
	void ReplaceTransport (Transport const & oldTransport, Transport const & transport);

	void Activate (Address const & address);

	ClusterRecipient * Find (Address const & address);
	ClusterRecipient const * Find (Address const & address) const;
	ClusterRecipient * FindRelaxed (Address const & address);
	Recipient * FindWildcard (std::string const & hubId, 
							 std::string const & projName, 
							 std::string const & senderHubId, 
							 std::string const & senderUserId);
	std::string FindPath (std::string const & computerName) const;
	int CountTransportUsers (Transport const & transport) const;
	int GetCount () const { return _recips.size (); }

	void GetActiveSatelliteTransports (std::set<Transport> & transports) const;
	void GetActiveProjectList (NocaseSet & projects) const;
	void GetActiveOffsiteSatList (NocaseSet & addresses) const;
public:
    typedef std::deque<ClusterRecipient> CluRecList;
    typedef std::deque<ClusterRecipient>::iterator iterator;
    typedef std::deque<ClusterRecipient>::const_iterator const_iterator;

    iterator begin () { return _recips.begin (); }
    iterator end () { return _recips.end (); }
    const_iterator begin () const { return _recips.begin (); }
    const_iterator end () const { return _recips.end (); }

private:
	iterator FindByAddress (Address const & addr);
	const_iterator FindByAddress (Address const & addr) const;
private:
	Catalog &	_catalog;
	CluRecList	_recips;
};

#endif
