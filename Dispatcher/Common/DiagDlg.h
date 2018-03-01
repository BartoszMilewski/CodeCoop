#if !defined (DIAGDLG_H)
#define DIAGDLG_H
//------------------------------------
//  (c) Reliable Software, 1999 - 2008
//------------------------------------

#include "DiagFeedback.h"
#include "EmailDiag.h"

#include <Ctrl/Button.h>
#include <Ctrl/Edit.h>
#include <Ctrl/ProgressBar.h>
#include <Win/Dialog.h>
#include <Sys/Timer.h>
#include <Win/Win.h>

namespace Win { class MessagePrepro; }
class ClusterRecipientList;
class ConfigData;
class DiagData;
class Transport;

//
// Helper class switching buttons for the test duration
//

class ButtonSwitch
{
public:
	ButtonSwitch (Win::Button & ok, Win::Button & start, Win::Button & stop);
	~ButtonSwitch ();

private:
	Win::Button &	_ok;
	Win::Button &	_start;
	Win::Button &	_stop;
};

//
// Test file used during mailbox and forwarding tests
//

class TestFile
{
public:
	TestFile ();
	~TestFile ();
	char const * GetFullPath () { return _tmp.GetFilePath (_name); }
	char const * GetName () const { return _name; }

private:
	static char const * _name;
	TmpPath				_tmp;
};

//
// Diagnostic dialog data
//

class DiagData
{
public:
	DiagData (Win::MessagePrepro * msgPrepro,
			  Email::Manager & emailMan,
			  ClusterRecipientList const & clusterRecip,
			  ConfigData const & config)
	: _msgPrepro (msgPrepro),
	  _emailMan (emailMan),
	  _clusterRecip (clusterRecip),
	  _config (config)
	{}

	Win::MessagePrepro * GetMsgPrepro () { return _msgPrepro; }
	void SetDlgFilter (Win::Dow::Handle hwndDlg);

	Email::Manager & GetEmailManager () const { return _emailMan; }
	ClusterRecipientList const & GetClusterRecipients () const { return _clusterRecip; }
	Transport const & GetTransportToHub () const;
	Transport const & GetInterClusterTransportToMe () const;

	bool IsStandalone () const;
	bool IsHubOrPeer () const;
	bool IsTmpHub () const;
	bool IsRemoteSat () const;

private:
	Win::MessagePrepro *			_msgPrepro;
	Email::Manager &				_emailMan;
	ClusterRecipientList const &	_clusterRecip;
	ConfigData const &				_config;
};

//
// Diagnostic dialog controller
//

class DiagCtrl : public Dialog::ControlHandler
{
public:
    DiagCtrl (DiagData & data);

    bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

private:
	void TestHubForwarding ();
	void TestSatForwarding ();

private:
    Win::RadioButton	_email;
    Win::RadioButton	_forwarding;
	Win::Edit			_status;
	Win::ProgressBar	_progressBar;
	Win::Button			_start;
	Win::Button			_stop;
	Win::Button			_ok;
	DiagFeedback		_feedback;
	DiagProgress		_diagProgress;
    DiagData &			_dlgData;
};

#endif
