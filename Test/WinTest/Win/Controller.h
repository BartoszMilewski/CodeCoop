#if !defined (CONTROLLER_H)
#define CONTROLLER_H
//----------------------------------------------------
// Controller.h
// (c) Reliable Software 2000 -- 2003
//----------------------------------------------------

#include <Win/Win.h>
#include <Win/Procedure.h>
#include <Win/Message.h>
#include <Win/Utility.h>

namespace Notify { class Handler; }

namespace Property 
{
	BOOL CALLBACK SheetProcedure (HWND win, UINT message, WPARAM wParam, LPARAM lParam);
}
namespace Dialog
{
	BOOL CALLBACK Procedure (HWND win, UINT message, WPARAM wParam, LPARAM lParam);
}

namespace Win
{
    class Controller
    {
        friend LRESULT CALLBACK Win::Procedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
        friend LRESULT CALLBACK Win::SubProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
		friend BOOL CALLBACK Dialog::Procedure (HWND win, UINT message, WPARAM wParam, LPARAM lParam);
		friend BOOL CALLBACK Property::SheetProcedure (HWND win, UINT message, WPARAM wParam, LPARAM lParam);
		friend class Maker;

	protected:
		Controller ()
			: _h (0)
		{}
		virtual ~Controller ()
		{}
		void SetWindowHandle (Win::Dow::Handle win) { _h = win; }
		virtual bool MustDestroy () throw ()
			{ return false; }

		// For internal use only
		static void Attach (Win::Dow::Handle win, Controller * ctrl);
		bool Dispatch (UINT message, WPARAM wParam, LPARAM lParam, LRESULT & result) throw ();
	public:
		Win::Dow::Handle GetWindow () const { return _h; }
		virtual bool OnCreate (Win::CreateData const * create, bool & success) throw ()
			{ return false; }
		virtual bool OnInitDialog () throw (Win::Exception)
			{ return false; }
        virtual bool OnDestroy () throw ()
			{ return false; }
		virtual bool OnShutdown (bool isEndOfSession, bool isLoggingOff) throw () { return false; }
		virtual bool OnActivateApp (unsigned long prevThreadId) throw ()
			{ return false; }
		virtual bool OnDeactivateApp (unsigned long nextThreadId) throw ()
			{ return false; }
		virtual bool OnActivate (bool isClickActivate, bool isMinimized, Win::Dow::Handle prevWnd) throw ()
			{ return false; }
		virtual bool OnDeactivate (bool isMinimized, Win::Dow::Handle nextWnd) throw ()
			{ return false; }
		virtual bool OnMouseActivate (Win::HitTest hitTest, Win::MouseActiveAction & activate) throw ()
			{ return false; }
		virtual bool OnFocus (Win::Dow::Handle winPrev) throw ()
			{ return false; }
		virtual bool OnKillFocus (Win::Dow::Handle winNext) throw ()
			{ return false; }
		virtual bool OnEnable() throw ()
			{ return false; }
		virtual bool OnDisable() throw ()
			{ return false; }
		virtual bool OnSize (int width, int height, int flag) throw ()
			{ return false; }
		virtual bool OnClose () throw ()
			{ return false; }
		virtual bool OnPaint () throw ()
			{ return false; }
		virtual bool OnShowWindow (bool show) throw ()
			{ return false; }			
		virtual bool OnSettingChange (Win::SystemWideFlags flags, char const * sectionName) throw ()
			{ return false; }
		virtual bool OnVScroll (int code, int pos) throw ()
			{ return false; }
		virtual bool OnHScroll (int code, int pos) throw ()
			{ return false; }

		virtual bool OnCommand (int cmdId, bool isAccel) throw ()
			{ return false; }
		virtual bool OnControl (Win::Dow::Handle control, int id, int notifyCode) throw ()
			{ return false; }

		virtual bool OnTimer (int id) throw ()
			{ return false; }
		// Mouse
		virtual bool OnMouseMove (int x, int y, Win::KeyState kState) throw ()
			{ return false; }
		virtual bool OnWheelMouse (int zDelta) throw ()
			{ return false; }
		virtual bool OnLButtonDown (int x, int y, Win::KeyState kState) throw ()
			{ return false; }
		virtual bool OnLButtonUp (int x, int y, Win::KeyState kState) throw ()
			{ return false; }
		virtual bool OnLButtonDblClick (int x, int y, Win::KeyState kState) throw ()
			{ return false; }
		virtual bool OnCaptureChanged (Win::Dow::Handle newCaptureWin) throw ()
			{ return false; }
		// Keyboard
		virtual bool OnChar (int vKey, int flags) throw ()
			{ return false; }
		virtual bool OnCharToItem(int vKey, int nCaret, Win::Dow::Handle listbox) throw()
			{ return false; }
		virtual bool OnVKeyToItem(int vKey, int nCaret, Win::Dow::Handle listbox) throw()
			{ return false; }
		// User messages
		virtual bool OnUserMessage (UserMessage & msg) throw ()
			{ return false; }
		// Registered messages
		virtual bool OnRegisteredMessage (Message & msg) throw ()
			{ return false; }

		virtual bool OnHelp () throw ()
			{ return false; }
    protected:
		Win::Dow::Handle	_h;
    };

	class TopController : public Controller
	{
		friend class Maker;

	protected:
		bool MustDestroy () throw ()
			{ return true; }
	};

	class SubController : public Controller
	{
	public:
		SubController ()
			: _prevProc (0),
			  _prevController (0)
		{} 
		void Init (HWND h, ProcPtr prevProc, Controller * prevCtrl)
		{
			SetWindowHandle (h);
			_prevProc = prevProc;

			assert(prevCtrl == 0);	//	subclassing of windows with controllers not yet supported
			_prevController = prevCtrl;
		}
		LRESULT CallPrevProc (UINT message, WPARAM wParam, LPARAM lParam)
		{
			return ::CallWindowProc (_prevProc, _h.ToNative (), message, wParam, lParam);
		}
		ProcPtr GetPrevProc ()
		{
			return _prevProc;
		}
		Controller * GetPrevController ()
		{
			return _prevController;
		}
	protected:
		ProcPtr			_prevProc;
		Controller *	_prevController;
	};
}

#endif
