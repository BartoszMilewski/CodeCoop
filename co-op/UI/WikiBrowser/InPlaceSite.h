#if !defined (INPLACESITE_H)
#define INPLACESITE_H
//--------------------------------
//  (c) Reliable Software, 2005-06
//--------------------------------
#include "WebBrowserSite.h"

namespace Ole
{
	// Manages the interaction between the container and the object's in-place client site. 
	// Recall that the client site is the display site for embedded objects, 
	// and provides position and conceptual information about the object.

	// Wikipedia: If a container implements this interface, it allows embedded 
	// objects to be activated in place, i.e. without opening in a separate window. 
	// It provides access to the container's IOleInPlaceUIWindow through GetWindowContext

	class InPlaceSite : public IOleInPlaceSite
	{
	public:
		InPlaceSite (WebBrowserSite & site) : _site (site) {}
		virtual ~InPlaceSite() {}

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

		// IOleInPlaceSite
		HRESULT STDMETHODCALLTYPE CanInPlaceActivate ()
		{
			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE OnInPlaceActivate ()
		{
			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE OnUIActivate ()
		{
			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE GetWindowContext(IOleInPlaceFrame **ppFrame,
												IOleInPlaceUIWindow **ppDoc, 
												LPRECT lprcPosRect,
												LPRECT lprcClipRect,
												LPOLEINPLACEFRAMEINFO lpFrameInfo)
		{
			return S_OK;
		}

		HRESULT STDMETHODCALLTYPE Scroll  (SIZE scrollExtent)
		{
			return E_NOTIMPL;
		}
		HRESULT STDMETHODCALLTYPE OnUIDeactivate (BOOL fUndoable)
		{
			return E_NOTIMPL;
		}
		HRESULT STDMETHODCALLTYPE OnInPlaceDeactivate ()
		{
			return E_NOTIMPL;
		}
		HRESULT STDMETHODCALLTYPE DiscardUndoState ()
		{
			return E_NOTIMPL;
		}
		HRESULT STDMETHODCALLTYPE DeactivateAndUndo ()
		{
			return E_NOTIMPL;
		}
		HRESULT STDMETHODCALLTYPE OnPosRectChange (LPCRECT lprcPosRect)
		{
			return E_NOTIMPL;
		}
	private:
		WebBrowserSite & _site;
	};

}
#endif
