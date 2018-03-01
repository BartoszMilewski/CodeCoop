#include "precompiled.h"
#include "License.h"

#include <Dbg/Assert.h>

namespace Test_License
{

void VerifyLicensePack (LicensePack const &licensePack, char const *licensee, char const *key, bool isCurrent, bool isValid)
{
	Assert (0 == strcmp (licensePack.GetLicensee (), licensee));
	Assert (0 == strcmp (licensePack.GetKey (), key));
	Assert (licensePack.GetSeatCount () == (isValid ? 1 : 0));
	Assert (isValid == licensePack.IsValid ());
	Assert (isCurrent == licensePack.IsCurrentVersion ());

	std::string license;
	license += licensee;
	license += '\n';
	license += key;
	Assert (licensePack.IsEqual (license));
	Assert (0 == strcmp (license.c_str (), licensePack.GetLicense ()));
}

void VerifyAllForms (char const *licensee, char const *key, bool isCurrent, bool isValid)
{
	std::string license;
	license += licensee;
	license += '\n';
	license += key;

	LicensePack licensePack1 (license.c_str ());
	VerifyLicensePack (licensePack1, licensee, key, isCurrent, isValid);

	LicensePack licensePack2 (license);
	VerifyLicensePack (licensePack2, licensee, key, isCurrent, isValid);

	LicensePack licensePack3 (licensee, key);
	VerifyLicensePack (licensePack3, licensee, key, isCurrent, isValid);
}

void Test_LicensePack ()
{
	VerifyAllForms ("TestMe", "ccNcEuit7w", true, true);
	VerifyAllForms ("Alex 3.5", "cRH3Jth", false, true);
	VerifyAllForms ("Test", "Invalid", false, false);
}

void RunAll ()
{
	Test_LicensePack ();
}

}