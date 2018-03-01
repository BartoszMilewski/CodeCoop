#if !defined DIALOG_H
#define DIALOG_H
//------------------------------------
//  (c) Reliable Software, 1996 - 2008
//------------------------------------

#include <Win/Controller.h>
#include <Win/ControlHandler.h>
#include <Win/MsgLoop.h>
#include <Win/WinResource.h>
#include <Win/DialogTemplate.h>

class NamedValues;
namespace Notify { class Handler; }
namespace Help { class Engine; }
namespace PropPage
{
	BOOL CALLBACK Procedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
}

namespace Dialog
{
	BOOL CALLBACK Procedure (HWND win, UINT message, WPARAM wParam, LPARAM lParam);
	class Modal;
	class Modeless;
	class ControlHandler;

	class Style: public Win::Style
	{
	public:
		enum Bits
		{
			AbsAlign =DS_ABSALIGN,
			SysModal =DS_SYSMODAL,
			LocalEdit =DS_LOCALEDIT,  /* Edit items get Local storage. */
			SetFont =DS_SETFONT,   /* User specified font for Dlg controls */
			ModalFrame =DS_MODALFRAME,   /* Can be combined with WS_CAPTION  */
			NoIdleMsg =DS_NOIDLEMSG,  /* WM_ENTERIDLE message will not be sent */
			SetForground =DS_SETFOREGROUND,  /* not in win3.1 */
			Look3D =DS_3DLOOK,
			FixedSys =DS_FIXEDSYS,
			NoFailCreate =DS_NOFAILCREATE,
			Control =DS_CONTROL,
			Center =DS_CENTER,
			CenterMouse =DS_CENTERMOUSE,
			ContextHelp =DS_CONTEXTHELP
		};
	};

	class Handle: public Win::Dow::Handle
	{
	public:
		Handle (Win::Dow::Handle win = 0) 
			: Win::Dow::Handle (win) 
		{}
		void Reposition ()
		{
			Win::Message msg (DM_REPOSITION);
			SendMsg (msg);
		}
		// takes rect in dialog template units and maps to pixels
		void MapRectangle (Win::Rect & rect);
	};

	// Template function used in dialog procedures to dispatch messages to the controller
	// Special handling of:
	// - WM_INITDIALOG to obtain the controller
	// - WM_COMMAND to dispatch control messages to the dialog control handler
	template<class ControllerT>
	bool DispatchMsg (ControllerT * ctrl, Win::Dow::Handle win, UINT message, WPARAM wParam, LPARAM lParam)
	{
		LRESULT result = 0;
		if (ctrl != 0)
		{
			// Commands from controls and Cancel from system menu (close box)
			if (message == WM_COMMAND && (lParam != 0 || (LOWORD (wParam) == Out::Cancel)))
			{
				// Window Control message
				Win::Dow::Handle ctrlWin = reinterpret_cast<HWND>(lParam);
				unsigned ctrlId = LOWORD (wParam);
				unsigned notifyCode = HIWORD (wParam);
				
				Dialog::ControlHandler * ctrlHandler = ctrl->GetDlgControlHandler ();
				if (ctrlHandler)
				{
					return ctrlHandler->OnControl (ctrlId, notifyCode);
				}
				else
					return ctrl->Dispatch (message, wParam, lParam, result);
			}
			else
				return ctrl->Dispatch (message, wParam, lParam, result);
		}
		else if (message == WM_INITDIALOG)
		{
			ControllerT * dialogCtrl = ControllerT::GetController (lParam);
			Controller::Attach (win, dialogCtrl);
			Dialog::ControlHandler * handler = dialogCtrl->GetDlgControlHandler ();
			if (handler)
			{
				handler->Attach (dialogCtrl);
				return handler->OnInitDialog ();
			}
			else
				return dialogCtrl->Dispatch (message, wParam, lParam, result);
		}
		return false;
	}

	// Dialog::Controller
	// Contains control handler for handling control messages
	// EndOk and EndCancel implemented in Modal and Modeless versions of Controller
	class Controller: public Win::Controller
	{
		friend BOOL CALLBACK Procedure (HWND win, UINT message, WPARAM wParam, LPARAM lParam);
	public:
		static Dialog::Controller * GetController (LPARAM lParam)
		{
			return reinterpret_cast<Dialog::Controller *> (lParam);
		}
	public:
		Controller (int dlgId)
			:_ctrlHandler (0)
		{}
		// Dialog end methods
		virtual void EndOk () throw () { Assert (!"EndOk must be overridden"); }
		virtual void EndCancel () throw () { Assert (!"EndCancel must be overridden"); }

		Dialog::ControlHandler * GetDlgControlHandler () throw ()
		{
			return _ctrlHandler;
		}
		Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ();
		void SetDlgControlHandler (Dialog::ControlHandler * handler)
		{
			_ctrlHandler = handler;
		}
	private:
		Dialog::ControlHandler *_ctrlHandler;
	};

	// Dialog::ControlHandler
	// To be subclassed by the client
	// Note: To process Windows notifications, override GetNotifyHandler
	class ControlHandler: public Control::Handler
	{
	public:
		ControlHandler (int dlgId)
			: Control::Handler (dlgId),
			  _helpEngine (0),
			  _ctrl (0)
		{}
		Win::Dow::Handle GetWindow () const 
		{
			if (_ctrl != 0)
				return _ctrl->GetWindow ();
			else
				return Win::Dow::Handle ();
		}
		// These methods are usually specialized by the client
		virtual bool OnInitDialog () throw (Win::Exception) 
			{ return true; }
		// Use to process messages from dialog controls other than IDOK, IDCANCEL, and IDHELP
		virtual bool OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception)
			{ return false; }
		// Called when user clicks the OK button (IDOK)
		virtual bool OnApply () throw () 
		{
			EndOk ();
			return true;
		}
		// Called when user clicks the CANCEL button (IDCANCEL)
		virtual bool OnCancel () throw () 
		{
			EndCancel ();
			return true;
		}
		// Called when user clicks the HELP button (IDHELP). Default implementation uses help engine.
		virtual bool OnHelp () throw (); 
		// Alternative dialog input (alternative to GUI)
		virtual bool GetDataFrom (NamedValues const & source)
			{ return false; }
		// Used in modeless dialogs
		virtual void OnActivate () throw (Win::Exception) {}
		virtual void OnDectivate () throw (Win::Exception) {}
		virtual void OnDestroy () throw () {}
		// These non-virtual methods should be called from OnApply and OnCancel
		void EndOk () throw ()
		{
			Assert (_ctrl != 0);
			_ctrl->EndOk ();
		}
		void EndCancel () throw ()
		{
			Assert (_ctrl != 0);
			_ctrl->EndCancel ();
		}
		void AttachHelp (Help::Engine * helpEngine) throw () 
		{
			_helpEngine = helpEngine;
		}
		virtual Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
		{
			return 0;
		}
	public: // For internal use: made public because of problems with templated friends
		void Attach (Dialog::Controller * ctrl) { _ctrl = ctrl; }
		bool OnControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	private:
		Dialog::Controller *_ctrl;
		Help::Engine *		_helpEngine;
	};

	// Sizes and units
	inline int BaseUnitX () { return LOWORD (::GetDialogBaseUnits ()); }
	inline int BaseUnitY () { return HIWORD (::GetDialogBaseUnits ()); }
	inline int FrameThickX () { return ::GetSystemMetrics (SM_CXDLGFRAME); }
	inline int FrameThickY () { return ::GetSystemMetrics (SM_CYDLGFRAME); }
	inline int CaptionHeight () { return ::GetSystemMetrics(SM_CYCAPTION); }

	// Dialog::ModalController
	// May be specialized by the client if necessary
	class ModalController: public Dialog::Controller
	{
		friend class Dialog::Modal;
	protected:
		ModalController (int dlgId)
			: Dialog::Controller (dlgId)
		{}
		void EndOk () throw ()
		{
			::EndDialog (GetWindow ().ToNative (), 1);
			Win::ClearError ();
		}
		void EndCancel () throw ()
		{
			::EndDialog (GetWindow ().ToNative (), 0);
			Win::ClearError ();
		}
	};

	// Dialog::ModelessController
	// May be specialized by the client if necessary
	class ModelessController: public Dialog::Controller
	{
	public:
		// Owned by Windows, accessed through window long, destroyed by Window procedure
		bool MustDestroy () throw ()
			{ return true; }

		ModelessController (Win::MessagePrepro & prepro, 
							int dlgId, 
							Win::Dow::Handle accelWin = Win::Dow::Handle (), 
							Accel::Handle accel = Accel::Handle ()) 
			: Dialog::Controller (dlgId),
			  _prepro (prepro),
			  _accelWin (accelWin),
			  _accel (accel)
		{}

		void EndOk () throw ()
		{
			Destroy ();
			Win::ClearError ();
		}
		void EndCancel () throw ()
		{
			Destroy ();
			Win::ClearError ();
		}
		void Destroy () throw ()
		{ 
			::DestroyWindow (GetWindow ().ToNative ());
		}
		bool OnActivate (bool isClickActivate, bool isMinimized, Win::Dow::Handle prevWnd) throw (Win::Exception); 
		bool OnDeactivate (bool isMinimized, Win::Dow::Handle prevWnd) throw (Win::Exception);
        bool OnDestroy () throw ();
	protected:
		Win::MessagePrepro & _prepro;
		Win::Dow::Handle	_accelWin;
		Accel::Handle		_accel;
	};

	// User constructs this object and passes it a user-defined control handler
	// Overriding the controller is optional and rarely used
	class Modal
	{
	public:
		Modal ( Win::Dow::Handle winParent, 
				Dialog::ControlHandler & handler,
				Dialog::ModalController * ctrl = 0,
				Win::Instance hInstance = Win::Instance ())
			: _ctrl (handler.GetId ())
		{
			if (ctrl == 0)
				ctrl = & _ctrl; // override the default controller
			if (hInstance.IsNull ())
				hInstance = winParent.GetInstance ();
			ctrl->SetDlgControlHandler (&handler);
			_result = ::DialogBoxParam (hInstance,
										MAKEINTRESOURCE (handler.GetId ()),
										winParent.ToNative (),
										static_cast<DLGPROC>(Dialog::Procedure),
										reinterpret_cast<LPARAM>(ctrl));
		}

		Modal (Win::Dow::Handle winParent,
			   Win::Instance hInstance, 
			   Dialog::Template const & templ, 
			   Dialog::ControlHandler & handler,
			   Dialog::ModalController * ctrl = 0)
			: _ctrl (handler.GetId ())
		{
			if (ctrl == 0)
				ctrl = & _ctrl;
			ctrl->SetDlgControlHandler (&handler);
			_result = ::DialogBoxIndirectParam (hInstance,
												templ.ToNative (),
												winParent.ToNative (),
												static_cast<DLGPROC>(Dialog::Procedure),
												reinterpret_cast<LPARAM>(ctrl));
		}

		bool IsOK () { return (_result == -1)? false: _result != 0; }

	private:
		int _result;
		Dialog::ModalController _ctrl;
	};

	// User constructs this object and passes it a user-defined control handler
	// Overriding the controller is optional and rarely used
	// Note: ControlHandler and the dialog handle returned from Create must be kept alive
	// as long as the dialog is active. ModelessMaker may be discarded.
	class ModelessMaker
	{
	public:
		// Important: ControlHandler's lifetime must exceed that of the dialog!
		ModelessMaker (Dialog::ControlHandler & handler, std::unique_ptr<Dialog::ModelessController> ctrl)
			: _handler (handler), 
			  _ctrl (std::move(ctrl))
		{
			Init ();
		}
		ModelessMaker (Dialog::ControlHandler & handler, 
			Win::MessagePrepro & msgPrepro, 
			Win::Dow::Handle parentWin, 
			Accel::Handle accel = Accel::Handle ())
			: _handler (handler), 
			  _ctrl (new ModelessController (msgPrepro, handler.GetId (), parentWin, accel))
		{
			Init ();
		}
		void Init ()
		{
			Assert (_ctrl->MustDestroy ());
			_ctrl->SetDlgControlHandler (&_handler);
			_handler.Attach (_ctrl.get ());
		}
		// Call only if you don't want to use winParent's instance
		void SetInstance (Win::Instance instance)
		{
			_instance = instance;
		}
		Dialog::Handle Create (Win::Dow::Handle winParent)
		{
			if (_instance.IsNull ())
				_instance = winParent.GetInstance ();
			Win::Dow::Handle h = ::CreateDialogParam (_instance,
											 MAKEINTRESOURCE (_handler.GetId ()),
											 winParent.ToNative (),
											 static_cast<DLGPROC>(Dialog::Procedure),
											 reinterpret_cast<LPARAM>(_ctrl.release ()));
			if (h.IsNull ())
				throw Win::Exception ("Internal error: Cannot create modeless dialog.");
			return h;
		}
		Dialog::Handle Create (Win::Dow::Handle winParent, Dialog::Template const & templ)
		{
			if (_instance.IsNull ())
				_instance = winParent.GetInstance ();
			Win::Dow::Handle h = ::CreateDialogIndirectParam (_instance,
											 templ.ToNative (),
											 winParent.ToNative (),
											 static_cast<DLGPROC>(Dialog::Procedure),
											 reinterpret_cast<LPARAM>(_ctrl.release ()));
			if (h.IsNull ())
				throw Win::Exception ("Internal error: Cannot create modeless dialog.");
			return h;
		}
	private:
		std::unique_ptr<Dialog::ModelessController> _ctrl; // not used after call to Create
		Dialog::ControlHandler &_handler;
		Win::Instance			_instance;
	};
}

namespace Win
{
	inline Win::Style & operator<< (Win::Style & style, Dialog::Style::Bits bits)
	{
		style.OrIn (static_cast<Win::Style::Bits> (bits));
		return style;
	}
}

#endif
