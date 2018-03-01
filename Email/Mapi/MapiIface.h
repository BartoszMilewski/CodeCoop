#if !defined (MAPIIFACE_H)
#define MAPIIFACE_H
//-----------------------------------------------------
//  MapiIface.h
//  (c) Reliable Software 2001 -- 2002
//-----------------------------------------------------

#include "MapiHelpers.h"
#include "MapiBuffer.h"
#include "MapiEx.h"

#include <Com/Com.h>
#include <LightString.h>

#include <mapix.h>

namespace Mapi
{
	// Mapi::Interface is capable of retrieving extended error information
	// in the case of Mapi interface method failure. ThrowIfError will call
	// GetLastError on that interface. This template is designed to use with
	// the following Mapi interfaces:
	//	- every interface inheriting from IMAPIProp for example IMAPIFolder, IMsgStore, IMessage
	//	- IABLogon
	//	- IABProvider
	//	- IMAPIControl
	//	- IMAPISession
	//	- IMAPISupport
	//	- IMAPITable
	//	- IMsgServiceAdmin
	//	- IMSLogon
	//	- IMSProvider
	//	- IProfAdmin
	//	- IProviderAdmin
	template <class T>
	class Interface : public Com::IfacePtr<T>
	{
	public:
		Interface () {}

		operator void * () { return _p; }

		void ThrowIfError (Result callResult, char const * msg) const throw (Mapi::Exception);

	private:
		class ErrorEx : public Buffer<MAPIERROR>
		{
		public:
			ErrorEx (T * pIf, Result callResult)
				: _isValid (true)
			{
				HRESULT hr = pIf->GetLastError (callResult, 0, &_p);
				_isValid = (S_OK == hr);
			}

			bool IsValid () const { return _isValid; }

			char const * GetError () const { return _p->lpszError; }
			char const * GetComponent () const { return _p->lpszError; }
			unsigned long GetLowLevelErr () const { return _p->ulLowLevelError; }
			unsigned long GetContextCode () const { return _p->ulContext; }

		private:
			bool	_isValid;
		};
	};

	template <class T>
	void Interface<T>::ThrowIfError (Result callResult, char const * msg) const throw (Mapi::Exception)
	{
		if (callResult.IsOk ())
			return;
		if (callResult.IsExtendedError ())
		{
			Msg infoEx;
			ErrorEx err (*this, callResult);
			if (err.IsValid ())
			{
				infoEx << "Extended error information -- ";
				if (err.GetError () != 0)
					infoEx << err.GetError ();
				else
					infoEx << "not specified";
				infoEx << "\nComponent: ";
				if (err.GetComponent () != 0)
					infoEx << err.GetComponent ();
				else
					infoEx << "not specified";
				infoEx << "\nLow level error code: " << std::hex << err.GetLowLevelErr ();
				infoEx << "\nContext code: " << std::hex << err.GetContextCode ();
				throw Exception (msg, callResult, infoEx.c_str ());
			}
		}
		throw Exception (msg, callResult);
	}
}

#endif
