#if !defined (CMPDIRCTRL_H)
#define CMPDIRCTRL_H
//------------------------------------
//  (c) Reliable Software, 2002 - 2005
//------------------------------------
#include "ListViewer.h"
#include "Session.h"
#include "CmdVector.h"
#include "Menu.h"
#include "DataPortal.h"

#include <Win/Controller.h>
#include <Win/Message.h>
#include <Ctrl/Accelerator.h>
#include <Graph/Cursor.h>
#include <Ctrl/Menu.h>
#include <Sys/Synchro.h>
#include <auto_vector.h>

class Commander;
class View;

namespace Notify { class Handler; }
namespace Win { class MessagePrepro; }

class Opener
{
public:
	virtual ~Opener () {}
	virtual void Open (int i) = 0;
};

class CmpDirCtrl: public Win::Controller, public Data::Sink, public Opener, public Cmd::Executor
{
public:
	CmpDirCtrl (char const * cmdLine, Win::MessagePrepro & msgPrepro);
	~CmpDirCtrl ();
	bool OnCreate (Win::CreateData const * create, bool & success) throw ();
	bool OnDestroy () throw ();
	bool OnCommand (int cmdId, bool isAccel) throw ();
	bool OnControl (Win::Dow::Handle control, unsigned id, unsigned notifyCode) throw ();
	Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ();
	Control::Handler * GetControlHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ();
	bool OnSize (int width, int height, int flag) throw ();
	bool OnInitPopup (Menu::Handle menu, int pos) throw ();
	bool OnMenuSelect (int id, Menu::State state, Menu::Handle menu) throw ();
	bool OnRegisteredMessage (Win::Message & msg) throw ();
	void DirUp ();
	bool IsDown () { return _curSession != 0 && _curSession->IsDown (); }
	void Refresh ();
	void StartSessionOld ();
	void StartSessionNew ();
	void StartSessionDiff ();
	// Data::Sink
	void DataReady (Data::ChunkHolder data, bool isDone, int srcId);
	void DataClear ();
	// Opener
	void Open (int i);
	// Cmd::Executor
	bool IsEnabled (std::string const & cmdName) const throw ();
	void ExecuteCommand (std::string const & cmdName) throw ();
	void ExecuteCommand (int cmdId) throw ();
	void DisableKeyboardAccelerators (Win::Accelerator * accel) throw () {}
	void EnableKeyboardAccelerators () throw () {}
private:
	void MenuCommand (int cmdId) throw ();
	void OnDataReady (bool isDone);
	void OnInit ();
	void StartSession (Session & session);
private:
	SessionDir						_sessionNew;
	SessionDir						_sessionOld;
	SessionDiff						_sessionDiff;
	Session						  * _curSession;
	Win::RegisteredMessage			_dataReadyMsg;
	Win::RegisteredMessage			_dataClearMsg;
	Win::RegisteredMessage			_initMsg;
	// multi-threaded access
	Data::ChunkBuffer				_data;

	Cursor::Hourglass				_hourglass;
	std::unique_ptr<Notify::Handler>	_viewHandler;
	std::unique_ptr<Notify::Handler>	_tabHandler;
	std::unique_ptr<Notify::Handler>	_toolTipHandler;
	std::unique_ptr<View>				_view;
	Win::MessagePrepro			&	_msgPrepro;
	std::unique_ptr<Commander>		_commander;
	std::unique_ptr<CmdVector>		_cmdVector;
	std::unique_ptr<Accel::Handler>	_kbdAccel;
	std::unique_ptr<Menu::DropDown>	_menu;
};

#endif
