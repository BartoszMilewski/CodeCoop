#if !defined (RECEIVERJOINDLG_H)
#define RECEIVERJOINDLG_H
//---------------------------------------------
// (c) Reliable Software 2005
//---------------------------------------------

#include "DistributorLicense.h"
#include "Resource.h"

#include <Win/Dialog.h>
#include <Ctrl/Static.h>
#include <Ctrl/Button.h>

class MemberState;
class Catalog;

class ReceiverJoinData
{
public:
	ReceiverJoinData (std::string const & caption,
					  std::string const & memberName,
					  std::string const & hubId);

	void SetReceiver (bool flag) { _receiver = flag; }
	void SetFullMember (bool flag) { _fullMember = flag; }
	void SetTrial (bool flag) { _trial = flag; }

	char const * GetCaption () const { return _caption.c_str (); }
	std::string const & GetName () const { return _memberName; }
	std::string const & GetHubId () const { return _hubId; }
	bool IsReceiver () const { return _receiver; }
	bool IsFullMember () const { return _fullMember; }
	bool IsTrial () const { return _trial; }
private:
	std::string	_memberName;
	std::string	_hubId;
	std::string	_caption;
	bool		_receiver;
	bool		_fullMember;
	bool		_trial;
};


class ReceiverJoinCtrl : public Dialog::ControlHandler
{
public:
	ReceiverJoinCtrl (DistributorLicensePool & distributorPool, ReceiverJoinData & dlgData)
		: Dialog::ControlHandler (IDD_RECEIVER_JOIN),
		  _dlgData (dlgData),
		  _distributorPool (distributorPool)
	{}

    bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

private:
	DistributorLicensePool& _distributorPool;
    Win::StaticText		_caption;
	Win::RadioButton	_fullMember;
	Win::RadioButton	_receiver;
	Win::CheckBox		_licensed;
	Win::StaticText		_licensePool;
	ReceiverJoinData  & _dlgData;
};

#endif
