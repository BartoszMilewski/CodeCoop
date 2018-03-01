#if !defined (DISPLAYPANE_H)
#define DISPLAYPANE_H
//------------------------------------
//  (c) Reliable Software, 2005 - 2006
//------------------------------------

#include <Win/Win.h>
#include "Table.h"

namespace FocusBar
{
	class Ctrl;
}
namespace Notify
{
	class Handler;
}
namespace Tool
{
	class DynamicRebar;
}
class FeedbackManager;
class InstrumentBar;
class TableController;
class TableBrowser;
class HierarchyController;
class TreeBrowser;
class WikiBrowserController;
class WidgetBrowser;
class RecordSet;
class Observer;

class Pane
{
public:
	class Bar
	{
	public:
		Bar (Win::Dow::Handle win)
			: _win (win)
		{}
		virtual ~Bar () {}

		void Hide () { _win.Hide (); }
		void Show () { _win.Show (); }
		void Move (unsigned left, unsigned top, unsigned width, unsigned height)
		{
			_win.Move (left, top, width, height);
		}

		virtual unsigned GetHeight () const;
		virtual void Refresh (std::string const & text, bool isActive) {}

	private:
		Win::Dow::Handle	_win;
	};

	class FocusBar : public Bar
	{
	public:
		FocusBar (::FocusBar::Ctrl * ctrl);

		unsigned GetHeight () const;
		void Refresh (std::string const & text, bool isActive);

	private:
		::FocusBar::Ctrl *	_ctrl;
	};

	class ToolBar : public Bar
	{
	public:
		ToolBar (Tool::DynamicRebar & rebar);

		unsigned GetHeight () const;
		void Refresh (std::string const & text, bool isActive);

	private:
		Tool::DynamicRebar	&	_rebar;
	};

public:
	Pane (unsigned id, std::unique_ptr<WidgetBrowser> browser)
		: _id (id),
		  _browser (std::move(browser))
	{}
	virtual ~Pane () {}

	void AddBar (std::unique_ptr<Pane::Bar> bar) { _bar = std::move(bar); }
	unsigned GetId () const { return _id; }
	void SetRestrictionFlag (std::string const & name, bool val = true);

	WidgetBrowser const * GetBrowser () const { return _browser.get (); }
	WidgetBrowser * GetBrowser () { return _browser.get (); }
	virtual RecordSet const * GetRecordSet() const;
	virtual void VerifyRecordSet() const;
	virtual bool HasWindow (Win::Dow::Handle h) const { return false; }
	virtual bool HasTable(Table::Id tableId) const;
	virtual Notify::Handler * GetNotifyHandler () const { return 0; } 

	virtual void Activate (FeedbackManager & feedback) = 0;
	virtual void DeActivate () = 0;
	virtual void Move (Win::Rect const & rect) = 0;
	void OnFocus (unsigned focusId);
	virtual void OnDoubleClick () {}
	virtual void Navigate (std::string const & target, int scrollPos) {}

	// Pane observation
	void Attach (Observer * observer, std::string const & topic = std::string ());
	void Detach (Observer * observer);
	Observer * GetObserver ();

	// Editing in the pane
	virtual void InPlaceEdit (int row) {}
	virtual void NewItemCreation () {}
	virtual void AbortNewItemCreation () {}

	void Invalidate ();
	void Clear (bool forGood);
	void RefreshBar (unsigned focusId, bool force);

protected:
	void ShowBar ();
	void HideBar ();
	void MoveBar (Win::Rect & paneRect);

protected:
	unsigned						_id;
	std::unique_ptr<Bar>				_bar;
	std::unique_ptr<WidgetBrowser>	_browser;
};

class TablePane : public Pane
{
public:
	TablePane (std::unique_ptr<TableController> ctrl,
			   std::unique_ptr<WidgetBrowser> browser);

	bool HasWindow (Win::Dow::Handle h) const;
	Notify::Handler * GetNotifyHandler () const; 

	void Activate (FeedbackManager & feedback);
	void DeActivate ();
	void Move (Win::Rect const & rect);
	void OnDoubleClick ();

	void InPlaceEdit (int row);
	void NewItemCreation ();
	void AbortNewItemCreation ();

private:
	std::unique_ptr<TableController>	_ctrl;
};

class TreePane : public Pane
{
public:
	TreePane (std::unique_ptr<HierarchyController> ctrl,
			  std::unique_ptr<WidgetBrowser> browser);

	bool HasWindow (Win::Dow::Handle h) const;
	Notify::Handler * GetNotifyHandler () const; 

	void Activate (FeedbackManager & feedback);
	void DeActivate ();
	void Move (Win::Rect const & rect);

private:
	std::unique_ptr<HierarchyController>	_ctrl;
};

class BrowserPane : public Pane
{
public:
	BrowserPane (std::unique_ptr<WikiBrowserController> ctrl, std::unique_ptr<WidgetBrowser> browser);
	void Activate (FeedbackManager & feedback);
	void DeActivate ();
	void Move (Win::Rect const & rect);
	void Navigate (std::string const & target, int scrollPos);
private:
	std::unique_ptr<WikiBrowserController>	_ctrl;
};

#endif
