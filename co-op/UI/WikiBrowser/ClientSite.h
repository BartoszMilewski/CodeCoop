#if !defined (CLIENTSITE_H)
#define CLIENTSITE_H
//--------------------------------
//  (c) Reliable Software, 2005-06
//--------------------------------
#include "WebBrowserSite.h"

namespace Ole
{
	// Container manages multiple sites

	// Provides the primary means by which an embedded object obtains
	// information about the location and extent of its display site, 
	// its moniker, its user interface, and other resources provided 
	// by its container. An object server calls IOleClientSite to request 
	// services from the container. A container must provide one instance 
	// of IOleClientSite for every compound-document object it contains.

	
	// Wikipedia: This interface allows the caller to obtain information 
	// on the container and location of an object, as well requesting that 
	// the object be saved, resized, shown, hidden, et cetera.

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
