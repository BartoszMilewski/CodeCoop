//------------------------------------
//  (c) Reliable Software, 2005 - 2008
//------------------------------------

#include "precompiled.h"
#include "PreferencesStorage.h"

#include <Win/Metrics.h>
#include <Win/Geom.h>
#include <Graph/Canvas.h>
#include <Graph/Font.h>

#include <StringOp.h>
#include <NamedBool.h>

void Preferences::Storage::Sequencer::GetAsNamedBool (NamedBool & options) const
{
	MultiString mString;
	GetMultiString (mString);
	std::copy (mString.begin (), mString.end (), std::inserter (options, options.end ()));
}

Preferences::Storage::Storage (std::string const & regKeyPath)
	: _keyPath (regKeyPath),
	  _cxChar (0),
	  _cyChar (0),
	  _cyCaption (0)
{
	RememberSystemMetrics ();
}

Preferences::Storage::Storage (Preferences::Storage const & storage, std::string const & keyName)
	: _keyPath (storage._keyPath.GetFilePath (keyName)),
	  _cxChar (0),
	  _cyChar (0),
	  _cyCaption (0)
{
	RememberSystemMetrics ();
}

bool Preferences::Storage::PathExists () const
{
	RegKey::Check key (_root, _keyPath.GetDir ());
	return key.Exists ();
}

void Preferences::Storage::CreatePath ()
{
	RegKey::New key (_root, _keyPath.GetDir ());
}

bool Preferences::Storage::IsValuePresent (std::string const & valueName) const
{
	for (Preferences::Storage::Sequencer seq (*this); ! seq.AtEnd (); seq.Advance ())
	{
		if (seq.GetValueName () == valueName)
			return true;
	}
	return false;
}

void Preferences::Storage::Read (std::string const & valueName, unsigned long & value) const
{
	RegKey::Existing key (_root, _keyPath.GetDir ());
	key.GetValueLong (valueName, value);
}

void Preferences::Storage::Read (std::string const & valueName, std::string & value) const
{
	RegKey::Existing key (_root, _keyPath.GetDir ());
	value = key.GetStringVal (valueName);
}

void Preferences::Storage::Read (std::string const & valueName, MultiString & value) const
{
	RegKey::Existing key (_root, _keyPath.GetDir ());
	key.GetMultiString (valueName, value);
}

void Preferences::Storage::Read (Win::Placement & placement) const
{
	RegKey::Existing key (_root, _keyPath.GetDir ());
	RegKey::ReadWinPlacement (placement, key);
}

void Preferences::Storage::Read (std::string const & valueName, NamedBool & options) const
{
	MultiString mString;
	Read (valueName, mString);
	std::copy (mString.begin (), mString.end (), std::inserter (options, options.end ()));
}

void Preferences::Storage::Save (std::string const & valueName, unsigned long value) throw ()
{
	RegKey::CheckWriteable key (_root, _keyPath.GetDir ());
	if (key.Exists ())
		key.SetValueLong (valueName, value, true);	// Quiet
}

void Preferences::Storage::Save (std::string const & valueName, std::string const & value) throw ()
{
	RegKey::CheckWriteable key (_root, _keyPath.GetDir ());
	if (key.Exists ())
		key.SetValueString (valueName, value, true);	// Quiet
}

void Preferences::Storage::Save (std::string const & valueName, MultiString const & value) throw ()
{
	RegKey::CheckWriteable key (_root, _keyPath.GetDir ());
	if (key.Exists ())
		key.SetValueMultiString (valueName, value, true);	// Quiet
}

void Preferences::Storage::Save (Win::Placement const & placement) throw ()
{
	RegKey::CheckWriteable key (_root, _keyPath.GetDir ());
	if (key.Exists ())
		RegKey::SaveWinPlacement (placement, key);
}

void Preferences::Storage::Save (std::string const & valueName, NamedBool const & options) throw ()
{
	MultiString mString;
	std::copy (options.begin (), options.end (), std::back_inserter (mString));
	Save (valueName, mString);
}

void Preferences::Storage::RememberSystemMetrics ()
{
	NonClientMetrics systemMetrics;
	Assert (systemMetrics.IsValid ());
	_cyCaption = systemMetrics.GetCaptionHeight ();
	Win::DesktopCanvas myCanvas;
	Font::Maker myFontMaker (systemMetrics.GetMessageFont ());
	Font::Holder myFont (myCanvas, myFontMaker.Create ());
	myFont.GetAveCharSize (_cxChar, _cyChar);
}

Preferences::TopWinPlacement::TopWinPlacement (Win::Dow::Handle win, std::string const & path)
	: Preferences::Storage (path),
	  _topWin (win)
{
	if (PathExists ())
		Read (_placement);
	else
		CreatePath ();

	Verify ();
}

Preferences::TopWinPlacement::~TopWinPlacement ()
{
	Win::Placement placement (_topWin);
	Save (placement);
}

void Preferences::TopWinPlacement::Verify ()
{
	Win::ClientRect screenRect (_topWin.GetParent ());
	Win::Rect myRect;
	_placement.GetRect (myRect);
	Win::Rect visibleRect;
	screenRect.Intersect (myRect, visibleRect);
	// When my window is not visible set default position and size
	if (visibleRect.IsEmpty ())
		SetDefaultPositionAndSize (screenRect);

	// When my window is bigger then the screen set default position and size 
	if (myRect.Width () > screenRect.Width () || myRect.Width () < TopWindowMinWidth
		|| myRect.Height () > screenRect.Height () || myRect.Height () < TopWindowMinHeight)
		SetDefaultPositionAndSize (screenRect);

	// When my window bottom right corner is close to the screen bottom right corner
	// then move my window to the screen upper left corner
	if (myRect.Right () >= screenRect.Right () || myRect.Bottom () >= screenRect.Bottom ())
	{
		// Too close to the screen bottom right corner
		int left = screenRect.Left ();
		int top = screenRect.Top ();
		int right = left + myRect.Width ();
		int bottom = top + myRect.Height ();
		Win::Rect myNewRect (left, top, right, bottom);
		_placement.SetRect (myNewRect);
	}

	if (_placement.IsHidden () || _placement.IsMinimized ())
		_placement.SetNormal ();
}

void Preferences::TopWinPlacement::PlaceWindow (Win::ShowCmd cmdShow, bool multipleWindows)
{
	_placement.CombineShowCmd (cmdShow);
	if (multipleWindows)
	{
		// Offset subsequent window by caption height
		Win::Rect myRect;
		_placement.GetRect (myRect);
		myRect.ShiftBy (_cyCaption, _cyCaption);
		_placement.SetRect (myRect);
		Verify ();
		Save (_placement);
	}
	_topWin.SetPlacement (_placement);
}

void Preferences::TopWinPlacement::Update ()
{
	Win::Placement currentPlacement (_topWin);
	_placement = currentPlacement;
}

void Preferences::TopWinPlacement::SetDefaultPositionAndSize (Win::ClientRect const & screenRect)
{
	// Position my window near the upper left screen corner.
	int left = screenRect.Left () + _cyCaption;
	int top = screenRect.Top () + _cyCaption;
	// Set my window dimensions based on the system font
	int right = left + 140 * _cxChar;
	int bottom = top + 3 * screenRect.Height () / 4;
	Win::Rect myNewRect (left, top, right, bottom);
	_placement.SetRect (myNewRect);
	_placement.SetNormal ();
}
