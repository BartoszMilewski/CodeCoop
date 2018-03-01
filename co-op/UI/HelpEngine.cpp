//----------------------------------
// (c) Reliable Software 2002 - 2006
// ---------------------------------

#include "precompiled.h"
#include "HelpEngine.h"
#include "OutputSink.h"
#include "resource.h"

HelpEngine::Item const HelpEngine::_items [] =
{
	{ IDD_CHECKIN, "Type check-in comment in the edit box.  If one of your scripts has recently been rejected,\n"
				   "you can retrieve its check-in comment by checking 'Use comment from my last rejected script'.\n"
				   "You can keep files checked out after this check-in.  If you always want to keep files checked out\n"
				   "then go to project properties (Project>Properties) and check 'After every check-in keep files checked out'."
	},
	{ IDD_SCRIPT_CONFLICT, "You have received scripts out of order and Code Co-op has to undo changes carried by the scripts\n"
						   "received out of order. Code Co-op will move the out of order scripts to the new branch in the history.\n\n"
						   "After executing the incoming script you can merge changes from the branch to the history trunk.\n\n"
						   "If you postpone the incoming script execution you will be able to continue developemnt in the new branch.\n"
						   "All your subsequent check-ins will be added to the branch and will require a merge when the incoming\n"
						   "script is executed."
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

