//------------------------------------
//  (c) Reliable Software, 2005 - 2006
//------------------------------------

#include <WinLibBase.h>
#include "Focus.h"

#include <dbg/Assert.h>

void Focus::Ring::ClearIds ()
{
	_ids.clear ();
	_focusId = invalidId;
}

void Focus::Ring::AddId (unsigned id)
{
	_ids.push_back (id);
	if (_focusId == invalidId)
		_focusId = id;
}

void Focus::Ring::RemoveId (unsigned id)
{
	Assert (!_ids.empty ());
	IdIter iter = std::find (_ids.begin (), _ids.end (), id);
	if (iter == _ids.end ())
		return;
	else if (_focusId == id)
		SwitchToNext ();

	_ids.erase (iter);
}

bool Focus::Ring::HasId (unsigned id) const
{
	ConstIdIter iter = std::find (_ids.begin (), _ids.end (), id);
	return iter != _ids.end ();
}

void Focus::Ring::SwitchToNext ()
{
	Assert (!_ids.empty ());
	if (_ids.size () == 1)
		return;

	ConstIdIter iter = std::find (_ids.begin (), _ids.end (), _focusId);
	Assert (iter != _ids.end ());
	++iter;
	if (iter == _ids.end ())
		_focusId = _ids.front ();
	else
		_focusId = *iter;
	Assert (_sink != 0);
	_sink->OnFocusSwitch ();
}

void Focus::Ring::SwitchToPrevious ()
{
	Assert (!_ids.empty ());
	if (_ids.size () == 1)
		return;

	ConstIdIter iter = std::find (_ids.begin (), _ids.end (), _focusId);
	Assert (iter != _ids.end ());
	if (iter == _ids.begin ())
	{
		_focusId = _ids.back ();
	}
	else
	{
		--iter;
		_focusId = *iter;
	}
	Assert (_sink != 0);
	_sink->OnFocusSwitch ();
}

void Focus::Ring::SwitchToThis (unsigned id)
{
	Assert (!_ids.empty ());
	Assert (std::find (_ids.begin (), _ids.end (), id) != _ids.end ());
	if (_focusId == id)
		return;

	_focusId = id;
	Assert (_sink != 0);
	_sink->OnFocusSwitch ();
}

void Focus::Ring::HideThis (unsigned id)
{
	if (_focusId == id)
		SwitchToNext ();

	Assert (_sink != 0);
	_sink->OnClose (id);
	_sink->OnFocusSwitch ();
}