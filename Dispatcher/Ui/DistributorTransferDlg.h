#if !defined (DISTRIBUTORTRANSFERDLG_H)
#define DISTRIBUTORTRANSFERDLG_H
//---------------------------
// (c) Reliable Software 2005
//---------------------------

#include <Win/Dialog.h>
#include <Ctrl/Edit.h>
#include <Ctrl/ComboBox.h>
#include <Ctrl/Button.h>

class RemoteHubList;

class DistributorTransferData
{
	friend class DistributorTransferCtrl;
public:
	DistributorTransferData (NocaseSet const & hubs, std::string const & licensee, int countLeft)
		: _hubs (hubs), _licensee (licensee), _countLeft (countLeft), _localSave (false)
	{}
	bool IsLocalSave () const { return _localSave; }
	std::string const & GetTargetHubId () const { return _targetHub; }
private:
	NocaseSet const & _hubs;
	std::string		_licensee;
	int				_countLeft;
	std::string		_targetHub;
	bool			_localSave;
};

class DistributorTransferCtrl: public Dialog::ControlHandler
{
public:
	DistributorTransferCtrl (DistributorTransferData & data);

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

private:
	DistributorTransferData & _data;
	Win::EditReadOnly	_countLeft;
	Win::EditReadOnly	_licensee;
	Win::ComboBox		_hubId;
	Win::CheckBox		_localSave;
};
#endif
