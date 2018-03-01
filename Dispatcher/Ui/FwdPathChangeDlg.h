#if !defined (FWDPATHCHANGEDLG_H)
#define FWDPATHCHANGEDLG_H
// ----------------------------------
// (c) Reliable Software, 2001 - 2005
// ----------------------------------

#include "Address.h"
#include "Transport.h"
#include <Win/Dialog.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Button.h>
#include <File/Path.h>

class FwdPathChangeData
{
public:
    FwdPathChangeData (Address const & address, 
                       Transport const & transport,
					   bool askToReplaceAll)
        : _address (address), 
		  _oldTransport (transport),
		  _newTransport (transport),
		  _askToReplaceAll (askToReplaceAll),
		  _replaceAll (false)
    {}

	Transport const & GetNewTransport () const { return _newTransport; }
	Transport const & GetOldTransport () const { return _oldTransport; }
	Address const & GetAddress () const { return _address; }
	bool IsToReplaceAll () const { return _askToReplaceAll; }
	void SetReplaceAll (bool value) { _replaceAll = value; }
	void SetNewTransport (Transport const & transport)
	{
		_newTransport = transport;
	}
	bool ReplaceAll () const { return _replaceAll; }

private:
	Address const &	_address;
	bool const		_askToReplaceAll;

    Transport const &_oldTransport;
	Transport		_newTransport;
	bool			_replaceAll;
};

class FwdPathChangeCtrl : public Dialog::ControlHandler
{
public:
    FwdPathChangeCtrl (FwdPathChangeData & data);

    bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

private:
    Win::Edit    _hubIdEdit;
    Win::Edit    _projEdit;
    Win::Edit    _locEdit;
    Win::Edit    _pathEdit;
    Win::Button  _browse;
	Win::CheckBox _replaceAll;

    FwdPathChangeData & _dlgData;
};

#endif
