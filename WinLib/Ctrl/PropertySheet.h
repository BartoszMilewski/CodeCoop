#if !defined (PROPERTYSHEET_H)
#define PROPERTYSHEET_H
// ----------------------------------
// (c) Reliable Software, 1999 - 2008
// ----------------------------------

#include <Win/Dialog.h>
#include <Win/Notification.h>
#include <auto_vector.h>
#include <stack>

class NamedValues;

namespace Win
{
	class CritSection;
}

namespace PropPage { class Navigator; }

namespace Notify
{
	//----------------------------------------------------
	// Notify::PageHandler
	// generic Windows WM_NOTIFY handler for property page
	//----------------------------------------------------
	class PageHandler : public Notify::Handler
	{
	public:
		PageHandler (unsigned id) : Notify::Handler (id), _navigator (0) {}
		// For use in wizards
		void SetNavigator (PropPage::Navigator * navigator)
		{
			_navigator = navigator;
		}
		virtual void OnSetActive (long & result) throw (Win::Exception)	{}
		virtual void OnKillActive (long & result) throw (Win::Exception) {}
		virtual void OnFinish (long & result) throw (Win::Exception) {}
		virtual void OnCancel (long & result) throw (Win::Exception) {}
		virtual void OnApply (long & result) throw (Win::Exception) {}
		virtual void OnReset () throw (Win::Exception) {}
		virtual void OnHelp () const throw (Win::Exception) {}
		virtual void OnPrev (long & result) throw (Win::Exception);
		virtual void OnNext (long & result) throw (Win::Exception);
	protected:
		bool OnNotify (NMHDR * hdr, long & result) throw (Win::Exception);
		// For use in wizards
		virtual bool GoNext (long & nextPage) { return false; }
		virtual bool GoPrevious () { return false; }
	protected:
		PropPage::Navigator * _navigator;
	};
}

namespace PropPage
{
	enum Button
	{
		Finish = PSWIZB_FINISH,
		Apply = PSBTN_APPLYNOW,
		Back = PSBTN_BACK,
		Cancel = PSBTN_CANCEL,
		Help = PSBTN_HELP,
		Ok = PSBTN_OK,
		Next = PSBTN_NEXT
	};

	class Wiz
	{
	public:
		enum Buttons
		{
			Next = PSWIZB_NEXT,
			NextBack = PSWIZB_NEXT | PSWIZB_BACK,
			BackFinish = PSWIZB_BACK | PSWIZB_FINISH,
			Finish = PSWIZB_FINISH,
		};
	};

	//--------------------------------------------
	// PropPage::ControlHandler 
	// Handles messages related to dialog controls
	//--------------------------------------------
	class ControlHandler: public Dialog::ControlHandler
	{
	public:
		ControlHandler (unsigned pageId, bool supportsContextHelp = false)
			: Dialog::ControlHandler (pageId),
			  _supportsContextHelp (supportsContextHelp)
		{}
		bool SupportsContextHelp () const { return _supportsContextHelp; }
		unsigned GetPageId () const { return GetId (); }
		void SetButtons (Wiz::Buttons buttons) const
		{ 
			::PropSheet_SetWizButtons (GetWindow ().GetParent ().ToNative (), buttons); 
		}
		void CancelToClose ()
		{
			::PropSheet_CancelToClose (GetWindow ().GetParent ().ToNative ());
		}

		void PressButton (PropPage::Button button)
		{
			::PropSheet_PressButton (GetWindow ().GetParent ().ToNative (), button); 
		}
	private:
		bool	_supportsContextHelp;
	};

	//-------------------------------------------
	// PropPage::Handler
	// Combines control and notification handlers
	//-------------------------------------------
	class Handler: public PropPage::ControlHandler, public Notify::PageHandler
	{
	public:
		Handler (unsigned pageId, bool supportsContextHelp = false)
			: PropPage::ControlHandler (pageId, supportsContextHelp),
			  Notify::PageHandler (pageId)
		{
		}
		Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
		{
			return this;
		}
	};

	//---------------------
	// PropPage::Controller
	//---------------------
	class Controller: public Dialog::Controller
	{
		friend BOOL CALLBACK Procedure (HWND win, UINT message, WPARAM wParam, LPARAM lParam);
	public:
		Controller (unsigned id) : Dialog::Controller (id) {}
		static PropPage::Controller * GetController (LPARAM lParam)
		{
			PROPSHEETPAGE const * page = reinterpret_cast<PROPSHEETPAGE *> (lParam);
			return reinterpret_cast<PropPage::Controller *> (page->lParam);
		}
	};

	//-------------------------------------------------------------------------------
	// PropPage::Sheet
	// Property Sheet, a collection of dialog pages with a tab control for navigation
	//-------------------------------------------------------------------------------
	class Sheet
	{
	protected:
		class Header;

		// PropPage::Sheet::Page
		class Page : public PROPSHEETPAGE
		{
		public:
			Page (PropPage::Controller * controller,
				  PropPage::Handler * handler, 
				  Header const & header);

			void SetTitle (std::string const & title)
			{
				dwFlags |= PSP_USETITLE;
				pszTitle = title.c_str ();
			}
			void SetNoApplyButton () { dwFlags |= PSH_NOAPPLYNOW; }
			void SetHasHelp () { dwFlags |= PSP_HASHELP; }

			long GetResourceId () const
			{
				Assert ((dwFlags & PSP_DLGINDIRECT) == 0);
				return reinterpret_cast<long>(pszTemplate);
			}
		};

		// PropPage::Sheet::Header
		class Header : public PROPSHEETHEADER
		{
		public:
			Header (Win::Dow::Handle win, std::string const & caption); 
			Header (Win::Instance hInst, std::string const & caption);

			void SetNoContextHelp ();
			void SetNoApplyButton () { dwFlags |= PSH_NOAPPLYNOW; }
			void SetWizardStyle () { dwFlags |= PSH_WIZARD; }
			void SetStartPage (unsigned long startPage) { nStartPage = startPage; }
			void Attach (std::vector<PROPSHEETPAGE> const & pages)
			{
				nPages = pages.size ();
				ppsp = &pages [0];
			}

			bool IsWizard () const { return (dwFlags & PSH_WIZARD) != 0; }
			bool IsModeless () const { return (dwFlags & PSH_MODELESS) != 0; }
			bool NoApplyButton () const { return (dwFlags & PSH_NOAPPLYNOW) != 0; }

			Win::Instance GetInstance () const { return hInstance; }
			std::string const & GetCaption () const { return _caption; }
			unsigned long GetStartPage () const { return nStartPage; }

		private:
			void Init (std::string const & caption);
			std::string _caption;
		};

	public:
		// PropPage::Sheet proper
		Sheet (Win::Dow::Handle win, std::string const & caption); 
		Sheet (Win::Instance hInst, std::string const & caption);

		void SetNoContextHelp () { _header.SetNoContextHelp (); }
		void SetNoApplyButton () { _header.SetNoApplyButton (); }
		void SetWizardStyle () { _header.SetWizardStyle (); }
		void SetStartPage (unsigned long startPage) { _header.SetStartPage (startPage); }

		virtual void AddPage (PropPage::Handler & handler, std::string const & title = std::string ());

		// Returns true when the user closed the property sheet using the OK button
		bool Display (Win::CritSection * critSect = 0);

	protected:
		Header						_header;
		auto_vector<PropPage::Controller> _controllers;
		std::vector<PROPSHEETPAGE>	_nativePages;
		std::list<std::string>		_titles; // list items are not reallocated!
	};

	class Navigator
	{
	public:
		Navigator () : _lastMoveFwd (true) {}
		virtual void Next (long result)
		{
			_pageNoStack.push (result);
			_lastMoveFwd = true;
		}
		virtual long Prev ()
		{
			Assert (!_pageNoStack.empty ());
			_pageNoStack.pop ();
			Assert (!_pageNoStack.empty ());
			_lastMoveFwd = false;
			return _pageNoStack.top ();
		}
		bool WasLastMoveFwd () const { return _lastMoveFwd; }
	protected:
		std::stack<long> _pageNoStack;
		bool _lastMoveFwd;
	};

	//-----------------------------------------------------------
	// PropPage::Wizard
	// A collection of dialog pages with Next/Previous navigation
	//-----------------------------------------------------------
	class Wizard : public PropPage::Sheet, public PropPage::Navigator
	{
	public:
		Wizard (Win::Dow::Handle win, std::string const & caption)
			: Sheet (win, caption)
		{
			SetWizardStyle ();
			SetNoApplyButton ();
		}
		Wizard (Win::Instance hInst, std::string const & caption)
			: Sheet (hInst, caption)
		{
			SetWizardStyle ();
			SetNoApplyButton ();
		}

		virtual void AddPage (PropPage::Handler & handler, std::string const & title = std::string ());

		// Returns true when user ended with OK button
		bool Run (Win::CritSection * critSect = 0);
	};

	//---------------------------
	// PropPage::WizardHandler
	//---------------------------
	class WizardHandler : public PropPage::Handler
	{
	public:
		WizardHandler (unsigned pageId, bool supportsContextHelp)
			: PropPage::Handler (pageId, supportsContextHelp)
		{}
	};

	//--------------------------------
	// PropPage::HandlerSet
	// A container of Page Handlers
	//--------------------------------
	class HandlerSet
	{
	private:
		struct HandlerInfo
		{
			HandlerInfo ()
				: _handler (0)
			{}

			HandlerInfo (PropPage::Handler * handler, std::string const & pageCaption)
				: _handler (handler),
				  _pageCaption (pageCaption)
			{}

			PropPage::Handler *	_handler;
			std::string			_pageCaption;
		};

	public:
		HandlerSet (std::string const & caption)
			: _caption (caption)
		{}

		void AddHandler (PropPage::Handler & ctrl, std::string const & pageCaption = std::string ())
		{
			HandlerInfo handlerInfo (&ctrl, pageCaption);
			_handlers.push_back (handlerInfo);
		}

		std::string const & GetCaption () const { return _caption; }
		virtual int GetStartPage () const { return 0; }
		virtual bool IsValidData () const { return false; }

		virtual bool GetDataFrom (NamedValues const & source)
		{
			for (Sequencer seq (*this); !seq.AtEnd (); seq.Advance ())
				seq.GetPageHandler ().GetDataFrom (source);

			return IsValidData ();
		}

		class Sequencer
		{
		public:
			Sequencer (HandlerSet const & ctrlSet)
				: _cur (ctrlSet._handlers.begin ()),
				  _end (ctrlSet._handlers.end ())
			{}

			bool AtEnd () const { return _cur == _end; }
			void Advance () { ++_cur; }

			PropPage::Handler & GetPageHandler () { return *_cur->_handler; }
			std::string const & GetPageCaption () const { return _cur->_pageCaption; }

		private:
			std::vector<HandlerInfo>::const_iterator	_cur;
			std::vector<HandlerInfo>::const_iterator	_end;
		};

		friend class Sequencer;

	private:
		std::string					_caption;
		std::vector<HandlerInfo>	_handlers;
	};
}

#endif
