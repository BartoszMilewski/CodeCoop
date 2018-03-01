#if !defined (JOINPROJECTDATA_H)
#define JOINPROJECTDATA_H
// ----------------------------------
// (c) Reliable Software, 2002 - 2008
// ----------------------------------

#include "ProjectBlueprint.h"
#include "Transport.h"

#include <StringOp.h>

class Catalog;
class NamedValues;

class JoinProjectData : public Project::Blueprint
{
public:
	JoinProjectData (Catalog & catalog, bool forceObserver, bool isInvitation = false);
	~JoinProjectData ();

	bool IsValid () const;
    void DisplayErrors (Win::Dow::Handle winOwner) const;


	void SetObserver (bool observer) { _observer = observer; }
	void SetAdminHubId (std::string const & hubId) { _adminHubId.Assign (hubId); }
	void SetAdminTransport (Transport const & tr) { _adminTransport = tr; }
	void SetScriptPath (std::string const & scriptPath) { _scriptPath = scriptPath; }
	void SetRemoteAdmin (bool flag) { _remoteAdmin = flag; }
	void SetConfigureFirst (bool flag) { _configureFirst = flag; }
	void SetValid (bool flag) { _isValid = flag; }
	void Clear ();

    bool IsObserver () const    { return _observer; }
	bool IsForceObserver () const { return _forceObserver; }
	bool IsRemoteAdmin () const { return _remoteAdmin; }
	bool ConfigureFirst () const { return _configureFirst; }
	bool IsInvitation () const { return _isInvitation; }
	bool HasFullSyncScript () const { return !_scriptPath.empty (); }

	std::string const & GetAdminHubId () const { return _adminHubId; }
	Transport const & GetAdminTransport () const { return _adminTransport; }
	std::string const & GetFullSyncScriptPath () const { return _scriptPath; }
	std::string const & GetMyHubId () const { return _myHubId; }
	Transport const & GetIntraClusterTransportToMe () const { return _myTransport; }

	std::string GetNamedValues ();
	void ReadNamedValues (NamedValues const & input);

private:
	bool					_forceObserver;
    bool					_observer;
	bool					_remoteAdmin;
	bool					_configureFirst;
	bool					_isInvitation;
	bool					_isValid;
	TrimmedString			_adminHubId;
	Transport				_adminTransport;
	std::string				_scriptPath; // path to full synch script
	std::string				_myHubId;
	Transport				_myTransport;
};

#endif
