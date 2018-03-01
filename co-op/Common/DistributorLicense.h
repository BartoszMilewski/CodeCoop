#if !defined (DISTRIBUTORLICENSE_H)
#define DISTRIBUTORLICENSE_H
//-----------------------------------
// (c) Reliable Software 2005
//-----------------------------------

class Catalog;
class ActivityLog;

class DistributorLicensePool
{
public:
	DistributorLicensePool (Catalog & catalog);
	bool empty () const { return _nextNumber >= _count; }
	std::string GetLicensesLeftText ();
	std::string NewLicense (ActivityLog & log);
private:
	void Refresh ();
private:
	Catalog & _catalog;
	unsigned _nextNumber;
	unsigned _count;
};

#endif
