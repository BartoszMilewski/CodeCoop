#if !defined (CLIENTSITE_H)
#define CLIENTSITE_H
//--------------------------------
//  (c) Reliable Software, 2005-06
//--------------------------------
#include "WebBrowserSite.h"

namespace Ole
{
	class ClientSite : public IOleClientSite
	{
	public:
		ClientSite (WebBrowserSite & site) : _site (site) {}
		virtual ~ClientSite () {}

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

		// IOleClientSite
		STDMETHODIMP ShowObject() { return S_OK; }

		STDMETHODIMP GetContainer (LPOLECONTAINER * ppContainer)
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP SaveObject()
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP GetMoniker (DWORD dwAssign, DWORD dwWhichMoniker, IMoniker ** ppmk)
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP OnShowWindow (BOOL fShow)
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP RequestNewObjectLayout ()
		{
			return E_NOTIMPL;
		}
	private:
		WebBrowserSite & _site;
	};
}
#endif
