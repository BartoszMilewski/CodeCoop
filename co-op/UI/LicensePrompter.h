#if !defined (LICENSEPROMPTER_H)
#define LICENSEPROMPTER_H
//---------------------------------------------
// (c) Reliable Software 2005
//---------------------------------------------

class License;
class DistributorLicense;

class LicensePrompter
{
public:
	LicensePrompter (std::string const & context)
		: _context (context)
	{}

	bool Query (License const & curLicense,
								unsigned int trialDaysLeft,
								DistributorLicense const & distribLicense);
	bool IsNewLicense () const { return !_newLicense.empty (); }
	std::string const & GetNewLicense () const { return _newLicense; }

private:
	std::string GetLicenseStatus (License const & curLicense, 
								unsigned int trialDaysLeft,
								DistributorLicense const & distribLicense) const;

private:
	std::string	_context;
	std::string	_newLicense;
};

#endif
