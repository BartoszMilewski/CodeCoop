//----------------------------------
// Reliable Software (c) 2005 - 2006
//----------------------------------

#include <WinLibBase.h>
#include "FocusBarWin.h"
#include "Focus.h"

#include <Win/Message.h>
#include <Win/Utility.h>
#include <Win/WinMaker.h>
#include <Win/WinClass.h>
#include <Win/Metrics.h>
#include <Ctrl/Button.h>
#include <Graph/Canvas.h>
#include <Graph/CanvTools.h>
#include <Graph/Font.h>
#include <Graph/Color.h>

bool FocusBar::Ctrl::OnCreate (Win::CreateData const * create, bool & success) throw ()
{
	UseSystemSettings ();
	Win::ButtonMaker buttonMaker (_h, CLOSE_BUTTON_ID);
	buttonMaker.Style () << Win::Button::Style::Flat
						 << Win::Button::Style::VCenter
						 << Win::Button::Style::HCenter;
	_closeButton = buttonMaker.Create ();
	// Revisit: poor man close button
	_closeButton.SetText ("X");
	_closeButton.Show ();
	success = true;
	return true;
}

bool FocusBar::Ctrl::OnSize (int width, int height, int flag) throw ()
{
	_cx = width;
	_cy = height;
	int size = _fontHeight + 4;
	_closeButton.Move (_cx - size - 4, (_cy - size) / 2, size, size);
	return true;
}

int FocusBar::Ctrl::GetHeight () const
{
	return _fontHeight + 4 + 4;
}

bool FocusBar::Ctrl::OnMouseActivate (Win::HitTest hitTest, Win::MouseActiveAction & activate) throw ()
{
	_focusRing.SwitchToThis (_associatedId);
	activate.SetNoActivate ();	// We are not activating this window
	return false;				// Pass this message to the parent window
}

bool FocusBar::Ctrl::OnPaint () throw ()
{
	Win::PaintCanvas canvas (_h);
	canvas.EraseBackground (_isOn ? Win::Color3d::Shadow () :
									Win::Color3d::Face ());
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
	if (_title.size () != 0)
	{
		Font::Holder useFont (canvas, _font);
		Win::TransparentBkgr bkg (canvas);
		int y = (_cy - _fontHeight) / 2;
		canvas.Text (10, y, _title.c_str (), _title.size ());
	}
	return true;
}

bool FocusBar::Ctrl::OnSettingChange (Win::SystemWideFlags flags, char const * sectionName) throw ()
{
	if (flags.IsNonClientMetrics ())
		UseSystemSettings ();
	return true;
}

bool FocusBar::Ctrl::OnControl (unsigned id, unsigned notifyCode) throw ()
{
	if (id == CLOSE_BUTTON_ID)
	{
		_focusRing.HideThis (_associatedId);
	}
	return true;
}

void FocusBar::Ctrl::TurnOn ()
{
	_isOn = true;
	_h.Invalidate ();
}

void FocusBar::Ctrl::TurnOff ()
{
	_isOn = false;
	_h.Invalidate ();
}

void FocusBar::Ctrl::UseSystemSettings ()
{
	NonClientMetrics metrics;
	if (metrics.IsValid ())
	{
		Font::Descriptor const & fontDesc = metrics.GetStatusFont ();
		Font::Maker fontMaker (fontDesc);
		_font = fontMaker.Create ();
		// Calculate font height
		Win::UpdateCanvas canvas (_h);
		Font::Holder useFont (canvas, _font);
		int width;
		useFont.GetAveCharSize (width, _fontHeight);
	}
}

char const * FocusBar::Use::_className = "FocusBarClass";

FocusBar::Use::Use (Win::Instance hInstance)
{
	Win::Class::Maker focusClass (_className, hInstance);
	focusClass.Style () << Win::Class::Style::RedrawOnSize;
	focusClass.Register ();
}

FocusBar::Use::~Use ()
{
	while (_ctrls.size () > 0)
	{
		std::unique_ptr<FocusBar::Ctrl> ctrl (_ctrls.pop_back ());
		Win::Dow::Handle win = ctrl->GetWindow ();
		win.Destroy ();
	}
}

FocusBar::Ctrl * FocusBar::Use::MakeBarWindow (Win::Dow::Handle hwndParent,
											   unsigned id,
											   unsigned associatedId,
											   Focus::Ring & focusRing)
{
	std::unique_ptr<FocusBar::Ctrl> newCtrl (new FocusBar::Ctrl (id, associatedId, focusRing));
	Win::ChildMaker focusMaker (_className, hwndParent, id);
	Win::Dow::Handle win = focusMaker.Create (newCtrl.get ());
	FocusBar::Ctrl * ctrl = newCtrl.get ();
	_ctrls.push_back (std::move(newCtrl));
	win.Show ();
	return ctrl;
}
