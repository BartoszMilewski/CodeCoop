#if !defined (ACTIVETRANSPORT_H)
#define ACTIVETRANSPORT_H
//----------------------------------
//  (c) Reliable Software, 2009
//----------------------------------
#include "Transport.h"
#include "ScriptStatus.h"
#include "Email.h"
#include "ScriptInfo.h"
#include <Sys/Active.h>
#include <Sys/Channel.h>

class ScriptTicket;
class HtoScriptMap;
class ScriptFileList;
class Mailer;

typedef int ScriptHandle;

class LanCopierReadyStore;
class EmailCopierReadyStore;

// Base classes

template<class Message, class Request, class MessageStore>
class ActiveCopier: public ActiveObject
{
public:
	ActiveCopier(bool goesToHub, MessageStore & copierReadyStore, ChannelSync & sync)
		: _goesToHub(goesToHub),
		_isClogged(false),
		_syncOut(sync),
		_copierReadyStore(copierReadyStore),
		_busy(false)
	{}
	void FlushThread () { _syncIn.Release(); }

	bool IsIdle() const { return !_busy; }
	bool IsClogged() const { return _isClogged; }
	bool HasWork() const { return !_requests.empty(); }
	void Clog() { _isClogged = true; }
	void Unclog() { _isClogged = false; }
	bool ClearRequests();
	void UpdateStatuses(HtoScriptMap const & scriptMap, ScriptFileList * scriptFileList, ScriptStatus::Dispatch::Bits status);
protected:
	// Immutable state
	bool				_goesToHub;
	// State operated by parent thread
	bool				_isClogged;
	std::list<Request>	_requests;
	bool				_busy;
	// Messages to parent thread
	ChannelSync &		_syncOut;
	MessageStore &		_copierReadyStore;
	// Messages from parent thread 
	ChannelSync			_syncIn;
	ValueStore<Message>	_startCopy;
};

template<class Request>
class IsSmallerScript : public std::binary_function<Request const &, Request const &, bool>
{
public:
	IsSmallerScript(HtoScriptMap const & scriptMap)
		: _scriptMap(scriptMap)
	{}
	bool operator()(Request const & r1, Request const & r2)
	{
		ScriptTicket const * script1 = _scriptMap.Get(r1._scriptHandle);
		ScriptTicket const * script2 = _scriptMap.Get(r2._scriptHandle);
		if (script1 == 0)
		{
			dbg << "    No script for handle " << r1._scriptHandle << std::endl;
			return false;
		}
		if (script2 == 0)
		{
			dbg << "    No script for handle " << r1._scriptHandle << std::endl;
			return true;
		}
		return script1->GetPartCount() < script2->GetPartCount() || script1->GetSize() < script2->GetSize();
	}
private:
	HtoScriptMap const & _scriptMap;
};

template<class Message, class Request, class MessageStore>
inline void ActiveCopier<Message, Request, MessageStore>::UpdateStatuses(HtoScriptMap const & scriptMap, ScriptFileList * scriptFileList, ScriptStatus::Dispatch::Bits status)
{
	std::list<Request>::const_iterator reqIt = _requests.begin ();
	for (; reqIt != _requests.end (); ++reqIt)
	{
		ScriptHandle h = reqIt->_scriptHandle;
		ScriptTicket const * script = scriptMap.Get(h);
		if (script != 0)
			scriptFileList->SetUiStatus(script->GetName(), status);
	}
}

template<class Message, class Request, class MessageStore>
bool ActiveCopier<Message, Request, MessageStore>::ClearRequests()
{
	// Copy thread has a copy of current requests
	_requests.clear();
	return IsIdle();
}

// immutable
struct BaseRequest // Value
{
	bool operator==(BaseRequest const & other) const
	{
		return _scriptHandle == other._scriptHandle;
	}
	ScriptHandle	_scriptHandle;
	bool			_isVerbose;
};

//----------------
// ActiveLanCopier
//----------------

// immutable
struct LanRequest: public BaseRequest
{
	bool _isRetry;
};

// Sent to copier thread
struct LanMessage // Value
{
	LanMessage() {}
	LanMessage(std::string path, bool isFwd, LanRequest const & request)
		: _path(path), _isFwd(isFwd), _request(request)
	{}
	std::string	_path;
	bool		_isFwd;
	LanRequest	_request;
};

class ActiveLanCopier: public ActiveCopier<LanMessage, LanRequest, LanCopierReadyStore>
{
	friend std::ostream& operator<<(std::ostream& os, ActiveLanCopier const & a);
	typedef ActiveCopier<LanMessage, LanRequest, LanCopierReadyStore> Parent;
public:
	ActiveLanCopier(Transport const & trans, bool goesToHub, LanCopierReadyStore & copierReadyStore, ChannelSync & sync);
	void AddRequest( 
		ScriptHandle scriptHandle, 
		bool isVerbose, 
		bool isRetry);
	bool CopyNextScript(HtoScriptMap const & scriptMap);
	Transport const & GetTransport() const { return _transport; }
	void Unclog();
	void RequestDone(ScriptHandle h, bool success);
private:
	LanRequest const * PickNextRequest(HtoScriptMap const & scriptMap);
	// Called only from copier thread
    void Run();
	void DoCopy(std::string const & srcPath, bool isFwd, LanRequest const & request);
private:
	// Immutable state
	Transport	_transport;
};

//------------------
// ActiveEmailCopier
//------------------

// immutable
struct EmailRequest: public BaseRequest // Value
{
	std::vector<std::string>	_addrVector;
};

// All the data needed for emailing one script
struct EmailMsgInfo // Value
{
	EmailMsgInfo(EmailRequest const & request, ScriptAbstract const & scriptAbstract) 
		: _request(request), 
		_scriptAbstract(scriptAbstract),
		_success(false)
	{}
	// immutable
	EmailRequest 	_request;
	ScriptAbstract 	_scriptAbstract;
	// mutable
	bool 			_success;
	std::string 	_badAddress;
};

typedef std::vector<EmailMsgInfo> EmailInfoList;

// Sent to copier thread
struct EmailMessage // Value
{
	EmailMessage() {}
	EmailMessage(EmailInfoList & emailInfoList)
	{
		_emailInfoList.swap(emailInfoList);
	}
	EmailInfoList	_emailInfoList;
};

class ActiveEmailCopier: public ActiveCopier<EmailMessage, EmailRequest, EmailCopierReadyStore>
{
	friend std::ostream& operator<<(std::ostream& os, ActiveEmailCopier const & a);
	typedef ActiveCopier<EmailMessage, EmailRequest, EmailCopierReadyStore> Parent;
public:
	ActiveEmailCopier(bool goesToHub, EmailCopierReadyStore & copierReadyStore, ChannelSync & sync);
	// AddRequest may be called concurrently
	void AddRequest( 
		ScriptHandle scriptHandle,
		std::vector<std::string> const & addrVector,
		bool isVerbose);

	bool CopyScripts(HtoScriptMap const & scriptMap);
	void RequestDone(EmailMsgInfo const & ticket);
private:
	void Run();
	void DoEmail(EmailInfoList & emailInfoList);
	bool EmailScript(Mailer & mailer, EmailMsgInfo & emailInfo, bool & isFirstError, bool isVerbose);
};

#endif
