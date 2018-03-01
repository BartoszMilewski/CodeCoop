#if !defined (NETWORKPROPERTIESDLG_H)
#define NETWORKPROPERTIESDLG_H
// ----------------------------------
// (c) Reliable Software, 2002 - 2005
// ----------------------------------

#include "Transport.h"

#include <Win/Dialog.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Button.h>
#include <File/Path.h>

class MyNetworkProperties
{
public:
	MyNetworkProperties (FilePath  const & publicInboxPath,
					      Transport const & myTransport)
		: _publicInboxPath (publicInboxPath),
		  _myTransport (myTransport)
	{}
	FilePath const & GetPublicInboxPath () const { return _publicInboxPath; }
	Transport const & GetIntraClusterTransportToMe () const { return _myTransport; }
	
	void SetPublicInboxPath (std::string const & publicInboxPath) { _publicInboxPath = publicInboxPath; }
	void SetIntraClusterTransportToMe (Transport const & myTransport) { _myTransport = myTransport; }

private:
	FilePath	_publicInboxPath;
    Transport	_myTransport;
};

class HubNetworkProperties
{
public:
	HubNetworkProperties (Transport const & hubTransport)
		: _hubTransport (hubTransport)
	{}
	Transport const & GetTransportToHub () const { return _hubTransport; }
	
	void SetTransportToHub (Transport const & hubTransport) { _hubTransport = hubTransport; }

private:
	Transport	_hubTransport;
};

class MyNetworkPropertiesCtrl : public Dialog::ControlHandler
{
public:
    MyNetworkPropertiesCtrl (MyNetworkProperties & data);

    bool OnInitDialog () throw (Win::Exception);
	bool OnApply () throw ();

private:
	MyNetworkProperties & _data;

	Win::Edit	_piPath;
	Win::Edit	_share;
};

class HubNetworkPropertiesCtrl : public Dialog::ControlHandler
{
public:
    HubNetworkPropertiesCtrl (HubNetworkProperties & data);

    bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

private:
	HubNetworkProperties & _data;

	Win::Edit		  _hubTransport;
    Win::Button       _browse;
};

#endif
