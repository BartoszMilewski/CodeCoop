//----------------------------------
// (c) Reliable Software 2005 - 2007
//----------------------------------

#include "precompiled.h"
#include "DistributorLicense.h"
#include "Catalog.h"
#include "Crypt.h"
#include "License.h"
#include "ActivityLog.h"
#include <StringOp.h>

DistributorLicensePool::DistributorLicensePool (Catalog & catalog)
	: _catalog (catalog),
	_nextNumber (catalog.GetNextDistributorNumber ()),
	_count (catalog.GetDistributorLicenseCount ())
{
}

void DistributorLicensePool::Refresh ()
{
	_nextNumber = _catalog.GetNextDistributorNumber ();
	_count = _catalog.GetDistributorLicenseCount ();
}

std::string DistributorLicensePool::GetLicensesLeftText ()
{
	std::string result = "You have ";
	if (!empty ())
	{
		result += ToString (_count - _nextNumber);
	}
	else
	{
		result += "no";
	}
	result += " distribution license(s).";
	return result;
}

std::string DistributorLicensePool::NewLicense (ActivityLog & log)
{
	std::string licensee (_catalog.GetDistributorLicensee ());
	licensee += " R-";
	licensee += ToString (_nextNumber);
	// Only in product PRO one can have distributor projects
	std::string key = EncodeLicense10 (licensee, 1, coopProId);
	License license (licensee, key);
	// log the license in activity log
	log.ReceiverLicense (licensee);
	_catalog.RemoveDistributorLicense ();
	Refresh ();
	Assert (_catalog.GetNextDistributorNumber () <= _catalog.GetDistributorLicenseCount ());
	return license.GetLicenseString ();
}


 
