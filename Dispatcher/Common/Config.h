#if !defined (CONFIG_H)
#define CONFIG_H
//----------------------------------
// (c) Reliable Software 2000 - 2008
//----------------------------------

#include "ConfigData.h"
#include "EmailMan.h"

class Catalog;

class Configuration
{
	friend class Model; // only Model can CommitEdit
public:
    Configuration (Catalog & catalog);
	bool OnStartup (Catalog & catalog);
	void Save (Catalog & catalog) const;

	// if you want to change configuration settings:
	// 1. begin a pseudo transaction
	// 2. apply your changes and
	// 3. throw ConfigExpt
	void BeginEdit ()
	{
		_xConfig = _config;
		_xEmailMan.reset (new Email::Manager);
		_xEmailMan->Refresh ();
	}
	void AbortEdit ()
	{
		_xEmailMan->AbortEdit ();
		_xEmailMan.reset ();
	}

	// original and editable copy of ConfigData
	ConfigData const & GetOriginalData () const { return _config; }
	ConfigData & XGetData () { return _xConfig; }
	Email::Manager & XGetEmailMan () 
	{
		Assert (_xEmailMan.get () != 0);
		return *_xEmailMan;
	}

	// access to sub components of ConfigData
	Topology GetTopology () const { return _config._topology; }
	std::string GetSatComputerName () const;
	// export all const methods of ConfigData
    FilePath const & GetPublicInboxPath () const { return _config._publicInboxPath; }
	Transport const & GetActiveIntraClusterTransportToMe () const { return _config._myTransports.GetActive (); }
	Transport const & GetIntraClusterTransportToMe (Transport::Method method) const 
	{
		return _config._myTransports.Get (method); 
	}
	TransportArray const & GetIntraClusterTransportsToMe () const { return _config._myTransports; }
    Transport const & GetActiveTransportToHub () const { return _config._hubTransports.GetActive (); }
	Transport const & GetTransportToHub (Transport::Method method) const 
	{
		return _config._hubTransports.Get (method); 
	}
	TransportArray const & GetTransportsToHub () const { return _config._hubTransports; }
	Transport const & GetInterClusterTransportToMe () const { return _config._hubRemoteTransports.GetActive (); }
	std::string const & GetHubId () const { return _config._hubId; }
	bool AskedToStayOffSiteHub () const { return _config._askedToStayOffSiteHub; }

private:
	void Read (Catalog & catalog);
	void CommitEdit ()
	{
		_config = _xConfig;
		// Write new config to registry
		_xEmailMan->GetEmailConfig ().CommitEdit ();
		// Read from registry
		TheEmail.Refresh ();
		_xEmailMan.reset ();
	}
private:
	ConfigData _config;
	ConfigData _xConfig;
	std::unique_ptr<Email::Manager> _xEmailMan;
};

#endif
