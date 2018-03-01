#if !defined (MSGLOOP)
#define MSGLOOP
//------------------------------
// (c) Reliable Software 1998-03
//------------------------------
#include <Win/Message.h>
#include <Ctrl/Accelerator.h>

namespace Win
{
	class MessagePrepro
	{
	public:
		MessagePrepro ()
			: _breakMsg (WM_NULL), _accel (0)
		{}
		MessagePrepro (unsigned int breakMsg)
			: _breakMsg (breakMsg), _accel (0)
		{}
		MessagePrepro (Win::Message const & breakMsg)
			: _breakMsg (breakMsg.GetMsg ()), _accel (0)
		{}
		void SetKbdAccelerator (Win::Accelerator * accel) 
		{
			_accel = accel;
		}
		void ResetKbdAccelerator () 
		{
			_accel = 0;
		}
		// Use with modeless dialogs
		void SetDialogFilter (Win::Dow::Handle winDlg, 
								Win::Dow::Handle winDlgParent = Win::Dow::Handle (), 
								Accel::Handle hDlgAccel = Accel::Handle ()) 
		{ 
			_winDlg = winDlg;
			_winDlgParent = winDlgParent;
			_hDlgAccel = hDlgAccel;
			//  use dialog accelerator , only if the target window for dialog accelerator is valid
			Assert (hDlgAccel.IsNull () || !winDlgParent.IsNull ());
		}
		void ResetDialogFilter () 
		{
			_winDlg.Reset ();
			_hDlgAccel.Reset ();
			_winDlgParent.Reset ();
		}
		void SetDialogAccel (Win::Dow::Handle winDlgParent, Accel::Handle hDlgAccel) 
		{ 
			_winDlgParent = winDlgParent;
			_hDlgAccel = hDlgAccel;
			Assert (hDlgAccel.IsNull () || !winDlgParent.IsNull ());
		}
		void ResetDialogAccel () 
		{
			_winDlgParent.Reset ();
			_hDlgAccel.Reset ();
		}
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
		Win::Accelerator *	_accel;
		Win::Dow::Handle	_winDlg;		// dialog window
		Accel::Handle		_hDlgAccel;		// dialog accelerator		                        
		Win::Dow::Handle	_winDlgParent;	// target window for dialog accelerator		
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
