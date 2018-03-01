// ----------------------------------
// (c) Reliable Software, 2000 - 2006
// ----------------------------------

#include "precompiled.h"
#include "Catalog.h"
#include "Registry.h"
#include "RegKeys.h"
#include "ClusterRecipient.h"
#include "Processor.h"
#include <Win/Win.h> // Win::Placement

// ---------------------
// Configuration section
// ---------------------
void Catalog::GetDispatcherSettings (
				Topology & topology,
				TransportArray & myTransports,
				TransportArray & hubTransports,
				TransportArray & hubRemoteTransports,
				std::string & hubId) 
{
	_globalDb.Refresh (_pathFinder);
	topology = _globalDb.GetTopology ();
	std::vector<Transport> tr;
	Transport::Method active;
	_globalDb.GetIntraClusterTransportsToMe (tr, active);
	myTransports.Init (tr, active);
	 _globalDb.GetTransportsToHub (tr, active);
	hubTransports.Init (tr, active);
	_globalDb.GetInterClusterTransportsToMe (tr, active);
	hubRemoteTransports.Init (tr, active);
	hubId = _globalDb.GetHubId ();
}

void Catalog::SetDispatcherSettings (
				Topology topology, 
				TransportArray const & myTransports,
				TransportArray const & hubTransports,
				TransportArray const & hubRemoteTransports,
				std::string const & hubId)
{
	CatTransaction xact (_globalDb, _pathFinder);

	_globalDb.XSetTopology (topology);
	_globalDb.XSetIntraClusterTransportsToMe (myTransports.Get (), myTransports.GetActiveMethod ());
	_globalDb.XSetTransportsToHub (hubTransports.Get (), hubTransports.GetActiveMethod ());
	_globalDb.XSetInterClusterTransportsToMe (hubRemoteTransports.Get (), hubRemoteTransports.GetActiveMethod ());
	_globalDb.XSetHubId (hubId);

	xact.Commit ();
}

void Catalog::GetIntraClusterTransportsToMe (TransportArray & transports)
{
	_globalDb.Refresh (_pathFinder);
	std::vector<Transport> tr;
	Transport::Method active;
	_globalDb.GetIntraClusterTransportsToMe (tr, active);
	transports.Init (tr, active);
}

void Catalog::SetIntraClusterTransportsToMe (TransportArray const & transports)
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XSetIntraClusterTransportsToMe (transports.Get (), transports.GetActiveMethod ());
	xact.Commit ();
}

void Catalog::GetTransportsToHub (TransportArray & transports)
{
	_globalDb.Refresh (_pathFinder);
	std::vector<Transport> tr;
	Transport::Method active;
	_globalDb.GetTransportsToHub (tr, active);
	transports.Init (tr, active);
}

void Catalog::SetTransportsToHub (TransportArray const & transports)
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XSetTransportsToHub (transports.Get (), transports.GetActiveMethod ());
	xact.Commit ();
}

void Catalog::AddRemoteHub (std::string const & hubId, Transport const & transport)
{
	CatTransaction xact (_globalDb, _pathFinder);
	if (!IsNocaseEqual (hubId, _globalDb.GetHubId ()))
		_globalDb.XAddRemoteHub (hubId, transport);
	xact.Commit ();
}

void Catalog::DeleteRemoteHub (std::string const & hubId)
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XDeleteRemoteHub (hubId);
	xact.Commit ();
}

// ----------------------------
// Recipient lists management
// ----------------------------

// Returns true, if any changes applied to recipient database
bool Catalog::PurgeRecipients (bool purgeLocal, bool purgeSatellite)
{
	CatTransaction xact (_globalDb, _pathFinder);
	
	bool anyLocalPurged = false;
	bool anySatPurged = false;
	if (purgeLocal)
		anyLocalPurged = _globalDb.XPurgeLocalRecipients ();
	if (purgeSatellite)
		anySatPurged = _globalDb.XPurgeSatelliteRecipients ();
	
	xact.Commit ();
	
	return anyLocalPurged || anySatPurged;
}

// -------------------------------
// Local recipient list management
// -------------------------------

void Catalog::KillLocalRecipient (Address const & address, int projectId)
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XKillLocalRecipient (address, projectId);
	xact.Commit ();
}

// Delete removed local recipient from database
// returns false if active recipient with the address is found
void Catalog::KillRemovedLocalRecipient (Address const & address)
{					
	CatTransaction xact (_globalDb, _pathFinder);
	if (_globalDb.XKillRemovedLocalRecipient (address))
	{
		xact.Commit ();
	}
}

// ---------------------------------
// Cluster recipient list management
// ---------------------------------

ClusterRecipSeq::ClusterRecipSeq (Catalog & cat)
	: _pImpl (new ClusterRecipSeqImpl (cat._globalDb, cat._pathFinder))
{}

void ClusterRecipSeq::GetClusterRecipient (ClusterRecipient & recip)
{
	Address address;
	bool isRemoved = false;
	Transport transport;
	_pImpl->GetClusterRecipient (address, transport, isRemoved);

	recip.ChangeAddress (address);
	recip.ChangeTransport (transport);
	recip.MarkRemoved (isRemoved);
}

ClusterRecipSeq::~ClusterRecipSeq ()
{
}

bool ClusterRecipSeq::AtEnd () const
{
	return _pImpl->AtEnd ();
}

void ClusterRecipSeq::Advance ()
{
	_pImpl->Advance ();
}

void Catalog::SetHubId (std::string const & hubId)
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XSetHubId (hubId);
	xact.Commit ();
}

void Catalog::AddClusterRecipient (Address const & address,
								   Transport const &  transport)
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XAddClusterRecipient (address, transport);
	xact.Commit ();
}

void Catalog::AddRemovedClusterRecipient (Address const & address,
										  Transport const &  transport)
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XAddClusterRecipient (address, transport);
	_globalDb.XRemoveClusterRecipient (address, false); // remove, not kill
	xact.Commit ();
}

void Catalog::RemoveClusterRecipient (Address const & address)
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XRemoveClusterRecipient (address, false);
	xact.Commit ();
}

void Catalog::ActivateClusterRecipient (Address const & address)
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XActivateClusterRecipient (address);
	xact.Commit ();
}

void Catalog::ChangeTransport (Address const & address, Transport const & newTransport)
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XChangeTransport (address, newTransport);
	xact.Commit ();
}

void Catalog::DeleteClusterRecipient (Address const & address)
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XRemoveClusterRecipient (address, true); // permanently
	xact.Commit ();
}

void Catalog::ClearClusterRecipients ()
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XClearClusterRecipients ();
	xact.Commit ();
}
