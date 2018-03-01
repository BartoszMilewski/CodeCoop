#if !defined (UPDATEDLG_H)
#define UPDATEDLG_H
// ----------------------------------
// (c) Reliable Software, 2003 - 2005
// ----------------------------------

#include "resource.h"

#include <Win/Dialog.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Spin.h>
#include <Ctrl/Button.h>
#include <Ctrl/Static.h>

class UpdateDlgData
{
	friend class UpdateCtrl;
public:
	UpdateDlgData (bool isAutoCheck,
				   bool isAutoDownload,
				   std::string const & versionHeadline,
				   std::string const & releaseNotesLink,
				   std::string const & bulletinHeadline,
				   std::string const & bulletinLink,
				   bool isExeDownloaded)
		: _isAutoCheck (isAutoCheck),
		  _isAutoDownload (isAutoDownload),
		  _versionHeadline (versionHeadline),
		  _releaseNotesLink (releaseNotesLink),
		  _bulletinHeadline (bulletinHeadline),
		  _bulletinLink (bulletinLink),
		  _isExeDownloaded (isExeDownloaded),
		  _isRemindMeLater (false),
		  _turnOnAutoDownload (false)
	{}
	bool IsRemindMeLater () const { return _isRemindMeLater; }
	bool IsTurnOnAutoDownload () { return _turnOnAutoDownload; }
private:
	bool const	_isExeDownloaded;
	bool const	_isAutoCheck;
	bool const	_isAutoDownload;
	int			_period;
	bool		_isRemindMeLater;
	bool		_turnOnAutoDownload;

	std::string		_versionHeadline;
	std::string		_releaseNotesLink;
	std::string		_bulletinHeadline;
    std::string		_bulletinLink;
};

class UpdateCtrl : public Dialog::ControlHandler
{
public:
    UpdateCtrl (UpdateDlgData & data);

	bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

private:
	UpdateDlgData & _data;

	Win::StaticText   _statusStr;
	Win::EditReadOnly _versionHeadline;
	Win::Button		  _releaseNotesButton;

	Win::StaticText  _bulletinHeadline;
	Win::Button		 _bulletinButton;

	Win::Button		  _remindMeLater;
	Win::CheckBox	  _turnOnAutoDownload;
};

class UpToDateDlgData
{
	friend class UpToDateCtrl;
public:
	UpToDateDlgData (std::string const & status,
					 std::string const & bulletinHeadline,
				     std::string const & bulletinLink)
		: _status (status),
		  _bulletinHeadline (bulletinHeadline),
		  _bulletinLink (bulletinLink)
	{}
private:
	std::string		_status;
	std::string		_bulletinHeadline;
    std::string		_bulletinLink;
};

class UpToDateCtrl : public Dialog::ControlHandler
{
public:
    UpToDateCtrl (UpToDateDlgData & data);

	bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

private:
	UpToDateDlgData & _data;

	Win::StaticText  _status;
	Win::StaticText  _bulletinHeadline;
	Win::Button		 _bulletinButton;
};

class UpdateOptionsDlgData
{
	friend class UpdateOptionsCtrl;
public:
	UpdateOptionsDlgData (bool isAutoCheck, bool isAutoDownload, int period)
		: _isAutoCheck (isAutoCheck),
		  _isAutoDownload (isAutoDownload),
	      _period (period)
	{}
	bool IsAutoCheck () const { return _isAutoCheck; }
	bool IsAutoDownload () const { return _isAutoDownload; }
	int  GetPeriod () const { return _period; }

private:
	bool	_isAutoCheck;
	bool	_isAutoDownload;
	int		_period;
};

class UpdateOptionsCtrl : public Dialog::ControlHandler
{
public:
    UpdateOptionsCtrl (UpdateOptionsDlgData & data);

	bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

private:
	void SetAutoCheckControls (bool isAuto);

private:
	UpdateOptionsDlgData & _data;

	Win::CheckBox    _autoCheck;
	Win::CheckBox    _inBackground;
	Win::Edit	     _period;
	Win::Spin        _periodSpin;
};

#endif
