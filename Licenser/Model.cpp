//---------------------------
// (c) Reliable Software 2005
//---------------------------
#include "precompiled.h"
#include "Model.h"
#include "Maker.h"

Model::~Model () {}

bool Model::PostRequest (LicenseRequest & request)
{
	CreateMaker (request);
	Assert (_maker.get () != 0);
	return _maker->IsValid ();
}

void Model::CreateLicense ()
{
	_maker->Make (_workFolder);
}

void Model::CreateMaker (LicenseRequest & req)
{
	switch (req.GetType ())
	{
	case LicenseRequest::Simple:
		_maker.reset (new SimpleLicenseMaker (req));
		break;
	case LicenseRequest::Distributor:
		_maker.reset (new DistributorLicenseMaker (req));
		break;
	case LicenseRequest::Block:
		_maker.reset (new BlockLicenseMaker (req));
		break;
	default:
		throw Win::Exception ("Invalid license type requested");
		break;
	}
}
