// ----------------------------------
// (c) Reliable Software, 1999 - 2008
// ----------------------------------

#include "precompiled.h"

#include "ConfigDlgData.h"

#include <StringOp.h>

ConfigDlgData::ConfigDlgData (Win::MessagePrepro * msgPrepro,
							  Email::Manager & newEmailMan,
							  ConfigData & newCfgData,
							  ConfigData const & originalCfgData)
	: _msgPrepro (msgPrepro),
	  _newConfig (newCfgData),
	  _originalConfig (originalCfgData),
	  _emailMan (newEmailMan)
{
	// Determine the current collaboration way.
	Topology topology = _originalConfig.GetTopology ();
	if (topology.IsStandalone ())
	{
		_collaboration = Standalone;
	}
	else if (topology.IsPeer ())
	{
		_collaboration = OnlyEmail;
	}
	else if (topology.IsHub ())
	{
		if (topology.UsesEmail ())
		{
			_collaboration = EmailAndLAN;
		}
		else
			_collaboration = OnlyLAN;
	}
	else
		_collaboration = topology.UsesEmail () ? EmailAndLAN : OnlyLAN;
}

void ConfigDlgData::SetNewHubId (std::string const & newHubId)
{
	if (!IsNocaseEqual (newHubId, _newConfig.GetHubId ()))
	{
		Transport const & oldTransport = _newConfig.GetInterClusterTransportToMe ();
		Transport newTransport (newHubId);
		if (oldTransport != newTransport && oldTransport.IsUnknown ())
			_newConfig.SetInterClusterTransportToMe (newTransport);
		_newConfig.SetHubId (newHubId);
	}
}

// returns true if any changes detected
bool ConfigDlgData::AnalyzeChanges ()
{
	if (_accepted.IsEmpty ())
	{
		_newConfig = _originalConfig;
		return false;
	}

	// based on accepted flags, reset all unconfirmed changes
	if (!_accepted.IsSet (ConfigData::bitTopologyCfg))
	{
		_newConfig.SetTopology (_originalConfig.GetTopology ());
	}
	if (!_accepted.IsSet (ConfigData::bitActiveIntraClusterTransportToMe))
	{
		_newConfig.SetActiveIntraClusterTransportToMe (_originalConfig.GetActiveIntraClusterTransportToMe ());
	}
	if (!_accepted.IsSet (ConfigData::bitActiveTransportToHub))
	{
		_newConfig.SetActiveTransportToHub (_originalConfig.GetActiveTransportToHub ());
	}
	if (!_accepted.IsSet (ConfigData::bitInterClusterTransportToMe))
	{
		_newConfig.SetInterClusterTransportToMe (_originalConfig.GetInterClusterTransportToMe ());
	}
	if (!_accepted.IsSet (ConfigData::bitHubId))
	{
		_newConfig.SetHubId (_originalConfig.GetHubId ());
	}

	return true;
}

void ConfigDlgData::AcceptChangesTo (ConfigData::Field field)
{
	_accepted.Set (field);
}

void ConfigDlgData::DisregardChangesTo  (ConfigData::Field field)
{
	_accepted.Clear (field);
}

void ConfigDlgData::AcceptChangesTo (BitFieldMask<ConfigData::Field> fields)
{
	_accepted.Union (fields);
}

void ConfigDlgData::DisregardChangesTo  (BitFieldMask<ConfigData::Field> fields)
{
	_accepted.Difference (fields);
}

bool ConfigDlgData::IsHubAdvanced () const 
{
	if (IsNocaseEqual (_newConfig.GetHubId (), "Unknown"))
		return false;

	Transport const & hubRemoteTransport = _newConfig.GetInterClusterTransportToMe ();
	if (hubRemoteTransport.IsUnknown ())
		return false;

	return !hubRemoteTransport.IsEmail () 
		|| !IsNocaseEqual (hubRemoteTransport.GetRoute (), _newConfig.GetHubId ());
}
