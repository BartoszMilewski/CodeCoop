#if !defined (TRANSPORTMANAGER_H)
#define TRANSPORTMANAGER_H
//----------------------------------
//  (c) Reliable Software, 2009
//----------------------------------
#include "Transport.h"
#include "ScriptManager.h"
#include "ActiveTransport.h"
#include "ScriptInfo.h"

class ConfigData;
class Mailer;

enum IgnoreAction
{
	ignoreNo,      // not on ignored list
	ignoreRetry,   // another dispatching attempt
	ignoreNetwork,
	ignoreDisk
};

class IgnoredDestinations
{
public:
    void Insert (Transport const & trans, IgnoreAction action) { _transports [trans] = action; }
	void Remove (Transport const & trans) { _transports.erase (trans); }
    IgnoreAction Find (Transport const & trans) const;
	void Refresh ();
    void Clear () { _transports.clear (); }
private:
	typedef std::map<Transport, IgnoreAction>::iterator iterator;
	typedef std::map<Transport, IgnoreAction>::const_iterator const_iterator;
private:
	std::map<Transport, IgnoreAction> _transports;
};

class HtoScriptMap
{
	friend std::ostream& operator<<(std::ostream& os, HtoScriptMap const & m);
public:
	HtoScriptMap(): _curHandle(0) {}
	bool IsEmpty() const { return _tickets.size() == 0; }
	ScriptTicket * Get(ScriptHandle h)
	{
		Map::iterator it = _map.find(h);
		if (it != _map.end())
			return it->second;
		else
			return 0;
	}
	ScriptTicket const * Get(ScriptHandle h) const
	{
		Map::const_iterator it = _map.find(h);
		if (it != _map.end())
			return it->second;
		else
			return 0;
	}
	ScriptHandle Insert(std::unique_ptr<ScriptTicket> ticket)
	{
		Assert (!Find(ticket.get()));
		ScriptHandle h = ++_curHandle;
		_tickets.push_back(std::move(ticket));
		_map[h] = _tickets.back();
		return h;
	}
	bool Find(ScriptTicket const * script) const;
	std::unique_ptr<ScriptTicket> Remove(ScriptTicket const & script);
	void RefreshRequests();
	void clear() 
	{
		_tickets.clear();
		_map.clear(); 
	}

private:
	ScriptHandle _curHandle;
	ScriptVector _tickets;
	typedef std::map<ScriptHandle, ScriptTicket*> Map;
	Map _map;
};

class EmailReadyMessage
{
};

class ActiveLanCopier;

class LanCopierReadyMessage
{
public:
	LanCopierReadyMessage(ActiveLanCopier * copier, ScriptHandle handle, bool success, bool driveNotReady)
		: _copier(copier), _handle(handle), _success(success), _driveNotReady(driveNotReady)
	{}
	ActiveLanCopier * GetCopier() { return _copier; }
	ScriptHandle GetScriptHandle() const { return _handle; }
	bool IsDriveNotReady() const { return _driveNotReady; }
	bool IsSuccess() const { return _success; }
private:
	ActiveLanCopier *	_copier;
	bool				_driveNotReady;
	ScriptHandle		_handle;
	bool				_success;
};

class LanCopierReadyStore
{
	typedef LanCopierReadyMessage Message;
public:
	void Put(Message const & msg)
	{
		_copiersDone.push_back(msg);
	}
	Message Get()
	{
		Assert (!_copiersDone.empty());
		Message msg = _copiersDone.back();
		_copiersDone.pop_back();
		return msg;
	}
	bool IsEmpty() const
	{
		return _copiersDone.empty();
	}
	void clear()
	{
		_copiersDone.clear();
	}
	void swap(std::vector<Message> & target)
	{
		_copiersDone.swap(target);
	}
private:
	std::vector<Message> _copiersDone;
};

template<class Fun>
class LanCopierReadyWrap: public WrappedChannel
{
public:
	LanCopierReadyWrap(LanCopierReadyStore & store, Fun f)
		: _store(store), _f(f)
	{}
	// Must be called under GetLock
	bool Peek()
	{
		return !_store.IsEmpty();
	}
	WrappedChannel * Receive()
	{
		if (_store.IsEmpty())
			return 0;
		_copiersDone.clear();
		_store.swap(_copiersDone);
		return this;
	}
	// Called outside of the lock
	void Execute()
	{
		for (std::vector<LanCopierReadyMessage>::iterator it = _copiersDone.begin(); it != _copiersDone.end(); ++it)
		{
			_f(*it);
		}
	}
private:
	LanCopierReadyStore & _store;
	Fun _f;
	std::vector<LanCopierReadyMessage> _copiersDone;
};

class EmailCopierReadyMessage
{
public:
	EmailCopierReadyMessage(ActiveEmailCopier * copier = 0) 
		: _copier(copier), 
		  _generalFailure(false), 
		  _isTimedOut(false) 
	{}
	EmailCopierReadyMessage(
		ActiveEmailCopier * copier, 
		EmailInfoList & emailInfoList, 
		bool generalFailure = false, 
		bool isTimedOut = false)
		: _copier(copier),
		  _generalFailure(generalFailure),
		  _isTimedOut(isTimedOut)
	{
		_emailInfoList.swap(emailInfoList);
	}
	bool IsGeneralFailure() const { return _generalFailure; }
	bool IsTimedOut() const { return _isTimedOut; }
	ActiveEmailCopier * GetCopier() { return _copier; }
	EmailInfoList::const_iterator begin() const { return _emailInfoList.begin(); }
	EmailInfoList::const_iterator end() const { return _emailInfoList.end(); }
private:
	ActiveEmailCopier * _copier;
	EmailInfoList _emailInfoList;
	bool _generalFailure;
	bool _isTimedOut;
};

class EmailCopierReadyStore: public ValueStore<EmailCopierReadyMessage>
{
};

template<class Fun>
class EmailCopierReadyWrap: public WrappedChannel
{
public:
	EmailCopierReadyWrap(EmailCopierReadyStore & store, Fun f)
		: _store(store), _f(f)
	{}
	// Must be called under GetLock
	bool Peek()
	{
		return !_store.IsEmpty();
	}
	WrappedChannel * Receive()
	{
		if (_store.IsEmpty())
			return 0;
		_msg = _store.Get();
		return this;
	}
	// Called outside of the lock
	void Execute()
	{
		_f(_msg);
	}
private:
	EmailCopierReadyStore & _store;
	Fun _f;
	EmailCopierReadyMessage _msg;
};

class TransportManager
{
	friend std::ostream& operator<<(std::ostream& os, TransportManager const & a);
	typedef std::map<ScriptHandle, NocaseSet> ScriptToEmails;

	class ActiveCopiers
	{
		friend std::ostream& operator<<(std::ostream& os, TransportManager const & t);
	public:
		ActiveCopiers(bool goesToHub, ChannelSync & sync) : _goesToHub(goesToHub), _sync(sync) {}
		ActiveLanCopier * GetLanCopier(Transport const & trans);
		ActiveEmailCopier * GetEmailCopier();
		void StartCopying(HtoScriptMap const & scriptMap);
		void RemoveTransport(Transport const & trans);
		void RemoveEmail();
		LanCopierReadyStore & GetLanCopierReadyStore() { return _readyCopiersStore; }
		EmailCopierReadyStore & GetEmailCopierReadyStore() { return _readyEmailCopierStore; }
		void Unclog();
		void ClearRequests();
	private:
		typedef std::map<Transport, ActiveLanCopier*>::const_iterator iterator;
		typedef auto_vector<auto_active<ActiveLanCopier> >::iterator vector_iterator;
	private:
		auto_vector<auto_active<ActiveLanCopier> >		_lanCopiers;
		auto_active<ActiveEmailCopier>					_emailCopier;
		bool 											_goesToHub;
		std::map<Transport, ActiveLanCopier*>			_transportMap;
		LanCopierReadyStore								_readyCopiersStore;
		EmailCopierReadyStore							_readyEmailCopierStore;
		ChannelSync &									_sync;
	};
public:
	TransportManager(Win::Dow::Handle winParent, ConfigData const & config, ChannelSync & sync);
	void SetScriptFileList(ScriptFileList * scriptFileList)
	{
		_scriptFileList = scriptFileList;
	}
	LanCopierReadyStore & GetLanCopierReadyStore() { return _copiers.GetLanCopierReadyStore(); }
	EmailCopierReadyStore & GetEmailCopierReadyStore() { return _copiers.GetEmailCopierReadyStore(); }
	bool HasWork() const { return !_scriptsInProgress.IsEmpty(); }
	// Moves scripts from toDoList to _scriptsInProgress, returns new handles
	void TransferToDoList(ScriptVector & toDoList, std::vector<ScriptHandle> & toDoHandles);
	void Distribute(std::vector<ScriptHandle> & toDoList, ScriptManager & scriptMan);
	void StartForwarding(std::vector<ScriptHandle> & toDoList, ScriptManager & scriptMan);
	void StartEmailing(std::vector<ScriptHandle> & toDoList, ScriptManager & scriptMan);
	void PostForwardRequest(
		ScriptManager & scriptMan,
		ScriptHandle handle, 
		Transport const & transport, 
		IgnoreAction action);
	void PostEmailRequest(
		ScriptManager & scriptMan,
		ScriptHandle handle, 
		std::vector<std::string> const & addrVector);
	void OnLanCopyDone(LanCopierReadyMessage & msg, ScriptManager & scriptMan);
	void OnEmailCopyDone(EmailCopierReadyMessage & msg, ScriptManager & scriptMan);
	void IgnorePath (Transport const & transport);
	void ScriptProgressMade (ScriptManager & scriptMan, ScriptTicket & script);
	static void StampFwdFlag (std::string const & scriptPath, bool isFwd);

	void UnIgnorePath (Transport const & transport);
	void Ignore(Transport const & trans, IgnoreAction action)
	{
		_ignoredDestinations.Insert(trans, action);
	}
	void RefreshIgnored()
	{
		_ignoredDestinations.Refresh ();
	}
	void ClearAll();
	void ClearIgnored()
	{
		_ignoredDestinations.Clear ();
	}
	IgnoreAction IsIgnoredDest (Transport const & transport)
	{
		return _ignoredDestinations.Find (transport);
	}
	bool IsTemporaryHub() const
	{
		return _topology.IsTemporaryHub ();
	}
	bool IsVerbose() const
	{
		return _isVerbose;
	}
	void SetVerbose(bool isVerbose)
	{
		_isVerbose = isVerbose;
	}
	void Reconfigure(Topology const & topology, Transport const & transportToHub)
	{
		_topology = topology;
		_hubTransport = transportToHub;
	}
	FilePath const & GetPublicInboxPath() const { return _publicInboxPath; }
private:
	void MapEmailsInScripts(std::vector<ScriptHandle> & toDoList, ScriptToEmails & scriptList);
private:
	Win::Dow::Handle		_winParent;
	ScriptFileList *		_scriptFileList; // shared monitor
	Topology				_topology;
	Transport				_hubTransport;
	bool					_goesToHub;
	bool					_isVerbose;
	FilePath				_publicInboxPath;
	// Main depository of scripts
	HtoScriptMap			_scriptsInProgress;

	ActiveCopiers			_copiers;

	// temporarily ureachable paths
    IgnoredDestinations		_ignoredDestinations;
	bool					_ignoreEmailErrors;
};
#endif
