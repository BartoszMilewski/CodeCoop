#if !defined (MSGLOOP)
#define MSGLOOP
//------------------------------
// (c) Reliable Software 1998-03
//------------------------------
#include <Win/Message.h>
#include <Win/Win.h>

namespace Win
{
	class MessagePrepro
	{
	public:
		MessagePrepro ()
			: _breakMsg (WM_NULL)
		{}
		MessagePrepro (unsigned int breakMsg)
			: _breakMsg (breakMsg)
		{}
		MessagePrepro (Win::Message const & breakMsg)
			: _breakMsg (breakMsg.GetMsg ())
		{}
		int Pump ();
		int Pump (Win::Dow::Handle hwnd);
		int PumpHidden (Win::Dow::Handle hwnd);
		void PumpPeek ();
		void PumpPeek (Win::Dow::Handle hwnd);
		void PumpDialog ();

	private:
		MessagePrepro (MessagePrepro const & prepro);
		MessagePrepro const & operator= (MessagePrepro const & prepro);
		bool TranslateDialogMessage (MSG & msg);

	private:
		Win::Dow::Handle	_hwndDlg;		// dialog window
		unsigned int		_breakMsg;
	};

	class ModalMessagePrepro
	{
	public:
		ModalMessagePrepro (unsigned int breakMsg)
			: _breakMsg (breakMsg)
		{}
		ModalMessagePrepro (Win::Message const & breakMsg)
			: _breakMsg (breakMsg.GetMsg ())
		{}
		int Pump (Win::Dow::Handle hwnd);
		int Pump ();

	private:
		ModalMessagePrepro (ModalMessagePrepro const & prepro);
		ModalMessagePrepro const & operator= (ModalMessagePrepro const & prepro);

	private:
		unsigned int	_breakMsg;
	};
}

#endif
