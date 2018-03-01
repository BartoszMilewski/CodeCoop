#if !defined (LICENSE_H)
#define LICENSE_H
//------------------------------------
//  (c) Reliable Software, 1997 - 2007
//------------------------------------

#include <string>

class Catalog;

class License
{
public:
	License ()
		: _isValid (false),
		  _seats (0),
		  _version (0)
	{}
	License (Catalog & catalog);
	License (std::string const & licenseString);
	License (std::string const & licensee, std::string const & key);

	void Init (std::string const & licenseString);
	std::string const & GetLicensee () const { return _licensee; }
	std::string const & GetKey () const { return _key; }
	int GetSeatCount () const { return _seats; }
	std::string GetLicenseString () const;
	std::string GetDisplayString () const;
	int GetVersion () const { return _version; }
	char GetProductId () const { return _productId; }
	bool IsValid () const { return _isValid; }
	bool IsCurrentVersion () const;
	bool IsValidProduct () const;

	bool IsEqual (std::string const & license) const 
	{ 
		return GetLicenseString () == license;
	}
	bool IsBetterThan (License const & license) const;

private:
	bool		_isValid;
	std::string	_licensee;
	std::string	_key;
	int			_seats;
	int			_version;
	char		_productId;
};

class DistributorLicense
{
public:
	DistributorLicense (Catalog & catalog);
	unsigned GetCount () const { return _count; }
	unsigned GetNext () const { return _next; }
	std::string const & GetLicensee () const { return _licensee; }
private:
	std::string _licensee;
	unsigned	_count;
	unsigned	_next;
};

#endif
