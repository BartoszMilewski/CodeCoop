#if !defined (GRAPHHANDLES_H)
#define GRAPHHANDLES_H
// (c) Reliable Software 2002
#include <Win/Handles.h>

namespace Gdi
{
	typedef Win::Handle<HGDIOBJ> Handle;
	// All GDI objects are disposed using ::DeleteObject
	// Use Gdi::Object as template argument for GDI handles
	template<class BaseHandle>
	struct Disposal
	{
		static void Dispose (BaseHandle h) throw () { ::DeleteObject (h.ToNative ()); }
	};
}

namespace Pen
{
	typedef Win::Handle<HPEN> Handle;
	typedef Win::AutoHandle<Handle, Gdi::Disposal<Pen::Handle> > AutoHandle;
}

namespace Accel
{
	class Handle;
	typedef Win::AutoHandle<Handle> AutoHandle;
}

namespace Font
{
	typedef Win::Handle<HFONT> Handle;
	typedef Win::AutoHandle<Handle, Gdi::Disposal<Font::Handle> > AutoHandle;
}

namespace ImageList
{
	class Handle;
	class AutoHandle;
}

namespace Brush
{
	typedef Win::Handle<HBRUSH> Handle;
	class AutoHandle;
}

namespace Bitmap
{
	class Handle;
	class AutoHandle;
}

namespace Cursor
{
	typedef Win::Handle<HCURSOR> Handle;
	typedef Win::AutoHandle<Cursor::Handle> AutoHandle;
}

namespace Icon
{
	typedef Win::Handle<HICON> Handle;
	typedef Win::AutoHandle<Icon::Handle> AutoHandle;
}

namespace Region
{
	class Handle;
	class AutoHandle;
}

#endif
