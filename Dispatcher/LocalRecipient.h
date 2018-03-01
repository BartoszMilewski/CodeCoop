#if !defined (LOCALRECIPIENT_H)
#define LOCALRECIPIENT_H
//----------------------------------
// (c) Reliable Software 2000 - 2006
//----------------------------------

#include "Recipient.h"
#include "Mailboxes.h"

#include <File/Path.h>
#include <auto_vector.h>

class Catalog;

class LocalRecipient : public Recipient
{
public:
    LocalRecipient (std::string const & hubId, 
					std::string const & projName, 
					std::string const & userId, 
					int projectId,
					FilePath const & inboxPath)
		: Recipient (hubId, projName, userId),
		  _projectId (projectId),  
		  _inboxPath (inboxPath)
		{}

    LocalRecipient (Address const & address,
					int projectId,
		            FilePath const & inboxPath);

	int GetProjectId () const { return _projectId; }
	void SetProjectId (int projId) { _projectId = projId; }
    FilePath const & GetInbox  () const { return _inboxPath; }

	// Recipient interface
    bool AcceptScript (ScriptTicket & script, MailTruck & truck, int recipIdx);

private:
	int			_projectId;
    FilePath	_inboxPath;
};

// Invariant:
// All active local recipients on the local recipient list have hub id
// the same as the current hub id.
class LocalRecipientList
{
public:
	LocalRecipientList (Catalog & catalog)
		: _catalog (catalog)
	{}

	void Refresh (std::string const & currentHubId = std::string ());
    void Clear ();
	void Add (LocalRecipient const & recip);
	void AddRemoved (LocalRecipient const & recip);
	void KillRecipient (Address const & address, int projectId);
	bool KillRemovedRecipient (Address const & address);
	void HubIdChanged (std::string const & oldHubId, std::string const & newHubId);
	void PurgeInMemory ();
	void GetActiveProjectList (NocaseSet & projects) const;

	LocalRecipient * Find (Address const & address);
	LocalRecipient * FindRelaxed (Address const & address);
	Recipient * FindWildcard (std::string const & hubId, 
							  std::string const & projName, 
							  std::string const & senderHubId, 
							  std::string const & senderLocation);
	bool FindProject (std::string const & project) const;
	bool FindActiveProject (std::string const & project) const;
	bool EnlistmentAwaitsFullSync (std::string const & projectName) const;

    void UpdateAddress (Address const & currentAddress,
						std::string const & newHubId,
						std::string const & newUserId);

    typedef std::deque<LocalRecipient>::iterator iterator;
    typedef std::deque<LocalRecipient>::const_iterator const_iterator;

    iterator begin () { return _recips.begin (); }
    iterator end () { return _recips.end (); }
    const_iterator begin () const { return _recips.begin (); }
    const_iterator end () const { return _recips.end (); }

private:
	void Insert (LocalRecipient const & newRecipient);
	iterator FindByAddress (Address const & addr);
private:
	typedef std::deque<LocalRecipient> LocRecList;

private:
	LocRecList	_recips; // "removed" are at the back
	Catalog	&	_catalog;
};

#endif
