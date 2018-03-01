#if !defined (PANE_H)
#define PANE_H
//---------------------------
// (c) Reliable Software 2009
//---------------------------
#include "ListBrowser.h"
#include "ListController.h"

class Pane
{
public:
	Pane(Win::Dow::Handle winParent)
		: _winParent(winParent)
	{
		_browser.reset(new ListBrowser());
		_controller.reset(new ListController(winParent));
	}
	void Move(Win::Rect & rect)
	{
		_controller->MoveView(rect);
	}
private:
	Win::Dow::Handle			_winParent;
	std::auto_ptr<ListBrowser>	_browser;
	std::auto_ptr<ListController> _controller;
};

#endif
