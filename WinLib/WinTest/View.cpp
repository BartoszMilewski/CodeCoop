//----------------------------
// (c) Reliable Software, 2004
//----------------------------
#include "precompiled.h"
#include "View.h"

View::View (Win::Dow::Handle win)
	: _win (win),
	  _xPix (0),
	  _yPix (0)
{
}

void View::Size (int width, int height) throw ()
{
	_xPix = width;
	_yPix = height;
}
