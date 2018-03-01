#if !defined (WIKIBROWSER_H)
#define WIKIBROWSER_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include <Win/Controller.h>
#include <Win/Message.h>

#include "CmdVector.h"

namespace Win
{
	class MessagePrepro;
	class Accelerator;
}
namespace Accel
{
	class Handler;
}
class Commander;
class UiManager;

namespace Wiki
{
	class BrowserCtrl : public Win::Controller, public Cmd::Executor
	{
	public:
		BrowserCtrl (Win::MessagePrepro & msgPump);
		~BrowserCtrl ();

		// Win::Controller
		Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ();
		bool OnCreate (Win::CreateData const * create, bool & success) throw ();
		bool OnDestroy () throw ();
		bool OnShutdown (bool isEndOfSession, bool isLoggingOff) throw ();
		bool OnClose () throw ();
		bool OnSize (int width, int height, int flag) throw ();
		bool OnFocus (Win::Dow::Handle winPrev) throw ();
		bool OnControl (Win::Dow::Handle control, int id, int notifyCode) throw ();
		bool OnKillFocus (Win::Dow::Handle winNext) throw ();
		bool OnSettingChange (Win::SystemWideFlags flags, char const * sectionName) throw ();
		bool OnInitPopup (Menu::Handle menu, int pos) throw ();
		bool OnMenuSelect (int id, Menu::State state, Menu::Handle menu) throw ();
		bool OnCommand (int cmdId, bool isAccel) throw ();

		// Cmd::Executor
		bool IsEnabled (std::string const & cmdName) const;
		void ExecuteCommand (std::string const & cmdName);
		void ExecuteCommand (int cmdId);
		void PostCommand (std::string const & cmdName, NamedValues const & namedArgs);
		void DisableKeyboardAccelerators (Win::Accelerator * accel);
		void EnableKeyboardAccelerators ();

		void SetWindowPlacement (Win::ShowCmd cmdShow, bool multipleWindows);

	public:
		static const char CLASS_NAME [];

	private:
		static const char PrefsKeyName [];
		typedef std::vector<std::pair<std::string, std::string> > ArgList;
		typedef std::pair<std::string, ArgList> CmdData;

	private:
		void MenuCommand (int cmdId);

	private:
		Win::MessagePrepro &			_msgPump;
		std::unique_ptr<CmdVector>		_cmdVector;
		std::unique_ptr<Commander>		_commander;
		std::unique_ptr<UiManager>		_uiManager;
		std::unique_ptr<Accel::Handler>	_kbdAccel;		// Keyboard shortcuts

		Win::RegisteredMessage			_msgCommand;
		std::deque<CmdData>				_cmdQueue; // arguments to _msgCommand
	};
};

#endif
