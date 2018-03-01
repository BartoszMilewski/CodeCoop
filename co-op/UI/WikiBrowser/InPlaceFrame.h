#if !defined (INPLACEFRAME_H)
#define INPLACEFRAME_H
//--------------------------------
//  (c) Reliable Software, 2005-06
//--------------------------------
#include "WebBrowserSite.h"

namespace Ole
{
    // This interface is used by object applications to control 
	// the display and placement of composite menus, keystroke accelerator translation, 
	// context-sensitive help mode, and modeless dialog boxes.

	class InPlaceFrame : public IOleInPlaceFrame
	{
	public:
		InPlaceFrame (WebBrowserSite & site) : _site (site) {}
		virtual ~InPlaceFrame() {}

		// IUnknown
		STDMETHODIMP QueryInterface(REFIID iid, void ** ppvObject)
		{
			return _site.QueryInterface (iid, ppvObject);
		}
		ULONG STDMETHODCALLTYPE AddRef ()
		{
			return _site.AddRef ();
		}
		ULONG STDMETHODCALLTYPE Release ()
		{
			return _site.Release ();
		}

		// IOleWindow
		HRESULT STDMETHODCALLTYPE GetWindow(HWND * phwnd)
		{
			*phwnd = _site.GetWindow ().ToNative ();
			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode)
		{
			return E_NOTIMPL;
		}

		// IOleInPlaceUIWindow

		HRESULT STDMETHODCALLTYPE GetBorder (LPRECT lprcRectBorder)
		{
			return E_NOTIMPL;
		}
		HRESULT STDMETHODCALLTYPE RequestBorderSpace(LPCBORDERWIDTHS pborderwidths)
		{
			return E_NOTIMPL;
		}
		HRESULT STDMETHODCALLTYPE SetActiveObject(IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
		{
			return E_NOTIMPL;
		}
		HRESULT STDMETHODCALLTYPE SetBorderSpace(LPCBORDERWIDTHS pborderwidths)
		{
			return E_NOTIMPL;
		}

		// IOleInPlaceFrame 

		HRESULT STDMETHODCALLTYPE EnableModeless(BOOL fEnable)
		{
			return E_NOTIMPL;
		}
		HRESULT STDMETHODCALLTYPE InsertMenus (HMENU hMenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
		{
			return E_NOTIMPL;
		}
		HRESULT STDMETHODCALLTYPE RemoveMenus (HMENU hMenuShared)
		{
			return E_NOTIMPL;
		}
		HRESULT STDMETHODCALLTYPE SetMenu (HMENU hMenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
		{
			return E_NOTIMPL;
		}
		HRESULT STDMETHODCALLTYPE SetStatusText (LPCOLESTR pszStatusText)
		{
			return E_NOTIMPL;
		}
		HRESULT STDMETHODCALLTYPE TranslateAccelerator (LPMSG lpmsg, WORD wId)
		{
			return E_NOTIMPL;
		}
private:
		WebBrowserSite & _site;
	};

}
#endif
