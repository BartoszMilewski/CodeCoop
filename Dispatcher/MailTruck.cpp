//-----------------------------------
// (c) Reliable Software 1998 -- 2003
//-----------------------------------
#include "precompiled.h"
#include "MailTruck.h"
#include "ScriptInfo.h"
#include "ConfigData.h"
#include "WorkQueue.h"
#include "FolderMan.h"
#include "Dialogs.h"
#include "AppInfo.h"
#include "ConfigExpt.h"
#include "LocalProjects.h"

#include <File/Path.h>


MailTruck::MailTruck (ConfigData const & config, LocalProjects & localProjects)
    : _topology (config.GetTopology ()),
	  _pathToPublicInbox (config.GetPublicInboxPath ()),
      _hubTransport (config.GetActiveTransportToHub ()),
	  _hubEmail (config.GetHubId ()),
	  _localProjects (localProjects)
{}

ScriptTicket & MailTruck::PutScript (std::unique_ptr<ScriptTicket> script)
{
	Assert (script->HasHeader ());
	_scriptTicketList.push_back(std::move(script));
	return *_scriptTicketList.back();
}

void MailTruck::RemoveScript(ScriptTicket const & script)
{
#if 0
	auto it = std::find_if(
		_scriptTicketList.begin(), 
		_scriptTicketList.end(), 
		[scriptInfo](ScriptTicket & tck) { return script == tck; });
#endif
	ScriptVector::iterator it = find_if(
		_scriptTicketList.begin(), 
		_scriptTicketList.end(), 
		IsEqualScript(script));

	Assert (it != _scriptTicketList.end());
	_scriptTicketList.erase(it);
}

void MailTruck::CopyLocal (ScriptTicket & script,
						   int addresseeIdx,
						   int projectId,
						   FilePath const & destPath)
{
	bool driveNotReady = false; // required, not used here
	if (FolderMan::CopyMaterialize (
					script.GetPath (),
					destPath,
					script.GetName (), driveNotReady))
	{
		_localProjects.MarkNewIncomingScripts (projectId);
		script.StampDelivery (addresseeIdx);
	}
}

bool MailTruck::GetDoneScripts (ScriptVector & doneList)
{
	// some scripts may already be done
	unsigned len = _scriptTicketList.size();
	for (unsigned i = 0; i < len; ++i)
	{
		ScriptTicket * script = _scriptTicketList[i];
		if (script != 0 && !script->AreRequestsPending ())
		{
			doneList.push_back (_scriptTicketList.extract(i));
		}
	}
	return doneList.size () != 0;
}

void MailTruck::Distribute (WorkQueue & workQueue)
{
	if (_scriptTicketList.size() != 0)
		workQueue.TransferRequests (_scriptTicketList);
}
