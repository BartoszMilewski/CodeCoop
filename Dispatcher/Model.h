#if !defined MODEL_H
#define MODEL_H
//----------------------------------
// (c) Reliable Software 1998 - 2007
// ---------------------------------

#include "Table.h"
#include "Settings.h"
#include "Config.h"
#include "AddressDb.h"
#include "Catalog.h"
#include "EmailMan.h"
#include "WorkQueue.h"
#include "ScriptQuarantine.h"
#include "AlertLog.h"
#include "LocalProjects.h"
#include "EmailChecker.h"
#include <File/FolderWatcher.h>
#include <Win/Win.h>

class ConfigDlgData;
class Address;
class LocalRecipientList;
class ClusterRecipientList;
class SelectionSeq;
class Transport;
class DispatcherScript;
class TransportHeader;

namespace Win { class MessagePrepro; }

class ProcessingResult
{
	enum { dispatchingDone, hasNewCoopRequests, numFlags };
public:
	ProcessingResult (bool done = true, bool hasNewRequests = false)
	{
		SetDispatchingDone (done);
		SetNewCoopRequests (hasNewRequests);
	}
	bool IsDispatchingDone () const { return _result.test (dispatchingDone); }
	void SetDispatchingDone (bool done) { _result.set (dispatchingDone, done); }
	void SetNewCoopRequests (bool flag) { _result.set (hasNewCoopRequests, flag); }
	bool HasNewRequestsForCoop () const { return _result.test (hasNewCoopRequests); }

private:
	std::bitset<numFlags> _result;
};

class Model : public TableProvider
{
    enum { MailboxProcessingTrials = 64 };

public:
    Model (Win::Dow::Handle winParent,
		   Win::MessagePrepro & msgPrepro,
		   DispatcherCmdExecutor & cmdExecutor);
	~Model ();
    void OnStartup ();
	void ShutDown ();
	void KeepAlive ();
	void OnMaintenance (bool isOverall);
	void RetrieveEmail ();
	void SendEmail ();
	void ClearCurrentDispatchState ();
	void Dispatch (bool isForceScattering); // force immediate scattering in verbose mode?
    void ForceDispatch ();

	Catalog & GetCatalog () { return _catalog; }

	// Requests to update data after Code Co-op notification
	void EnlistmentListChanged  ();
	void EnlistmentEmailChanged ();
	void OnCoopNotification (TransportHeader & txHdr, DispatcherScript const & script);
	bool OnProjectStateChange (int projectId);

	void BroadcastChunkSizeChange (unsigned long newChunkSize);
	void PullScriptsFromHub ();
	bool OnFolderChange ();
    ProcessingResult ProcessInbox (bool isForceScattering);
	void OnFinishScripts ();
	void FinishScripts (ScriptVector const & finishedList);

	void DisplayHeaderDetails (SelectionSeq & seq);
	void DeleteScript (std::string const & scriptName, bool isAllChunks = false);
	bool DeleteRecipients (SelectionSeq & seq);
	void DeleteRemoteHubs (SelectionSeq & seq);
	bool Purge (bool purgeLocal, bool purgeSat);

	LocalRecipientList const & GetLocalRecipients () const 
		{ return _addressDb.GetLocalRecipients (); }
    ClusterRecipientList const & GetClusterRecipients () const 
		{ return _addressDb.GetClusterRecips (); }
	void GetActiveProjectList (NocaseSet & projectList) const
		{ return _addressDb.GetActiveProjectList (projectList); }
	void GetActiveOffsiteSatList (NocaseSet & addresses) const
		{ return _addressDb.GetActiveOffsiteSatList (addresses); }

	Topology GetTopology () const { return _config.GetTopology (); }
	bool IsHub () const { return _config.GetTopology ().IsHub (); }
	bool IsHubOrPeer () const { return _config.GetTopology ().IsHubOrPeer (); }
	bool IsSatellite () const { return _config.GetTopology ().IsSatellite (); }
	bool IsConfigured () const { return !_config.GetTopology ().IsStandalone (); }
	bool IsConfigWithEmail () const
	{
		Topology topology = _config.GetTopology ();
		return topology.UsesEmail () &&
			  !(topology.IsSatellite () || topology.IsStandalone ()); 
	}
	bool IsAutoReceive () const
	{ 
		return IsConfigWithEmail () && TheEmail.GetEmailConfig ().IsAutoReceive (); 
	}
	unsigned int GetAutoReceivePeriod () const 
	{ 
		return TheEmail.GetEmailConfig ().GetAutoReceivePeriod (); 
	}
    
	void StartUsingEmail ();
	void Reconfigure (Win::MessagePrepro * msgPrepro);
	void OnConnectProxy (bool connected);
	bool OnCoopTimer ();

	ConfigData const & GetConfigData () const { return _config.GetOriginalData (); }
	ConfigData & XGetConfigData () {	return _config.XGetData (); }
	Email::Manager & XGetEmailMan () { return _config.XGetEmailMan (); }
	void BeginConfigTransaction () { _config.BeginEdit (); }
	void AbortConfigTransaction () { _config.AbortEdit (); }

    int GetIncomingCount () const;
    int GetOutgoingCount () const;
	FilePath const & GetPublicInboxPath () const { return _config.GetPublicInboxPath (); }
	FilePath const & GetUpdatesPath () const { return _catalog.GetUpdatesPath (); }

	std::string FindSatellitePath (std::string const & computerName) const;
	void AddClusterRecipient (Address const & address, FilePath const & forwardingPath);
	void AddTempLocalRecipient (Address const & address) 
	{ 
		_addressDb.AddTempLocalRecipient (address, false); // active
	}
	bool EditTransport (SelectionSeq const & seq);
	bool EditInterClusterTransport (SelectionSeq const & seq);
	void AddRemoteHub (std::string const & hubId, Transport const & transport);
	RemoteHubList const & GetRemoteHubList () const { return _addressDb.GetRemoteHubList (); }
	void AddDistributorLicense (std::string const & licensee, unsigned startNum, unsigned count);
	// TableProvider interface
    std::unique_ptr<RecordSet> Query (std::string const & name, Restriction const & restrict);
	std::string QueryCaption (std::string const & tableName, Restriction const & restrict) const;

	void ReleaseAllFromQuarantine ();
	void ReleaseFromQuarantine (std::string const & scriptFilename);
	bool HasQuarantineScripts () const { return !_scriptQuarantine.IsEmpty (); }
	bool HasInvitation() const { return _scriptQuarantine.HasInvitation(); }
	void ReleaseInvitationFromQuarantine();
	void ViewAlertLog  () { _alertLog.View ();  }
	void ClearAlertLog () { _alertLog.Clear (); }
	bool IsAlertLogEmpty () const { return _alertLog.IsEmpty (); }
private:
	void FinishScript(ScriptTicket const & script);
	void VerifyConfig ();
	void ResetPublicInbox ();
	void ResetInfrastructure ();
	void BroadcastIntraClusterTransportToMeChange (Transport const & oldTrans, Transport const & newTrans) const;
	void BroadcastInterClusterTransportToMeChange (std::string const & oldHubId, 
											std::string const & newHubId, 
											Transport const & newTrans) const;
	void RegisterSatelliteOnHub () const;
	void PublishEnlistmentsToHub (std::string const & oldHubId, std::string const & newHubId) const;
	void SendDispatcherCommand (std::unique_ptr<DispatcherCmd> cmd, char const * senderId, char const * addresseeId) const;
	
    void ChangeSettings ();

	void HandleInvitationScripts ();	
private:
	Win::Dow::Handle				_winParent;

	Catalog							_catalog; // must be first
    Configuration					_config;  // must be in front of AddressDb
	LocalProjects				 	_localProjects;
	DispatcherCmdExecutor &			_cmdExecutor;

    std::unique_ptr<PublicInbox>		_publicInbox;
    AddressDatabase					_addressDb;
    ScriptQuarantine				_scriptQuarantine;
	AlertLog						_alertLog;
	bool							_isInboxFolderChanged;
    auto_active<FolderWatcher>	    _folderWatcher;
	auto_active<WorkQueue>			_workQueue;
	auto_active<EmailChecker>		_emailChecker;
};

#endif
