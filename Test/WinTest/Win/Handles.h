#if !defined (HANDLES_H)
#define HANDLES_H
//  (c) Reliable Software, 2001-03
#include <windows.h>

namespace Win
{
	// Generic handle to Windows resources
	template<class NativeHandle>
	class Handle
	{
	public:
		typedef typename NativeHandle Type;
		Handle (NativeHandle h = NullValue ()) : _h (h) {}
		bool IsNull () const throw () { return _h == NullValue (); }
		NativeHandle ToNative () const { return _h; }
		void Reset (NativeHandle h = NullValue ()) { _h = h; }
		bool operator== (Handle h) const { return _h == h.ToNative (); }
		bool operator!= (Handle h) const { return _h != h.ToNative (); }
		static NativeHandle NullValue () throw () { return 0; }
	protected:
		NativeHandle _h;
	};

	// Handle disposal policy
	template<class BaseHandle>
	struct Disposal
	{
		static void Dispose (BaseHandle) throw ();
	};

	// Forward declaration
	template<class BaseHandle, class DisposalPolicy>
	class AutoHandle;

	// auxiliary struct for returning by value
	template<class BaseHandle, class DisposalPolicy>
	struct auto_handle_ref
	{
		auto_handle_ref (AutoHandle<BaseHandle, DisposalPolicy> & ah)
			: _ah (ah)
		{}
		AutoHandle<BaseHandle, DisposalPolicy> & _ah;
	};

	template<class BaseHandle, class DisposalPolicy = Disposal<BaseHandle> >
	class AutoHandle: public BaseHandle
	{
	public:
		explicit AutoHandle (typename BaseHandle::Type nh)
			: BaseHandle (nh)
		{}
		AutoHandle () {}
		AutoHandle (AutoHandle & ah) throw ()
			: BaseHandle (ah.release ())
		{}
		~AutoHandle () throw ()
		{
			if (!IsNull ())
				DisposalPolicy::Dispose (*this);
		}
		typename BaseHandle::Type release () throw ()
		{
			typename BaseHandle::Type tmp = _h;
			_h = NullValue ();
			return tmp;
		}
		void Reset (typename BaseHandle::Type h = NullValue ()) throw ()
		{
			if (h != _h)
			{
				if (!IsNull ())
					DisposalPolicy::Dispose (*this);
				BaseHandle::Reset (h);
			}
		}
		void Reset (BaseHandle h = BaseHandle ())
		{
			if (h != _h)
			{
				if (!IsNull ())
					DisposalPolicy::Dispose (*this);
				BaseHandle::Reset (h.ToNative ());
			}
		}
		AutoHandle & operator = (AutoHandle & h)
		{
			AutoHandle tmp = h;
			std::swap (_h, tmp._h);
			return *this;
		}
		// return by value helpers
		AutoHandle (auto_handle_ref<BaseHandle, DisposalPolicy> r) throw ()
			: BaseHandle (r._ah.release ())
		{}
		operator auto_handle_ref<BaseHandle, DisposalPolicy> ()
		{
			return auto_handle_ref<BaseHandle, DisposalPolicy> (*this);
		}
	};
}

#endif
