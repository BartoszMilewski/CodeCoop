#if !defined (WINMESSAGE_H)
#define WINMESSAGE_H
//----------------------------------------------------
// (c) Reliable Software 2000 -- 2002
//----------------------------------------------------

namespace Win
{
	class Message
	{
	public:
		Message (unsigned int msg, unsigned int wParam = 0, long lParam = 0)
			: _msg (msg),
			  _wParam (wParam),
			  _lParam (lParam),
			  _result (0)
		{}
		void SetLParam (long lParam) { _lParam = lParam; }
		void SetLParam (void const * ptr) { _lParam = reinterpret_cast<LPARAM> (ptr); }
		void SetLParam (Win::Dow::Handle win) 
			{ _lParam = reinterpret_cast<LPARAM> (win.ToNative ()); }
		void SetWParam (unsigned int wParam) { _wParam = wParam; }
		void SetWParam (Win::Dow::Handle win) 
			{ _wParam = reinterpret_cast<WPARAM> (win.ToNative ()); }
		void SetResult (long result) { _result = result; }
		void MakeWParam (unsigned int lo, unsigned int hi) { _wParam = MAKEWPARAM (lo, hi); }

		unsigned int GetMsg () const { return _msg; }
		// Revisit: this is not portable
		unsigned int GetWParam () const { return _wParam; }
		void UnpackWParam (unsigned int & lo, unsigned int & hi)
		{
			lo = LOWORD (_wParam);
			hi = HIWORD (_wParam);
		}
		// Revisit: this is not portable
		long GetLParam () const { return _lParam; }
		// Revisit: this is not portable
		long GetResult () const { return _result; }
		bool operator == (Win::Message const & msg) const
		{
			return msg.GetMsg () == GetMsg ();
		}

	protected:
		UINT	_msg;
		WPARAM	_wParam;
		LPARAM	_lParam;
		LRESULT	_result;
	};

	class UserMessage : public Message
	{
	public:
		// pass zero-based message number
		UserMessage (unsigned int msg, unsigned int wParam = 0, long lParam = 0)
			: Message (WM_USER + msg, wParam, lParam)
		{}
		// return zero-based message number
		unsigned int GetMsg () const { return _msg - WM_USER; }
	};

	// Create this message up front and send/post it repeatedly
	// possibly resetting its parameters
	class RegisteredMessage: public Message
	{
	public:
		RegisteredMessage (std::string const & msgName)
			: Message (::RegisterWindowMessage (msgName.c_str ()))
		{
			if (_msg == 0)
				throw Win::Exception ("Cannot register Window message");
		}
		RegisteredMessage (char const * msgName)
			: Message (::RegisterWindowMessage (msgName))
		{
			if (_msg == 0)
				throw Win::Exception ("Cannot register Window message");
		}
		void SetWParam (unsigned int wParam)
		{
			_wParam = wParam;
		}
	};

	class CommandMessage: public Message
	{
	public:
		CommandMessage (int itemId, bool isAccel = false)
			: Message (WM_COMMAND)
		{
			MakeWParam (itemId, isAccel);
		}
	};

	class ControlMessage: public Message
	{
	public:
		// winParent is the parent of the control sending the message :
		ControlMessage (unsigned ctlId, unsigned notifyCode, Win::Dow::Handle winParent)
			: Message (WM_COMMAND)
		{
			// winCtl = window of the control sending the message :
			Win::Dow::Handle winCtl = ::GetDlgItem (winParent.ToNative (), ctlId);
			SetLParam (winCtl);
			MakeWParam (ctlId, notifyCode);
		}
	};

	// Note: Don't post it if the message object goes out of scope right after posting
	class NotifyMessage: public Message
	{
	public:
		// winParent is the parent of the control sending the message :
		NotifyMessage (unsigned ctlId, unsigned notifyCode, Win::Dow::Handle winParent)
			: Message (WM_NOTIFY)
		{
			_hdr.code = notifyCode;
			_hdr.idFrom = ctlId;
			// window of the control sending the message :
			_hdr.hwndFrom = ::GetDlgItem (winParent.ToNative (), ctlId);
			SetLParam (&_hdr);
			SetWParam (ctlId);
		}
	private:
		NMHDR _hdr; // can't be deallocated before message is processed!
	};

	class NextDlgCtrl : public Message
	{
	public:
		NextDlgCtrl (unsigned ctrlId)
			: Message (WM_NEXTDLGCTL, ctrlId, TRUE)
		{
		}
		NextDlgCtrl (bool isForward)
			: Message (WM_NEXTDLGCTL, isForward ? 0 : 1, FALSE)
		{
		}
	};

	class CloseMessage : public Message
	{
	public:
		CloseMessage ()
			: Message (WM_CLOSE)
		{}
	};

}

#endif
