//----------------------------------
// (c) Reliable Software 2000 - 2009
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING true

#include "WorkQueue.h"
#include "ScriptFileList.h"
#include "FolderMan.h"
#include "TransportHeader.h"
#include "Processor.h"
#include "ScriptProcessorConfig.h"
#include "ConfigData.h"
#include "DispatcherMsg.h"
#include "DispatcherParams.h"
#include "AlertMan.h"
#include "FeedbackMan.h"
#include "ScriptSubject.h"
#include "EmailMan.h"
#include "Email.h"
#include "EmailMessage.h"
#include "BadEmailExpt.h"
#include "AppHelp.h"
#include "Registry.h"

#include <File/SafePaths.h>
#include <Win/Message.h>
#include <Com/Shell.h>
#include <Dbg/Out.h>
#include <StringOp.h>
#include <Net/Socket.h>
#include <Mail/Pop3.h>

// predicates
bool HasForwards (ScriptTicket const & script)
{
	return script.FwdReqCount () != 0;
}

bool HasEmails (ScriptTicket const * script)
{
	return script->EmailReqCount () != 0;
}

WorkQueue::WorkQueue (ConfigData const & config, ScriptFileList * scriptFileList, Win::Dow::Handle winParent)
	: _winParent(winParent), 
	  _scriptFileList(scriptFileList)
{
	std::unique_ptr<TransportManager> transMan(new TransportManager(winParent, config, _sync));
	if (transMan->IsTemporaryHub ())
	{
		// Treat hub path as ignored on startup.
		// This prevents displaying network copy alert.
		transMan->Ignore(config.GetActiveTransportToHub (), ignoreNetwork);
	}
	// pass them to the worker thread (see Run())
	_transMan = std::move(transMan);
	_scriptFileListStore.Put(scriptFileList);
}

WorkQueue::~WorkQueue ()
{
	dbg << "~WorkQueue" << std::endl;
}

//--------------------------------
// Methods called from the outside
//--------------------------------

// return the list of new script files
// mark known scripts "InProgress"
void WorkQueue::GetNewAndMarkOldScripts(
	std::vector<std::string> & newFiles)
{
	Win::MutexLock lock (_mutex);
	if (_scriptFileList == 0)
		return;
	dbg << "-->WorkQueue::CountNewAndMarkOldScripts" << std::endl;
	_scriptFileList->GetSetDifference(_knownScriptFiles, newFiles);
	dbg << "<--WorkQueue::CountNewAndMarkOldScripts: new scripts: " << newFiles.size() << std::endl;
}

void WorkQueue::TransferRequests(ScriptVector & scriptTicketList)
{
	dbg << "WorkQueue::TransferRequests" << std::endl;
	Win::MutexLock lock (_mutex);
	// Transfer tickets to _toDoList
	while (scriptTicketList.size() != 0)
	{
		std::unique_ptr<ScriptTicket> scriptHolder = scriptTicketList.pop_back();
		if (scriptHolder.get () != 0)
		{
			Assert (std::find_if(_toDoList.begin(), _toDoList.end(), IsEqualScript(*scriptHolder.get())) == _toDoList.end());
			_knownScriptFiles.insert(scriptHolder->GetName());
			_toDoList.push_back(std::move(scriptHolder));
		}
	}
	ResetTimeout();
}

// Instead of this, find script ticket by handle
void WorkQueue::TransferDoneScripts (ScriptVector & doneList)
{
	Win::MutexLock lock (_mutex);
	while (_finished.size() != 0)
	{
		doneList.push_back(_finished.pop_back());
	}
}

void WorkQueue::FinishScript(std::unique_ptr<ScriptTicket> script)
{
	dbg << "WorkQueue::FinishScript: " << script->GetName() << std::endl;
	Win::MutexLock lock (_mutex);
	_finished.push_back(std::move(script));
	Win::UserMessage msg (UM_FINISH_SCRIPT);
	_winParent.PostMsg (msg);
}

//-----------------------------
// Internal 
//-----------------------------

// cheap
class TimeoutResetHandler
{
public:
	TimeoutResetHandler(WorkQueue::Timeout & timeout, bool & doWork)
		:_timeout(timeout), _doWork(doWork)
	{}
	void operator()() 
	{
		_timeout.Reset();
		_doWork = true;
	}
private:
	WorkQueue::Timeout & _timeout;
	bool & _doWork;
};

class UnIgnoreHandler
{
public:
	UnIgnoreHandler(TransportManager & transMan, WorkQueue::Timeout & timeout, bool & doWork)
		:_transMan(transMan), _timeout(timeout), _doWork(doWork)
	{}
	void operator()()
	{ 
		_transMan.ClearIgnored();
		_timeout.Reset();
		_doWork = true;
	}
private:
	TransportManager & _transMan;
	WorkQueue::Timeout & _timeout;
	bool & _doWork;
};

class ScatteringRequestHandler
{
public:
	ScatteringRequestHandler(TransportManager & transMan, bool & doWork)
		: _transMan(transMan), _doWork(doWork)
	{}
	void operator()()
	{
		_transMan.SetVerbose(true);
		_doWork = true;
	}
private:
	TransportManager & _transMan;
	bool & _doWork;
};

// expensive
class ClearStateHandler
{
public:
	ClearStateHandler(TransportManager & transMan, WorkQueue & workQueue)
		: _transMan(transMan), _workQueue(workQueue)
	{}
	void operator()()
	{
#if !defined(NDEBUG)
		dbg << "===ClearState===\n" << _transMan << "===End ClearState===" << std::endl;
#endif
		_transMan.ClearAll();
		_workQueue.DoClearState();
	}
private:
	TransportManager & _transMan;
	WorkQueue & _workQueue;
};

class ClearBlacklistHandler
{
public:
	void operator()()
	{
		TheEmail.ClearBlacklist ();
	}
};

class ScriptFileListHandler
{
public:
	ScriptFileListHandler(TransportManager & transMan)
		: _transMan(transMan)
	{}
	void operator()(ScriptFileList * sfl)
	{
		_transMan.SetScriptFileList(sfl);
	}
private:
	TransportManager & _transMan;
};

class ConfigChangeHandler
{
public:
	ConfigChangeHandler(TransportManager & transMan, WorkQueue & workQueue)
		: _transMan(transMan), _workQueue(workQueue)
	{}
	void operator()(WorkQueue::ConfigChangeMsg & msg)
	{
		_transMan.Reconfigure(msg._topology, msg._transportToHub);
		_transMan.ClearAll();
		_workQueue.DoClearState();
	}
private:
	TransportManager & _transMan;
	WorkQueue & _workQueue;
};

class LanCopierReadyHandler
{
public:
	LanCopierReadyHandler(TransportManager & transMan, ScriptManager & scriptMan)
		: _transMan(transMan), _scriptMan(scriptMan)
	{}
	void operator()(LanCopierReadyMessage & msg)
	{
		_transMan.OnLanCopyDone(msg, _scriptMan);
	}
private:
	TransportManager & _transMan;
	ScriptManager & _scriptMan;
};

class EmailCopierReadyHandler
{
public:
	EmailCopierReadyHandler(TransportManager & transMan, ScriptManager & scriptMan)
		: _transMan(transMan), _scriptMan(scriptMan)
	{}
	void operator()(EmailCopierReadyMessage & msg)
	{
		_transMan.OnEmailCopyDone(msg, _scriptMan);
	}
private:
	TransportManager & _transMan;
	ScriptManager & _scriptMan;
};

void WorkQueue::Run ()
{
	Com::Use useCom;
	// Transfer initial arguments
	std::unique_ptr<TransportManager> transManPtr = std::move(_transMan);
	TransportManager & transMan = *transManPtr.get();

	ScriptFileList * sfl = _scriptFileListStore.Get();
	Assert(sfl != 0);
	transMan.SetScriptFileList(sfl);

	Timeout timeout;
	bool doWork = true;

	RcvMultiChannel channel(_sync);

	LanCopierReadyHandler scriptCopiedHandler(transMan, *this);
	LanCopierReadyWrap<LanCopierReadyHandler> scriptCopiedWrapper(transMan.GetLanCopierReadyStore(), scriptCopiedHandler);
	channel.RegisterChannel(&scriptCopiedWrapper);

	EmailCopierReadyHandler scriptEmailedHandler(transMan, *this);
	EmailCopierReadyWrap<EmailCopierReadyHandler> scriptEmailedWrapper(transMan.GetEmailCopierReadyStore(), scriptEmailedHandler);
	channel.RegisterChannel(&scriptEmailedWrapper);

	UnIgnoreHandler unIgnoreHandler(transMan, timeout, doWork);
	BoolWrapper<UnIgnoreHandler> unIgnoreWrapper(_unIgnore, unIgnoreHandler);
	channel.RegisterChannel(&unIgnoreWrapper);

	TimeoutResetHandler timeoutHandler(timeout, doWork);
	BoolWrapper<TimeoutResetHandler> timeoutWrapper(_resetTimeout, timeoutHandler);
	channel.RegisterChannel(&timeoutWrapper);

#if 1
	ScatteringRequestHandler scatteringHandler(transMan, doWork);
	BoolWrapper<ScatteringRequestHandler> scatteringWrapper(_scatteringRequest, scatteringHandler);
	channel.RegisterChannel(&scatteringWrapper);
#else
	auto myWrapper = CreateBoolWrapper(_scatteringRequest, [&]()
	{
		transMan.SetVerbose(true);
		doWork = true;
	});
#endif

	// Use delay wrapper for expensive handlers
	ClearStateHandler clearStateHandler(transMan, *this);
	BoolDelayWrapper<ClearStateHandler> clearWrapper(_clearState, clearStateHandler);
	channel.RegisterChannel(&clearWrapper);

	ClearBlacklistHandler clearBlacklistHandler;
	BoolDelayWrapper<ClearBlacklistHandler> clearBlacklistWrapper(_clearBlacklist, clearBlacklistHandler);
	channel.RegisterChannel(&clearBlacklistWrapper);

	ScriptFileListHandler scriptFileListHandler(transMan);
	WrappedValueChannel<ScriptFileList*, ScriptFileListHandler> scriptFileListWrapper(_scriptFileListStore, scriptFileListHandler);
	channel.RegisterChannel(&scriptFileListWrapper);

	ConfigChangeHandler configChangeHandler(transMan, *this);
	WrappedValueChannel<ConfigChangeMsg, ConfigChangeHandler> configChangeWrapper(_configChangeStore, configChangeHandler);
	channel.RegisterChannel(&configChangeWrapper);

	for (;;)
	{
		doWork = !channel.Wait(timeout.Get()); // do work on timeout or on specific messages

		if (IsDying ())
		{
			// must be done from this thread
			TheEmail.ShutDown ();
			return;
		}

		channel.Receive();

		if (doWork)
		{
			ScriptVector toDoTickets;
			this->AcquireToDoList(toDoTickets);
			// Moves scripts from toDoTickets to _scriptsInProgress 
			// Returns list of new handles
			std::vector<ScriptHandle> toDoHandles;
			transMan.TransferToDoList(toDoTickets, toDoHandles);

			if (transMan.HasWork())
			{
				transMan.Distribute (toDoHandles, *this);
			}

			if (transMan.HasWork())
			{
				timeout.Double();
			}
			else
			{
				timeout.SetMax();
			}
		}
	}
}

void WorkQueue::AcquireToDoList(ScriptVector & toDoList)
{
	Win::MutexLock lock (_mutex);
	_toDoList.swap(toDoList);
}

void WorkQueue::ResetTimeout() 
{
	BoolSendChannel	chan(_sync, _resetTimeout);
	chan.Put(); 
}

void WorkQueue::ClearState ()
{ 
	BoolSendChannel	chan(_sync, _clearState);
	chan.Put(); 
	_eventDone.Wait ();
}

void WorkQueue::ClearEmailBlacklist ()
{
	BoolSendChannel	chan(_sync, _clearBlacklist);
	chan.Put(); 
}

void WorkQueue::UnIgnore ()
{
	BoolSendChannel chan(_sync, _unIgnore);
	chan.Put();
}

void WorkQueue::ForceScattering ()
{
	BoolSendChannel chan(_sync, _scatteringRequest);
	chan.Put();
}

void WorkQueue::ChangeConfig (ConfigData const & config)
{
	// make a copy of settings. They will be changed
	// next time the processing starts--avoids contention
	ConfigChangeMsg msg(config.GetTopology (), config.GetActiveTransportToHub ());
	SendChannel<ConfigChangeMsg, ValueStore> chan(_sync, _configChangeStore);
	chan.Put(msg);
	_eventDone.Wait ();
}

void WorkQueue::ResetScriptFileList(ScriptFileList * scriptFileList)
{
	SendChannel<ScriptFileList *, ValueStore> chan(_sync, _scriptFileListStore);
	chan.Put(scriptFileList);
}


void WorkQueue::DoClearState()
{
	Win::MutexLock lock (_mutex);
	_toDoList.clear ();
	_knownScriptFiles.clear();
	_eventDone.Release ();
}

void WorkQueue::ForgetScript(std::string const & scriptName)
{
	Win::MutexLock lock (_mutex);
	_knownScriptFiles.erase(scriptName);
}

std::ostream& operator<<(std::ostream& os, WorkQueue & w)
{
	os << "Known Script Files" << std::endl;
	for (std::set<std::string>::const_iterator it = w._knownScriptFiles.begin(); it != w._knownScriptFiles.end(); ++it)
		os << "   -o " << *it << std::endl;
	os << std::endl;
	w.ClearState();
	return os;
}