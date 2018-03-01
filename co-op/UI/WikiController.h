#if !defined (WIKICONTROLLER_H)
#define WIKICONTROLLER_H
//------------------------------------
//  (c) Reliable Software, 2005 - 2006
//------------------------------------

#include "WebBrowserEvents.h"
#include "WikiBrowser.h"
#include "Sql.h"

#include <Ctrl/DropDown.h>
#include <Ctrl/ProgressBar.h>
#include <Win/ControlHandler.h>

class InPlaceBrowser;
namespace Cmd { class Executor; }
namespace Focus { class Ring; }

class WikiBrowserController: public WebBrowserEvents, public Observer
{
public:
	WikiBrowserController (int id,
					 Win::Dow::Handle win,
					 Cmd::Executor & executor,
					 Focus::Ring & focusRing);
	~WikiBrowserController ();
	int GetId () const { return _id; }
	WebBrowserView & GetView ();
	void MoveView (Win::Rect const & viewRect);
	bool IsInsideView (Win::Point const & screenPt) const;
	void ShowView ();
	void HideView ();
	void Activate (WikiBrowser * browser);
	void StartNavigation (FeedbackManager & feedback);
	void UpdateAll ()
	{
		Assert (!"shouldn't be called");
	}
	void Update (std::string const & topic);
	void Navigate (std::string const & target, int scrollPos);
	// WebBrowserEvents
	void BeforeNavigate (std::string const & url, Automation::Bool & cancel);
	void DownloadBegin ();
	void DownloadComplete ();
	// progress = -1 means completion
	void ProgressChange (long progress, long progressMax);
	void DocumentComplete (std::string const & url);
private:
	bool Reload (std::string const & queryString, std::string const & label);
	bool NewFile (std::string const & nameSpace, bool doOpen, bool isSystemDir);
	void DeleteFile ();
	bool DoInsert (Sql::InsertCommand * insert);
	void InsertIntoRegistry (Sql::InsertCommand * insert);
private:
	int				_id;
	Cmd::Executor & _executor;
	FeedbackManager * _feedback;
	Focus::Ring &	_focusRing;
	Win::Dow::Handle _win;
	std::unique_ptr<InPlaceBrowser>	_webBrowser;
	WikiBrowser *	_browser;

	FilePath		_wikiRoot;		// current directory
	FilePath		_globalDir;		// global wiki directory
	std::string		_srcPath;		// current wiki file
	std::string		_srcUrl;		// with query string
	int				_scrollPos;
	std::string		_redirPath;		// temporary html file
	std::string		_targetPath;	// url being loaded
};

class UrlCtrl : public Control::Handler
{
public:
	UrlCtrl (int id,
			Win::Dow::Handle topWin,
			Win::Dow::Handle canvasWin,
			Cmd::Executor & executor);

	int GetId () const { return _id; }
	void MoveView (Win::Rect const & viewRect) { _dropDown.Move (viewRect); }
	void HideView () { _dropDown.Hide (); }
	void ShowView () { _dropDown.Show (); }
	void SetFont (Font::Descriptor const & font) { _dropDown.SetFont (font); }
	Win::Dow::Handle GetEditWindow () const { return _dropDown.GetEditWindow (); }
	Win::Dow::Handle GetView () const { return _dropDown; }
	unsigned GetHeight () const { return _dropDown.GetHeight (); }

	bool OnControl (unsigned id, unsigned notifyCode) throw (Win::Exception);
	void RefreshUrlList (std::string const & urls);

private:
	Win::DropDown	_dropDown;
	Cmd::Executor &	_executor;
};

#endif
