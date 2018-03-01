#if !defined (HUBREMOTECONFIG_H)
#define HUBREMOTECONFIG_H
// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------

#include "Transport.h"

class HubRemoteConfig
{
public:
	std::string const & GetHubId () const { return _hubId; }
	Transport const & GetTransport () const { return _hubTransport; }
	void SetHubId (std::string const & hubId)
	{
		_hubId.assign (hubId);
	}
	void SetTransport (Transport const & transport)
	{
		_hubTransport = transport;
	}
private:
	std::string _hubId;
	Transport	_hubTransport;
};

#endif
