//-----------------------------------
//  (c) Reliable Software 2000 - 2008
//-----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "IpcConversation.h"
#include "GlobalLock.h"
#include "CoopExec.h"
#include "CoopCmdLine.h"
#include "OutputSink.h"
#include "Global.h"
#include "Serialize.h"
#include "SerString.h"
#include "SerVector.h"

#include <Dbg/Out.h>
#include <Win/Message.h>

ClientConversation::ClientConversation ()
	: _keepAliveTimeout (3 * OneSecond),
	  _stayInProject (true)
{}

bool ClientConversation::ExecuteCommand (FileListClassifier::ProjectFiles const * files, 
										 std::string const & cmdName, 
										 CmdOptions cmdOptions)
{
	dbg << "--> ClientConversation::ExecuteCommand" << std::endl;
	if (TheGlobalLock.IsNonZero ())
	{
		dbg << "<-- ClientConversation::ExecuteCommand - SCCDLL LOCKED" << std::endl;
		return false;
	}

	bool result = false;
	if (IsInConversation ())
	{
		dbg << "    Already in conversation." << std::endl;
		result = PostCmd (cmdName, files, cmdOptions);
	}
	else
	{
		dbg << "    Conversation not established yet." << std::endl;
		CoopFinder::InstanceType coopInstanceType = FindServer (files,
																cmdOptions.IsSkipGUICoopInProject ());
		if (coopInstanceType == CoopFinder::CoopNotFound)
		{
			result = false;
		}
		else if (coopInstanceType == CoopFinder::GuiCoopInProject && cmdOptions.IsSkipGUICoopInProject ())
		{
			// Ignore GUI Code Co-op -- pretend command was executed
			result = true;
		}
		else
		{
			result = PostCmd (cmdName, files, cmdOptions);
		}
	}

	dbg << "<-- ClientConversation::ExecuteCommand" << std::endl;
	return result;
}

// Start co-op not in project and execute command (new or join)
bool ClientConversation::ExecuteCommand (std::string const & cmd, 
										 CmdOptions cmdOptions)
{
	Assert (!IsInConversation ());
	if (TheGlobalLock.IsNonZero ())
	{
		dbg << "<-- ClientConversation::ExecuteCommand (2) - SCCDLL LOCKED" << std::endl;
		return false;
	}
	_exchange.MakeInvitation (-1, _myEvent.GetName ());
	{
		// Execute invisible co-op
		CoopExec coopExec;
		if (!coopExec.StartServer (_exchange.GetBufferName (), _keepAliveTimeout, _stayInProject))
			return false;
	}

	// Server found or executed
	dbg << "    Waiting for the server response"  << std::endl;
	bool success = _myEvent.Wait (10*OneSecond);
	dbg << "    Server responded -- conversation ";
	if (!success || !_exchange.IsAck ())
	{
		dbg << "REJECTED";
		_exchange.MakeErrorReport ("Server is busy -- request rejected");
		dbg << std::endl;
		return false;
	}
	std::string name (reinterpret_cast<char const *>(_exchange.GetReadData ()));
	dbg << "ACCEPTED" << std::endl;
	_serverEvent.reset (new Win::ExistingEvent (name));
	dbg << std::endl;

	return PostCmd (cmd, 0, cmdOptions);
}

bool ClientConversation::ExecuteCommandAndStayInGUI (FileListClassifier::ProjectFiles const * files, 
													 std::string const & cmdName, 
													 CmdOptions cmdOptions)
{
	dbg << "--> ClientConversation::ExecuteCommandAndStayInGUI" << std::endl;
	Assert (!IsInConversation ());
	if (TheGlobalLock.IsNonZero ())
	{
		dbg << "<-- ClientConversation::ExecuteCommandAndStayInGUI - SCCDLL LOCKED" << std::endl;
		return false;
	}
	std::string stayInGUIcmd ("GUI -");
	stayInGUIcmd += cmdName;
	bool result = false;
	CoopFinder::InstanceType coopInstanceType = FindServer (files);
	if (coopInstanceType != CoopFinder::CoopNotFound)
	{
		result = PostCmd (stayInGUIcmd, files, cmdOptions);
	}

	dbg << "<-- ClientConversation::ExecuteCommandAndStayInGUI" << std::endl;
	return result;
}

bool ClientConversation::Query (FileListClassifier::ProjectFiles const * files, 
								QueryType type)
{
	dbg << "--> ClientConversation::Query" << std::endl;

	bool result = false;
	CoopFinder::InstanceType coopInstanceType = FindServer (files);
	if (coopInstanceType != CoopFinder::CoopNotFound)
	{
		if (type == FileState)
		{
			_exchange.MakeStateRequest ();
		}
		else if (type == CurrentProjectVersion)
		{
			_exchange.MakeVersionIdRequest (true);
		}
		else
		{
			Assert (type == NextProjectVersion);
			_exchange.MakeVersionIdRequest (false);
		}
		result = PostRequest (files);
	}

	dbg << "<-- ClientConversation::Query" << std::endl;
	return result;
}

bool ClientConversation::Report (FileListClassifier::ProjectFiles const * files, 
								 GlobalId versionGid, 
								 std::string & buf)
{
	dbg << "--> ClientConversation::Report" << std::endl;

	bool result = false;
	CoopFinder::InstanceType coopInstanceType = FindServer (files);
	if (coopInstanceType != CoopFinder::CoopNotFound)
	{
		result = PostReport (files, versionGid, buf);
	}

	dbg << "<-- ClientConversation::Report" << std::endl;
	return result;
}

bool ClientConversation::QueryForkIds (FileListClassifier::ProjectFiles const * files,
									   bool deepForks,
									   GidList const & myForkIds,
									   GlobalId & youngestFoundScriptId,
									   GidList & targetYoungerForkIds)
{
	dbg << "--> ClientConversation::QueryForkIds" << std::endl;
	CoopFinder::InstanceType coopInstanceType = FindServer (files);
	if (coopInstanceType != CoopFinder::CoopNotFound)
	{
		_exchange.MakeForkIdsRequest (myForkIds, deepForks);
		_serverEvent->Release ();	// Wakeup server
		bool success = _myEvent.Wait (10*OneSecond);
		dbg << "    Posted fork ids request; server result:" << (success ? (_exchange.IsAck () ? "SUCCESS" : "FAILED") : "TIMEOUT") << std::endl;
		if (success && _exchange.IsAck ())
		{
			// Got data from server
			MemoryDeserializer in (_exchange.GetReadData (), _exchange.GetBufSize ());
			youngestFoundScriptId = in.GetLong ();
			SerVector<GlobalId> list;
			list.Deserialize (in, 0);
			targetYoungerForkIds.assign (list.begin (), list.end ());
			return true;
		}
	}
	dbg << "<-- ClientConversation::QueryForkIds" << std::endl;
	return false;
}

bool ClientConversation::QueryTargetPath (FileListClassifier::ProjectFiles const * files,
										  GlobalId gid,
										  std::string const & sourceProjectPath,
										  std::string & targetAbsolutePath,
										  unsigned long & targetType,
										  unsigned long & statusAtTarget)
{
	dbg << "--> ClientConversation::QueryTargetPath" << std::endl;
	CoopFinder::InstanceType coopInstanceType = FindServer (files);
	if (coopInstanceType != CoopFinder::CoopNotFound)
	{
		_exchange.MakeTargetPathRequest (gid, sourceProjectPath);
		_serverEvent->Release ();	// Wakeup server
		bool success = _myEvent.Wait (10*OneSecond);
		dbg << "    Posted fork ids request; server result:" << (success ? (_exchange.IsAck () ? "SUCCESS" : "FAILED") : "TIMEOUT") << std::endl;
		if (success && _exchange.IsAck ())
		{
			// Got data from server
			MemoryDeserializer in (_exchange.GetReadData (), _exchange.GetBufSize ());
			SerString path (in, 0);
			targetAbsolutePath = path;
			targetType = in.GetLong ();
			statusAtTarget = in.GetLong ();
			return true;
		}
	}
	dbg << "<-- ClientConversation::QueryTargetPath" << std::endl;
	return false;
}

char const * ClientConversation::GetErrorMsg ()
{
	if (_exchange.IsErrorMsg ())
		return reinterpret_cast<char const *>(_exchange.GetReadData ());

	return 0;
}

CoopFinder::InstanceType ClientConversation::FindServer (FileListClassifier::ProjectFiles const * files, 
														 bool skipGuiInProject)
{
	Assert (!IsInConversation ());
	Assert (files->GetProjectId () > 0);
	_exchange.MakeInvitation (files->GetProjectId (), _myEvent.GetName ());
	CoopFinder finder;
	CoopFinder::InstanceType coopInstanceType = finder.FindAnyCoop (files);
	Assert (coopInstanceType != CoopFinder::GuiCoopNotInProject);

	if (coopInstanceType == CoopFinder::GuiCoopInProject &&	skipGuiInProject)
	{
		dbg << "	ClientConversation found GUI Coop in Project" << std::endl;
		return CoopFinder::GuiCoopInProject;
	}

	if (coopInstanceType == CoopFinder::CoopNotFound)
	{
		CoopExec coopExec;
		if (coopExec.StartServer (_exchange.GetBufferName (), _keepAliveTimeout, _stayInProject))
			coopInstanceType = CoopFinder::CoopServer;
	}
	else
	{
		dbg << "    Found running server -- posting UM_IPC_INITIATE" << std::endl;
		Assert (coopInstanceType == CoopFinder::CoopServer || coopInstanceType == CoopFinder::GuiCoopInProject);
		Win::RegisteredMessage msgInitiate (UM_IPC_INITIATE);
		msgInitiate.SetLParam (_exchange.GetBufferId ());
		Win::Dow::Handle serverWin (finder.GetCoopWin ());
		serverWin.PostMsg (msgInitiate);
	}

	if (coopInstanceType != CoopFinder::CoopNotFound)
	{
		// Server found or executed
		dbg << "    Waiting for the server response" << std::endl;
		bool success = _myEvent.Wait (10*OneSecond);
		dbg << "    Server responded -- conversation ";
		if (success && _exchange.IsAck ())
		{
			std::string name (reinterpret_cast<char const *>(_exchange.GetReadData ()));
			dbg << "ACCEPTED" << std::endl;
			_serverEvent.reset (new Win::ExistingEvent (name));
		}
		else
		{
			dbg << "REJECTED";
			coopInstanceType = CoopFinder::CoopNotFound;
			_exchange.MakeErrorReport ("Server is busy -- request rejected");
		}
		dbg << std::endl;
	}
	return coopInstanceType;
}

bool ClientConversation::PostCmd (std::string const & cmdName, 
								  FileListClassifier::ProjectFiles const * files,
								  CmdOptions cmdOptions)
{
	CmdLine cmdLine (cmdName, files);
	_exchange.MakeCmd (cmdLine, cmdOptions.IsLastCommand ());
	_serverEvent->Release ();	// Wakeup server
	bool success;
	if (cmdOptions.IsNoCommnandTimeout ())
		success = _myEvent.Wait ();
	else
		success = _myEvent.Wait (15*OneSecond);
	dbg << "    Posted '" << cmdName << "'; server result:" << (success ? (_exchange.IsAck () ? "SUCCESS" : "FAILED") : "TIMEOUT") << std::endl;
	return success && _exchange.IsAck ();
}

bool ClientConversation::PostRequest (FileListClassifier::ProjectFiles const * files)
{
	// File state request layout in the shared memory buffer
	//
	//    +--------------+
	//    |      N       |  <-- file count
	//    +--------------+
	//    |              |
	//    .              .
	//    .              .  <-- N * sizeof (unsigned int) -- returned file state or version id is placed here
	//    .              .
	//    |              |
	//    +--------------+  <-- lenOff
	//    |              |
	//    .              .
	//    .              .  <-- N * sizeof (unsigned int) -- vector of file path lengths
	//    .              .
	//    |              |
	//    +--------------+ <-- pathOff
	//    |              |
	//    .              .
	//    .              .
	//    .              .  <-- N null terminated file paths 
	//    .              .
	//    .              .
	//    |              |
	//    +--------------+

	// Calculate buffer size
	unsigned int fileCount = files->GetFileCount ();
	unsigned int lenOff = (fileCount + 1) * sizeof (unsigned int);
	unsigned int pathOff = lenOff + fileCount * sizeof (unsigned int);
	unsigned int bufSize = pathOff;
	std::vector<unsigned int> pathLen;
	for (FileListClassifier::ProjectFiles::Sequencer seq (*files); !seq.AtEnd (); seq.Advance ())
	{
		char const * path = seq.GetFilePath ();
		unsigned int len = 0;
		if (path != 0)
			len = strlen (path);
		bufSize += len + 1;
		pathLen.push_back (len);
	}
	// Fill-in the buffer
	MemorySerializer out (_exchange.GetWriteData (bufSize), bufSize);
	out.PutLong (fileCount);
	unsigned int i;
	for (i = 0; i < fileCount; ++i)
		out.PutLong (0);
	for (i = 0; i < fileCount; ++i)
		out.PutLong (pathLen [i]);
	i = 0;
	for (FileListClassifier::ProjectFiles::Sequencer seq1 (*files);
	     !seq1.AtEnd ();
		 seq1.Advance (), ++i)
	{
		if (pathLen [i] > 0)
			out.PutBytes (seq1.GetFilePath (), pathLen [i]);
		out.PutByte (0);
	}
	Assert (_exchange.IsDataRequest ());
	_serverEvent->Release ();	// Wakeup server
	bool success = _myEvent.Wait (10*OneSecond);
	dbg << "    Posted request; server result:" << (success ? (_exchange.IsAck () ? "SUCCESS" : "FAILED") : "TIMEOUT") << std::endl;
	return success && _exchange.IsAck ();
}

bool ClientConversation::PostReport (FileListClassifier::ProjectFiles const * files, 
									 GlobalId versionGid, 
									 std::string & buf)
{
	_exchange.MakeReportRequest (versionGid);
	_serverEvent->Release ();	// Wakeup server
	bool success = _myEvent.Wait (10*OneSecond);
	dbg << "    Posted report request; server result:" << (success ? (_exchange.IsAck () ? "SUCCESS" : "FAILED") : "TIMEOUT") << std::endl;
	if (success && _exchange.IsAck ())
	{
		char const * report = reinterpret_cast<char const *> (_exchange.GetReadData ());
		buf.assign (report);
	}
	return success && _exchange.IsAck ();
}
