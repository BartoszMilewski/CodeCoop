#if !defined (MAINCTRL_H)
#define MAINCTRL_H
// Reliable Software (c) 2005
#include "DlgCtrl.h"

#include <Win/Dialog.h>
#include <Graph/Icon.h>
#include <Ctrl/Tabs.h>
#include "Model.h"

namespace Notify { class Handler; }

enum LicenseTab
{
	LicRegular, LicDistribution, LicReseller
};

class TabCtrl
{
public:
	typedef EnumeratedTabs<LicenseTab> View;
public:
	TabCtrl (Win::Dow::Handle parentWin, int id)
		: _view (parentWin, id)
	{}
	View & GetView () { return _view; }
private:
	View _view;
};


class MainController: public Dialog::ModelessController, public Notify::TabHandler
{
	friend class MainControlHandler;
public:
	MainController(Win::MessagePrepro & prepro);
	~MainController ();
	void Initialize () throw (Win::Exception);
	bool OnDestroy () throw () { Win::Quit (); return true; }
	bool OnClose () throw () { GetWindow ().Destroy (); return true; }
	bool OnRegisteredMessage (Win::Message & msg) throw ();
	bool OnApply () throw ();
	Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ();
	// TabHandler
	bool OnSelChange () throw ();
private:
	void ClearDialog ();
private:
	Win::MessagePrepro & _prepro;
	LicenseRequest		_request;
	Model				_model;

	Win::RegisteredMessage	_initMsg;
	Icon::Handle		_icon;
	std::unique_ptr<TabCtrl>	_tabs;
	Win::Rect			_dispRect; // where the dialogs go inside tab control

	Win::Dow::Handle	_curDialog; // currently displayed dialog
	DlgCtrlHandler	*	_curCtrlHandler;

	// Dialog templates and controllers
	Dialog::Template	_dlgTmp1;
	DlgCtrlHandlerOne	_dlgCtrl1;
	Dialog::Template	_dlgTmp2;
	DlgCtrlHandlerTwo	_dlgCtrl2;
	Dialog::Template	_dlgTmp3;
	DlgCtrlHandlerThree	_dlgCtrl3;
};

class MainControlHandler: public Dialog::ControlHandler
{
public:
	MainControlHandler (int dlgId, MainController * ctrl)
		: Dialog::ControlHandler (dlgId),
		  _ctrl (ctrl)
	{}
	bool OnInitDialog () throw (Win::Exception)
	{
		_ctrl->Initialize ();
		return true;
	}
	bool OnApply () throw ();
private:
	MainController * _ctrl;
};

#endif
