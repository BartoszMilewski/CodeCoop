//-----------------------------------
//  (c) Reliable Software 2000 - 2010
//-----------------------------------

#include "precompiled.h"
#include "IpcConversation.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "Global.h"
#include "Commander.h"
#include "InputSource.h"
#include "FileStateList.h"
#include "FeedbackUser.h"
#include "FeedbackMan.h"
#include "SerString.h"
#include "SerVector.h"
#include "ProjectData.h"

#include <Win/Message.h>
#include <Ex/Error.h>
#include <time.h>

//
// Conversation context
//

bool CommandIpc::Context::VisitProject (int projectId, Win::CritSection & queueCritSect)
{
	// To avoid a deadlock, we have to release Queue lock
	Win::Unlock unlock (queueCritSect);
	Win::Lock lock (_modelCritSect);
	return _commander.VisitProject (projectId);
}

SelectionManager & CommandIpc::Context::XGetSelectionManager ()
{
	return *_commander.GetActiveSelectionMan ();
}

bool CommandIpc::Context::XExecuteCommand (std::string const & command)
{
	int cmdId = _cmdVector.Cmd2Id (command.c_str ());
	if (cmdId == -1)
		return false;

	_cmdVector.Execute (cmdId);
	return true;
}

//
// Conversation data
//

CommandIpc::ConversationData::ConversationData (std::string const & convName, bool quiet)
	: _exchange (convName),
	  _projectId (-1)
{
	if (_exchange.IsInvitation ())
	{
		MemoryDeserializer in (_exchange.GetReadData ());
		IpcInvitation invitation (in);
		dbg << "    Conversation data: project id " << invitation.GetProjId ();
		dbg << "; client event name " << invitation.GetEventName () << std::endl;
		_projectId = invitation.GetProjId ();
		_clientEvent.reset (new Win::ExistingEvent (invitation.GetEventName (), quiet));
	}
}

void CommandIpc::ConversationData::ChangeClientTimeout (unsigned int timeout)
{
	// Note: _clientEvent maybe null if !_exchange.IsInvitation, see above!
	//Assert (_clientEvent.get () != 0);
	// Revisit: can't do it with events
	// Could write something to buffer to let the client
	// repeat the wait after waking up
	//_clientEvent->Set (timeout);
}

void CommandIpc::ConversationData::WakeupClient ()
{
	if (_clientEvent.get () != 0)
		_clientEvent->Release ();
}

//
// Server conversation
//
void CommandIpc::ServerConversation::Run ()
{
	bool abnormalExit = false;
	try
	{
		std::srand (static_cast<unsigned int>(time (0)));
		dbg << "+++++++++++++++++++++++++++++++++++++++++++" << std::endl;
		dbg << "--> CommandIpc::ServerConversation::Run" << std::endl;
		// Acknowledge the invitation
		_convData->MakeAck (_myEvent.GetName ());
		_convData->WakeupClient ();
		while (true)
		{
			if (IsDying ())
			{
				dbg << "<-- CommandIpc::ServerConversation::Run -- dying (1)" << std::endl;
				dbg << "===========================================" << std::endl;
				return;
			}
			dbg << "    Waiting for command" << std::endl;
			// Wait for the command
			bool success = _myEvent.Wait (_waitTimeout);

			if (IsDying ())
			{
				dbg << "<-- CommandIpc::ServerConversation::Run -- dying (2)" << std::endl;
				dbg << "===========================================" << std::endl;
				return;
			}
			//---------------DEBUGGING------------
#if 0
			dbg << "    Received: ";
			if (_convData->IsCmd () || _convData->IsDataRequest ())
			{
				if (_convData->IsCmd ())
					dbg << "command";
				else if (_convData->IsDataRequest ())
				{
					dbg << "data request: ";
					if (_convData->IsStateRequest ())
						dbg << "get file state";
					else if (_convData->IsVersionIdRequest ())
						dbg << "get version id";
					else if (_convData->IsForkIdsRequest ())
						dbg << "get fork ids";
					else if (_convData->IsTargetPathRequest ())
						dbg << "get target path";
					else
						dbg << "UNKNOWN";
				}
				dbg << "; project id " << std::dec << _convData->GetProjectId ();
				if (_convData->IsLast ())
					dbg << "; Last -- will post UM_IPC_TERMINATE";
			}
			else
				dbg << "NO command -- posting UM_IPC_TERMINATE";
			dbg << std::endl;
#endif
			if (success && (_convData->IsCmd () || _convData->IsDataRequest ()))
			{
				// The model must be locked for the duration
				//----------------------------------
				Win::Lock lockModel (_context.GetModelCtricalSection ());

				//	Switch out default FeedbackManager here
				//  because we can't do anything that would
				//	result in SendMessage (progress bar does!)
				FeedbackManager feedbackMan;	//  null feedback manager
				FeedbackHolder feedbackHolder(_context.XGetFeedbackManager (), &feedbackMan);

				if (_convData->IsCmd ())
				{
					ExecuteCmd ();
				}
				else
				{
					Assert (_convData->IsDataRequest ());
					if (_convData->IsStateRequest ())
					{
						ReturnState ();
					}
					else if (_convData->IsVersionIdRequest ())
					{
						ReturnVersionId ();
					}
					else if (_convData->IsReportRequest ())
					{
						ReturnReport ();
					}
					else if (_convData->IsForkIdsRequest ())
					{
						ReturnForkIds ();
					}
					else if (_convData->IsTargetPathRequest ())
					{
						ReturnTargetPath ();
					}
				}

				//------- unlock model ----------
			}

            if (_convData != nullptr)
			{
				_convData->WakeupClient ();
				if (_convData->IsLast ())
					break;
			}
		} // --- end while
	}
	catch (Win::ExitException e)
	{
		_convData->MakeErrorReport (Out::Sink::FormatExceptionMsg (e));
		abnormalExit = true;
	}
	catch (Win::Exception e)
	{
		if (e.GetMessage () != 0)
			_convData->MakeErrorReport (Out::Sink::FormatExceptionMsg (e));
		else
			_convData->MakeErrorReport ("Operation aborted by the user.");
		abnormalExit = true;
	}
	catch ( ... )
	{
		// Revisit: alert
		// Don't try to access _convData, as it might be already corrupt!
        _convData = nullptr;
		std::string msg ("Unknown exception during external command execution.\n");
		abnormalExit = true;
	}

	if (abnormalExit)
	{
		dbg << "     Abnormal exit" << std::endl; 
		Win::ClearError ();
        if (_convData != nullptr)
		{
			_convData->WakeupClient ();
			// Command string has been replaced with error message
			dbg << _convData->GetCmdStr () << std::endl;
		}
	}

	Win::Lock lock (_critSect);
	if (_convStarter)
		_convStarter->OnFinishConv (_stayInGui);
	dbg << "<-- CommandIpc::ServerConversation::Run" << std::endl;
	dbg << "===========================================" << std::endl;
}

void CommandIpc::ServerConversation::Detach ()
{
	Win::Lock lock (_critSect);
	_convStarter = 0;
}


// Model is locked!
void CommandIpc::ServerConversation::ExecuteCmd ()
{
	// Parse command line received in IPC buffer
	InputSource input;
	InputParser parser (_convData->GetCmdStr (), input);
	parser.Parse ();

	_stayInGui = input.StayInGui ();
    // Command line can be divided into two command groups:
    //    - background commands - exeute commands in server mode (no GUI)
    //    - GUI commands - execute commands in GUI
    // The command grups are separated by command line switch -GUI.
    // Any command following -GUI will be executed in GUI. Currently we
    // expect just one command executed in GUI.
    // If command line contains -GUI switch execute GUI command group, otherwise execute background command group.
    // We assume that one IPC conversation has only one command group.
    if (_stayInGui)
    {
        input.SwitchToGuiCommands();
    }
	InputSource::Sequencer & cmdSeq = input.GetCmdSeq ();
	Assert (!cmdSeq.AtEnd ());
	// Get command name and load selection manager with selected files
	std::string const & command = cmdSeq.GetCommand (_context.XGetSelectionManager ());
	// Connect commander to the parsed command line for additional information retrieval
	InputSourceConnector connector (_context.XGetCommander (), &input);
	dbg << "    Executing Command: " << command 
		<< "; Stay in GUI: " << _stayInGui << std::endl;
	// Execute command
	if (_context.XExecuteCommand (command))
	{
		_convData->MakeAck ();
	}
	else
	{
		std::string info ("Unrecognized command -- '");
		info += command;
		info += '\'';
		dbg << "    " << info.c_str () << std::endl;
		_convData->MakeErrorReport (info);
	}
}

// Model is locked!
void CommandIpc::ServerConversation::ReturnState ()
{
	FileStateList list (_convData->GetData ());
	if (!list.empty ())
		_context.XGetCommander ().GetFileState (list);
	_convData->MakeAck ();
}

// Model is locked!
void CommandIpc::ServerConversation::ReturnVersionId ()
{
	FileStateList list (_convData->GetData ());
	if (!list.empty ())
		_context.XGetCommander ().GetVersionId (list, _convData->IsCurrent ());
	_convData->MakeAck ();
}

// Model is locked!
void CommandIpc::ServerConversation::ReturnReport ()
{
	GlobalId versionGid;
	{
		MemoryDeserializer in (_convData->GetData ());
		versionGid = in.GetLong ();
	}
	std::string versionDescr;
	_context.XGetCommander ().GetVersionDescription (versionGid, versionDescr);
	if (versionDescr.empty ())
	{
		MemorySerializer out (_convData->AllocBuf (sizeof (unsigned char)), sizeof (unsigned char));
		out.PutByte (0);
	}
	else
	{
		unsigned int size = versionDescr.length () + 1;
		MemorySerializer out (_convData->AllocBuf (size), size);
		out.PutBytes (versionDescr.c_str (), versionDescr.length ());
		out.PutByte (0);
	}
	_convData->MakeAck ();
}

// Model is locked!
void CommandIpc::ServerConversation::ReturnForkIds ()
{
	GidList clientForkIds;
	{
		// Read client fork ids
		MemoryDeserializer in (_convData->GetData ());
		SerVector<GlobalId> list;
		list.Deserialize (in, 0);
		clientForkIds.assign (list.begin (), list.end ());
	}

	// Process client fork ids
	GlobalId youngestFoundScriptId = gidInvalid;
	GidList myYoungerForkIds;
	_context.XGetCommander ().GetForkIds (clientForkIds,
										 _convData->IsDeepForkRequest (),
										 youngestFoundScriptId,
										 myYoungerForkIds);

	// Save result
	CountingSerializer counter;
	counter.PutLong (youngestFoundScriptId);
	SerVector<GlobalId> list (myYoungerForkIds);
	list.Serialize (counter);
	Assert (!LargeInteger(counter.GetSize ()).IsLarge ());
	unsigned int size = static_cast<unsigned>(counter.GetSize ());
	MemorySerializer out (_convData->AllocBuf (size), size);
	out.PutLong (youngestFoundScriptId);
	list.Serialize (out);
	_convData->MakeAck ();
}

// Model is locked!
void CommandIpc::ServerConversation::ReturnTargetPath ()
{
	GlobalId sourceGid;
	std::string sourcePath;
	{
		// Read source global id and source path
		MemoryDeserializer in (_convData->GetData (), _convData->GetDataSize ());
		sourceGid = in.GetLong ();
		SerString str (in, 0);
		sourcePath = str;
	}

	// Get target path and status
	std::string targetPath;
	unsigned long targetType = FileType ().GetValue ();
	unsigned long targetStatus = 0;
	_context.XGetCommander ().GetTargetPath (sourceGid,
											sourcePath,
											targetPath,
											targetType,
											targetStatus);

	// Save result
	CountingSerializer counter;
	SerString str (targetPath);
	str.Serialize (counter);
	counter.PutLong (targetType);
	counter.PutLong (targetStatus);
	Assert (!LargeInteger(counter.GetSize ()).IsLarge ());
	unsigned int size = static_cast<unsigned>(counter.GetSize ());
	MemorySerializer out (_convData->AllocBuf (size), size);
	str.Serialize (out);
	out.PutLong (targetType);
	out.PutLong (targetStatus);
	_convData->MakeAck ();
}

CommandIpc::InputSourceConnector::InputSourceConnector (Commander & commander, 
														InputSource * inputSource)
	: _commander (commander)
{
	_commander.SetInputSource (inputSource);
}

CommandIpc::InputSourceConnector::~InputSourceConnector ()
{
	_commander.SetInputSource (0);
}
