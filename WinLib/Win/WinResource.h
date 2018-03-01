#if !defined (WINRESOURCE_H)
#define WINRESOURCE_H
//---------------------------
// (c) Reliable Software 2005
//---------------------------
namespace Win { class Instance; }

template<class T> class Resource
{
public:
	Resource (Win::Instance inst, int resId, char const * type)
		: _hRes (0),
		_hGlob (0),
		_size (0)
	{
		_hRes = ::FindResource (inst, MAKEINTRESOURCE (resId), type);
		if (_hRes != 0)
		{
			_hGlob = ::LoadResource (inst, _hRes);
			if (_hGlob)
				_size = ::SizeofResource (inst, _hRes);
		}
	}

	bool IsOk () const { return _hRes != 0 && _hGlob !=0; }
	unsigned long GetSize () const { return _size; }

	T * Lock ()
	{
		return static_cast<T *> (::LockResource (_hGlob));
	}

private:
	HRSRC	_hRes;
	HGLOBAL	_hGlob;
	unsigned long	_size;
};

#endif
