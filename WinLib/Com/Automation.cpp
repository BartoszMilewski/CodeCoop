//----------------------------------
//  (c) Reliable Software, 2005
//----------------------------------
#include "WinLibBase.h"
#include "Automation.h"

namespace Automation
{
	SObject::SObject (CLSID const & classId, bool running)
		:_iUnk (0)
	{
		HRESULT hr = S_OK;
		if (running)
		{
			::GetActiveObject (classId, 0, &_iUnk);
		}
		if (_iUnk == 0)
		{
			hr = ::CoCreateInstance (classId, 0, CLSCTX_SERVER, IID_IUnknown, (void**)&_iUnk);
		}
		if (FAILED (hr))
			throw Com::Exception (hr, "Couldn't create instance");
	}

	SObject::SObject (TypeInfo & info)
		: _iUnk (0)
	{
		_iUnk = reinterpret_cast<IUnknown *>(info.CreateInstance (IID_IUnknown));
	}

	void * SObject::AcquireInterface (IID const & iid)
	{
		void * p = 0;
		HRESULT hr = _iUnk->QueryInterface (iid, &p);
		if (FAILED (hr))
		{
			if (hr == E_NOINTERFACE)
				throw Com::Exception (hr, "No such interface" );
			else
				throw Com::Exception (hr, "Couldn't query interface" );
		}
		IUnknown * unk = static_cast<IUnknown *> (p);
		unk->AddRef ();
		return p;
	}
	void DispObject::GetProperty (DISPID propId, VARIANT & result)
	{
		// In parameters
		DISPPARAMS args = { 0, 0, 0, 0 };
		EXCEPINFO except;
		UINT argErr;
		HRESULT hr = _iDisp->Invoke (propId, 
			IID_NULL, 
			GetUserDefaultLCID (), 
			DISPATCH_PROPERTYGET, 
			&args,
			&result,
			&except,
			&argErr);
		if (FAILED (hr))
			throw Com::Exception (hr, "Couldn't get property");
	}

	void * DispObject::AcquireInterface (IID const & iid)
	{
		void * p = 0;
		HRESULT hr = _iDisp->QueryInterface (iid, &p);
		if (FAILED (hr))
		{
			if (hr == E_NOINTERFACE)
				throw Com::Exception (hr, "No such interface");
			else
				throw Com::Exception (hr, "Couldn't query interface");
		}
		return p;
	}

	TypeLibrary::TypeLibrary (WCHAR * path)
	{
		HRESULT hr = ::LoadTypeLib (path, &_iLib);
		if (FAILED (hr))
		{
			if (hr == TYPE_E_CANTLOADLIBRARY)
				_iLib = 0;
			else
				throw Com::Exception (hr, "Couldn't load type library");
		}
		if (_iLib != 0)
		{
			hr = RegisterTypeLib (_iLib, path, 0);
			if (FAILED (hr))
				throw Com::Exception (hr, "Couldn't register type library");
		}
	}

	ITypeInfo * TypeLibrary::GetTypeInfo (int idx)
	{
		ITypeInfo * info;
		HRESULT hr = _iLib->GetTypeInfo (idx, &info);
		if (FAILED (hr))
			throw Com::Exception (hr, "Couldn't get type info");
		return info;
	}

	ITypeInfo * TypeLibrary::GetTypeInfo (WCHAR * name)
	{
		ITypeInfo * info = 0;
		MEMBERID id = 0;
		unsigned short cFound = 1; // look for first match only
		HRESULT hr = _iLib->FindName (name, 0, &info, &id, &cFound);
		if (FAILED (hr) || info == 0)
			throw Com::Exception (hr);
		return info;
	}

	TypeInfo::TypeInfo (TypeLibrary & lib, int idx)
		: SFace<ITypeInfo> (lib.GetTypeInfo (idx))
	{
		HRESULT hr = _i->GetTypeAttr (&_attr);
		if (FAILED (hr))
			throw Com::Exception (hr, "Couldn't get type attributes");
	}

	TypeInfo::TypeInfo (TypeLibrary & lib, WCHAR * name)
		: SFace<ITypeInfo> (lib.GetTypeInfo (name))
	{
		HRESULT hr = _i->GetTypeAttr (&_attr);
		if (FAILED (hr))
			throw Com::Exception (hr, "Couldn't get type attributes");
	}

	void * TypeInfo::CreateInstance (IID const & iid)
	{
		void * inst = 0;
		HRESULT hr = _i->CreateInstance (0, iid, &inst);
		if (FAILED (hr))
			throw Com::Exception (hr, "Couldn't create instance from type info");
		return inst;
	}
}

