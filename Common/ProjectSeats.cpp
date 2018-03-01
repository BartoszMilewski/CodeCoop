//---------------------------------------------
// (c) Reliable Software 2005
//---------------------------------------------

#include "precompiled.h"
#include "ProjectSeats.h"
#include "ProjectDb.h"
#include "License.h"
#include "MemberInfo.h"

using namespace Project;

Seats::Seats (Project::Db const & projectDb, std::string const & additionalLicense)
{
    std::vector<MemberInfo> memberData(projectDb.RetrieveMemberList());
	Assert (memberData.size () != 0);
	CountSeats (memberData, additionalLicense);
}

Seats::Seats (std::vector<MemberInfo> const & memberData, std::string const & additionalLicense)
{
	Assert (memberData.size () != 0);
	CountSeats (memberData, additionalLicense);
}

void Seats::Refresh (Project::Db const & projectDb)
{
	std::vector<MemberInfo> memberData(projectDb.RetrieveMemberList());
	Assert (memberData.size () != 0);
	CountSeats (memberData, "");	// NO additional license, just count missing seats in the project
}

void Seats::XRefresh (Project::Db const & projectDb)
{
	std::vector<MemberInfo> memberData(projectDb.XRetrieveMemberList());
	Assert (memberData.size () != 0);
	CountSeats (memberData, "");	// NO additional license, just count missing seats in the project
}

void Seats::CountSeats (std::vector<MemberInfo> const & memberData, std::string const & additionalLicense)
{
	LicenseArray seats;
	typedef std::vector<MemberInfo>::const_iterator memberIter;
	for (memberIter iter = memberData.begin (); iter != memberData.end (); ++iter)
	{
		if (iter->IsVoting ())
		{
			License license (iter->License ());
			if (license.IsValid ())
			{
				// Count free seats only among voting licensed users (regardles of the license version)
				seats.Add ( license.GetLicensee (), 
							license.GetSeatCount (),
							license.GetVersion(),
							license.GetProductId());
			}
		}
	}

	License extraLicense (additionalLicense);
	if (extraLicense.IsValid ())
		seats.Add ( extraLicense.GetLicensee (), 
					extraLicense.GetSeatCount (),
					extraLicense.GetSeatCount(),
					extraLicense.GetProductId());

	_missing = seats.GetMissing ();
}

void Seats::LicenseArray::Add (std::string const & name, int seats, int version, char prodId)
{
	for (size_t i = 0; i < _arr.size (); i++)
	{
		if (_arr [i].IsEqual (name, version, prodId))
		{
			_arr [i].Update (seats);
			return;
		}
	}
	_arr.push_back (NameCount (name, seats, version, prodId));
}

int Seats::LicenseArray::GetMissing () const
{
	int total = 0;
	for (size_t i = 0; i < _arr.size (); i++)
	{
		total += _arr [i].GetMissing ();
	}
	return total;
}

