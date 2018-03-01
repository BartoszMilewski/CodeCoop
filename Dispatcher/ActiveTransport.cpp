//----------------------------------
//  (c) Reliable Software, 2009-10
//----------------------------------
#include "precompiled.h"
#undef DBG_LOGGING
#define DBG_LOGGING true
#include "ActiveTransport.h"
#include "ScriptInfo.h"
#include "FolderMan.h"
#include "AlertMan.h"
#include "TransportManager.h"
#include "ScriptFileList.h"
#include "EmailMan.h"
#include "Processor.h"
#include "ScriptSubject.h"
#include "EmailMessage.h"
#include "Registry.h"
#include "AppHelp.h"
#include "BadEmailExpt.h"
#include "FileLocker.h"
#include <File/SafePaths.h>
#include <Com/Shell.h>
#include <Net/Socket.h>

//----------------
// ActiveLanCopier
//----------------

ActiveLanCopier::ActiveLanCopier(Transport const & trans, bool goesToHub, LanCopierReadyStore & copierReadyStore, ChannelSync & sync)
	: ActiveCopier<LanMessage, LanRequest, LanCopierReadyStore>(goesToHub, copierReadyStore, sync),
	_transport(trans)
{
}

void ActiveLanCopier::Run()
{
	RcvChannel<LanMessage, ValueStore> chan(_syncIn, _startCopy);
	LanMessage msg;
	for(;;)
	{
		_syncIn.Wait();
		if (IsDying())
			break;
		{
			GetLock lock(_syncIn);
			msg = _startCopy.Get();
		}
		try {
			DoCopy(msg._path, msg._isFwd, msg._request);
		} 
		catch(...)
		{
			dbg << "ActiveLanCopier: unexpected error" << std::endl;
		}
	}
	dbg << " ===> Closing Lan Copier: " << std::endl;
	dbg << *this << std::endl;
}

void ActiveLanCopier::AddRequest(ScriptHandle scriptHandle, 
								 bool isVerbose, 
								 bool isRetry)
{
	LanRequest req;
	req._scriptHandle = scriptHandle;
	req._isVerbose = isVerbose;
	req._isRetry = isRetry;

	_requests.push_back(req);
	dbg << "   AddRequest: " << scriptHandle << " -> " << _transport.GetRoute() << std::endl;
}

bool ActiveLanCopier::CopyNextScript(HtoScriptMap const & scriptMap)
{
	if (!HasWork())
		return false;
	dbg << "ActiveLanCopier::CopyNextScript to " << _transport.GetRoute() << std::endl;
	LanRequest const * bestRequest = PickNextRequest(scriptMap);
	ScriptHandle h = bestRequest->_scriptHandle;

	ScriptTicket const * script = scriptMap.Get(h);
	Assume(script != 0, "PickNextRequest returned obsolete handle");

	dbg << "    -> " << script->GetName() << std::endl;
	_busy = true;
	SendChannel<LanMessage, ValueStore> chan(_syncIn, _startCopy);
	chan.Put(LanMessage(script->GetPath(), script->ToBeFwd(), *bestRequest));
	return true;
}

LanRequest const * ActiveLanCopier::PickNextRequest(HtoScriptMap const & scriptMap)
{
	dbg << "-->ActiveLanCopier::PickNextRequest" << std::endl;

	std::list<LanRequest>::const_iterator reqIt = _requests.begin ();
	Assert (reqIt != _requests.end());
	LanRequest const * bestRequest = &*reqIt;
	++reqIt;
	IsSmallerScript<LanRequest> isSmallerScript(scriptMap);
	while (reqIt != _requests.end ())
	{
		if (isSmallerScript(*reqIt, *bestRequest))
			bestRequest = &*reqIt;
		++reqIt;
	}
	return bestRequest;
}

void ActiveLanCopier::DoCopy(std::string const & srcPath, bool isFwd, LanRequest const & request)
{
	PathSplitter pathSplitter(srcPath);
	std::string fileName = pathSplitter.GetFileName();
	fileName += pathSplitter.GetExtension();

	// set forward flag
	// do it on temporary copy (if forwarding fails we stay with untouched script)
	std::string tmpSourcePath = TheFileLocker.AcquireFileCopy(srcPath, fileName, isFwd, _goesToHub);
	if (tmpSourcePath.empty())
	{
		TheAlertMan.PostInfoAlert ("Temporary copy of a script could not be made.", 
							request._isVerbose,
							false,
							"Check your TMP directory");

		LanCopierReadyMessage msg(this, request._scriptHandle, false, true); // pretend drive not ready?
		PutLock lock(_syncOut);
		_copierReadyStore.Put(msg);
		return;
	}

	bool driveNotReady = false;
	bool isSuccess = false;

	// releases reference to file copy in its destructor
	FileLocker::Guard guard(TheFileLocker, fileName);  

	try
	{
		isSuccess = FolderMan::CopyMaterialize (
			tmpSourcePath.c_str (), 
			_transport.GetRoute (), 
			fileName.c_str(), 
			driveNotReady, 
			request._isRetry); // don't alert if it's a retry
		dbg << "    Copy of " << fileName << " -> " << _transport.GetRoute() << ": " << (isSuccess? "successful": "failed") << std::endl;
	}
	catch (Win::Exception e)
	{
		TheAlertMan.PostInfoAlert (e, request._isVerbose);
	}
	catch (...)
	{
		Win::ClearError ();
		TheAlertMan.PostInfoAlert ("Unknown error during script forwarding.", request._isVerbose);
	}

	LanCopierReadyMessage msg(this, request._scriptHandle, isSuccess, driveNotReady);
	PutLock lock(_syncOut);
	_copierReadyStore.Put(msg);
}

void ActiveLanCopier::Unclog()
{
	std::list<LanRequest>::iterator reqIt = _requests.begin ();
	for (; reqIt != _requests.end (); ++reqIt)
		reqIt->_isRetry = true;
	Parent::Unclog();
}

class IsSameHandle: public std::unary_function<BaseRequest const &, bool>
{
public:
	IsSameHandle(ScriptHandle h) : _h(h)
	{}
	bool operator()(BaseRequest const & req)
	{
		return req._scriptHandle == _h;
	}
private:
	ScriptHandle _h;
};

void ActiveLanCopier::RequestDone(ScriptHandle h, bool success)
{
	if (!success)
		Clog();
	if (!_isClogged)
	{
		std::list<LanRequest>::iterator it = 
			std::find_if(_requests.begin(), _requests.end(), IsSameHandle(h));
		if (it != _requests.end())
		{
			dbg << "ActiveLanCopier::RequestDone, handle: " << h << " -> " << _transport.GetRoute() << std::endl;
			_requests.erase(it);
		}
	}
	_busy = false;
}

//------------------
// ActiveEmailCopier
//------------------

ActiveEmailCopier::ActiveEmailCopier(bool goesToHub, 
									 EmailCopierReadyStore & copierReadyStore, 
									 ChannelSync & sync)
	: ActiveCopier<EmailMessage, EmailRequest, EmailCopierReadyStore>(goesToHub, copierReadyStore, sync)
{
}

void ActiveEmailCopier::Run()
{
	RcvChannel<EmailMessage, ValueStore> chan(_syncIn, _startCopy);
	EmailMessage msg;
	for(;;)
	{
		_syncIn.Wait();
		if (IsDying())
			break;
		{
			GetLock lock(_syncIn);
			msg = _startCopy.Get();
		}
		try {
			DoEmail(msg._emailInfoList);
		} catch(...) 
		{
			dbg << "ActiveEmailCopier: unexpected error" << std::endl;
		}
	}
}

void ActiveEmailCopier::AddRequest( ScriptHandle scriptHandle, 
									std::vector<std::string> const & addrVector,
									bool isVerbose)
{
	EmailRequest req;
	req._scriptHandle = scriptHandle;
	req._addrVector = addrVector;
	req._isVerbose = isVerbose;

	_requests.push_back(req);
}

class IsSmallerEmailMsgInfo : public std::binary_function<EmailMsgInfo const &, EmailMsgInfo const &, bool>
{
public:
	IsSmallerEmailMsgInfo(HtoScriptMap const & scriptMap)
		: _lessRequest(scriptMap)
	{}
	bool operator()(EmailMsgInfo const & t1, EmailMsgInfo const & t2)
	{
		return _lessRequest(t1._request, t2._request);
	}
private:
	IsSmallerScript<EmailRequest> _lessRequest;
};

bool ActiveEmailCopier::CopyScripts(HtoScriptMap const & scriptMap)
{
	if (_requests.empty())
		return false;
	dbg << "ActiveEmailCopier::CopyScripts" << std::endl;
	EmailInfoList emailList;
	std::list<EmailRequest>::const_iterator it = _requests.begin(); 
	while (it != _requests.end())
	{
		ScriptTicket const * script = scriptMap.Get(it->_scriptHandle);
		if (script == 0)
		{
			it = _requests.erase(it);
			continue;
		}
		dbg << "    + " << script->GetName() << std::endl;
		emailList.push_back(EmailMsgInfo(*it, ScriptAbstract(script->GetInfo())));
		++it;
	}
	if (emailList.empty())
		return false;

	std::sort(emailList.begin(), emailList.end(), IsSmallerEmailMsgInfo(scriptMap));
	_busy = true;

	SendChannel<EmailMessage, ValueStore> chan(_syncIn, _startCopy);
	chan.Put(EmailMessage(emailList));
	return true;
}

void ActiveEmailCopier::DoEmail(EmailInfoList & emailInfoList)
{
	std::unique_ptr<Mailer> mailer;
	bool isVerbose = false;
	for (EmailInfoList::iterator it = emailInfoList.begin(); it != emailInfoList.end(); ++it)
	{
		if (it->_request._isVerbose)
		{
			isVerbose = true;
			break;
		}
	}

	bool isFailure = false;
	bool isTimedOut = false;
	try
	{
		mailer.reset(new Mailer(TheEmail));
	}
	catch (Win::SocketException e)
	{
		isTimedOut = e.IsTimedOut ();
		TheAlertMan.PostInfoAlert (e, isVerbose);
		isFailure = true;
	}
	catch (Win::Exception e)
	{
		TheAlertMan.PostInfoAlert (e, isVerbose);
		isFailure = true;
	}
	catch (...)
	{
		Win::ClearError ();
		TheAlertMan.PostInfoAlert ("Unknown error when establishing session with Email Server.", isVerbose);
		isFailure = true;
	}

	if (isFailure)
	{
		EmailCopierReadyMessage msg(this, emailInfoList, true, isTimedOut); // general failure
		PutLock lock(_syncOut);
		_copierReadyStore.Put(msg);
		return;
	}

	bool isFirstError = true;
	EmailInfoList::iterator it;
	for (it = emailInfoList.begin(); it != emailInfoList.end(); ++it)
	{
		if (!EmailScript(*mailer, *it, isFirstError, isVerbose))
			break;
	}
	// mark remaining requests
	while (it != emailInfoList.end())
	{
		it->_success = false;
		++it;
	}

	EmailCopierReadyMessage msg(this, emailInfoList);
	PutLock lock(_syncOut);
	_copierReadyStore.Put(msg);
}

bool ActiveEmailCopier::EmailScript(Mailer & mailer, 
									EmailMsgInfo & emailInfo, 
									bool & isFirstError, 
									bool isVerbose)
{
	emailInfo._success = false; // change only if success
	EmailRequest const & request = emailInfo._request;
	std::vector<std::string> const & addrVector = request._addrVector;
	for (std::vector<std::string>::const_iterator it = addrVector.begin ();
		 it != addrVector.end ();
		 ++it)
	{
		std::string const & currAddress = *it;
		if (TheEmail.IsBlacklisted (currAddress))
		{
			emailInfo._badAddress = currAddress;
			EmailCopierReadyMessage msg(this);
			PutLock lock(_syncOut);
			_copierReadyStore.Put(msg);
			return true;
		}
	}

	ScriptAbstract const & scriptAbs = emailInfo._scriptAbstract;
	PathSplitter pathSplitter(scriptAbs._scriptPath);
	std::string baseFileName = pathSplitter.GetFileName();
	std::string fileExtension = pathSplitter.GetExtension();

	std::string fileName = baseFileName;
	fileName += fileExtension;

	// set forward flag
	// do it on temporary copy (if forwarding fails we stay with untouched script)
	std::string tmpSourcePath = TheFileLocker.AcquireFileCopy(scriptAbs._scriptPath, fileName, scriptAbs._toBeFwd, _goesToHub);
	if (tmpSourcePath.empty())
	{
		TheAlertMan.PostInfoAlert ("Temporary copy of a script could not be made.", 
							isVerbose,
							false,
							"Check your TMP directory");
		return false;
	}

	// releases reference to file copy in its destructor
	FileLocker::Guard guard(TheFileLocker, fileName);  

	if (fileExtension [0] == '.')
		fileExtension = fileExtension.substr (1);

	ScriptProcessorConfig processorCfg = TheEmail.GetScriptProcessorConfig ();
	ScriptProcessor processor (processorCfg);
	SafePaths tempScriptCopies;
	TmpPath tmpFolder;
	// Notice: Pack adds resultFile to tempScriptCopies
	std::string resultFile = processor.Pack (tempScriptCopies, tmpSourcePath, tmpFolder, fileExtension, scriptAbs._projectName);

	Subject::Maker subject (fileExtension, scriptAbs._projectName, scriptAbs._scriptId);
	OutgoingMessage mailMsg;

	unsigned partNumber = 1;
	unsigned partCount = 1;
	if (!scriptAbs._isControl)
	{
		partNumber = scriptAbs._partNumber;
		partCount = scriptAbs._partCount;
	}
	mailMsg.SetSubject (subject.Get (partNumber, partCount));
	mailMsg.AddFileAttachment (resultFile);
	mailMsg.SetBccRecipients (scriptAbs._useBccRecipients);
	// put script comment as message text
	mailMsg.SetText (scriptAbs._comment);
	if (TheEmail.GetOutgoingAccount ().GetTechnology ().IsSimpleMapi ())
	{
		Registry::UserDispatcherPrefs prefs;
		if (prefs.IsFirstEmail ())
		{
			AppHelp::Display (AppHelp::OESecurity, "OE Security");
			prefs.SetFirstEmail (false);
		}
	}

	try
	{
		mailer.Send (mailMsg, addrVector);
		emailInfo._success = true;
	}
	catch (Win::SocketException e)
	{
		if (isFirstError)
		{
			isFirstError = false;
			TheAlertMan.PostInfoAlert (e, isVerbose);
		}
		if (e.IsTimedOut ())
		{
			return false;
		}
	}
	catch (Email::BadEmailException e)
	{
		emailInfo._badAddress = e.GetAddress();
		if (isFirstError)
		{
			isFirstError = false;
			// Revisit: use PostQuarantineAlert?
			TheAlertMan.PostInfoAlert (e, isVerbose);
		}
	}
	catch (Win::InternalException e)
	{
		if (isFirstError)
		{
			isFirstError = false;
			// Revisit: use PostQuarantineAlert?
			TheAlertMan.PostInfoAlert (e, isVerbose);
		}
	}
	catch (Win::Exception e)
	{
		if (isFirstError)
		{
			isFirstError = false;
			TheAlertMan.PostInfoAlert (e, isVerbose);
		}
	}
	catch (...)
	{
		if (isFirstError)
		{
			isFirstError = false;
			TheAlertMan.PostInfoAlert ("Unknown error during e-mail processing.", isVerbose);
		}
	}
	return true;
}

void ActiveEmailCopier::RequestDone(EmailMsgInfo const & emailInfo)
{
	if (!emailInfo._success)
		Clog();
	if (!_isClogged)
	{
		std::list<EmailRequest>::iterator it = 
			std::find_if(_requests.begin(), _requests.end(), IsSameHandle(emailInfo._request._scriptHandle));
		if (it != _requests.end())
			_requests.erase(it);
	}
	_busy = false;
}

std::ostream& operator<<(std::ostream& os, ActiveLanCopier const & a)
{
	os << "LanCopier -> " << a.GetTransport();
	if (a._isClogged)
		os << " [clogged] ";
	os << std::endl;
	for (std::list<LanRequest>::const_iterator it = a._requests.begin(); it != a._requests.end(); ++it)
		os << "   -o " << it->_scriptHandle << std::endl;
	return os;
}

std::ostream& operator<<(std::ostream& os, ActiveEmailCopier const & a)
{
	os << "EmailCopier";
	if (a._isClogged)
		os << " [clogged] ";
	os << std::endl;
	for (std::list<EmailRequest>::const_iterator it = a._requests.begin(); it != a._requests.end(); ++it)
		os << "   -o " << it->_scriptHandle << std::endl;
	return os;
}



