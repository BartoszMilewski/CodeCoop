#include "precompiled.h"
#include "DlgCtrl.h"
#include "resource.h"

#include <Ctrl/FileGet.h>
#include <File/Dir.h>

bool DlgCtrlHandler::OnApply () throw ()
{
	Win::Dow::Handle top = GetWindow ().GetParent ().GetParent ();
	Win::ControlMessage msg (IDOK, 0, top);
	top.SendMsg (msg);
	return true;
}

bool DlgCtrlHandlerOne::OnInitDialog () throw (Win::Exception)
{
	_templateFile.Init (GetWindow (), IDC_COMBO1);
    _email.Init		(GetWindow (), IDC_EDIT1);
    _licensee.Init	(GetWindow (), IDC_EDIT2);
	_seats.Init		(GetWindow (), IDC_EDIT3);
	_bcSeats.Init	(GetWindow (), IDC_EDIT4);
	_price.Init     (GetWindow (), IDC_EDIT5);
	_preview.Init   (GetWindow (), IDC_PREVIEW);
	_pro.Init		(GetWindow (), IDC_RADIO_PRO);
	_lite.Init		(GetWindow (), IDC_RADIO_LITE);
    // Other initializations...
	for (FileSeq seq (_workFolder.GetFilePath ("*.txt")); !seq.AtEnd (); seq.Advance ())
	{
		_templateFile.AddToList (seq.GetName());
	}
	_templateFile.SelectString (_request.TemplateFile ().c_str ());
	_email.SetString (_request.Email ());
	_licensee.SetString (_request.Licensee ());
	_seats.SetString (ToString (_request.Seats ()));
	_bcSeats.SetString (ToString (_request.BCSeats ()));
	_price.SetString (ToString (_request.Price ()));
	if (_request.Preview ())
		_preview.Check ();
	_pro.Check ();

	return true;
}

bool DlgCtrlHandlerTwo::OnInitDialog () throw (Win::Exception)
{
    _email.Init		(GetWindow (), IDC_EDIT1);
    _licensee.Init	(GetWindow (), IDC_EDIT2);
    _startNum.Init	(GetWindow (), IDC_EDIT3);
    _numLicenses.Init	(GetWindow (), IDC_EDIT4);
	_preview.Init   (GetWindow (), IDC_PREVIEW);
    // Other initializations...
	_email.SetString (_request.Email ());
	_licensee.SetString (_request.Licensee ());
	_numLicenses.SetString (ToString (_request.LicenseCount ()));
	_startNum.SetString (ToString (_request.StartNum ()));
	if (_request.Preview ())
		_preview.Check ();

	return true;
}

bool DlgCtrlHandlerThree::OnInitDialog () throw (Win::Exception)
{
    _numLicenses.Init (GetWindow (), IDC_EDIT1);
    _startNum.Init	(GetWindow (), IDC_EDIT2);
    _seats.Init		(GetWindow (), IDC_EDIT3);
    // Other initializations...
	_numLicenses.SetString (ToString (_request.LicenseCount ()));
	_startNum.SetString (ToString (_request.StartNum ()));
	_seats.SetString (ToString (_request.Seats ()));
	return true;
}

void DlgCtrlHandlerOne::OnApplyMsg ()
{
	_request.TemplateFile () = _templateFile.RetrieveEditText ();
	_request.Email () = _email.GetTrimmedString ();
	_request.Licensee () = _licensee.GetTrimmedString ();
	_seats.GetInt (_request.Seats ());
	_bcSeats.GetInt (_request.BCSeats ());
	_price.GetInt (_request.Price ());
	_request.Preview () = _preview.IsChecked ();
	_request.SetType (LicenseRequest::Simple);
	_request.Product () = _lite.IsChecked ()? 'L': 'P';
}

void DlgCtrlHandlerTwo::OnApplyMsg ()
{
	_request.Email () = _email.GetTrimmedString ();
	_request.Licensee () = _licensee.GetTrimmedString ();
	_numLicenses.GetInt (_request.LicenseCount ());
	_startNum.GetInt (_request.StartNum ());
	_request.Preview () = _preview.IsChecked ();
	_request.SetType (LicenseRequest::Distributor);
}

void DlgCtrlHandlerThree::OnApplyMsg ()
{
	_numLicenses.GetInt (_request.LicenseCount ());
	_seats.GetInt (_request.Seats ());
	_startNum.GetInt (_request.StartNum ());
	_request.SetType (LicenseRequest::Block);
}

