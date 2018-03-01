#if !defined (HUBIDDLG_H)
#define HUBIDDLG_H
// ----------------------------------
// (c) Reliable Software, 2002 - 2005
// ----------------------------------

#include "Transport.h"

#include <Win/Dialog.h>
#include <Ctrl/Edit.h>

class HubTransportCtrl : public Dialog::ControlHandler
{
public:
	HubTransportCtrl (std::string const & hubId);

	Transport const & GetTransport () const { return _transport; }
    bool OnInitDialog () throw (Win::Exception);
    bool OnApply () throw ();

private:
	Win::Edit	_readEdit;
	Win::Edit	_edit;
	
	std::string const & _hubId;
	Transport	_transport;
};

#endif
