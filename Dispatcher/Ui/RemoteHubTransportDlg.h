#if !defined (REMOTEHUBTRANSPORTDLG_H)
#define REMOTEHUBTRANSPORTDLG_H
// ----------------------------------
// (c) Reliable Software, 2002 - 2005
// ----------------------------------

#include "Transport.h"

#include <Win/Dialog.h>
#include <Ctrl/Edit.h>

class InterClusterTransportData
{
public:
	InterClusterTransportData (std::string const & hubId, 
							Transport const & oldTransport)
        : _hubId (hubId), 
		  _oldTransport (oldTransport),
		  _newTransport (oldTransport)
    {}

	Transport const & GetNewTransport () const { return _newTransport; }
	Transport const & GetOldTransport () const { return _oldTransport; }
	std::string const & GetHubId () const { return _hubId; }

	void SetNewTransport (Transport const & transport)
	{
		_newTransport = transport;
	}
private:
	std::string const & _hubId;
    Transport const &   _oldTransport;
	Transport		    _newTransport;
};

class InterClusterTransportCtrl : public Dialog::ControlHandler
{
public:
    InterClusterTransportCtrl (InterClusterTransportData & data);

    bool OnInitDialog () throw (Win::Exception);
    bool OnApply () throw ();

private:
    Win::Edit    _hubIdEdit;
    Win::Edit    _transportEdit;
    Win::Edit    _transportTypeEdit;

    InterClusterTransportData & _dlgData;
};

#endif
