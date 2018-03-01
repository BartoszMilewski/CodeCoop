#if !defined (DDE_H)
#define DDE_H
//----------------------------------------------------
// (c) Reliable Software 2000
//----------------------------------------------------
#include <Win/Atom.h>
#include <Win/Message.h>
#include <Sys/Timer.h>
#include <Sys/WinGlobalMem.h>
#include <Sys/Clipboard.h>

namespace DDE
{
	LRESULT CALLBACK Procedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	class Controller;

	// Data structures used during DDE conversation

	class Ack
	{
	public:
		Ack (int value)
			: _value (value)
		{}

		int GetValue () const { return _value; }
		int GetAckCode () const { return _ack.bAppReturnCode; }
		bool IsPositiveAck () const { return _ack.fAck != 0; }
		bool IsBusy () const { return _ack.fBusy != 0; }

		void SetPositive () { _ack.fAck = true; }
		void SetBusy () { _ack.fBusy = true; }

	private:
		union
		{
			unsigned short	_value;
			DDEACK			_ack;
		};

	};

	class Advise
	{
	private:
		DDEADVISE	_advise;
	};

	class Data : public GlobalBuf
	{
	public:
		Data (GlobalMem & mem)
			: GlobalBuf (mem)
		{}
		Data (HGLOBAL handle)
			: GlobalBuf (handle)
		{}
		static int SizeOfHeader () { return sizeof (DDEDATA); }
		void SetResponse (bool flag)
		{
			(reinterpret_cast<DDEDATA *>(_buf))->fResponse = flag;
		}
		void SetRelease (bool flag)
		{
			(reinterpret_cast<DDEDATA *>(_buf))->fRelease = flag;
		}
		void SetAckRequired (bool flag)
		{
			(reinterpret_cast<DDEDATA *>(_buf))->fAckReq = flag;
		}
		void SetFormat (Clipboard::Format format)
		{
			(reinterpret_cast<DDEDATA *>(_buf))->cfFormat = format;
		}
		bool IsResponse () const
		{
			return (reinterpret_cast<DDEDATA *>(_buf))->fResponse;
		}
		bool IsRelease () const
		{
			return (reinterpret_cast<DDEDATA *>(_buf))->fRelease;
		}
		bool IsAckRequired () const
		{
			return (reinterpret_cast<DDEDATA *>(_buf))->fAckReq;
		}
		bool IsText () const
		{
			return (reinterpret_cast<DDEDATA *>(_buf))->cfFormat == Clipboard::Text;
		}
		unsigned char * GetPayload () const
		{
			return &(reinterpret_cast<DDEDATA *>(_buf))->Value [0];
		}
	};

	class Poke : public GlobalBuf
	{
	public:
		Poke (GlobalMem & mem)
			: GlobalBuf (mem)
		{}
		Poke (HGLOBAL handle)
			: GlobalBuf (handle)
		{}
		static int SizeOfHeader () { return sizeof (DDEPOKE); }
		void SetRelease (bool flag)
		{
			(reinterpret_cast<DDEPOKE *>(_buf))->fRelease = flag;
		}
		void MakeText ()
		{
			(reinterpret_cast<DDEPOKE *>(_buf))->cfFormat = Clipboard::Text;
		}
		bool IsRelease () const
		{
			return (reinterpret_cast<DDEPOKE *>(_buf))->fRelease;
		}
		bool IsText () const
		{
			return (reinterpret_cast<DDEPOKE *>(_buf))->cfFormat == Clipboard::Text;
		}
		unsigned char * GetBuf ()
		{
			return &((reinterpret_cast<DDEPOKE *>(_buf))->Value [0]);
		}
		char const * GetText () const
		{
			return reinterpret_cast<char const *>(&((reinterpret_cast<DDEPOKE *>(_buf))->Value [0]));
		}
	};

	// DDE messages

	class InitiateMsg : public Win::Message
	{
	public:
		InitiateMsg (Win::Dow::Handle client, ATOM const & app, ATOM const & topic)
			: Win::Message (WM_DDE_INITIATE, 0,
							::PackDDElParam (WM_DDE_INITIATE, 
												static_cast<UINT_PTR> (app),
												static_cast<UINT_PTR> (topic)))
		{
			SetWParam (client);
		}
	};

	class AckMsg : public Win::Message
	{
	public:
		AckMsg (Win::Dow::Handle server, ATOM const & app, ATOM const & topic)
			: Win::Message (WM_DDE_ACK, 0, MAKELPARAM (app, topic))
		{
			SetWParam (server);
		}
		AckMsg (Win::Dow::Handle server, DDE::Ack const & ack, unsigned int lParam)
			: Win::Message (WM_DDE_ACK, 0,
							::PackDDElParam (WM_DDE_ACK, ack.GetValue (), lParam))
		{
			SetWParam (server);
		}
		AckMsg (Win::Dow::Handle server, DDE::Ack const & ack, ATOM const & item)
			: Win::Message (WM_DDE_ACK, 0,
							::PackDDElParam (WM_DDE_ACK, ack.GetValue (), item))
		{
			SetWParam (server);
		}
	};

	class TerminateMsg : public Win::Message
	{
	public:
		TerminateMsg (Win::Dow::Handle win)
			: Win::Message (WM_DDE_TERMINATE)
		{
			SetWParam (win);
		}
	};

	class ExecuteMsg : public Win::Message
	{
	public:
		ExecuteMsg (Win::Dow::Handle win, GlobalMem const & buf)
			: Win::Message (WM_DDE_EXECUTE, 0,
							::PackDDElParam (WM_DDE_EXECUTE, 0, reinterpret_cast<unsigned int>(buf.GetHandle ())))
		{
			SetWParam (win);
		}
	};

	class DataMsg : public Win::Message
	{
	public:
		DataMsg (Win::Dow::Handle win, ATOM const & item, GlobalMem const & buf)
			: Win::Message (WM_DDE_DATA, 0,
							::PackDDElParam (WM_DDE_DATA,
											 reinterpret_cast<unsigned int>(buf.GetHandle ()),
											 item))
		{
			SetWParam (win);
		}
	};

	class PokeMsg : public Win::Message
	{
	public:
		PokeMsg (Win::Dow::Handle win, ATOM const & item, GlobalMem const & buf)
			: Win::Message (WM_DDE_POKE, 0,
							::PackDDElParam (WM_DDE_POKE,
											 reinterpret_cast<unsigned int>(buf.GetHandle ()),
											 item))
		{
			SetWParam (win);
		}
	};

	class RequestMsg : public Win::Message
	{
	public:
		RequestMsg (Win::Dow::Handle win, ATOM const & item, Clipboard::Format format)
			: Win::Message (WM_DDE_REQUEST, 0,
							::PackDDElParam (WM_DDE_REQUEST, format, item))
		{
			SetWParam (win);
		}
	};

	class Controller
	{
        friend LRESULT CALLBACK DDE::Procedure (HWND hwnd, 
                        UINT message, WPARAM wParam, LPARAM lParam);
		friend class ConversationProxy;

	public:
		virtual ~Controller () {}
		virtual bool OnCreate () throw ()
			{ return false; }
        virtual bool OnDestroy () throw ()
			{ return false; }
		virtual bool OnTimer (int id) throw ()
			{ return false; }
		virtual bool OnTimeout () throw ()
			{ return false; }
		virtual bool OnInitiate (Win::Dow::Handle client, std::string const & app, std::string const & topic, Win::Dow::Handle & server) throw () = 0;
		virtual void OnAck (Win::Dow::Handle partner, DDE::Ack ack, int handle) throw () = 0;
		virtual bool OnAdvise (Win::Dow::Handle client, ATOM requestedItem, Advise const * advise) throw () = 0;
		virtual bool OnData (Win::Dow::Handle server, ATOM dataItem, DDE::Data const & data, DDE::Ack & ack) throw () = 0;
		virtual bool OnExecute (Win::Dow::Handle clinet, char const * cmd, DDE::Ack & ack) throw () = 0;
		virtual bool OnPoke (Win::Dow::Handle client,  ATOM dataItem, DDE::Poke const & poke) throw () = 0;
		virtual bool OnRequest (Win::Dow::Handle client, ATOM requestedItem, Clipboard::Format format) throw () = 0;
		virtual void OnTerminate (Win::Dow::Handle partner) throw () = 0;
		virtual bool OnUnadvise (Win::Dow::Handle client, ATOM requestedItem, Clipboard::Format format) throw () = 0;

		bool IsTerminating () const { return _terminating; }
		void SetTerminating (bool flag) { _terminating = flag; }
		void StartTimeout (int timeout)	// timeout in miliseconds
		{
			_timer.Set (timeout);
		}
		void KillTimeout ()
		{
			_timer.Kill ();
		}

	protected:
		Controller ()
			: _h (0),
			  _timer (TimeoutTimerId),
			  _terminating (false)
		{}

		void SetWindowHandle (Win::Dow::Handle win)
		{
			_h.Reset (win.ToNative ());
			_timer.Attach (win);
		}
		void Terminate ()
		{
			_terminating = true;
			StartTimeout (TerminateTimeout);
		}
		bool ProcessTimer (int id);
		void ProcessAck (Win::Dow::Handle client, long lParam);
		virtual bool IsInitiatingDdeConversation () const = 0;
		virtual void ProcessInitAck (Win::Dow::Handle partner, std::string const & server, std::string const & topic) {}

	protected:
		Win::Dow::Handle	_h;					// Hidden window identyfying DDE conversation

	private:
		enum { TimeoutTimerId = 7531	};
		enum { TerminateTimeout  = 1000 };// One second

	private:
		Win::Timer	_timer;
		bool		_terminating;
	};

	class ServerCtrl : public DDE::Controller
	{
	public:
		// For DDE servers this methods do nothing
		bool OnData (Win::Dow::Handle server, ATOM dataItem, Data const & data, DDE::Ack & ack) throw ()
			{ return false; }

	protected:
		bool IsInitiatingDdeConversation () const { return false; }
	};

	class ClientCtrl : public DDE::Controller
	{
	public:
		ClientCtrl ()
			: _server (0),
			  _initiating (false)
		{}

		// For DDE clients this methods do nothing
		bool OnInitiate (Win::Dow::Handle client, std::string const & app, std::string const & topic, Win::Dow::Handle & server) throw ()
			{ return false; }
		bool OnAdvise (Win::Dow::Handle client, ATOM requestedItem, Advise const * advise) throw ()
			{ return false; }
		bool OnExecute (Win::Dow::Handle clinet, char const * cmd, DDE::Ack & ack) throw ()
			{ return false; }
		bool OnPoke (Win::Dow::Handle client, ATOM dataItem, Poke const & poke) throw ()
			{ return false; }
		bool OnRequest (Win::Dow::Handle client, ATOM requestedItem, Clipboard::Format format) throw ()
			{ return false; }
		bool OnUnadvise (Win::Dow::Handle client, ATOM requestedItem, Clipboard::Format format) throw ()
			{ return false; }

		Win::Dow::Handle Initiate (std::string const & app, std::string const & topic);
		Win::Dow::Handle Initiate (Win::Dow::Handle server, std::string const & app, std::string const & topic);

	protected:
		bool IsInitiatingDdeConversation () const { return _initiating; }
		void ProcessInitAck (Win::Dow::Handle partner, std::string const & server, std::string const & topic);

	protected:
		Win::Dow::Handle	_server;
		bool		_initiating;

	private:
		std::string _app;
		std::string _topic;
	};

	class ConversationProxy
	{
	public:
		ConversationProxy (DDE::Controller * ctrl)
			: _myCtrl (ctrl),
			  _myWin (0),
			  _partnerWin (0)
		{}

		void SetMyWin (Win::Dow::Handle win) { _myWin = win; }
		Win::Dow::Handle GetMyWin () const { return _myWin; }
		Win::Dow::Handle GetPartnerWin () const { return _partnerWin; }
		void Terminate ();

	protected:
		DDE::Controller *	_myCtrl;
		Win::Dow::Handle			_myWin;
		Win::Dow::Handle			_partnerWin;
	};

	// Conversation proxy created by the server when accepting WM_DDE_INITIATE
	// request from Win::Dow::Handle client. Server part of the conversation is handled
	// by the ServerCtrl.
	class ClientProxy : public ConversationProxy
	{
	public:
		ClientProxy (Win::Dow::Handle client, ServerCtrl * ctrl)
			: ConversationProxy (ctrl)
		{
			_partnerWin = client;
		}

		void Acknowledge (std::string const & app, std::string const & topic);
	};

	// Conversation proxy created by the client when initiating DDE conversation.
	// Client part of the conversation is handled by the ClientCtrl.
	class ServerProxy : public ConversationProxy
	{
	public:
		ServerProxy (ClientCtrl * ctrl)
			: ConversationProxy (ctrl),
			  _clientCtrl (ctrl)
		{}

		bool Start (std::string const & app, std::string const & topic);

	private:
		ClientCtrl * _clientCtrl;
	};
}

#endif
