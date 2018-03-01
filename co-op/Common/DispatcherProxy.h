#if !defined (DISPATCHERPROXY_H)
#define DISPATCHERPROXY_H
//------------------------------------
//  (c) Reliable Software, 1998 - 2007
//------------------------------------

#include <Sys/Process.h>
#include <Win/Message.h>

class TransportHeader;
class DispatcherScript;

class DispatcherProxy : public Win::ProcessProxy
{
public:
	DispatcherProxy ();

	void ProjectChange () { PostMsg (_msgProjChange); }
	void JoinRequest (bool onlyAsObserver)
	{
		_msgJoinRequest.SetWParam (onlyAsObserver ? 1 : 0);
		PostMsgSetForeground (_msgJoinRequest);
	}
	void CheckForUpdate ()
	{
		_msgCoopUpdate.SetWParam (0);
		PostMsgSetForeground (_msgCoopUpdate);
	}
	void ShowConfigurationWizard ()
	{
		PostMsgSetForeground (_msgConfigWizard);
	}
	void Notify (TransportHeader const & txHdr, DispatcherScript const & script);
	void ChangeChunkSize (unsigned int newChunkSize)
	{
		_msgChunkSize.SetWParam (newChunkSize);
		PostMsg (_msgChunkSize);
	}

private:
	void PostMsgSetForeground (Win::RegisteredMessage & msg)
	{
		//	We must manually set the Dispatcher to the foreground
		//	(because Windows won't let processes make themselves the foreground)
		GetWin ().SetForeground ();
		PostMsg (msg);
	}
	void PostMsgSetForeground (Win::UserMessage & msg)
	{
		//	We must manually set the Dispatcher to the foreground
		//	(because Windows won't let processes make themselves the foreground)
		GetWin ().SetForeground ();
		PostMsg (msg);
	}
private:
	Win::RegisteredMessage	_msgProjChange;
	Win::RegisteredMessage	_msgJoinRequest;
	Win::RegisteredMessage	_msgCoopUpdate;
	Win::RegisteredMessage	_msgNotification;
	Win::RegisteredMessage	_msgChunkSize;
	Win::UserMessage		_msgConfigWizard;
};

#endif
