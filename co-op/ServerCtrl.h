#if !defined (SERVERCTRL_H)
#define SERVERCTRL_H
//------------------------------------
//  (c) Reliable Software, 2000 - 2007
//------------------------------------

#include "IpcQueue.h"
#include "CmdVector.h"
#include "GuiCritSect.h"
#include "Global.h"
#include "FeedbackMan.h"

#include <Win/Controller.h>
#include <Sys/Timer.h>
#include <Com/Shell.h>
#include <File/Path.h>
#include <Win/Message.h>

namespace Win
{
	class MessagePrepro;
}

class UiManager;
class CmdLineSelection;
class DisplayManager;
class Commander;
class Model;

// Server controller

class ServerCtrl : public Win::Controller
{
public:
	ServerCtrl (unsigned int keepAliveTimeout, bool stayInProject);

	bool StayInGui () const { return _ipcQueue->StayInGui (); }
	int  GetProjectId () const { return _ipcQueue->GetProjectId (); }

	// Win::Controller
	bool OnCreate (Win::CreateData const * create, bool & success) throw ();
	bool OnDestroy () throw ();
	bool OnTimer (int id) throw ();
	bool OnRegisteredMessage (Win::Message & msg) throw ();

private:
	static int const KeepAliveTimerId = 333;
	static int const PendingMergeTimerId = 555;
	static int const OneSecond = 1000;

private:
	void OnQueueDone ();
	void SetKeepAliveTimer ();

private:
	Com::Use						_comUser;	// Must be first
	std::unique_ptr<Model>			_model;

	GuiCritSect						_critSect;

	// User Interface
	std::unique_ptr<Commander>		_commander;
	std::unique_ptr<CmdVector>		_cmdVector;
	FeedbackManager					_blindFeedback;
	std::unique_ptr<DisplayManager>	_displayMan;
	std::unique_ptr<CmdLineSelection>	_selectionMan;
	// IPC Conversation
	bool							_stayInProject;
	unsigned int					_keepAliveTimeout;
	Win::Timer				 		_keepAliveTimer;
	unsigned int					_pendingMergeTimeoutCount;
	Win::Timer				 		_pendingMergeTimer;
	Win::RegisteredMessage			_msgIpcInitiate;
	Win::RegisteredMessage			_msgIpcTerminate;
	Win::RegisteredMessage			_msgIpcAbort;
	Win::RegisteredMessage			_msgAutoMergeCompleted;
	Win::RegisteredMessage			_msgBackup;
	auto_active<CommandIpc::Queue>	_ipcQueue;
};

#endif
