//------------------------------------
//  (c) Reliable Software, 1997 - 2007
//------------------------------------

#include "precompiled.h"
#include "License.h"
#include "Catalog.h"
#include "Crypt.h"

License::License (Catalog & catalog)
	: _licensee (catalog.GetLicensee ()),
	  _key (catalog.GetKey ()),
	  _seats (0),
	  _version (0),
	  _productId ('\0'),
	  _isValid (false)
{
	if (!_licensee.empty () && !_key.empty ())
	{
		_isValid = ::DecodeLicense (GetLicenseString (), _version, _seats, _productId);
	}
}

License::License (std::string const & licenseString)
	: _seats (0),
	  _version (0),
	  _productId ('\0'),
	  _isValid (false)
{
	Init (licenseString);
}

License::License (std::string const & licensee, std::string const & key)
	: _licensee (licensee),
	  _key (key),
	  _seats (0),
	  _version (0),
	  _productId ('\0'),
	  _isValid (false)
{
	_isValid = ::DecodeLicense (GetLicenseString (), _version, _seats, _productId);
}

void License::Init (std::string const & licenseString)
{
	std::string::size_type markerPos = licenseString.find ('\n');
	if (markerPos != std::string::npos)
	{
		_licensee = licenseString.substr (0, markerPos);
		_key = licenseString.substr (markerPos + 1);
		_isValid = ::DecodeLicense (licenseString, _version, _seats, _productId);
	}
	else
	{
		_isValid = false;
		_licensee.clear ();
		_key.clear ();
		_seats = 0;
		_productId = '\0';
	}
}

std::string License::GetLicenseString () const
{
	std::string str (_licensee);
	str += '\n';
	str += _key;
	return str;
}

std::string License::GetDisplayString () const
{
	if (_seats == 0)
		return "Expired evaluation copy.";

	std::string msg ("Licensee: ");
	msg += _licensee;
	msg += " ( ";
	msg += ToString (_seats);
	msg += " seats for version ";
	msg += ToString (_version);
	msg += ".x ";
	if (_productId == coopLiteId)
		msg += "Lite)";
	else if (_productId == clubWikiId)
		msg += "Club-Wiki)";
	else
		msg += "Pro)";
	return msg;
}

bool License::IsCurrentVersion () const
{
	return _isValid && ::IsCurrentVersion (_version);
}

bool License::IsValidProduct () const
{
	return _isValid && ::IsValidProduct (_productId);
}

bool License::IsBetterThan (License const & license) const
{
	if (!IsValid ())
		return false;
	if (!license.IsValid ())
		return true;

	Assert (GetLicensee () == license.GetLicensee ());
	// Both valid
	if (GetVersion () > license.GetVersion ())
		return true;
	if (GetVersion () < license.GetVersion ())
		return false;
	// Same licensee
	return GetSeatCount () > license.GetSeatCount ();
}

DistributorLicense::DistributorLicense (Catalog & catalog)
	: _licensee (catalog.GetDistributorLicensee ()),
	  _count (catalog.GetDistributorLicenseCount ()),
	  _next (catalog.GetNextDistributorNumber ())
{}
