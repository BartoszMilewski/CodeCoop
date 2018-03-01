#if !defined (AUTOMATION_H)
#define AUTOMATION_H
//----------------------------------
//  (c) Reliable Software, 2005
//----------------------------------
#include <Com/Com.h>
#include <objbase.h>
#include <unknwn.h>

namespace Automation
{
	class TypeInfo;

	// COM object: provider of interfaces
	class CoObject
	{
	public:
		virtual void * AcquireInterface (IID const & iid) = 0;
	};

	// Smart COM object created using CoCreateInstance
	class SObject: public CoObject
	{
	public:
		SObject (CLSID const & classId, bool running = false);
		SObject (TypeInfo & info);
		~SObject ()
		{
			if (_iUnk)
				_iUnk->Release ();
		}
		void * AcquireInterface (IID const & iid);

	private:
		IUnknown * _iUnk;
	};
	
	class DispObject: public CoObject
	{
	public:
		DispObject (CLSID const & classId) : _iDisp ( 0 )
		{
			HRESULT hr = ::CoCreateInstance (classId, 0, CLSCTX_ALL, IID_IDispatch, (void**)&_iDisp);
			if (FAILED (hr))
			{
				if (hr == E_NOINTERFACE)
					throw Com::Exception (hr, "No IDispatch interface");
				else
					throw Com::Exception (hr, "Couldn't create DispObject");
			}
		}

		~DispObject ()
		{
			if (_iDisp)
				_iDisp->Release ();
		}

		operator bool () const { return _iDisp != 0; }
		bool operator ! () const { return _iDisp == 0; }

		DISPID GetDispId (WCHAR * funName)
		{
			DISPID dispid;
			HRESULT hr = _iDisp->GetIDsOfNames (IID_NULL, &funName, 1, GetUserDefaultLCID (), &dispid);
			if (FAILED (hr))
				throw Com::Exception (hr, "Cannot get type ID");
			return dispid;
		}

		void GetProperty (DISPID propId, VARIANT & result);
		void * AcquireInterface (IID const & iid);

	protected:
		DispObject (IDispatch * iDisp) : _iDisp (iDisp) {}
		DispObject () : _iDisp (0) {}
	protected:
		IDispatch * _iDisp;
	};

	template<class I>
	class SFace
	{
	public:
		~SFace ()
		{
			if (_i)
				_i->Release ();
		}
		I * operator-> () { return _i; }
		I * ptr () { return _i; }
	protected:
		SFace () : _i (0) {}
		SFace (void * i)
		{
			_i = static_cast<I*> (i);
		}
	protected:
		I * _i;
	};

	// Interface from Object

	template<class I, IID const * iid>
	class SObjFace: public SFace<I>
	{
	public:
		SObjFace (CoObject & obj)
			: SFace<I> (obj.AcquireInterface (*iid))
		{}
	};

	class Bool
	{
	public:
		Bool (VARIANT_BOOL * varBool)
			: _var (*varBool)
		{
			Assert (_var == 0 || _var == -1);
		}
		void Set (bool val)
		{
			_var = val? -1: 0;
		}
		bool Get ()
		{
			Assert (_var == 0 || _var == -1);
			return (_var == 0)? false: true;
		}
	private:
		/* 0 == FALSE, -1 == TRUE */
		VARIANT_BOOL & _var;
	};

	// Strings
	class BString
	{
		friend class CString;
	public:
		BString ()
			:_str (0), _own (false)
		{}
		BString (VARIANT & var)
			: _own (false)
		{
			if (var.vt != VT_BSTR)
			{
				throw Win::InternalException ("Variant type is not a BSTR");
			}
			_str = var.bstrVal;
		}
		BString (std::wstring const & str)
			:_str (::SysAllocString (str.c_str ())),
			 _own (true)
		{}
		~BString ()
		{
			// Works for null string, too.
			if (_own)
				::SysFreeString (_str);
		}
		BSTR * GetPointer () { return &_str; }
	protected:
		BSTR _str;
		bool _own;
	};

	class CString
	{
	public:
		CString (BString & bstr)
			: _str (0), _len (0)
		{
			Init (bstr._str);
		}
		CString (VARIANT & var)
			: _str (0), _len (0)
		{
			if (var.vt != VT_BSTR)
			{
				throw Win::InternalException ("Variant type is not a BSTR");
			}
			Init (var.bstrVal);
		}
		~CString ()
		{
			delete []_str;
		}
		operator char const * () const { return _str; }
		char const * c_str () const { return _str; }
		int Len () { return _len; }
		bool IsEqual (char const * str)
		{
			Assert (str != 0);
			Assert (_str != 0);
			return strcmp (str, _str) == 0;
		}
	private:
		void Init (BSTR bstr)
		{
			if (bstr != 0)
			{
				_len = wcstombs (0, bstr, 0);
				_str = new char [_len + 1];
				wcstombs (_str, bstr, _len);
				_str [_len] = '\0';
			}
		}
	protected:
		char * _str;
		int    _len;
	};
	
	// Type Library from exe or dll
	class TypeLibrary
	{
		friend class TypeInfo;
	public:
		// Can use name if found on the PATH
		TypeLibrary (WCHAR * path);
		~TypeLibrary ()
		{
			if (_iLib != 0)
				_iLib->Release ();
		}
		bool IsOk () const { return _iLib != 0; }
		int GetCount ()
		{
			return _iLib->GetTypeInfoCount ();
		}
		void GetDocumentation (int idx, Automation::BString & name, Automation::BString & doc);
//		void GetDocumentation (int idx, _bstr_t & name, _bstr_t & doc);
	private:
		ITypeInfo * GetTypeInfo (int idx);
		ITypeInfo * GetTypeInfo (WCHAR * name);
	private:
		ITypeLib * _iLib;
	};

	// Type info for a given element of the library
	class TypeInfo: public SFace<ITypeInfo>
	{
		friend class SObject;
	public:
		TypeInfo (TypeLibrary & lib, int idx);
		TypeInfo (TypeLibrary & lib, WCHAR * name);
		~TypeInfo ()
		{
			_i->ReleaseTypeAttr (_attr);
		}
		GUID & GetGuid () const { return _attr->guid; }
		void GetDocumentation (Automation::BString & name, Automation::BString & doc);
//		void GetDocumentation (_bstr_t & name, _bstr_t & doc);
	private:
		void * CreateInstance (IID const & iid);
	private:
		TYPEATTR  * _attr;
	};
}

#endif
