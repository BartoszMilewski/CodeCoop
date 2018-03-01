#if !defined (POPUPDISPLAY_H)
#define POPUPDISPLAY_H
//---------------------------------------
//  PopupDisplay.h
//  (c) Reliable Software, 2002
//---------------------------------------

#include <Win/Win.h>

namespace Win
{
	class Point;
}
class FormattedText;

class PopupDisplay
{
public:
	PopupDisplay ()
	{}
	PopupDisplay (Win::Dow::Handle appWin)
		: _appWin (appWin)
	{}

	void Init (Win::Dow::Handle appWin) { _appWin = appWin; }
	void Show (FormattedText const & text,
			   Win::Point & anchorPoint,
			   int maxWidth,
			   int maxHeigth);

private:
	Win::Dow::Handle		_appWin;	// Window over which popup window is displayed
	Win::Dow::Handle		_frame;		// Frame window controlling display pane
};

#endif
