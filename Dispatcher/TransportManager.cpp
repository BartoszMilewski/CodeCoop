//----------------------------------
//  (c) Reliable Software, 2009
//----------------------------------
#include "precompiled.h"
#undef DBG_LOGGING
#define DBG_LOGGING true
#include "TransportManager.h"
#include "ScriptInfo.h"
#include "AlertMan.h"
#include "FolderMan.h"
#include "TransportHeader.h"
#include "ConfigData.h"
#include "EmailMan.h"
#include "Processor.h"
#include "ScriptManager.h"
#include "Email.h"
#include "EmailMessage.h"
#include "Registry.h"
#include "AppHelp.h"
#include "ScriptSubject.h"
#include "BadEmailExpt.h"
#include "DispatcherMsg.h"
#include "FeedbackMan.h"
#include "ScriptFileList.h"
#include "FileLocker.h"

#include <File/Path.h>
#include <File/SafePaths.h>
#include <Com/Shell.h>
#include <Net/Socket.h>
#include <Mail/Pop3.h>

TransportManager::TransportManager(Win::Dow::Handle winParent, ConfigData const & config, ChannelSync & sync)
	: _winParent (winParent),
	_topology(config.GetTopology ()),
	_hubTransport(config.GetActiveTransportToHub ()),
	_isVerbose (false),
	_scriptFileList(0),
	_publicInboxPath (config.GetPublicInboxPath ()),
	_goesToHub(_topology.HasHub() && !_topology.HasSat()), // scripts go to hub
	_copiers(_goesToHub, sync),
	_ignoreEmailErrors(false)
{}

void TransportManager::PostForwardRequest(
	ScriptManager & scriptMan,
	ScriptHandle handle,
	Transport const & transport,
	IgnoreAction action)
{
	// Queue script for copying to that location
	if (transport.IsNetwork ())
	{
		ActiveLanCopier * copier = _copiers.GetLanCopier(transport);
		Assume(copier != 0, "Internal error: cannot create a copier");

		copier->AddRequest(
			handle, 
			_isVerbose, 
			action == ignoreRetry);
	}
	else
	{
		throw Win::InternalException("Unknown transport for script", transport.GetRoute().c_str());
	}
	// other transports in the future
}

void TransportManager::PostEmailRequest(
	ScriptManager & scriptMan,
	ScriptHandle handle,
	std::vector<std::string> const & addrVector)
{
	ActiveEmailCopier * copier = _copiers.GetEmailCopier();
	Assume(copier != 0, "Internal error: cannot create a copier");
	copier->AddRequest(
		handle,
		addrVector,
		_isVerbose);
}

void TransportManager::OnLanCopyDone(LanCopierReadyMessage & msg, ScriptManager & scriptMan)
{
	ActiveLanCopier * copier = msg.GetCopier();
	dbg << "TransportManager::OnLanCopyDone: " << copier->GetTransport() << std::endl;
	Assume(copier != 0, "OnLanCopyDone called with null copier");

	ScriptHandle h = msg.GetScriptHandle();
	copier->RequestDone(msg.GetScriptHandle(), msg.IsSuccess());
	ScriptTicket * script = _scriptsInProgress.Get(h);
	if (script == 0)
		return;

	dbg << "    " << script->GetName() << std::endl;	
	
	Transport const & transport = copier->GetTransport();

	bool transportFailed = true;
	bool progressMade = false;
	ScriptStatus::Dispatch::Bits status;

	// second part
	if (msg.IsSuccess())
	{
		progressMade = true;
		transportFailed = false;
		UnIgnorePath (transport);
		dbg << "    NETWORK COPY: done!" << std::endl;
	}
	else // failure
	{
		dbg << "    NETWORK COPY: failed!" << std::endl;

		if (msg.IsDriveNotReady())
		{
			status = ScriptStatus::Dispatch::NoDisk;
			Ignore(transport, ignoreDisk);
			FullPathSeq dest (transport.GetRoute ().c_str ());
			// treat floppy same as network
			Ignore(Transport(dest.GetHead(), Transport::Network), 
							ignoreDisk);
		}
		else
		{
			status = ScriptStatus::Dispatch::NoNetwork;
			IgnorePath(transport);
		}
		_scriptFileList->SetUiStatus(script->GetName(), status);
	}


	// Mark requests with this transport "done"
	// It will decrement the count of forwards for this script
	script->MarkRequestDone (transport, !transportFailed);
	Assert (script->RequestsTally ());

	if (progressMade)
		ScriptProgressMade (scriptMan, *script);

	if (copier->IsClogged())
	{
		copier->UpdateStatuses(_scriptsInProgress, _scriptFileList, status);
	}
	else if (!copier->CopyNextScript(_scriptsInProgress))
	{
		_copiers.RemoveTransport(transport);
	}
}

void TransportManager::OnEmailCopyDone(EmailCopierReadyMessage & msg, ScriptManager & scriptMan)
{
	dbg << "TransportManager::OnEmailCopyDone: " << std::endl;
	if (msg.IsGeneralFailure())
		TheEmail.ShutDown ();
	else
		_ignoreEmailErrors = false;

	ActiveEmailCopier * copier = msg.GetCopier();
	Assume(copier != 0, "OnEmailCopyDone called with null copier");
	bool isFirstError = true;
	for (EmailInfoList::const_iterator it = msg.begin(); it != msg.end(); ++it)
	{
		EmailRequest const & request = it->_request;
		ScriptHandle h = request._scriptHandle;
		std::vector<std::string> const & addrVector = request._addrVector;

		copier->RequestDone(*it);
		ScriptTicket * script = _scriptsInProgress.Get(h);
		if (script == 0)
			continue;

		dbg << "    " << script->GetName() << std::endl;

		if (msg.IsGeneralFailure())
		{
			_scriptFileList->SetUiStatus(script->GetName(), 
				msg.IsTimedOut()? ScriptStatus::Dispatch::ServerTimeout : ScriptStatus::Dispatch::NoEmail);
			if (isFirstError)
			{
				dbg << "    EMAIL: Cannot start mailer" << std::endl;
				TheAlertMan.PostInfoAlert ("Cannot connect to mail server", !_ignoreEmailErrors);
				_ignoreEmailErrors = true;
				isFirstError = false;
			}
		}
		else if (it->_success)
		{
			dbg << "    EMAIL: done! " << script->GetName() << std::endl;
		}
		else
		{
			dbg << "    EMAIL: failed! " << script->GetName() << std::endl;
			script->SetEmailError();
			_scriptFileList->SetUiStatus(script->GetName(), ScriptStatus::Dispatch::NoEmail);
			if (!it->_badAddress.empty())
			{
				dbg << "    EMAIL: failed on bad address! " << script->GetName() << std::endl;
				TheEmail.InsertBlacklisted (it->_badAddress);
				if (isFirstError)
				{
					isFirstError = false;
					// Revisit: use PostQuarantineAlert?
					std::string msg = "Bad e-mail address: ";
					msg += it->_badAddress;
					TheAlertMan.PostInfoAlert (msg);
				}
			}
		}
		
		// Mark requests with these addresses "done"
		// It will decrement the count of emails for this script
		script->MarkEmailReqDone(addrVector, it->_success);
		Assert (script->RequestsTally ());

		ScriptProgressMade (scriptMan, *script);
	}

	if (copier->HasWork())
	{
		if (!copier->IsClogged())
		{
			if (!copier->CopyScripts(_scriptsInProgress))
				_copiers.RemoveEmail();
		}
		else
		{
			// all scripts in a clogged copier have the same status
			copier->UpdateStatuses(_scriptsInProgress, _scriptFileList, ScriptStatus::Dispatch::NoEmail);
		}
	}
	else
		_copiers.RemoveEmail();
}

void TransportManager::StampFwdFlag (std::string const & scriptPath, bool isFwd)
{
	MemMappedHeader tmpScript (scriptPath.c_str ());
	tmpScript.StampForwardFlag (isFwd);
}

void TransportManager::MapEmailsInScripts(std::vector<ScriptHandle> & toDoList, ScriptToEmails & scriptList)
{
	for (std::vector<ScriptHandle>::iterator it = toDoList.begin(); it != toDoList.end(); ++it)
	{
		ScriptHandle scriptHandle = *it;
		ScriptTicket * script = _scriptsInProgress.Get(scriptHandle);
		if (script == 0)
			continue;
		for (ScriptTicket::RequestIter ri = script->beginReq(); ri != script->endReq(); ++ri)
		{
			DispatchRequest const & req = ri->second;
			if (!req.WasTried() && !req.IsForward())
			{
				// email request
				std::string const & emailAddr = script->GetEmail(ri);
				scriptList[scriptHandle].insert(emailAddr);
			}
		}
	}
}

void TransportManager::StartEmailing(std::vector<ScriptHandle> & toDoList, ScriptManager & scriptMan)
{
	ScriptToEmails scriptToEmails;
	ScriptToEmails::iterator scriptToEmailsIter;
	MapEmailsInScripts(toDoList, scriptToEmails);
	if (scriptToEmails.size () == 0)
		return;

	dbg << "Post Email Requests" << std::endl;
	
	ActivityIndicator sending (TheFeedbackMan, SendingEmail);
	bool isFirstError = true; // display one error per loop
	for (scriptToEmailsIter = scriptToEmails.begin ();
		 scriptToEmailsIter != scriptToEmails.end ();
		 ) // 'scriptToEmailsIter' is advanced during erase or explicitly at the end of the loop
	{
		ScriptHandle handle = scriptToEmailsIter->first;
		ScriptTicket * script = _scriptsInProgress.Get(handle);
		Assume(script != 0, "Can't find script ticket to be emailed");
		if (!File::Exists (script->GetPath ()))
		{
			dbg << "= File doesn't exist. removing from list: " << script->GetName() << std::endl;
			scriptMan.ForgetScript(script->GetName());
			std::unique_ptr<ScriptTicket> scriptHolder = _scriptsInProgress.Remove(*script);
			Assert (scriptHolder.get() != 0);
			scriptToEmailsIter = scriptToEmails.erase (scriptToEmailsIter); // the iter points to the successor
			continue;
		}

		NocaseSet const & addresses = scriptToEmailsIter->second;
		std::vector<std::string> addrVector;
		std::copy(addresses.begin(), addresses.end(), std::back_inserter(addrVector));
		PostEmailRequest(scriptMan, handle, addrVector);
		++scriptToEmailsIter;
	}
}

void TransportManager::StartForwarding(std::vector<ScriptHandle> & toDoList, ScriptManager & scriptMan)
{
	ActivityIndicator forwarding (TheFeedbackMan, Forwarding);
	dbg << "TransportManager::Forward " << toDoList.size() << std::endl;
	for (std::vector<ScriptHandle>::iterator hit = toDoList.begin(); hit != toDoList.end(); ++hit)
	{
		ScriptTicket * script = _scriptsInProgress.Get(*hit);
		if (script == 0)
			continue;
		if (!File::Exists (script->GetPath ()))
		{
			dbg << "= File doesn't exist. removing from list: " << script->GetName() << std::endl;
			scriptMan.ForgetScript(script->GetName());
			std::unique_ptr<ScriptTicket> scriptHolder = _scriptsInProgress.Remove(*script);
			Assert (scriptHolder.get() != 0);
			continue;
		}

		// Fill the transport table
		std::set<Transport> transTable;
		for (ScriptTicket::RequestIter ri = script->beginReq (); ri != script->endReq (); ++ri)
		{
			DispatchRequest & req = ri->second;
			if (!req.WasTried () && req.IsForward ())
				transTable.insert(req.GetTransport ());
		}

		bool progressMade = false;
		// for each fast transport for this script (LAN or local dir/floppy)
		for (std::set<Transport>::iterator it = transTable.begin ();
			it != transTable.end ();
			++it)
		{
			Transport const & transport = *it;
			IgnoreAction action = IsIgnoredDest (transport);
			if (action == ignoreNetwork || action == ignoreDisk)
			{
				ScriptStatus::Dispatch::Bits status = 
					(action == ignoreNetwork)
						? ScriptStatus::Dispatch::NoNetwork 
						: ScriptStatus::Dispatch::NoDisk;
				_scriptFileList->SetUiStatus(script->GetName(), status);
			}
			// Queue it up for execution
			PostForwardRequest(
				scriptMan,
				*hit, // handle
				transport,
				action);
		} // <-- End for each transport
	} // <-- End for each script
}

void TransportManager::ScriptProgressMade(ScriptManager & scriptMan, ScriptTicket & script)
{
	dbg << "TransportManager::ScriptProgressMade: " << script.GetName() << std::endl;
	try
	{
		// stamp delivery
		for (ScriptTicket::RequestIter ri = script.beginReq (); ri != script.endReq (); ++ri)
		{
			int idx = ri->first;
			DispatchRequest & req = ri->second;
			if (req.IsDelivered())
			{
				TheFileLocker.StampFinalDelivery(script, idx);
			}
		}
	}
	catch (...)
	{
		// ignore stamping errors
		Win::ClearError ();
	}

	if (!script.AreRequestsPending () // All requests delivered
		|| (script.IsEmailError () && !script.AreForwardRequestsPending()))
	{
		dbg << "= No requests pending. removing from list: " << script.GetName() << std::endl;
		scriptMan.ForgetScript(script.GetName());
		std::unique_ptr<ScriptTicket> scriptHolder = _scriptsInProgress.Remove(script);
		Assert (scriptHolder.get() != 0);
		scriptMan.FinishScript(std::move(scriptHolder));
	}
}

void TransportManager::TransferToDoList(ScriptVector & toDoTickets, 
									 std::vector<ScriptHandle> & toDoHandles)
{
	while (toDoTickets.size() != 0)
	{
		ScriptTicket * script = toDoTickets.back();
		Assert (!_scriptsInProgress.Find(script));
		ScriptHandle h = _scriptsInProgress.Insert(toDoTickets.pop_back());
		toDoHandles.push_back(h);
	}
}

// Add handle list to argumenst
void TransportManager::Distribute (std::vector<ScriptHandle> & toDoHandles, ScriptManager & scriptMan) // nothrow
{
	try
	{
		// toDoHandles may be empty
		// Re-activate all clogged copiers
		_copiers.Unclog();
		if (!toDoHandles.empty())
		{
			StartForwarding(toDoHandles, scriptMan);
			StartEmailing(toDoHandles, scriptMan);
		}

		// loop over copiers, pick the next script for each copier and start copying
		_copiers.StartCopying(_scriptsInProgress);

		if (_scriptFileList->AcceptUiStatusChange())
		{
			Win::UserMessage msg (UM_SCRIPT_STATUS_CHANGE);
			_winParent.PostMsg (msg);
		}
		_isVerbose = false; // toggle it back

		RefreshIgnored();
		_scriptsInProgress.RefreshRequests();
	}
	catch (Win::Exception e)
	{
		TheAlertMan.PostInfoAlert (e, _isVerbose);
	}
	catch (...)
	{
		TheAlertMan.PostInfoAlert ("Unknown error while processing scripts", _isVerbose);
	}
}

void TransportManager::ClearAll()
{
	ClearIgnored();
	_scriptsInProgress.clear(); // must be cleared before _copiers
	_copiers.ClearRequests();
}

void TransportManager::IgnorePath (Transport const & transport)
{
	_ignoredDestinations.Insert(transport, ignoreNetwork);
	if (_topology.IsTemporaryHub () && transport == _hubTransport)
	{
		Win::UserMessage msg (UM_PROXY_CONNECTED, 0);
		_winParent.PostMsg (msg);
	}
}

void TransportManager::UnIgnorePath (Transport const & transport)
{
	_ignoredDestinations.Remove (transport);

	if (_topology.IsTemporaryHub () && transport == _hubTransport)
	{
		Win::UserMessage msg (UM_PROXY_CONNECTED, 1);
		_winParent.PostMsg (msg);
	}
}

//-------- IgnoredDestinations --------------

IgnoreAction IgnoredDestinations::Find (Transport const & transport) const
{
	if (transport.IsNetwork ())
	{
		if (!FilePath::IsAbsolute (transport.GetRoute ()))
			return ignoreNo;

		// for floppies check both the drive letter and the actual folder 
		FullPathSeq dest (transport.GetRoute ().c_str ());
		if (dest.HasDrive ())
		{
			Transport drive (dest.GetHead (), Transport::Network); // floppy as network
			const_iterator it = _transports.find (drive);

			if (it != _transports.end ())
			{
				return it->second;
			}
		}
	}
	const_iterator it = _transports.find (transport);
	if (it != _transports.end ())
	{
		return it->second;
	}
	return ignoreNo;
}

void IgnoredDestinations::Refresh ()
{
	for (iterator it = _transports.begin (); it != _transports.end (); ++it)
		it->second = ignoreRetry;
}

void TransportManager::ActiveCopiers::StartCopying(HtoScriptMap const & scriptMap)
{
	dbg << "ActiveCopiers::StartCopying" << std::endl;
	unsigned i = 0; 
	while (i < _lanCopiers.size())
	{
		ActiveLanCopier * copier = _lanCopiers[i]->get();
		if (copier->IsIdle())
		{
			if (!copier->CopyNextScript(scriptMap))
			{
				copier->Kill();
				_lanCopiers.erase(i);
				continue;
			}
		}
		++i;
	}
	if (!_emailCopier.empty())
	{
		ActiveEmailCopier * copier = _emailCopier.get();
		if (copier->IsIdle() && copier->HasWork())
		{
			if (!copier->CopyScripts(scriptMap))
			{
				copier->Kill();
				_emailCopier.reset();
			}
		}
	}
}

void TransportManager::ActiveCopiers::Unclog()
{
	for (auto_vector<auto_active<ActiveLanCopier> >::iterator it = _lanCopiers.begin(); it != _lanCopiers.end(); ++it)
	{
		ActiveLanCopier * copier = (*it)->get();
		copier->Unclog();
	}
	if (!_emailCopier.empty())
		_emailCopier.get()->Unclog();
}

void TransportManager::ActiveCopiers::ClearRequests()
{
	_readyCopiersStore.clear();
	auto_vector<auto_active<ActiveLanCopier> >::iterator it = _lanCopiers.begin();
	while ( it != _lanCopiers.end())
	{
		ActiveLanCopier * copier = (*it)->get();
		if (copier->ClearRequests())
		{
			_transportMap.erase(copier->GetTransport());
			copier->Kill();
			it = _lanCopiers.erase(it);
		}
		else
			++it;
	}
	if (!_emailCopier.empty())
	{
		if (_emailCopier.get()->ClearRequests())
			_emailCopier.reset();
	}
}

ActiveLanCopier * TransportManager::ActiveCopiers::GetLanCopier(Transport const & trans)
{
	iterator it = _transportMap.find(trans);
	if (it == _transportMap.end())
	{
		std::unique_ptr<ActiveLanCopier> active(new ActiveLanCopier(trans, _goesToHub, _readyCopiersStore, _sync));
		std::unique_ptr<auto_active<ActiveLanCopier> > cp(new auto_active<ActiveLanCopier>(active.release()));
		_lanCopiers.push_back(std::move(cp));
		_transportMap[trans] = _lanCopiers.back()->get();
		return _lanCopiers.back()->get();
	}
	return it->second;
}

ActiveEmailCopier * TransportManager::ActiveCopiers::GetEmailCopier()
{
	if (_emailCopier.empty())
	{
		_emailCopier.reset(new ActiveEmailCopier(_goesToHub, _readyEmailCopierStore, _sync));
	}
	return _emailCopier.get();
}

void TransportManager::ActiveCopiers::RemoveTransport(Transport const & trans)
{
	iterator it = _transportMap.find(trans);
	if (it != _transportMap.end())
	{
		vector_iterator vit = _lanCopiers.begin();
		while (vit != _lanCopiers.end())
		{
			if ((*vit)->get() == it->second)
			{
				(**vit)->Kill();
				_lanCopiers.erase(vit);
				break;
			}
			++vit;
		}
		_transportMap.erase(it);
	}
}

void TransportManager::ActiveCopiers::RemoveEmail()
{
	if (!_emailCopier.empty())
	{
		_emailCopier.reset();
	}
}

std::unique_ptr<ScriptTicket> HtoScriptMap::Remove(ScriptTicket const & script)
{
	std::unique_ptr<ScriptTicket> result;

	unsigned i = 0;
	for ( ; i < _tickets.size(); ++i)
		if (*_tickets[i] == script)
			break;

	if (i == _tickets.size())
		return result;

	for (Map::iterator mit = _map.begin(); mit != _map.end(); ++mit)
	{
		if (*mit->second == script)
		{
			_map.erase(mit);
			break;
		}
	}
	result = _tickets.extract(i);
	_tickets.erase(i);
	return result;
}

bool HtoScriptMap::Find(ScriptTicket const * script) const
{
	for (ScriptVector::const_iterator it = _tickets.begin();
		it != _tickets.end(); ++it)
	{
		if (**it == *script)
		{
			return true;
		}
	}
	return false;
}

void HtoScriptMap::RefreshRequests()
{
	for (ScriptVector::iterator it = _tickets.begin(); it != _tickets.end(); ++it)
	{
		(*it)->RefreshRequests();
	}
}

std::ostream& operator<<(std::ostream& os, HtoScriptMap const & m)
{
	os << "Handle to Script Map" << std::endl;
	for (HtoScriptMap::Map::const_iterator it = m._map.begin(); it != m._map.end(); ++it)
		os << "    " << it->first << " -> " << it->second->GetName() << std::endl;
	return os;
}

std::ostream& operator<<(std::ostream& os, TransportManager const & t)
{
	os << ">> TransportManager 6/15/2010 <<" << std::endl;
	os << t._scriptsInProgress << std::endl;
	os << "ActiveCopiers"  << std::endl;
	for (auto_vector<auto_active<ActiveLanCopier> >::const_iterator it = t._copiers._lanCopiers.begin(); it != t._copiers._lanCopiers.end(); ++it)
	{
		ActiveLanCopier const * copier = (*it)->get();
		os << *copier << std::endl;
	}
	if (!t._copiers._emailCopier.empty())
	{
		ActiveEmailCopier const & copier = *t._copiers._emailCopier;
		os << copier << std::endl;
	}
	return os;
}