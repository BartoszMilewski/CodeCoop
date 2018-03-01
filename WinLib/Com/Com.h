#if !defined (COM_HEADER)
#define COM_HEADER
//--------------------------------
//  (c) Reliable Software, 2001-03
//--------------------------------

// MS note: You must include the #define _WIN32_DCOM preprocessor
// directive at the beginning of your code to be able to use CoInitializeEx
// It's been added to WinLibBase.h

namespace Com
{
	class Exception: public Win::Exception 
	{
	public:
		Exception (HRESULT hresult, char const * msg = 0, char const * objName = 0)
			: Win::Exception (msg, objName, hresult)
		{}
	};
	// Position this object in such a way that its destructor
	// is called on application shutdown, as the last call made 
	// to the COM library after the application hides its main windows 
	// and falls through its main message loop
	// Create one Use object per each thread that has its own message loop
	class Use
	{
	public:
		Use ()
		{
			HRESULT hr = ::CoInitializeEx (0, COINIT_APARTMENTTHREADED);
			if (hr != S_OK && hr != S_FALSE)
				throw Com::Exception (hr, "Couldn't initialize COM.");
		}
		~Use ()
		{
			::CoUninitialize ();
		}
	};

	// Create one MainUse object per process, in the main thread.
	class MainUse : public Use
	{
	public:
		MainUse ()
		{
			HRESULT hr = ::CoInitializeSecurity (
								NULL,						// security descriptor
								-1,							// authentication services
								NULL,						// array of authentication services
								NULL,						// reserved
								RPC_C_AUTHN_LEVEL_CONNECT,  // default authentication level
								RPC_C_IMP_LEVEL_IMPERSONATE,// default impersonation level
								NULL,						// information for authentication services
								EOAC_NONE,					// additional capabilities
								0);							// reserved
			if (hr != S_OK)
				throw Com::Exception (hr, "Couldn't initialize security.");
		};
	};
	
	class UseOle
	{
	public:
		UseOle ()
		{
			HRESULT hr = ::OleInitialize (0);
			if (hr != S_OK)
				throw Com::Exception (hr, "Couldn't initialize OLE");
		}
		~UseOle ()
		{
			::OleUninitialize ();
		}
	};

	class UnknownPtr
    {
    public:
		UnknownPtr (IUnknown * p = 0)
			: _p (p)
		{}
		~UnknownPtr ()
		{
			Free ();
		}
        IUnknown ** operator & () { return &_p; }
		operator IUnknown * () const { return _p; }
		IUnknown * operator-> () { return _p; }
		bool IsNull () const { return _p == 0; }
		unsigned long GetRefCount ()
		{
			_p->AddRef ();
			return _p->Release ();
		}

    protected:
	    void Free ()
		{
			if (_p != 0)
				_p->Release ();
			_p =  0;
		}

	protected:
		IUnknown *	_p;

	private:
		UnknownPtr (UnknownPtr const & src);
		UnknownPtr & operator = (UnknownPtr const & src);
    };

	template <class T>
	class IfacePtr : public UnknownPtr
	{
	public:
		explicit IfacePtr (T * src = 0)
			: UnknownPtr (src)
		{}
		IfacePtr (IfacePtr<T> const & src)
		{
			if (!src.IsNull ())
				src._p->AddRef ();
			_p = src._p;
		}
		IfacePtr & operator = (IfacePtr const & src)
		{
			if (&src != this)
			{
				Free ();
				if (src._p != 0)
					src._p->AddRef ();
				_p = src._p;
			}
			return *this;
		}
		T ** operator & () { return reinterpret_cast<T **>(&_p); }
		T * operator->() { return GetPtr (); }
		T const * operator->() const { return GetPtr (); }
		operator T const * () const { return GetPtr (); }
		operator T * () const { return GetPtr (); }
		T const & GetAccess () const { return *(GetPtr ()); }

	private:
		T * GetPtr () const { return static_cast<T *>(_p); }
	};
}

#endif
