#if !defined (CONFIGDATA_H)
#define CONFIGDATA_H
// --------------------------------
// (c) Reliable Software, 2002 - 06
// --------------------------------

#include "Transport.h"
#include <File/Path.h>
#include <Dbg/Assert.h>
#include <Win/Win.h>

class TransportValidator
{
public:
	TransportValidator (FilePath const & pi, TransportArray const & myTransports)
		: _pi (pi),
		  _myTransports (myTransports)
	{}
	bool Validate (Transport const & transport, Win::Dow::Handle win) const;
	bool ValidateRemoteHub (Transport const & transport, Win::Dow::Handle win) const;
	bool ValidateExcludePublicInbox (Transport const & transport,
									 char const * errmsg,
									 Win::Dow::Handle win) const;
private:
    FilePath const       & _pi;
    TransportArray const & _myTransports;
};

extern std::unique_ptr<TransportValidator> TheTransportValidator;

class ConfigData
{
	friend class Configuration;
public:
	enum Field
	{
		bitTopologyCfg = 0x1,
		bitActiveIntraClusterTransportToMe = 0x2,
		bitActiveTransportToHub = 0x4,
		bitInterClusterTransportToMe = 0x8,
		bitHubId = 0x10
	};
public:
    FilePath const & GetPublicInboxPath () const { return _publicInboxPath; }
	// My transports
	Transport const & GetActiveIntraClusterTransportToMe () const { return _myTransports.GetActive (); }
	void SetActiveIntraClusterTransportToMe (Transport const & trans)
	{
		_myTransports.ResetActive (trans);
	}
	Transport const & GetIntraClusterTransportToMe (Transport::Method method) const 
	{
		return _myTransports.Get (method); 
	}
	TransportArray const & GetIntraClusterTransportsToMe () const { return _myTransports; }

	// Hub transports
    Transport const & GetActiveTransportToHub () const { return _hubTransports.GetActive (); }
	void SetActiveTransportToHub (Transport const & transport) { _hubTransports.ResetActive (transport); }
	Transport const & GetTransportToHub (Transport::Method method) const 
	{
		return _hubTransports.Get (method); 
	}
	TransportArray const & GetTransportsToHub () const { return _hubTransports; }
	// Hub remote transports
	Transport const & GetInterClusterTransportToMe () const { return _hubRemoteTransports.GetActive (); }
	void SetInterClusterTransportToMe (Transport const & transport) 
	{
		_hubRemoteTransports.ResetActive (transport);
	}
	TransportArray const & GetInterClusterTransportsToMe () const { return _hubRemoteTransports; }

	std::string const & GetHubId () const { return _hubId; }
	void SetHubId (std::string const & hubId) { _hubId = hubId; }

	Topology GetTopology () const { return _topology; }
	void SetTopology (Topology const & top) { _topology = top; }
	// Topology manipulations
	void SetUseEmail (bool useEmail) { _topology.SetUseEmail (useEmail); }
	void SetUseNet (bool useNet) { _topology.SetUseNet (useNet); }

	void MakeStandalone () { _topology.MakeStandalone (); }
	void MakeHub () { _topology.MakeHub ();	}
	void MakeHubWithEmail () 
	{ 
		_topology.MakeHub ();
		_topology.SetUseEmail (true);
	}
	void MakeHubNoEmail () 
	{ 
		_topology.MakeHub ();
		_topology.SetUseEmail (false);
		_topology.SetUseNet (true);
	}
	void MakeTemporaryHub () { _topology.MakeTemporaryHub (); }
	void MakePeer () { _topology.MakePeer (); }
	void MakeSatellite () { _topology.MakeSatellite ();	}
	void MakeSatellite (Transport const & hubTransport)
	{
		_topology.MakeSatellite ();
		if (hubTransport.IsEmail ())
			_topology.SetUseEmail (true);
		_hubTransports.ResetActive (hubTransport);
	}
	void MakeRemoteSatellite ()
	{
		_topology.MakeSatellite ();
		_topology.SetUseEmail (true);
	}

	bool AskedToStayOffSiteHub () const { return _askedToStayOffSiteHub; }
	void SetAskedToStayOffSiteHub (bool stayProxy) { _askedToStayOffSiteHub = stayProxy; }

private:
	FilePath		_publicInboxPath; // only for displaying, not editing
    TransportArray	_myTransports;
	TransportArray	_hubTransports;
	TransportArray	_hubRemoteTransports;

	Topology		_topology;
	std::string		_hubId;

	bool			_askedToStayOffSiteHub; // valid only on proxy hub
};

#endif
