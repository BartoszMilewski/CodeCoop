//-------------------------------------
// (c) Reliable Software 2001 -- 2002
// ------------------------------------

#include "precompiled.h"
#include "MapiUser.h"
#include "MapiDefDir.h"
#include "MapiBuffer.h"
#include "MapiProperty.h"
#include "MapiEx.h"
#include "MapiGPF.h"
#include <Dbg/Assert.h>

using namespace Mapi;

CurrentUser::CurrentUser (DefaultDirectory & defDir)
	: _isValid (false)
{
	PrimaryIdentityId id;
	defDir.QueryIdentity (id);
	if (id.IsValid ())
	{
		_isValid = defDir.OpenMailUser (id, *this);
	}
}

void CurrentUser::GetIdentity (std::string & name, std::string & emailAddr)
{
	Assert (_isValid);
	RetrievedProperty values;
	PropertyList props;
	props.Add (PR_DISPLAY_NAME);
	props.Add (PR_EMAIL_ADDRESS);
	Result result;
	try
	{
		result = _user->GetProps (props.Cnv2TagArray (),	// [in] Pointer to an array of property tags
													// identifying the properties whose values are to be retrieved.
						   0,						// [in] Bitmask of flags that indicates the format for properties
													// that have the type PT_UNSPECIFIED.
						   values.GetCountBuf (),	// [out] Pointer to a count of property values pointed
													// to by the lppPropArray parameter.
						   values.GetBuf ());		// [out] Pointer to a pointer to the retrieved property values.
	}
	catch (...)
	{
		Mapi::HandleGPF ("IMailUser::GetProps");
		throw;
	}
	_user.ThrowIfError (result, "MAPI -- Cannot retrieve current logged user name and e-mail address.");
	Assert (values.GetCount () == 2);
	if (values [0].Value.lpszA != reinterpret_cast<char *>(MAPI_E_NOT_FOUND))
		name.assign (values [0].Value.lpszA);
	if (values [1].Value.lpszA != reinterpret_cast<char *>(MAPI_E_NOT_FOUND))
		emailAddr.assign (values [1].Value.lpszA);
}
