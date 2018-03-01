#if !defined (SOLUTIONVIEW_H)
#define SOLUTIONVIEW_H
//----------------------------
// (c) Reliable Software, 2004
//----------------------------
#include <Win/Win.h>

class View
{
public:
	View (Win::Dow::Handle win);
	void Size (int width, int height) throw ();
private:
	Win::Dow::Handle	_win;
	int _xPix;
	int _yPix;
};

#endif
