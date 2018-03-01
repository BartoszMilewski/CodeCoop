//------------------------------------
//  (c) Reliable Software, 1999 - 2007
//------------------------------------

#include "precompiled.h"
#include "Messengers.h"
#include "Global.h"

#include <Win/Message.h>
#include <Ctrl/Messages.h>

void StateChangeNotify::DeliverMessage (Win::Dow::Handle targetWindow)
{
	Win::RegisteredMessage msg (UM_COOP_PROJECT_STATE_CHANGE);
	msg.SetWParam (_projectId);
	msg.SetLParam (_isNewMissing? 1: 0);
	targetWindow.PostMsg (msg);
}

void BackupNotify::DeliverMessage (Win::Dow::Handle targetWindow)
{
	if (_sourceWin == targetWindow)
		return;

	Win::RegisteredMessage msg (UM_COOP_BACKUP);
	targetWindow.PostMsg (msg);
	_notification = true;
}

void CoopDetector::DeliverMessage (Win::Dow::Handle targetWindow)
{
	if (_sourceWin == targetWindow)
		return;

	_otherCoopInstance = true;
}
