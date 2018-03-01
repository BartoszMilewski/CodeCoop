//------------------------------
// (c) Reliable Software 2000-04
//------------------------------

#include <WinLibBase.h>
#include <Win/Dde.h>
#include <Win/Utility.h>
#include <Win/MsgLoop.h>
#include <Sys/WinGlobalMem.h>

using namespace DDE;

// Code Co-op DDE procedure -- understands controller factory.
// Process messages from DDE conversation.

LRESULT CALLBACK DDE::Procedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Win::Dow::Handle win (hwnd);
	DDE::Controller * pCtrl = win.GetLong<DDE::Controller *> ();
	switch (message)
	{
    case WM_NCCREATE:
        {
			// Connect DDE controller to the hidden DDE window
			Win::CreateData const * create = 
                reinterpret_cast<Win::CreateData const *> (lParam);
            pCtrl = static_cast<DDE::Controller *> (create->GetCreationData ());
            pCtrl->SetWindowHandle (win);
			win.SetLong<DDE::Controller *> (pCtrl);
        }
        break;
	case WM_CREATE:
		{
			bool success = false;
			// Make DDE window controller operational
			if (pCtrl->OnCreate (), success)
				return success ? 0 : -1;
			break;
		}
	case WM_DESTROY:
		// We're no longer needed for this DDE conversation
        pCtrl->OnDestroy ();
        return 0;
	case WM_TIMER:
		if (pCtrl->ProcessTimer (wParam))
			return 0;
		break;
	case WM_DDE_INITIATE:
		{
			Win::Dow::Handle client (reinterpret_cast<HWND>(wParam));
			ATOM app (LOWORD (lParam));
			ATOM topic (HIWORD (lParam));
			Win::GlobalAtom::String appStr (LOWORD (lParam));
			Win::GlobalAtom::String topicStr (HIWORD (lParam));
			Win::Dow::Handle server (0);
			if (pCtrl->OnInitiate (client, appStr, topicStr, server))
			{
				// Conversation accepted -- send acknowledgement to client
				AckMsg ack (server, app, topic);
				client.SendMsg (ack);
			}
		}
		return 0;
	case WM_DDE_ACK:
		{
			Win::Dow::Handle partner (reinterpret_cast<HWND>(wParam));
			pCtrl->ProcessAck (partner, lParam);
		}
		return 0;
	case WM_DDE_ADVISE:
		{
			Win::Dow::Handle client (reinterpret_cast<HWND>(wParam));
			ATOM dataItem (HIWORD (lParam));
			GlobalBuf buf (reinterpret_cast<HGLOBAL>(LOWORD (lParam)));
			if (pCtrl->OnAdvise (client, dataItem, reinterpret_cast<Advise const *>(buf.Get ())))
			{
				// Revisit: post ACK
			}
		}
		return 0;
	case WM_DDE_DATA:
		{
			Win::Dow::Handle server (reinterpret_cast<HWND>(wParam));
			bool releaseMem = false;
			HGLOBAL globalMem;
			ATOM item;
			::UnpackDDElParam (WM_DDE_DATA, lParam,
							   reinterpret_cast<unsigned int *>(&globalMem),
							   reinterpret_cast<unsigned int *>(&item));
			{
				// Scope for DDE::Data
				ATOM dataItem (item);
				DDE::Data data (globalMem);
				releaseMem = data.IsRelease ();
				DDE::Ack ack (0);	// Default ack is negative
				if (pCtrl->OnData (server, dataItem, data, ack))
				{
					DDE::AckMsg ackMsg (win, ack, dataItem);
					Assert (!server.IsNull ());
					if (!server.PostMsg (ackMsg))
					{
						// Revisit: now what ?
						// Cannot post message to server after receiving data
					}
				}
			}
			if (releaseMem)
			{
	            ::GlobalFree (globalMem);
			}
		}
		return 0;
	case WM_DDE_EXECUTE:
		{
			Win::Dow::Handle client (reinterpret_cast<HWND>(wParam));
			GlobalBuf buf (reinterpret_cast<HGLOBAL>(lParam));
			DDE::Ack ack (0);	// Default ack is negative
			if (pCtrl->OnExecute (client, reinterpret_cast<char const *>(buf.Get ()), ack))
			{
				ATOM lparamAtom (static_cast<ATOM> (lParam));
				DDE::AckMsg ackMsg (win, ack, lparamAtom);
				Assert (!client.IsNull ());
				if (!client.PostMsg (ackMsg))
				{
					// Revisit: now what ?
					// Cannot post message to client after command execution
				}
			}
		}
		return 0;
	case WM_DDE_POKE:
		{
			Win::Dow::Handle client (reinterpret_cast<HWND>(wParam));
			HGLOBAL globalMem;
			ATOM item;
			::UnpackDDElParam (WM_DDE_POKE, lParam,
							   reinterpret_cast<unsigned int *>(&globalMem),
							   reinterpret_cast<unsigned int *>(&item));
			DDE::Poke poke (globalMem);
			if (pCtrl->OnPoke (client, item, poke))
			{
				// Revisit: post ACK
			}
		}
		return 0;
	case WM_DDE_REQUEST:
		{
			Win::Dow::Handle client (reinterpret_cast<HWND>(wParam));
			int format;
			ATOM item;
			::UnpackDDElParam (WM_DDE_REQUEST, lParam,
							   reinterpret_cast<unsigned int *>(&format),
							   reinterpret_cast<unsigned int *>(&item));
			if (!pCtrl->OnRequest (client, item, static_cast<Clipboard::Format>(format)))
			{
				// Server cannot satisfy the request -- send negative ack to client
				DDE::Ack ack (0);	// Default ack is negative
				DDE::AckMsg ackMsg (win, ack, item);
				Assert (!client.IsNull ());
				if (!client.PostMsg (ackMsg))
				{
					// Revisit: now what ?
					// Cannot post message to client after unsucessfull data request
				}
			}
		}
		return 0;
	case WM_DDE_TERMINATE:
		{
			Win::Dow::Handle client (reinterpret_cast<HWND>(wParam));
			pCtrl->OnTerminate (client);
		}
		return 0;
	case WM_DDE_UNADVISE:
		{
			Win::Dow::Handle client (reinterpret_cast<HWND>(wParam));
			int format;
			ATOM item;
			::UnpackDDElParam (WM_DDE_REQUEST, lParam,
							   reinterpret_cast<unsigned int *>(&format),
							   reinterpret_cast<unsigned int *>(&item));
			if (pCtrl->OnUnadvise (client, item, static_cast<Clipboard::Format>(format)))
			{
				// Revisit: post ACK
			}
		}
		return 0;
	}
	return DefWindowProc (hwnd, message, wParam, lParam);
}

bool DDE::Controller::ProcessTimer (int id)
{
	if (id == TimeoutTimerId)
	{
		// Timeout timer
		if (_terminating)
		{
			Win::Quit (0);
			return true;
		}
		else
		{
			return OnTimeout ();
		}
	}
	else
	{
		// User timer
		return OnTimer (id);
	}
}

void DDE::Controller::ProcessAck (Win::Dow::Handle partner, long lParam)
{
	if (IsInitiatingDdeConversation ())
	{
		// The controller is in the process of establishing
		// DDE conversation -- remember conversation partner window
		Win::GlobalAtom::String app (LOWORD (lParam));
		Win::GlobalAtom::String topic (HIWORD (lParam));
		ProcessInitAck (partner, app, topic);
	}
	else
	{
		// Acknowledgement received during conversation
		unsigned int lo, hi;
		::UnpackDDElParam (WM_DDE_ACK, lParam, &lo, &hi);
		Ack ack (lo);
		OnAck (partner, ack, hi);
		::FreeDDElParam (WM_DDE_ACK, lParam);
	}
}

Win::Dow::Handle ClientCtrl::Initiate (std::string const & app, std::string const & topic)
{
	// Client initiates DDE conversation
	_app = app;
	_topic = topic;

	Win::GlobalAtom requestedServer (app);
	Win::GlobalAtom requestedTopic (topic);
	Win::Dow::Handle allTopWindows (HWND_BROADCAST);
	InitiateMsg initiate (_h, requestedServer.GetAtom (), requestedTopic.GetAtom ());
	_initiating = true;
	allTopWindows.SendMsg (initiate);
	_initiating = false;
	return _server;
}

Win::Dow::Handle ClientCtrl::Initiate (Win::Dow::Handle server, std::string const & app, std::string const & topic)
{
	// Client initiates DDE conversation with specified server
	_app = app;
	_topic = topic;

	Win::GlobalAtom requestedServer (app);
	Win::GlobalAtom requestedTopic (topic);
	InitiateMsg initiate (_h, requestedServer.GetAtom (), requestedTopic.GetAtom ());
	_initiating = true;
	server.SendMsg (initiate);
	_initiating = false;
	// When conversation has been accepted the server will return
	// different window handle from the one we used to contact the server
	return _server;
}

void ClientCtrl::ProcessInitAck (Win::Dow::Handle partner, std::string const & server, std::string const & topic)
{
	// Receiving WM_DDE_ACK for WM_DDE_INITIATE
	if (server == _app)
	{
		// We got ack from the right server
		if (topic == _topic)
		{
			// We got ack for the right topic
			// Revisit: in general there can be many responding servers
			_server = partner;
		}
	}
}

void ConversationProxy::Terminate ()
{
	if (!_myCtrl->IsTerminating ())
	{
		TerminateMsg terminate (_myWin);
		_myCtrl->Terminate ();
		if (!_partnerWin.IsNull ())
		{
			_partnerWin.PostMsg (terminate);
			// Wait till partner responds with WM_DDE_TERMINATE or time-out
			Win::MessagePrepro myWinMsgPrepro;
			myWinMsgPrepro.Pump (_myWin);
		}
	}
}

void ClientProxy::Acknowledge (std::string const & app, std::string const & topic)
{
	Assert (!_myWin.IsNull ());
	Assert (!_partnerWin.IsNull ());
	Win::GlobalAtom serverApp (app);
	Win::GlobalAtom acceptedTopic (topic);
	AckMsg ack (_myWin, serverApp.GetAtom (), acceptedTopic.GetAtom ());
	_partnerWin.SendMsg (ack);
}

bool ServerProxy::Start (std::string const & app, std::string const & topic)
{
	Assert (!_myWin.IsNull ());
	_partnerWin = _clientCtrl->Initiate (app, topic);
	return !_partnerWin.IsNull ();
}
