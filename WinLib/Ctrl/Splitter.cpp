//----------------------------------
// Reliable Software (c) 1998 - 2006
//----------------------------------

#include <WinLibBase.h>

#include "Splitter.h"
#include "Messages.h"

#include <Win/Message.h>
#include <Win/Utility.h>
#include <Win/WinMaker.h>
#include <Win/WinClass.h>
#include <Graph/Canvas.h>
#include <Graph/CanvTools.h>
#include <Graph/Font.h>

//
// Any splitter
//

bool Splitter::Ctrl::OnCreate (Win::CreateData const * create, bool & success) throw ()
{
	_hParent = create->GetParent ();
	success = true;
	return true;
}

bool Splitter::Ctrl::OnLButtonDown (int x, int y, Win::KeyState kState) throw ()
{
	_parentStyle = _hParent.GetStyle ();
	Win::Style tmpStyle (_parentStyle);
	tmpStyle.DontClipChildren ();
	_hParent.SetStyle (tmpStyle);

	_h.CaptureMouse ();
	_dragging = true;

	Win::Point parentOrigin;
	_hParent.ClientToScreen (parentOrigin);
	Win::Point splitterOrigin;
	_h.ClientToScreen (splitterOrigin);
	BeginDrag (parentOrigin, splitterOrigin, x, y);

	Win::UpdateCanvas canvas (_hParent);
	Win::ForegrMode mode (canvas, R2_NOTXORPEN);
	DrawDivider (canvas);

	return true;
}

bool Splitter::Ctrl::OnLButtonUp (int x, int y, Win::KeyState kState) throw ()
{
	// Calling ReleaseCapture will send us the WM_CAPTURECHANGED
	_h.ReleaseMouse ();
	_dragging = false;
	NotifyParent (x, y);
	return true;
}

bool Splitter::Ctrl::OnMouseMove (int x, int y, Win::KeyState kState) throw ()
{
	if (_dragging)
	{
		Win::UpdateCanvas canvas (_hParent);
		Win::ForegrMode mode (canvas, R2_NOTXORPEN);
		// Erase previous divider (drawing over old one will erase it)
		DrawDivider (canvas);
		UpdateDragPosition (x, y);
		// Draw new one
		DrawDivider (canvas);
	}
	return true;
}

bool Splitter::Ctrl::OnCaptureChanged (Win::Dow::Handle newCaptureWin) throw ()
{
	// We are losing capture
	// End drag selection -- for whatever reason
	Win::UpdateCanvas canvas (_hParent);
	Win::ForegrMode mode (canvas, R2_NOTXORPEN);
	// Erase divider (drawing over old one will erase it)
	DrawDivider (canvas);
	_hParent.SetStyle (_parentStyle);
	_dragging = false;
	return true;
}

//
// Vertical splitter
//

bool Splitter::VerticalCtrl::OnPaint () throw ()
{
	Win::PaintCanvas canvas (_h);
	{
		Pen::Holder pen (canvas, _pens.Light ());
		canvas.Line (0, 0, 0, _cy - 1);
	}
	{
		Pen::Holder pen (canvas, _pens.Hilight ());
		canvas.Line (1, 0, 1, _cy - 1);
	}
	{
		Pen::Holder pen (canvas, _pens.Shadow ());
		canvas.Line (_cx - 2, 0, _cx - 2, _cy - 1);
	}
	{
		Pen::Holder pen (canvas, _pens.DkShadow ());
		canvas.Line (_cx - 1, 0, _cx - 1, _cy - 1);
	}
	return true;
}

void Splitter::VerticalCtrl::BeginDrag (Win::Point const & parentOrigin,
										Win::Point const & splitterOrigin,
										int xMouse,
										int yMouse)
{
	_h.Hide ();
	// Find x offset of splitter
	// with respect to parent client area
	_dragStart = splitterOrigin.x - parentOrigin.x + _cx / 2 - xMouse;
	_dragX = _dragStart + xMouse;
	_dragY = splitterOrigin.y - parentOrigin.y;
}

void Splitter::VerticalCtrl::UpdateDragPosition (int xMouse, int yMouse)
{
	_dragX = _dragStart + xMouse;
}

void Splitter::VerticalCtrl::DrawDivider (Win::UpdateCanvas & canvas)
{
	canvas.Line (_dragX, _dragY, _dragX, _dragY + _cy - 1);
}

void Splitter::VerticalCtrl::NotifyParent (int xMouse, int yMouse)
{
	Win::UserMessage msg (UM_VSPLITTERMOVE, _splitterId, _dragStart + xMouse);
	_hParent.SendMsg (msg);
	_h.Show ();
}

//
// Horizontal splitter
//

bool Splitter::HorizontalCtrl::OnPaint () throw ()
{
	Win::PaintCanvas canvas (_h);
	{
		Pen::Holder pen (canvas, _pens.Light ());
		canvas.Line (0, 0, _cx - 1, 0);
	}
	{
		Pen::Holder pen (canvas, _pens.Hilight ());
		canvas.Line (0, 1, _cx - 1, 1);
	}
	{
		Pen::Holder pen (canvas, _pens.Shadow ());
		canvas.Line (0, _cy - 2, _cx - 1, _cy - 2);
	}
	{
		Pen::Holder pen (canvas, _pens.DkShadow ());
		canvas.Line (0, _cy - 1, _cx - 1, _cy - 1);
	}
	return true;
}

void Splitter::HorizontalCtrl::BeginDrag (Win::Point const & parentOrigin,
										  Win::Point const & splitterOrigin,
										  int xMouse,
										  int yMouse)
{
	_h.Hide ();
	// Find y offset of splitter
	// with respect to parent client area
	_dragStart = splitterOrigin.y - parentOrigin.y + _cy / 2 - yMouse;
	_dragX = splitterOrigin.x - parentOrigin.x;
	_dragY = _dragStart + yMouse;
}

void Splitter::HorizontalCtrl::UpdateDragPosition (int xMouse, int yMouse)
{
	_dragY = _dragStart + yMouse;
}

void Splitter::HorizontalCtrl::DrawDivider (Win::UpdateCanvas & canvas)
{
	canvas.Line (_dragX, _dragY, _dragX + _cx - 1, _dragY);
}

void Splitter::HorizontalCtrl::NotifyParent (int xMouse, int yMouse)
{
	Win::UserMessage msg (UM_HSPLITTERMOVE, _splitterId, _dragStart + yMouse);
	_hParent.SendMsg (msg);
	_h.Show ();
}

char const * Splitter::UseVertical::_className = "RsVerticalSplitterClass";

Splitter::UseVertical::UseVertical (Win::Instance hInstance)
{
	Win::Class::Maker splitterClass (_className, hInstance);
	splitterClass.Style () << Win::Class::Style::RedrawOnSize;
	splitterClass.SetSysCursor (IDC_SIZEWE);
	splitterClass.SetBgSysColor (COLOR_3DFACE);
	splitterClass.Register ();
}

Splitter::UseVertical::~UseVertical ()
{
	for (auto_vector<VerticalCtrl>::const_iterator iter = _ctrls.begin ();
		 iter != _ctrls.end ();
		 ++iter)
	{
		VerticalCtrl const * ctrl = *iter;
		Win::Dow::Handle splitterWin = ctrl->GetWindow ();
		splitterWin.Destroy ();
	}
}

Win::Dow::Handle Splitter::UseVertical::MakeSplitter (Win::Dow::Handle hwndParent,
													  unsigned splitterId)
{
	std::unique_ptr<VerticalCtrl> newCtrl (new VerticalCtrl (splitterId));
	Win::ChildMaker splitterMaker (_className, hwndParent, splitterId);
	Win::Dow::Handle hwndSplitter = splitterMaker.Create (newCtrl.get ());
	_ctrls.push_back (std::move(newCtrl));
	hwndSplitter.Show ();
	return hwndSplitter;
}

char const * Splitter::UseHorizontal::_className = "RsHorizontalSplitterClass";

Splitter::UseHorizontal::UseHorizontal (Win::Instance hInstance)
{
	Win::Class::Maker splitterClass (_className, hInstance);
	splitterClass.Style () << Win::Class::Style::RedrawOnSize;
	splitterClass.SetSysCursor (IDC_SIZENS);
	splitterClass.SetBgSysColor (COLOR_3DFACE);
	splitterClass.Register ();
}

Splitter::UseHorizontal::~UseHorizontal ()
{
	for (auto_vector<HorizontalCtrl>::const_iterator iter = _ctrls.begin ();
		 iter != _ctrls.end ();
		 ++iter)
	{
		HorizontalCtrl const * ctrl = *iter;
		Win::Dow::Handle splitterWin = ctrl->GetWindow ();
		splitterWin.Destroy ();
	}
}

Win::Dow::Handle Splitter::UseHorizontal::MakeSplitter (Win::Dow::Handle hwndParent,
													    unsigned splitterId)
{
	std::unique_ptr<HorizontalCtrl> newCtrl (new HorizontalCtrl (splitterId));
	Win::ChildMaker splitterMaker (_className, hwndParent, splitterId);
	Win::Dow::Handle hwndSplitter = splitterMaker.Create (newCtrl.get ());
	_ctrls.push_back (std::move(newCtrl));
	hwndSplitter.Show ();
	return hwndSplitter;
}
