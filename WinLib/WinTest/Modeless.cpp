//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include "precompiled.h"
#include "Modeless.h"
#include "Resource/resource.h"

void ModelessManager::Open (Win::Dow::Handle parentWin)
{
	if (IsActive ())
		_win.SetFocus ();
	else
	{
		if (_handler.get () == 0)
			_handler.reset (new ModelessHandler (*this));
		Dialog::ModelessMaker maker (*_handler.get (), _msgPrepro, parentWin);
		_win = maker.Create (parentWin);
		_win.Show ();
	}
}

ModelessHandler::ModelessHandler (ModelessManager & man)
	: Dialog::ControlHandler (IDD_MODELESS),
	  _man (man)
{
}

bool ModelessHandler::OnInitDialog () throw (Win::Exception)
{
	_check.Init (GetWindow (), IDC_CHECK);
	if (_man.IsFlag ())
		_check.Check ();
	return true;
}
bool ModelessHandler::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_CHECK:
		_man.SetFlag (_check.IsChecked ());
		break;
	}
	return true;
}