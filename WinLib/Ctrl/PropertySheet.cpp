// --------------------------------
// (c) Reliable Software, 1999-2005
// --------------------------------

#include <WinLibBase.h>
#include "PropertySheet.h"

#include <Sys/Dll.h>
#include <Sys/Synchro.h>
#include <Sys/SysVer.h>
#include <Ctrl/Output.h>

void Notify::PageHandler::OnPrev (long & result) throw (Win::Exception)
{
	result = -1;
	if (_navigator)
	{
		if (GoPrevious ())
			result = _navigator->Prev ();
	}
}
void Notify::PageHandler::OnNext (long & result) throw (Win::Exception)
{
	result = -1;
	if (_navigator)
	{
		if (GoNext (result))
		{
			Assert (result != -1);
			_navigator->Next (result);
		}
	}
}

namespace PropPage
{
	BOOL CALLBACK Procedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		Win::Dow::Handle win (hwnd);
		PropPage::Controller * ctrl = win.GetLong<PropPage::Controller *> ();
		try
		{
			if (Dialog::DispatchMsg (ctrl, win, message, wParam, lParam))
				return TRUE;
		}
		catch (Win::Exception e)
		{
			Out::Sink::DisplayException (e, win);
			// exit property sheet / wizard
			Win::Dow::Handle sheet (::GetParent (win.ToNative ()));
			::PropSheet_PressButton (sheet.ToNative (), PSBTN_CANCEL);
		}
		return FALSE;
	}

	Sheet::Header::Header (Win::Dow::Handle win, std::string const & caption)
	{
		Init (caption);
		hwndParent = win.ToNative ();
		hInstance = win.GetInstance ();
	}

	Sheet::Header::Header (Win::Instance hInst, std::string const & caption)
	{
		Init (caption);
		hInstance = hInst;
	}

	void Sheet::Header::SetNoContextHelp ()
	{
		SystemVersion sysVer;
		if (sysVer.IsWin95 () || (sysVer.IsWinNT () && sysVer.MajorVer () == 4))
		{
			//	Revisit: Emergency fix for Bug #711
			return;
		}
		dwFlags |= PSH_NOCONTEXTHELP;
	}

	void Sheet::Header::Init (std::string const & caption)
	{
		_caption = caption;
		::ZeroMemory (this, sizeof (PROPSHEETHEADER));

		Dll comctl32Dll ("comctl32.dll");
		DllVersion ver (comctl32Dll);
		if (ver.IsOk ())
		{
			if (ver.GetMajorVer () == 4 && ver.GetMinorVer () <= 70)
				dwSize  = PROPSHEETHEADER_V1_SIZE;
			else
				dwSize  = sizeof (PROPSHEETHEADER);
		}
		else
		{
			throw Win::Exception ("Cannot determine the Property Sheet control version");
		}
		dwFlags = PSH_PROPSHEETPAGE;
		pszCaption = _caption.c_str ();
	}

	Sheet::Page::Page (PropPage::Controller * controller,
		PropPage::Handler * handler,
		PropPage::Sheet::Header const & header)
	{
		::ZeroMemory (this, sizeof (PROPSHEETPAGE));
		dwSize = sizeof (PROPSHEETPAGE);
		dwFlags = PSH_PROPSHEETPAGE;
		hInstance = header.GetInstance ();
		pfnDlgProc = PropPage::Procedure;
		pszTemplate = MAKEINTRESOURCE (handler->GetPageId ()); 
		lParam = reinterpret_cast<LPARAM>(controller);

		if (handler->SupportsContextHelp ())
			SetHasHelp ();

		if (header.NoApplyButton ())
			SetNoApplyButton ();
	}

	Sheet::Sheet (Win::Dow::Handle win, std::string const & caption)
		: _header (win, caption)
	{}
	
	Sheet::Sheet (Win::Instance hInst, std::string const & caption)
		: _header (hInst, caption)
	{}

	void Sheet::AddPage (PropPage::Handler & handler, std::string const & title)
	{
		Assert (!title.empty ());
		std::unique_ptr<PropPage::Controller> ctrl (new PropPage::Controller (handler.GetPageId ()));
		ctrl->SetDlgControlHandler (&handler);
		handler.Attach (ctrl.get ());
		Page page (ctrl.get (), &handler, _header);
		_controllers.push_back (std::move(ctrl));

		// own the string
		_titles.push_back (title);
		page.SetTitle (_titles.back ());

		_nativePages.push_back (page);
	}

	bool Sheet::Display (Win::CritSection * critSect)
	{
		Assert (!_header.IsModeless ());
		_header.Attach (_nativePages);
		// Exit critical section while running property sheet
		Assert (Dbg::IsMainThread ());
		Win::UnlockPtr unlock (critSect);
		int result = ::PropertySheet (&_header);
		if (result == -1)
			throw Win::Exception ("Property Sheet creation failed.");
		return result > 0;	// result == 0 -- user ended with Cancel button
	}

	void Wizard::AddPage (PropPage::Handler & handler, std::string const & title)
	{
		// use wizard caption if page doesn't specify its own title
		handler.SetNavigator (this);
		Sheet::AddPage (handler, 
			            title.empty () ? _header.GetCaption () : title);
	}

	bool Wizard::Run (Win::CritSection * critSect)
	{ 
		PROPSHEETPAGE const & firstPage = _nativePages [_header.GetStartPage ()];
		_pageNoStack.push (reinterpret_cast<long>(firstPage.pszTemplate));
		return Display (critSect); 
	}
}

namespace Notify
{
	bool PageHandler::OnNotify (NMHDR * hdr, long & result) throw (Win::Exception)
	{
		// hdr->code
		// hdr->idFrom;
		// hdr->hwndFrom;
		switch (hdr->code)
		{
		case PSN_SETACTIVE:
			OnSetActive (result);
			return true;
		case PSN_KILLACTIVE:
			OnKillActive (result);
			return true;
		case PSN_WIZNEXT :
			OnNext (result);
			return true;
		case PSN_WIZBACK :
			OnPrev (result);
			return true;
		case PSN_WIZFINISH :
			OnFinish (result);
			return true;
		case PSN_QUERYCANCEL:
			OnCancel (result);
			return true;
		case PSN_APPLY:
			OnApply (result);
			return true;
		case PSN_RESET: // no return value
			OnReset ();
			return true;
		case PSN_HELP: // no return value
			OnHelp ();
			return true;
		};
		return false;
	}
}
