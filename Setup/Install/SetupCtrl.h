#if !defined SETUPCTRL_H
#define SETUPCTRL_H
//------------------------------------
//  (c) Reliable Software, 1996 - 2007
//------------------------------------

#include "Installer.h"

#include <Win/Controller.h>
#include <Graph/Font.h>
#include <Sys/Timer.h>
#include <Com/Shell.h>

namespace Win { class MessagePrepro; }
namespace Progress { class Dialog; }

class SetupController : public Win::Controller
{
private:
	enum { TimerId  = 125 };

	// Setup messages
	static const unsigned UM_START_INSTALLATION = 1;	// wParam & lParam not used

public:
	enum InstallMode
	{
		Full = 0,
		TemporaryUpdate,
		PermanentUpdate,
		CmdLineTools
	};

public:
	SetupController (Win::MessagePrepro & msgPrepro, InstallMode mode);

	~SetupController ();

	std::string GetCaption () const { return _setupCaption; }

	bool OnCreate (Win::CreateData const * create, bool & success) throw ();
	bool OnDestroy () throw ();
	bool OnEraseBkgnd(Win::Canvas canvas) throw();
	bool OnSize (int width, int height, int flag) throw ();
	bool OnPaint () throw ();
	bool OnTimer (int id) throw ();
	bool OnUserMessage (Win::UserMessage & msg) throw ();

private:
	void StartInstallation ();

private:
	Com::Use				_comUser;           // must be first

	Win::MessagePrepro &	_msgPrepro;
	std::unique_ptr<Installer>		_installer;
	std::unique_ptr<Progress::Dialog>	 _meterDialog;
	std::string				_productName;
	std::string				_setupCaption;
	Win::Timer				_timer;
	InstallMode				_mode;
	int						_cx;
	int						_cy;

	Win::Color				_bgColor;
	Win::Color				_hiColor;
	Win::Color				_loColor;
	Font::AutoHandle		_font;
	Brush::AutoHandle		_brush;
	Bitmap::AutoHandle		_bitmap;
	int						_cxChar;
	int						_cyChar;
};

#endif


