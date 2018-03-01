//-----------------------------------
//  (c) Reliable Software 2000 - 2007
//-----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "IpcQueue.h"
#include "Commander.h"
#include "Global.h"

CommandIpc::Queue::Queue (Win::Dow::Handle parentWin, Context & context)
	: _parentWin (parentWin),
	  _stayInGui (false),
	  _lastProjectId (-1),
	  _context (context),
	  _timeout (INFINITE)
{
	_isIdle.Inc ();
}

// Returns true when conversation request started conversation
void CommandIpc::Queue::InitiateConv (std::string const & convName)
{
	dbg << "--> CommandIpc::Queue::InitiateConv" << std::endl;
	dbg << "    Conversation shared buffer name: " << convName << std::endl;

	Win::Lock lock (_critSect);
	std::unique_ptr<CommandIpc::ConversationData> convData 
	(
		new CommandIpc::ConversationData (convName, true) // quiet
	);

	if (convData->IsValid())
	{
		_requests.Add (std::move(convData));
		_isIdle.Reset ();
	}
	_event.Release ();
}

// called by conversation thread
void CommandIpc::Queue::OnFinishConv (bool stayInGui)
{
	dbg << "--> CommandIpc::Queue::OnFinishConv - continue in GUI: " << (stayInGui ? "yes" : "no") << std::endl;
	// only change to true, never back
	if (stayInGui)
		_stayInGui = true;
	_activeConvKill.Inc ();
	_event.Release ();
	dbg << "<-- CommandIpc::Queue::OnFinishConv" << std::endl;
}

bool CommandIpc::Queue::IsIdle ()
{
	return _isIdle.IsNonZero ();
}

// Returns true when next conversation started
bool CommandIpc::Queue::NextConversation ()
{
	dbg << "--> CommandIpc::Queue::NextConversation" << std::endl;
	Win::Lock lock (_critSect);
	while (!_requests.IsEmpty ())
	{
		dbg << "    Taking request from the queue" << std::endl;
		ConversationData * request = _requests.GetRequest ();
		// Revisit: a request with a project id set to -1 may actually 
		// need to leave the currently visited project
		if (request->GetProjectId () != -1)
		{
			// Visit project specified by the conversation data
			dbg << "    Visit project " << std::dec << request->GetProjectId () << std::endl;
			try
			{
				// VisitProject temporarily unlocks queue and locks model
				if (!_context.VisitProject (request->GetProjectId (), _critSect))
				{
					request->WakeupClient ();
					dbg << "    Cannot visit project " << request->GetProjectId () << std::endl;
					continue;	// Take next request from the queue
				}
			}
			catch ( ... )
			{
				Win::ClearError ();
				request->WakeupClient ();
				dbg << "    Exception: cannot visit project " 
					<< request->GetProjectId () << std::endl;
				continue;
			}
		}

		dbg << "<-- CommandIpc::Queue::NextConversation -- starting conversation" << std::endl;
		_activeConvKill.Reset ();
		_timeout = IpcTimeout;
		_lastProjectId = request->GetProjectId ();
		_activeConv.reset (new ServerConversation (request, _context, this)); // <- ConvStarter

		_activeConv.SetWaitForDeath (LongCommandTimeout);
		return true;
	}
	dbg << "<-- CommandIpc::Queue::NextConversation -- no more requests" << std::endl;
	_requests.Clear ();
	_isIdle.Inc ();
	return false;
}

void CommandIpc::Queue::Run ()
{
	dbg << "--> CommandIpc::Queue::Run" << std::endl;
	while (true)
	{
		try
		{
			bool success = _event.Wait (_timeout);
			if (IsDying ())
				return;

			// Three possibilities:
			// - timeout
			// - new request added
			// - current conversation finished
			if (!success)
			{
				// Timeout: kill the conversation
				_activeConvKill.Set (1);
			}

			if (_activeConvKill.IsNonZeroReset ())
			{
				// The only place we reset the conversation!
				_activeConv.reset ();
				if (_activeConv.IsAlive ())
				{
					// still alive: this is a desparete step!
					Win::RegisteredMessage msg (UM_IPC_ABORT);
					_parentWin.PostMsg (msg);
					dbg << "<-- CommandIpc::Queue::Run - posting UM_IPC_ABORT" << std::endl;
					return;
				}
			}

			if (_activeConv.empty ())
			{
				if (!NextConversation ())
				{
					Win::RegisteredMessage msg (UM_IPC_TERMINATE);
					_parentWin.PostMsg (msg);
					_timeout = INFINITE;
					dbg << "     Queue is idle - posting UM_IPC_TERMINATE" << std::endl;
				}
			}
		}
		catch (Win::Exception e)
		{
			dbg << "     Exception: " << Out::Sink::FormatExceptionMsg (e) << std::endl;
			e; // revisit: alert
		}
		catch ( ... )
		{
			dbg << "     Unknown exception: " << std::endl;
			// revisit: alert
		}
	}
	dbg << "<-- CommandIpc::Queue::Run" << std::endl;
}


//
// Request queue
//

void CommandIpc::RequestQueue::Add (std::unique_ptr<ConversationData> request)
{
	unsigned int timeout = VisitProjectTimeout;
	if (_requests.size () != 0)
		timeout += OneConversationTimeout;
	timeout += _queue.size () * OneConversationTimeout;
	request->ChangeClientTimeout (timeout);
	_requests.push_back (std::move(request));
	_queue.push_back (_requests.size () - 1);
}

CommandIpc::ConversationData * CommandIpc::RequestQueue::GetRequest ()
{
	Assert (!_queue.empty ());
	unsigned int next = _queue.front ();
	Assert (next < _requests.size ());
	_queue.pop_front ();
	Assert (_requests [next] != 0);
	return _requests [next];
}

void CommandIpc::RequestQueue::Clear ()
{
	Assert (_queue.empty ());
	_requests.clear ();
}
