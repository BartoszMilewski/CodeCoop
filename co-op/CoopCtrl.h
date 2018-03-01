#if !defined (COOPCTRL_H)
#define COOPCTRL_H
//------------------------------------
//	(c) Reliable Software, 1996 - 2008
//------------------------------------

#include "CmdVector.h"
#include "FolderEvent.h"
#include "IpcQueue.h"
#include "DisplayMan.h"
#include "GuiCritSect.h"

#include <Win/Controller.h>
#include <Win/Message.h>
#include <Sys/Timer.h>
#include <Com/Shell.h>
#include <Ctrl/Accelerator.h>
#include <Graph/Cursor.h>
#include <Win/AppCmdHandler.h>

namespace Win
{
	class MessagePrepro;
	class FileDropHandle;
}

namespace Accel
{
	class Maker;
}

namespace Notify
{
	class Handler;
}

class UiManager;
class Model;
class Commander;
class WinTimer;
class ExecutionStateMemento;
class FWatcher;
class FeedbackManager;

class CoopController : public Win::Controller, public Cmd::Executor, public AppCmdHandler
{
public:
	CoopController (Win::MessagePrepro & msgPrepro, int startupProjectId);
	~CoopController ();

	// Win::Controller
	Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ();
	Control::Handler * GetControlHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ();
	AppCmdHandler * GetAppCmdHandler () throw () { return this; }
	bool OnCreate (Win::CreateData const * create, bool & success) throw ();
	bool OnDestroy () throw ();
	bool OnFocus (Win::Dow::Handle winPrev) throw ();
	bool OnSize (int width, int height, int flag) throw ();
	bool OnSettingChange (Win::SystemWideFlags flags, char const * sectionName) throw ();
	bool OnCommand (int cmdId, bool isAccel) throw ();
	bool OnInitPopup (Menu::Handle menu, int pos) throw ();
	bool OnMenuSelect (int id, Menu::State state, Menu::Handle menu) throw ();
	bool OnContextMenu (Win::Dow::Handle wnd, int xPos, int yPos) throw ();
	bool OnChar (int vKey, int flags) throw ();
	bool OnTimer (int id) throw ();
	bool OnIpcInitiate (Win::Dow::Handle client, 
						std::string const & app, 
						std::string const & topic) throw ();
	bool OnUserMessage (Win::UserMessage & msg) throw ();
	bool OnRegisteredMessage (Win::Message & msg) throw ();
	bool OnInterprocessPackage (unsigned int msg, 
						char const * package, 
						unsigned int errCode, 
						long & result) throw ();
	// Cmd::Executor
	bool IsEnabled (std::string const & cmdName) const throw ();
	void ExecuteCommand (std::string const & cmdName) throw ();
	void ExecuteCommand (int cmdId) throw ();
	void PostCommand (std::string const & cmdName, NamedValues const & namedArgs);
	void DisableKeyboardAccelerators (Win::Accelerator * accel) throw ();
	void EnableKeyboardAccelerators () throw ();
	// AppCmdHandler
	bool OnBrowserBackward ();
	bool OnBrowserForward ();

	void SetWindowPlacement (Win::ShowCmd cmdShow, bool multipleWindows);

private:
	static unsigned const RetryTimerId = 666;
	static unsigned const RetryPeriod = 500;
	typedef std::vector<std::pair<std::string, std::string> > ArgList;
	typedef std::pair<std::string, ArgList> CmdData;

private:
	// Helpers
	bool ProcessRegisteredMessage (Win::Message & msg);
	void OnStartup ();
	void VerifySccSettings ();
	void VerifyDifferMergerRegistry ();
	void RestoreState ();
	void VerifyProject () throw ();
	void ExecuteQueuedCommand ();
	void MenuCommand (int cmdId);
	void OnFolderChange (FWatcher * watcher) throw ();
	void ProcessNotifications () throw ();
	void SwitchToCmdLine ();
	void SwitchToGUI (bool bring2Top);
	void EndIpcConversation () throw ();
	void CheckFolderRequests ();
	bool IsBusy () const { return _critSect.IsBusy (); }
	GuiCritSect & GetCritSect () { return _critSect; }

private:
	Com::UseOle						_comUser;		// Must be first
	std::unique_ptr<Model>			_model;

	// Synchronization
	GuiCritSect						_critSect;

	// User Interface
	std::unique_ptr<Commander>		_commander;
	std::unique_ptr<CmdVector>		_cmdVector;
	std::unique_ptr<UiManager>		_uiMan;
	std::unique_ptr<Accel::Handler>	_kbdAccel;		// Keyboard shortcuts
	Win::MessagePrepro &			_msgPrepro; 	// Code Co-op message pomp
	bool							_needRefresh;	// Redisplay screen
	bool							_isOperational;	// True, when controller is fully operational
	std::list<FolderRequest>		_folderChangeRequests;	// List of changed folders
	Win::Timer				 		_retryTimer;
	int 							_curRetryPeriod;
	Win::RegisteredMessage			_msgStartup;
	Win::RegisteredMessage			_msgIpcInitiate;
	Win::RegisteredMessage			_msgIpcTerminate;
	Win::RegisteredMessage			_msgIpcAbort;
	Win::RegisteredMessage			_msgProjectStateChange;
	Win::RegisteredMessage			_msgAutoMergeCompleted;

	Win::RegisteredMessage			_msgCommand;
	std::deque<CmdData>				_cmdQueue; // arguments to _msgCommand

	// IPC Conversation
	auto_active<CommandIpc::Queue>			_ipcQueue;
	std::unique_ptr<ExecutionStateMemento>	_executionStateMemento;
	// Info from command line mode
	int								_startupProjectId; // Continue GUI mode in this project
};

#endif
