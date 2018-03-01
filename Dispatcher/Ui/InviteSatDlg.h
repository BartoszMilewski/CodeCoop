#if !defined (INVITESATDLG_H)
#define INVITESATDLG_H
// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------
#include "resource.h"
#include "Invitee.h"
#include <Win/Dialog.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Button.h>
#include <Ctrl/ListBox.h>
#include <Ctrl/Static.h>

class NocaseSet;

class InviteSatData
{
public:
	InviteSatData (Invitee const & invitee, NocaseSet const & offsiteSats)
		: _invitee (invitee),
		  _offsiteSats (offsiteSats)
	{}

	std::string const & GetUserName () const { return _invitee.GetUserName (); }
	std::string const & GetProject () const { return _invitee.GetProjectName (); }
	std::string const & GetComputer () const { return _invitee.GetComputerName (); }
	NocaseSet const & GetOffsiteSats () const { return _offsiteSats; }
	bool HasOffsiteSats () const { return !_offsiteSats.empty (); }

	bool IsRemove () const { return _isRemove; }
	std::string const & GetPath () const { return _path; }
	void SetPath (std::string const & path) { _path = path; }
	void SetIsRemove (bool remove) { _isRemove = remove; }

	bool IsValid () const;
	void DisplayError (Win::Dow::Handle owner);
private:
	bool IsValidUNC () const;
private:
	Invitee		const & _invitee;
	NocaseSet   const & _offsiteSats;
	// out
	bool		_isRemove;
	std::string	_path;
};

class InviteSatCtrl : public Dialog::ControlHandler
{
public:
	InviteSatCtrl (InviteSatData & data)
		: Dialog::ControlHandler (IDD_INVITE_SAT_PATH),
		  _data (data)
	{}

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

private:
	Win::EditReadOnly _userName;
	Win::EditReadOnly _project;
	Win::EditReadOnly _computerName;
	Win::RadioButton  _isOnSat;
	Win::RadioButton  _isRemove;
	Win::Edit         _path;
	Win::Button		  _browse;

	InviteSatData	& _data;
};

class SatTransportCtrl : public Dialog::ControlHandler
{
public:
	SatTransportCtrl (
		std::string const & computerName, 
		std::string & transport, 
		NocaseSet const & offsiteSats)
		: Dialog::ControlHandler (IDD_INVITE_OFFSITE_SAT_TRANSPORT),
		  _computerName (computerName),
		  _transport (transport),
		  _offsiteSats (offsiteSats)
	{}

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

private:
	void ActivateNetControls ();
	void ActivateOffsiteControls ();
private:
	std::string const & _computerName;
	std::string		  & _transport;
	NocaseSet   const & _offsiteSats;

	Win::StaticText	  _info;
	Win::RadioButton  _isNetSat;
	Win::RadioButton  _isOffSat;
	Win::Edit         _path;
	Win::Button		  _browse;
	Win::ListBox::SingleSel	_offsites;
};

#endif
