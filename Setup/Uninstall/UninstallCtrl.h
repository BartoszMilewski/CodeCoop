#if !defined UNINSTALLCTRL_H
#define UNINSTALLCTRL_H
//------------------------------------
//  (c) Reliable Software, 1996 - 2007
//------------------------------------

#include <Win/Controller.h>
#include <Com/Shell.h>
#include <Sys/Timer.h>

namespace Win { class MessagePrepro; }
namespace Progress { class Dialog; }

class UninstallController : public Win::Controller
{
private:
	static const unsigned UM_START_DEINSTALLATION = 1;
	static const unsigned TIMER_ID = 225;

public:
	UninstallController (Win::MessagePrepro * msgPrepro, bool isFullUninstall);
	~UninstallController ();

	bool OnCreate (Win::CreateData const * create, bool & success) throw ();
	bool OnDestroy () throw ();
	bool OnTimer (int id) throw ();
	bool OnUserMessage (Win::UserMessage & msg) throw ();

private:
	bool UserWantsToRemoveUs () const;
	bool CompleteUninstall ();
	void RestoreOriginalVersion ();
	bool UserCanRemoveUs () const;
	bool UserDefectedFromAllProjects ();
	bool StartDefectTool ();
	void Uninstall ();
	bool IsPublicInboxEmpty () const;
	void KeepWaiting ();

private:
	Com::Use				_comUser;           // must be first
	Win::MessagePrepro *	_msgPrepro;
	std::unique_ptr<Progress::Dialog>	 _meterDialog;
	Win::Timer				_timer;
	unsigned				_retryCount;
	bool					_defectToolFinished;
	bool					_isFullUninstall;
};

#endif
