#if !defined (DBGMONITORCTRL_H)
#define DBGMONITORCTRL_H
//------------------------------------
//  (c) Reliable Software, 2003 - 2006
//------------------------------------

#include "RichDumpWin.h"
#include "CmdVector.h"

#include <Win/Controller.h>
#include <Win/Message.h>
#include <Ctrl/Menu.h>
#include <Ctrl/RichEditCtrl.h>

class Commander;

class DbgMonitorCtrl : public Win::Controller, public Notify::RichEditHandler
{
	enum
	{
		DisplayId = 254
	};

public:
	explicit DbgMonitorCtrl (bool addScrollBars = true);
	~DbgMonitorCtrl ();

	// Controller
	Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ();
	bool OnCreate (Win::CreateData const * create, bool & success) throw ();
    bool OnDestroy () throw ();
	bool OnShutdown (bool isEndOfSession, bool isLoggingOff) throw ();
	bool OnClose () throw ();
	bool OnSize (int width, int height, int flag) throw ();
	bool OnControl (Win::Dow::Handle control, int id, int notifyCode) throw ();
	bool OnKillFocus (Win::Dow::Handle winNext) throw ();
	bool OnSettingChange (Win::SystemWideFlags flags, char const * sectionName) throw ();
	bool OnInitPopup (Menu::Handle menu, int pos) throw ();
	bool OnMenuSelect (int id, Menu::State state, Menu::Handle menu) throw ();
	bool OnCommand (int cmdId, bool isAccel) throw ();
	bool OnRegisteredMessage (Win::Message & msg) throw ();
	bool OnInterprocessPackage (unsigned int msg, char const * package, unsigned int errCode, long & result) throw ();

	// Rich Edit Handler
	bool OnRequestResize (Win::Rect const & rect) throw ();

	void SaveUIPrefs () { _flags.set (SavePrefs, true); }
	void CloseOnLostFocus () { _flags.set (DestroyOnLostFocus, true); }
	Win::Rect const & GetResizeRequest () const { return _sizeRequest; }
	RichDumpWindow * GetRichDumpWindow () { return &_display; }

private:
	void OnDbgOutput (char const * buf);
	void AddScrollBars () { _flags.set (ScrollBars, true); }
	bool IsAddScrollBars () const { return _flags.test (ScrollBars); }
	bool IsSaveUIPrefs () const	{ return _flags.test (SavePrefs); }
	bool IsCloseOnLostFocus () const { return _flags.test (DestroyOnLostFocus); }

	enum
	{
		SavePrefs,
		DestroyOnLostFocus,
		ScrollBars
	};

	class ColorMap
	{
	public:
		ColorMap ();

		Win::Color	GetColor (std::string const & threadId);

	private:
		std::vector<Win::Color>			_colors;
		unsigned int					_firstAvailable;
		std::map<std::string, unsigned>	_map;
	};

private:
	RichDumpWindow					_display;			// Display pane
	std::auto_ptr<Menu::DropDown>	_menu;
	Win::RegisteredMessage			_msgPutLine;
	Win::Rect						_sizeRequest;
	std::auto_ptr<CmdVector>		_cmdVector;
	std::auto_ptr<Commander>		_commander;
	std::bitset<std::numeric_limits<unsigned long>::digits>	_flags;
	ColorMap						_colorMap;
};

#endif
