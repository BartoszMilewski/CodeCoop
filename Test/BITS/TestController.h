#if !defined (TESTCONTROLLER_H)
#define TESTCONTROLLER_H
// ---------------------------
// (c) Reliable Software, 2003
// ---------------------------
#include <Win/Controller.h>
#include <Sys/Timer.h>
#include <Ctrl/Edit.h>
#include <Com/Com.h>
#include <Net/BITSDownloader.h>

class TestController: public Win::Controller, public DownloadSink
{
public:
	TestController () : _timer (0) {}
	~TestController () {}
	bool OnCreate (Win::CreateData const * create, bool & success) throw ();
	bool OnDestroy () throw ()
	{
		Win::Quit ();
		return true;
	}
	bool OnClose () throw ()
	{
		if (_downloader.get () != 0)
			_downloader->CancelCurrent ();

		_downloader.reset ();
		return false;
	}

	bool OnTimer (int id) throw ();
	bool OnSize (int width, int height, int flag) throw ()
	{
		_edit.Move (0, 0, width, height);
		return true;
	}

	void PutLine (char const * text)
	{
		_edit.Append (text);
		_edit.Append ("\r\n");
	}

	// DownloadSink interface
	bool OnConnecting ();
	 // returns false, if the download should be terminated
	bool OnProgress (int bytesTotal, int bytesTransferred);
	bool OnCompleted (CompletionStatus status);
	void OnTransientError ();
	void OnFatalError ();
private:
	void GetSingleFileCallbackPureAPI ();
	void GetSingleFileCallback ();
private:
	Com::MainUse   _useCom;

	Com::IfacePtr<IBackgroundCopyManager> service;
	Com::IfacePtr<IBackgroundCopyJob> job;

	std::auto_ptr<BITS::Downloader> _downloader;

	Win::Timer _timer;
	Win::Edit  _edit;
};

#endif
