#if !defined (HOLDER_H)
#define HOLDER_H
// (c) Reliable Software 2002

#include <Graph/Canvas.h>

namespace Gdi
{
	template<class ObjectHandle>
	class ObjectHolder
	{
	public:
		typedef ObjectHandle Handle;
	public:
		ObjectHolder (Win::Canvas canvas, ObjectHandle object)
			: _canvas (canvas),
			  _oldObject (::SelectObject (_canvas.ToNative (), object.ToNative ()))
		{}
		~ObjectHolder ()
		{
			::SelectObject (_canvas.ToNative (), _oldObject.ToNative ());
		}
	protected:
		Win::Canvas _canvas;
	private:
		Gdi::Handle _oldObject;
	};
}

#endif
