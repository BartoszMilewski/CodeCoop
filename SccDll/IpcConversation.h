#if !defined (IPCCONVERSATION_H)
#define IPCCONVERSATION_H
//-----------------------------------
//  (c) Reliable Software 2000 - 2008
//-----------------------------------

#include "IpcExchange.h"
#include "FileClassifier.h"
#include "FileStateList.h"
#include "CoopFinder.h"
#include "GlobalId.h"

#include <Sys/Synchro.h>
#include <Bit.h>

class ReportBuf;

class ClientConversation
{
	friend class StatusSequencer;

public:
	enum QueryType
	{
		FileState = 0,
		CurrentProjectVersion = 1,
		NextProjectVersion = 2
	};

public:
	class CmdOptions
	{
	public:
		CmdOptions ()
		{
			_options.init (0);
			SetLastCommand (true);
		}

		void SetLastCommand (bool bit) { _options.set (LastCommand, bit); }
		void SetSkipGUICoopInProject (bool bit) { _options.set (SkipGUICoopInProject, bit); }
		void SetNoCommandTimeout (bool bit) { _options.set (NoCommandTimeout, bit); }

		bool IsLastCommand () const { return _options.test (LastCommand); }
		bool IsSkipGUICoopInProject () const { return _options.test (SkipGUICoopInProject); }
		bool IsNoCommnandTimeout () const { return _options.test (NoCommandTimeout); }

	private:
		enum OptionBits
		{
			LastCommand,
			SkipGUICoopInProject,
			NoCommandTimeout
		};

	private:
		BitSet<OptionBits>	_options;
	};

public:
	ClientConversation ();

	void SetKeepAliveTimeout (unsigned int timeout) { _keepAliveTimeout = timeout; }
	void SetStayInProject (bool flag) { _stayInProject = flag; }

	bool ExecuteCommand (FileListClassifier::ProjectFiles const * files,
						 std::string const & cmdName,
						 CmdOptions cmdOptions = CmdOptions ());
	bool ExecuteCommand (std::string const & cmd,
						 CmdOptions cmdOptions = CmdOptions ());
	bool ExecuteCommandAndStayInGUI (FileListClassifier::ProjectFiles const * files,
									 std::string const & cmdName,
									 CmdOptions cmdOptions = CmdOptions ());
	bool Query (FileListClassifier::ProjectFiles const * files,
				QueryType type = FileState);
	bool Report (FileListClassifier::ProjectFiles const * files,
				 GlobalId versionGid,
				 std::string & buf);
	bool QueryForkIds (FileListClassifier::ProjectFiles const * files,
					   bool deepForks,
					   GidList const & myForkIds,
					   GlobalId & youngestFoundScriptId,
					   GidList & targetYoungerForkIds);
	bool QueryTargetPath (FileListClassifier::ProjectFiles const * files,
						  GlobalId gid,
						  std::string const & sourceProjectPath,
						  std::string & targetAbsolutePath,
						  unsigned long & targetType,
						  unsigned long & statusAtTarget);
	char const * GetErrorMsg ();

private:
	enum
	{
		OneSecond = 1000	// One second in milliseconds
	};

private:
	bool IsInConversation () const { return _serverEvent.get () != 0; }
	CoopFinder::InstanceType FindServer (FileListClassifier::ProjectFiles const * files,
										 bool skipGuiInProject = false);
	bool PostCmd (std::string const & cmdName,
				  FileListClassifier::ProjectFiles const * files = 0,
				  CmdOptions cmdOptions = CmdOptions ());
	bool PostRequest (FileListClassifier::ProjectFiles const * files);
	bool PostReport (FileListClassifier::ProjectFiles const * files, GlobalId versionGid, std::string & Buf);

private:
	IpcExchangeBuf				_exchange;
	Win::NewEvent				_myEvent;
	std::unique_ptr<Win::Event>	_serverEvent;
	unsigned int				_keepAliveTimeout;
	bool						_stayInProject;
};

class StatusSequencer
{
public:
	StatusSequencer (ClientConversation & conversation)
		: _list (conversation._exchange.GetReadData ()),
		  _iter (_list)
	{}


	void Advance () { _iter.Advance (); }
	bool AtEnd () const { return _iter.AtEnd (); }

	unsigned int GetState () const { return _iter.GetState (); }

private:
	FileStateList					_list;
	FileStateList::ConstIterator	_iter;
};

#endif
