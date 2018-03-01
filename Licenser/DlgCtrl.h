#ifndef _DLGCTRL_H_
#define _DLGCTRL_H_

#include <Win/Dialog.h>
#include <Ctrl/Button.h>
#include <Ctrl/Edit.h>
#include <Ctrl/ComboBox.h>
#include "LicenseRequest.h"
#include "Resource/resource.h"

class DlgCtrlHandler: public Dialog::ControlHandler
{
public:
	DlgCtrlHandler (int id, LicenseRequest & req)
		: Dialog::ControlHandler (id), _request (req)
	{}
	bool OnApply () throw ();

	virtual void OnApplyMsg () = 0;
protected:
	LicenseRequest & _request;
};

class DlgCtrlHandlerOne: public DlgCtrlHandler
{
public:
	DlgCtrlHandlerOne (LicenseRequest & req, 
					  FilePath const & workFolder)
		: DlgCtrlHandler (IDD_SINGLE, req),
		_workFolder (workFolder)
	{}
	bool OnInitDialog () throw (Win::Exception);
	void OnApplyMsg ();
private:
	FilePath const & _workFolder;
	Win::ComboBox _templateFile;
	Win::Edit     _email;
	Win::Edit     _licensee;
	Win::Edit     _seats;
	Win::Edit     _bcSeats;
	Win::Edit	  _price;
	Win::CheckBox _preview;
	Win::RadioButton _pro;
	Win::RadioButton _lite;
};

class DlgCtrlHandlerTwo: public DlgCtrlHandler
{
public:
	DlgCtrlHandlerTwo (LicenseRequest & req)
		: DlgCtrlHandler (IDD_DISTRIB, req)
	{}
	bool OnInitDialog () throw (Win::Exception);
	void OnApplyMsg ();
private:
	Win::Edit     _email;
	Win::Edit     _licensee;
	Win::Edit     _startNum;
	Win::Edit     _numLicenses;
	Win::CheckBox _preview;
};

class DlgCtrlHandlerThree: public DlgCtrlHandler
{
public:
	DlgCtrlHandlerThree (LicenseRequest & req)
		: DlgCtrlHandler (IDD_BLOCK, req)
	{}
	bool OnInitDialog () throw (Win::Exception);
	void OnApplyMsg ();
private:
	Win::Edit     _numLicenses;
	Win::Edit     _startNum;
	Win::Edit     _seats;
};

#endif // _DLGCTRL_H_
