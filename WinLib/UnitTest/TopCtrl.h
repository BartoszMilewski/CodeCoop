#if !defined (TOPCTRL_H)
#define TOPCTRL_H
//-----------------------------
//  (c) Reliable Software, 2005
//-----------------------------
#include <Win/Controller.h>
#include <Win/Message.h>
#include <Ctrl/Edit.h>
#include <Sys/Timer.h>

class Watch;
class FWatcher;

class TopCtrl: public Win::Controller
{
public:
	TopCtrl ();
	~TopCtrl ();
	bool OnCreate (Win::CreateData const * create, bool & success) throw ();
	bool OnDestroy () throw ();
	bool OnSize (int width, int height, int flag) throw ();
	bool OnRegisteredMessage (Win::Message & msg) throw ();
	bool OnUserMessage (Win::UserMessage & msg) throw ();
	bool OnTimer (int id) throw ();

	void FolderChange (FWatcher * watcher);
private:
	void OnInit ();
private:
	bool							_ready;
	Win::RegisteredMessage			_initMsg;
	Win::StreamEdit					_output;
	std::unique_ptr<Watch>			_watch;
	Win::Timer						_timer;
};


#endif
