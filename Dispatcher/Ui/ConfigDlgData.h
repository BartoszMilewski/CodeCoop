#if !defined (CONFIGDLGDATA_H)
#define CONFIGDLGDATA_H
// ----------------------------------
// (c) Reliable Software, 1999 - 2008
// ----------------------------------

#include "ConfigData.h"

#include <Bit.h>

// ConfigDlgData class represents data shared among Configuration Wizard pages
// The class also serves as dialog data in Preferences dialog

// ConfigDlgData has write access to a copy of the Dispatcher Configuration
// Only after the edit transaction commits, will this copy become the official
// configuration and get written to database

namespace Win { class MessagePrepro; }
namespace Email { class Manager; }

class ConfigDlgData
{
public:
	ConfigDlgData (Win::MessagePrepro * msgPrepro,
				   Email::Manager & newEmailMan,
				   ConfigData & newCfgData,
				   ConfigData const & originalCfgData);
	Win::MessagePrepro * GetMsgPrepro () { return _msgPrepro; }

	// commit / abort changes
	bool AnalyzeChanges ();
	void AcceptChangesTo (ConfigData::Field field);
	void DisregardChangesTo  (ConfigData::Field field);
	void AcceptChangesTo (BitFieldMask<ConfigData::Field> fields);
	void DisregardChangesTo  (BitFieldMask<ConfigData::Field> fields);

	ConfigData & GetNewConfig () { return _newConfig; }
	Topology GetNewTopology () const { return _newConfig.GetTopology (); }
	Email::Manager & GetEmailMan () { return _emailMan; }

	void SetHubProperties (std::string const & hubId, Transport const & hubRemoteTransport)
	{
		_newConfig.SetHubId (hubId);
		_newConfig.SetInterClusterTransportToMe (hubRemoteTransport);
	}
	void SetIntraClusterTransportToMe (Transport const & myTransport)
	{
		_newConfig.SetActiveIntraClusterTransportToMe  (myTransport);
	}
	void SetTransportToHub (Transport const & hubTransport)
	{
		_newConfig.SetActiveTransportToHub (hubTransport);
	}
	void SetNewHubId (std::string const & newHubId);

	bool IsHubAdvanced () const;

	void UseStandalone  ()
	{ 
		_collaboration = Standalone;  
		_newConfig.MakeStandalone ();
	}
	void UseOnlyOnLAN   ()
	{ 
		_collaboration = OnlyLAN;     
		_newConfig.SetUseEmail (false);
	}
	void UseOnlyEmail   () 
	{ 
		_collaboration = OnlyEmail;   
		_newConfig.MakePeer ();
	}
	void UseEmailAndLAN () 
	{ 
		_collaboration = EmailAndLAN; 
		_newConfig.SetUseEmail (true);
	}
	bool IsUsedStandalone  () const { return _collaboration == Standalone;  }
	bool IsUsedOnlyOnLAN   () const { return _collaboration == OnlyLAN;     }
	bool IsUsedOnlyEmail   () const { return _collaboration == OnlyEmail;   }
	bool IsUsedEmailAndLAN () const { return _collaboration == EmailAndLAN; }
	
	bool WasProxy () const { return _originalConfig.GetTopology ().IsTemporaryHub (); }
	std::string GetOldHubId () const { return _originalConfig.GetHubId (); }

private:
	Win::MessagePrepro *_msgPrepro;

	// Editable copy of dispatcher configuration
	ConfigData &		_newConfig;
	// Read only copy of the current dispatcher configuration
	ConfigData const &	_originalConfig;
	// Temporary Email::Manager
	Email::Manager &	_emailMan;
	// Remember which changes are committed
	BitFieldMask<ConfigData::Field> _accepted;

	// Used by ConfigWizard to maintain the current configuration context.
	enum Collaboration
	{
		Standalone,
		OnlyLAN,
		OnlyEmail,
		EmailAndLAN
	};

	Collaboration	_collaboration;
};

#endif
