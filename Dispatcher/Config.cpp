//----------------------------------
// (c) Reliable Software 2000 - 2007
//----------------------------------

#include "precompiled.h"
#include "Config.h"
#include "Catalog.h"
#include "Registry.h"
#include "Email.h"
#include "EmailMan.h"
#include "OutputSink.h"
#include "AlertMan.h"
#include "DispatcherParams.h"

#include <Mail/EmailAddress.h>
#include <File/Path.h>

Configuration::Configuration (Catalog & catalog)
{
	Read (catalog);
}

void Configuration::Read (Catalog & catalog)
{
	catalog.GetDispatcherSettings ( 
		_config._topology,
		_config._myTransports,
		_config._hubTransports,
		_config._hubRemoteTransports,
		_config._hubId);

	_config._publicInboxPath.Clear ();
	_config._publicInboxPath = catalog.GetPublicInboxDir ();
	Registry::UserDispatcherPrefs prefs;
	_config.SetAskedToStayOffSiteHub (prefs.IsStayOffSiteHub ());
}

void Configuration::Save (Catalog & catalog) const
{
	catalog.SetDispatcherSettings (	_config.GetTopology (),
									_config.GetIntraClusterTransportsToMe (),
									_config.GetTransportsToHub (),
									_config.GetInterClusterTransportsToMe (),
									_config.GetHubId ());

	Registry::UserDispatcherPrefs prefs;
	prefs.SetStayOffSiteHub (_config.AskedToStayOffSiteHub ());
}

std::string Configuration::GetSatComputerName () const
{
	Assert (_config.GetTopology ().HasHub ());
	std::string computerName;
	TransportArray const & transports = _config._myTransports;
	Transport const & networkTransport = transports.Get (Transport::Network);
	if (networkTransport.IsUnknown ())
		throw Win::InternalException ("Dispatcher has invalid configuration."
									"\nNot able to retrieve computer name."
									"\nPlease contact support@relisoft.com");
	FullPathSeq share (networkTransport.GetRoute ().c_str ());
	return share.GetServerName ();
}
