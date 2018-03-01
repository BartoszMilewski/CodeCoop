#if !defined (DISPLAYFRAMECTRL_H)
#define DISPLAYFRAMECTRL_H
//---------------------------------------
//  DisplayFrameCtrl.h
//  (c) Reliable Software, 2002
//---------------------------------------

#include "RichDumpWin.h"

#include <Win/Controller.h>
#include <Ctrl/RichEditCtrl.h>

class DisplayFrameController : public Win::TopController, public Notify::RichEditHandler
{
	enum
	{
		DisplayId = 125
	};

public:
	explicit DisplayFrameController (bool addScrollBars = true);
	// Top controller
	Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ();
	bool OnCreate (Win::CreateData const * create, bool & success) throw ();
    bool OnDestroy () throw ();
	bool OnShutdown (bool isEndOfSession, bool isLoggingOff) throw ();
	bool OnClose () throw ();
	bool OnSize (int width, int height, int flag) throw ();
	bool OnControl (Win::Dow::Handle control, unsigned id, unsigned notifyCode) throw ();
	bool OnKillFocus (Win::Dow::Handle winNext) throw ();
	bool OnSettingChange (Win::SystemWideFlags flags, char const * sectionName) throw ();

	// Rich Edit Handler
	bool OnRequestResize (Win::Rect const & rect) throw ();

	void SaveUIPrefs () { _flags.set (SavePrefs, true); }
	void CloseOnLostFocus () { _flags.set (DestroyOnLostFocus, true); }
	Win::Rect const & GetResizeRequest () const { return _sizeRequest; }
	RichDumpWindow * GetRichDumpWindow () { return &_display; }

private:
	bool IsSaveUIPrefs () const	{ return _flags.test (SavePrefs); }
	bool IsCloseOnLostFocus () const { return _flags.test (DestroyOnLostFocus); }

	enum
	{
		SavePrefs,
		DestroyOnLostFocus
	};

private:
	RichDumpWindow		_display;			// Display pane
	Win::Rect			_sizeRequest;
	bool				_addScrollBars;
	std::bitset<std::numeric_limits<unsigned long>::digits>	_flags;
};

#endif
