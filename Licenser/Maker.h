#if !defined (MAKER_H)
#define MAKER_H
//---------------------------
// (c) Reliable Software 2005
//---------------------------

class LicenseRequest;
class FilePath;

class LicenseMaker
{
public:
	LicenseMaker (LicenseRequest & req)
		: _req (req)
	{}
	virtual ~LicenseMaker ();
	void SetComment (std::string const & comment);
	virtual bool IsValid () = 0;
	virtual void Make (FilePath const & workFolder) = 0;
protected:
	LicenseRequest & _req;
};

class SimpleLicenseMaker: public LicenseMaker
{
public:
	SimpleLicenseMaker (LicenseRequest & req)
		: LicenseMaker (req)
	{}
	bool IsValid ();
	void Make (FilePath const & workFolder);
};

class DistributorLicenseMaker: public LicenseMaker
{
public:
	DistributorLicenseMaker (LicenseRequest & req)
		: LicenseMaker (req)
	{}
	bool IsValid ();
	void Make (FilePath const & workFolder);
};

class BlockLicenseMaker: public LicenseMaker
{
public:
	BlockLicenseMaker (LicenseRequest & req)
		: LicenseMaker (req)
	{}
	bool IsValid ();
	void Make (FilePath const & workFolder);
};

#endif
