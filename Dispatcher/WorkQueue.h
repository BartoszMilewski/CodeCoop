#if !defined (WORKQUEUE_H)
#define WORKQUEUE_H
//--------------------------------
// (c) Reliable Software 2000 - 09
//--------------------------------

#include "ScriptStatus.h"
#include "ScriptInfo.h"
#include "Transport.h"
#include "TransportManager.h"
#include "ScriptManager.h"

#include <Win/Win.h>
#include <File/Path.h>
#include <Sys/Synchro.h>
#include <Sys/Active.h>
#include <Sys/Channel.h>

class ScriptFileList;
class ConfigData;
class Mailer;

class WorkQueue: public ActiveObject, public ScriptManager
{
	friend std::ostream& operator<<(std::ostream& os, WorkQueue & m);
//private:
public: // temporarily for Wrapper to use
	class Timeout
	{
	public:
		Timeout(): _timeout(BaseTimeoutPeriod) {}
		int Get() const
		{
			return _timeout;
		}
		void Reset()
		{
			_timeout = BaseTimeoutPeriod;
		}
		void Double()
		{
			_timeout *= 2;
			if (_timeout > MaxTimeoutPeriod)
				_timeout = MaxTimeoutPeriod;
		}
		void SetMax()
		{
			_timeout = MaxTimeoutPeriod;
		}
	private:
		int						_timeout;
		static const int BaseTimeoutPeriod = 5 * 1000; // 5 sec
		static const int MaxTimeoutPeriod = 100 * 1000;
	};

	class ConfigChangeMsg
	{
	public:
		ConfigChangeMsg() {}
		ConfigChangeMsg(Topology topology, Transport transportToHub)
			: _topology(topology), _transportToHub(transportToHub)
		{}

		Topology 	_topology;
		Transport	_transportToHub;
	};

	friend class ClearStateHandler;
	friend class ConfigChangeHandler;
public:
	WorkQueue (ConfigData const & config, ScriptFileList * scriptFileList, Win::Dow::Handle winParent);
	~WorkQueue ();
	void GetNewAndMarkOldScripts(std::vector<std::string> & newFiles);
	void TransferRequests (ScriptVector & scriptTicketList);
	void TransferDoneScripts (ScriptVector & doneList);
	// Messaging
	void ForceScattering ();
	void ChangeConfig (ConfigData const & config);
	void ClearEmailBlacklist ();
	void ResetScriptFileList(ScriptFileList * scriptFileList);

	void UnIgnore ();
	void ClearState ();
	// ScriptManager
	void ForgetScript(std::string const & scriptName);
	void FinishScript(std::unique_ptr<ScriptTicket> script);
	void ResetTimeout();
	ChannelSync & GetSync() { return _sync; }
	// Active Object
    void Run ();
    void FlushThread () { _sync.Release(); }
private:
	void AcquireToDoList(ScriptVector & toDoList);
	// Inter-thread communication

	void IgnorePath (TransportManager & transMan, Transport const & fwdPath);
	void UnIgnorePath (TransportManager & transMan, Transport const & fwdPath, bool isIgnored);
	void DoClearState();

private:
	Win::Dow::Handle		_winParent;
	mutable Win::Mutex		_mutex;
	mutable Win::Event		_eventDone;

	// Elements of the message queue
	mutable ChannelSync		_sync;
	BoolStore				_resetTimeout;
	BoolStore				_clearState;
	BoolStore				_clearBlacklist;
	BoolStore				_unIgnore;
	BoolStore				_scatteringRequest;
	ValueStore<ScriptFileList*>	_scriptFileListStore;
	ValueStore<ConfigChangeMsg> _configChangeStore;
	std::unique_ptr<TransportManager> _transMan;

	ScriptFileList *		_scriptFileList;

	std::set<std::string>	_knownScriptFiles;
	// Only for passing to and from the worker queue
	ScriptVector	_toDoList;
	ScriptVector	_finished;
};

#endif
