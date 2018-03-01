//------------------------------------------------
// HelpEngine.cpp
// (c) Reliable Software 2002
// -----------------------------------------------

#include "precompiled.h"
#include "HelpEngine.h"
#include "OutputSink.h"
#include "resource.h"

HelpEngine::Item const HelpEngine::_items [] =
{
	{ IDD_JOIN_PROJECT, "Join project help"
	},
	{ -1, "" }
};

bool HelpEngine::OnDialogHelp (int dlgId) throw ()
{
	for (int i = 0; _items [i]._id != -1; ++i)
	{
		if (_items [i]._id == dlgId)
		{
			TheOutput.Display (_items [i]._info);
			return true;
		}
	}
	return false;
}

