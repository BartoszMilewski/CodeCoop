// ---------------------------
// (c) Reliable Software, 2000
// ---------------------------

#include "AddressPage.h"
#include "HeaderDetails.h"
#include "resource.h"

void AddressCtrl::OnInitDialog () throw (Win::Exception)
{
	_emailKnown.Init (GetWindow (), IDC_EMAIL_KNOWN);
	_emailList.Init (GetWindow (), IDC_EMAIL_COMBO);
	_emailUnknown.Init (GetWindow (), IDC_EMAIL_UNKNOWN);
	_emailDefine.Init (GetWindow (), IDC_EMAIL_DEFINE);
	_email.Init (GetWindow (), IDC_EMAIL);

	_idKnown.Init (GetWindow (), IDC_USERID_KNOWN);
	_idList.Init (GetWindow (), IDC_USERID_COMBO);
	_idUnknown.Init (GetWindow (), IDC_USERID_UNKNOWN);
	_idDefine.Init (GetWindow (), IDC_USERID_DEFINE);
	_id.Init (GetWindow (), IDC_USERID);

//	_emailKnown.Check ();
//	_email.Disable ();
	_emailDefine.Check ();

//	_idKnown.Check ();
//	_id.Disable ();
	_idDefine.Check ();
}

bool AddressCtrl::OnCommand (int ctrlId, int notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_EMAIL_KNOWN:
		_email.Disable ();
		_emailList.Enable ();
		return true;
	case IDC_EMAIL_UNKNOWN:
		_email.Disable ();
		_emailList.Disable ();
		return true;
	case IDC_EMAIL_DEFINE:
		_email.Enable ();
		_emailList.Disable ();
		return true;
	case IDC_USERID_KNOWN:
		_id.Disable ();
		_idList.Enable ();
		return true;
	case IDC_USERID_UNKNOWN:
		_id.Disable ();
		_idList.Disable ();
		return true;
	case IDC_USERID_DEFINE:
		_id.Enable ();
		_idList.Disable ();
		return true;
	};
	return false;
}

void AddressCtrl::RetrieveData ()
{
	if (_emailKnown.IsChecked ())
	{
		// Revisit:
	}
	else if (_emailUnknown.IsChecked ())
	{
		// Revisit:
	}
	else
	{
		SetEmail (_email.GetString ().c_str ());
	}

	if (_idKnown.IsChecked ())
	{
		// Revisit:
	}
	else if (_idUnknown.IsChecked ())
	{
		// Revisit:
	}
	else
	{
		SetUserId (_id.GetString ().c_str ());
	}
}

void AddressCtrl::SetEmail (char const * email)
{
	if (_isForRecip)
		_details._recipEmail = email;
	else
		_details._senderEmail = email;
}

void AddressCtrl::SetUserId (char const * id)
{
	if (_isForRecip)
		_details._recipId = id;
	else
		_details._senderId = id;
}
