#if !defined (MODEL_H)
#define MODEL_H
//---------------------------
// (c) Reliable Software 2005
//---------------------------
#include "LicenseRequest.h"
#include "Maker.h"
#include <File/Path.h>

class Model
{
public:
	Model () {}
	~Model ();
	bool PostRequest (LicenseRequest & request);
	void CreateLicense ();

	FilePath const & GetWorkFolder () const { return _workFolder; }
private:
	void CreateMaker (LicenseRequest & req);
private:
	CurrentFolder		  const	_workFolder;
	std::unique_ptr<LicenseMaker> _maker;
};

#endif
